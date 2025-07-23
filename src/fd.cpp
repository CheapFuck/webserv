#include "print.hpp"
#include "fd.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <memory>

FD::FD() : _fd(-1), _epollFd(-1) {}

FD::FD(int fd)
    : _fd(fd), _epollFd(-1) {}

bool FD::operator<(const FD& other) const {
    return _fd < other._fd;
}

bool FD::operator<(int otherFd) const {
    return _fd < otherFd;
}

FD::~FD() {}

int FD::p_cleanUp() {
    if (isConnectedToEpoll())
        ERROR_IF(disconnectFromEpoll() == -1, "Failed to disconnect fd: " << _fd << " from epoll");

    if (isValidFd()) {
        int result = ::close(_fd);
        _fd = -1;
        return (result);
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
int FD::p_connectToEpoll(int epoll_fd, uint32_t events) {
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

    _epollFd = -1;
    return 0;
}

/// @brief Sets the epoll events for the file descriptor.
/// @param events The events to set for the file descriptor (e.g., `EPOLLIN`, `EPOLLOUT`, etc.).
/// @return 0 on success, -1 on failure.
int FD::p_setEpollEvents(uint32_t events) {
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

    return 0;
}

int FD::setEpollEvents(uint32_t events) {
    return p_setEpollEvents(events);
}

int FD::connectToEpoll(int epollFd, uint32_t events) {
    return p_connectToEpoll(epollFd, events);
}

int FD::setEpollReadNotifications() {
    return setEpollEvents(DEFAULT_EPOLLIN_EVENTS);
}

int FD::setEpollWriteNotifications() {
    return setEpollEvents(DEFAULT_EPOLLOUT_EVENTS);
}

int FD::close() {
    return (p_cleanUp());
}

ReadableFD::ReadableFD() 
    : FD(), FDReader() {
}

ReadableFD::ReadableFD(int fd, int maxBufferSize, FDState state)
    : FD(fd), FDReader(fd, maxBufferSize, state) {
}

WritableFD::WritableFD() 
    : FD(), FDWriter() {
}

WritableFD::WritableFD(int fd, FDState state)
    : FD(fd), FDWriter(fd, state) {
}

SocketFD::SocketFD(int fd, int maxReadBufferSize) :
    FD(fd), FDReader(fd, maxReadBufferSize, FDState::OtherFunctionality), FDWriter(fd, FDState::OtherFunctionality) {
}
