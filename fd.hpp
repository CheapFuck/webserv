#pragma once

#include <sys/epoll.h>
#include <functional>
#include <string>
#include <memory>

#define DEFAULT_EPOLLIN_EVENTS (EPOLLIN | EPOLLET)
#define DEFAULT_EPOLLOUT_EVENTS (EPOLLOUT | EPOLLET)

#define READ_BUFFER_SIZE 4096

class BaseHandlerObject;

enum class FDType {
    SOCKET,
    DEFAULT
};

class FD {
private:
    int _fd;
    int _epollFd;
    uint32_t _epollEvents;
    FDType _type;
    std::shared_ptr<BaseHandlerObject> _connectedObject;

    int _cleanUp();

    int _recvToBuffer();
    int _readToBuffer();
    int _sendFromBuffer();
    int _writeFromBuffer();

public:
    std::string readBuffer;
    std::string writeBuffer;

    FD();
    FD(int fd, FDType type = FDType::DEFAULT, std::shared_ptr<BaseHandlerObject> connectedObject = nullptr);
    FD(const FD& other) = default;
    FD& operator=(const FD& other) = default;
    ~FD();

    bool operator<(const FD& other) const;
    bool operator<(int otherFd) const;

    int setNonBlocking();

    int connectToEpoll(int epollFd, uint32_t events = DEFAULT_EPOLLIN_EVENTS);
    int setEpollEvents(uint32_t events);
    int disconnectFromEpoll();

    int writeToBuffer(const std::string& data);
    int writeToBuffer(const char* data, size_t size);
    int writeToBuffer(const char *data);
    int read();
    int write();
    int close();

    void triggerWriteCallback();

    static FD fromPipeReadEnd(int pipe[2], std::shared_ptr<BaseHandlerObject> connectedObject = nullptr);
    static FD fromPipeWriteEnd(int pipe[2], std::shared_ptr<BaseHandlerObject> connectedObject = nullptr);

    static FD socket(int fd, std::shared_ptr<BaseHandlerObject> connectedObject = nullptr);
    static FD file(int fd, std::shared_ptr<BaseHandlerObject> connectedObject = nullptr);

    /// @brief Set the file descriptor to monitor for input events (EPOLLIN).
    inline int setEpollIn() { return setEpollEvents(DEFAULT_EPOLLIN_EVENTS); }

    /// @brief Set the file descriptor to monitor for output events (EPOLLOUT).
    inline int setEpollOut() { return setEpollEvents(DEFAULT_EPOLLOUT_EVENTS); }

    /// @brief Check if the file descriptor is valid (not -1).
    inline explicit operator bool() const { return _fd != -1; }

    /// @brief Implicit conversion to int for easy use with system calls.
    inline operator int() const { return _fd; }

    /// @brief Get the file descriptor value.
    inline int get() const { return _fd; }

    /// @brief Get the epoll file descriptor if connected.
    inline int getEpollFd() const { return _epollFd; }

    /// @brief Check if the file descriptor is connected to an epoll instance.
    inline bool isConnectedToEpoll() const { return _epollFd != -1; }

    /// @brief Check if the file descriptor is valid (open).
    inline bool isValidFd() const { return _fd != -1; }

    /// @brief Get the epoll events currently set for the file descriptor.
    inline uint32_t getEpollEvents() const { return _epollEvents; }
};
