#pragma once

#include <sys/epoll.h>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <memory>

#define DEFAULT_EPOLLIN_EVENTS (EPOLLIN)
#define DEFAULT_EPOLLOUT_EVENTS (EPOLLOUT)

#define DEFAULT_MAX_BUFFER_SIZE (1024 * 256) // 256 KB
#define READ_BUFFER_SIZE (32 * 1024) // 32 KB
// #define READ_BUFFER_SIZE (512) // 512 bytes

enum class FDState {
    /// @brief The file descriptor is in an invalid state (e.g., not initialized).
    Invalid,

    /// @brief The file descriptor is a socket, and is not currently allowing this operation.
    OtherFunctionality,

    /// @brief The file descriptor is ready for reading/writing.
    Ready,

    /// @brief The file descriptor is waiting for the next operation through epoll.
    Awaiting,

    /// @brief The file descriptor is closed, and no further operations can be performed.
    Closed,
};

class FD {
private:
    int _fd;
    int _epollFd;

protected:
    int p_cleanUp();
    int p_setEpollEvents(uint32_t events);
    int p_connectToEpoll(int epollFd, uint32_t events);

public:
    FD();
    FD(int fd);
    FD(const FD& other) = default;
    FD& operator=(const FD& other) = default;
    ~FD();

    bool operator<(const FD& other) const;
    bool operator<(int otherFd) const;

    int setNonBlocking();

    int setEpollEvents(uint32_t events);
    int connectToEpoll(int epollFd, uint32_t events);

    int disconnectFromEpoll();
    int close();

    /// @brief Set the file descriptor to monitor for input events (EPOLLIN).
    int setEpollReadNotifications();

    /// @brief Set the file descriptor to monitor for output events (EPOLLOUT).
    int setEpollWriteNotifications();

    /// @brief Check if the file descriptor is valid (not -1).
    explicit operator bool() const { return _fd != -1; }

    /// @brief Implicit conversion to int for easy use with system calls.
    operator int() const { return _fd; }

    /// @brief Get the file descriptor value.
    int get() const { return _fd; }

    /// @brief Get the epoll file descriptor if connected.
    int getEpollFd() const { return _epollFd; }

    /// @brief Check if the file descriptor is connected to an epoll instance.
    bool isConnectedToEpoll() const { return _epollFd != -1; }

    /// @brief Check if the file descriptor is valid (open).
    bool isValidFd() const { return _fd != -1; }
};

class FDReader {
private:
    int _fd;
    size_t _maxBufferSize;
    ssize_t _totalReadBytes;
    std::string _readBuffer;
    FDState _state;

public:
    struct HTTPChunk {
        std::string data;
        size_t size;

        HTTPChunk(std::string data, size_t size);
        static constexpr size_t noChunk = static_cast<size_t>(-1);
    };

    enum class HTTPChunkStatus {
        Ongoing,
        Complete,
        Error,
    };

    FDReader();
    FDReader(int fd, int maxBufferSize, FDState state);
    FDReader(const FDReader &other) = default;
    FDReader& operator=(const FDReader &other) = default;
    ~FDReader() = default;

    ssize_t read();
    ssize_t getTotalReadBytes() const;
    size_t getReadBufferSize() const;

    void clearReadBuffer();
    void setReaderFDState(FDState state);

    bool wouldReadExceedMaxBufferSize() const;

    FDState getReaderFDState() const;

    std::string extractHeadersFromReadBuffer();
    HTTPChunk extractHTTPChunkFromReadBuffer();
    HTTPChunkStatus returnHTTPChunkStatus() const;
    std::string extractChunkFromReadBuffer(size_t chunkSize);
    std::string extractFullBuffer();

    const std::string &peekReadBuffer() const;
};

class FDWriter {
private:
    int _fd;
    FDState _state;

public:
    FDWriter();
    FDWriter(int fd, FDState state);
    FDWriter(const FDWriter &other) = default;
    FDWriter& operator=(const FDWriter &other) = default;
    ~FDWriter() = default;

    ssize_t writeAsString(const std::string &data);
    ssize_t writeAsChunk(const std::string &data);

    void setWriterFDState(FDState state);

    FDState getWriterFDState() const;
};

class ReadableFD : public FD, public FDReader {
public:
    ReadableFD();
    ReadableFD(int fd, int maxBufferSize, FDState state);
    ReadableFD(const ReadableFD &other) = default;
    ReadableFD& operator=(const ReadableFD &other) = default;
    ~ReadableFD() = default;

    static ReadableFD file(int fd, int maxBufferSize = DEFAULT_MAX_BUFFER_SIZE);
    static ReadableFD pipe(int *fd, int maxBufferSize = DEFAULT_MAX_BUFFER_SIZE);
};

class WritableFD : public FD, public FDWriter {
public:
    WritableFD();
    WritableFD(int fd, FDState state);
    WritableFD(const WritableFD &other) = default;
    WritableFD& operator=(const WritableFD &other) = default;
    ~WritableFD() = default;

    static WritableFD pipe(int *fd);
};

class SocketFD : public FD, public FDReader, public FDWriter {
public:
    SocketFD(int fd, int maxReadBufferSize);
    SocketFD(const SocketFD &other) = default;
    SocketFD& operator=(const SocketFD &other) = default;
    ~SocketFD() = default;
};
