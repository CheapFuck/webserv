#include "baseHandlerObject.hpp"
#include "print.hpp"
#include "fd.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <memory>

FD::FD(int fd, FDType type, std::shared_ptr<BaseHandlerObject> connectedObject)
    : _fd(fd), _epollFd(-1), _type(type), _connectedObject(std::move(connectedObject)) {}

bool FD::operator<(const FD& other) const {
    return _fd < other._fd;
}

bool FD::operator<(int otherFd) const {
    return _fd < otherFd;
}

FD::~FD() {}

int FD::_cleanUp() {
    DEBUG("Closing file descriptor: " << _fd);

    if (isConnectedToEpoll()) {
        int ret = disconnectFromEpoll();
        ERROR_IF(ret == -1, "Failed to disconnect fd: " << _fd << " from epoll");
    }

    if (isValidFd()) {
        int result = ::close(_fd);
        _fd = -1;
        return (result);
    }

    if (_connectedObject) {
        _connectedObject->handleDisconnectCallback(*this);
        _connectedObject = nullptr;
    }

    return (0);
}

/// @brief Sets the file descriptor to non-blocking mode.
/// @return 0 on success, -1 on failure.
int FD::setNonBlocking() {
    if (isValidFd()) {
        int flags = fcntl(_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
            return -1;
    } else
        ERROR("Cannot set non-blocking mode on an invalid file descriptor: " << _fd);
    return 0;
}

/// @brief Connects the file descriptor to an epoll instance.
/// @param epoll_fd The file descriptor of the epoll instance to connect to.
/// @param events The events to monitor for the file descriptor (default is `DEFAULT_EPOLLIN_EVENTS`).
/// @return 0 on success, -1 on failure.
int FD::connectToEpoll(int epoll_fd, uint32_t events) {
    if (!isValidFd()) {
        ERROR("Cannot connect to epoll with an invalid file descriptor: " << _fd);
        return -1;
    }

    if (isConnectedToEpoll()) {
        ERROR("Epoll already connected to fd: " << _fd);
        return -1;
    }

    epoll_event event{};
    event.events = events;
    event.data.fd = _fd;

    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd, &event);
    if (ret != -1) _epollFd = epoll_fd;
    return (ret);
}

/// @brief Disconnects the file descriptor from the epoll instance.
/// @return 0 on success, -1 on failure.
int FD::disconnectFromEpoll() {
    if (!isConnectedToEpoll()) {
        ERROR("Epoll not connected for fd: " << _fd);
        return -1;
    }

    int result = epoll_ctl(_epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    if (result == -1) {
        ERROR("Failed to disconnect fd: " << _fd << " from epoll, errno: " << errno << " (" << strerror(errno) << ")");
        return -1;
    }

    DEBUG("Disconnected fd: " << _fd << " from epoll");
    _epollFd = -1;
    return 0;
}

/// @brief Sets the epoll events for the file descriptor.
/// @param events The events to set for the file descriptor (e.g., `EPOLLIN`, `EPOLLOUT`, etc.).
/// @return 0 on success, -1 on failure.
int FD::setEpollEvents(uint32_t events) {
    if (!isConnectedToEpoll()) {
        ERROR("Epoll not connected for fd: " << _fd);
        return -1;
    }

    epoll_event event{};
    event.events = events;
    event.data.fd = _fd;
    int result = epoll_ctl(_epollFd, EPOLL_CTL_MOD, _fd, &event);
    if (result == -1) {
        ERROR("Failed to set epoll events for fd: " << _fd << ", errno: " << errno << " (" << strerror(errno) << ")");
        return -1;
    }

    DEBUG("Set epoll events for fd: " << _fd << ", events: " << events);
    _epollEvents = events;
    return 0;
}

/// @brief Reads data from the file descriptor into the internal read buffer.
/// @return The number of bytes read, or -1 on error, or 0 if the connection is closed.
int FD::_readToBuffer() {
    char buffer[READ_BUFFER_SIZE];
    ssize_t totalBytesRead = 0;
    ssize_t bytesRead;

    do {
        bytesRead = ::read(_fd, buffer, sizeof(buffer) - 1);

        if (bytesRead <= 0) return (bytesRead);

        buffer[bytesRead] = '\0';
        readBuffer.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
    } while (bytesRead > 0);

    return (totalBytesRead);
}

/// @brief Receives data from the file descriptor into the internal read buffer.
/// @return The number of bytes received, or -1 on error, or 0 if the connection is closed.
int FD::_recvToBuffer() {
    char buffer[READ_BUFFER_SIZE];
    ssize_t totalBytesRead = 0;
    ssize_t bytesRead;

    do {
        bytesRead = ::recv(_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead == 0) return (0);
        if (bytesRead < 0) return (-1);

        buffer[bytesRead] = '\0';
        readBuffer.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
    } while (bytesRead > 0);

    return (totalBytesRead);
}

/// @brief Writes data from the internal write buffer to the file descriptor.
/// @return The number of bytes written, or -1 on error, or 0 if there is no data to write.
int FD::_writeFromBuffer() {
    if (writeBuffer.empty()) {
        DEBUG("No data to write from buffer for fd: " << _fd);
        return (0);
    }

    ssize_t bytesWritten = ::write(_fd, writeBuffer.c_str(), writeBuffer.size());
    if (bytesWritten < 0) {
        ERROR("Failed to write to fd: " << _fd << ", errno: " << errno << " (" << strerror(errno) << ")");
        return (-1);
    }

    DEBUG("Wrote " << bytesWritten << " bytes to fd: " << _fd);
    writeBuffer.erase(0, bytesWritten);
    return static_cast<int>(bytesWritten);
}

/// @brief Sends data from the internal write buffer to the file descriptor.
/// @return The number of bytes sent, or -1 on error, or 0 if there is no data to send.
int FD::_sendFromBuffer() {
    if (writeBuffer.empty()) {
        DEBUG("No data to send from buffer for fd: " << _fd);
        return (0);
    }

    ssize_t bytesWritten = ::send(_fd, writeBuffer.c_str(), writeBuffer.size(), 0);
    if (bytesWritten < 0) {
        ERROR("Failed to send to fd: " << _fd << ", errno: " << errno << " (" << strerror(errno) << ")");
        return (-1);
    }

    DEBUG("Sent " << bytesWritten << " bytes to fd: " << _fd);
    writeBuffer.erase(0, bytesWritten);
    return static_cast<int>(bytesWritten);
}

/// @brief Writes data to the internal write buffer.
/// @param data The data to write to the buffer.
/// @return The number of bytes written to the buffer, or 0 if no data was provided.
int FD::writeToBuffer(const std::string& data) {
    if (data.empty()) {
        DEBUG("No data to write to buffer for fd: " << _fd);
        return 0;
    }
    writeBuffer.append(data);
    return static_cast<int>(data.size());
}

/// @brief Writes data to the internal write buffer.
/// @param data The data to write to the buffer.
/// @param size The size of the data to write to the buffer.
/// @return The number of bytes written to the buffer, or 0 if no data was provided.
int FD::writeToBuffer(const char* data, size_t size) {
    if (data == nullptr || size == 0) {
        DEBUG("No data to write to buffer for fd: " << _fd);
        return 0;
    }
    writeBuffer.append(data, size);
    return static_cast<int>(size);
}

/// @brief Writes a null-terminated string to the internal write buffer.
/// @param data The null-terminated string to write to the buffer.
/// @return The number of bytes written to the buffer, or 0 if no data was provided.
int FD::writeToBuffer(const char* data) {
    if (data == nullptr) {
        DEBUG("No data to write to buffer for fd: " << _fd);
        return 0;
    }
    writeBuffer.append(data);
    return static_cast<int>(strlen(data));
}

/// @brief Reads data from the file descriptor into the internal read buffer.
/// @return The number of bytes read, or -1 on error, or 0 if the connection is closed.
int FD::read() {
    int ret;

    if (_type == FDType::SOCKET) {
        ret = _recvToBuffer();
    } else {
        ret = _readToBuffer();
    }

    if (_connectedObject && ret != 0)
        _connectedObject->handleReadCallback(*this, ret);

    return (ret);
}

/// @brief Writes data from the internal write buffer to the file descriptor.
/// @return The number of bytes written, or -1 on error, or 0 if there is no data to write.
int FD::write() {
    int ret;

    if (_type == FDType::SOCKET) {
        ret = _sendFromBuffer();
    } else {
        ret = _writeFromBuffer();
    }

    return (ret);
}

/// @brief  Closes the file descriptor and cleans up resources.
/// @details This function disconnects the file descriptor from epoll if connected, and closes the file descriptor. Triggers the disconnect callback if set.
/// @return  0 on success, -1 on failure.
int FD::close() {
    return _cleanUp();
}

/// @brief Triggers the write callback for the connected object, if any.
void FD::triggerWriteCallback() {
    if (_connectedObject) {
        DEBUG("Triggering write callback for fd: " << _fd);
        _connectedObject->handleWriteCallback(*this);
    } else {
        DEBUG("No connected object to trigger write callback for fd: " << _fd);
    }
}

/// @brief Creates an FD object from the read end of a pipe; closes the write end.
/// @param pipe The pipe file descriptors (pipe[0] is read end).
FD FD::fromPipeReadEnd(int pipe[2], std::shared_ptr<BaseHandlerObject> connectedObject) {
    if (pipe[0] == -1) {
        ERROR("Invalid pipe read end file descriptor: " << pipe[0]);
        return FD(-1);
    }
    ::close(pipe[1]);
    return FD(pipe[0], FDType::DEFAULT, connectedObject);
}

/// @brief Creates an FD object from the write end of a pipe; closes the read end.
/// @param pipe The pipe file descriptors (pipe[1] is write end).
FD FD::fromPipeWriteEnd(int pipe[2], std::shared_ptr<BaseHandlerObject> connectedObject) {
    if (pipe[1] == -1) {
        ERROR("Invalid pipe write end file descriptor: " << pipe[1]);
        return FD(-1);
    }
    ::close(pipe[0]);
    return FD(pipe[1], FDType::DEFAULT, connectedObject);
}

/// @brief Creates an FD object from a socket file descriptor.
FD FD::socket(int fd, std::shared_ptr<BaseHandlerObject> connectedObject) {
    return FD(fd, FDType::SOCKET, std::move(connectedObject));
}

/// @brief Creates an FD object from a file descriptor.
FD FD::file(int fd, std::shared_ptr<BaseHandlerObject> connectedObject) {
    return FD(fd, FDType::DEFAULT, std::move(connectedObject));
}
