#include "print.hpp"
#include "fd.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <chrono>

FDReader::HTTPChunk::HTTPChunk()
    : data(""), size(FDReader::HTTPChunk::noChunk) {}

FDReader::HTTPChunk::HTTPChunk(std::string data, size_t size)
    : data(std::move(data)), size(size) {}

FDReader::FDReader() 
    : _fd(-1), _maxBufferSize(DEFAULT_MAX_BUFFER_SIZE), _readBuffer(), _state(FDState::Invalid), _lastReadTime(std::chrono::steady_clock::time_point::max()), _totalReadBytes(0), _totalBodyBytes(0), _isLastChunkRead(false) {}

FDReader::FDReader(int fd, int maxBufferSize, FDState state)
    : _fd(fd), _maxBufferSize(maxBufferSize), _readBuffer(), _state(state), _lastReadTime(std::chrono::steady_clock::time_point::max()), _totalReadBytes(0), _totalBodyBytes(0), _isLastChunkRead(false) {}

ssize_t FDReader::read() {
    if (_fd < 0) {
        ERROR("Trying to read from an invalid file descriptor");
        return -1;
    }

    if (_readBuffer.size() + READ_BUFFER_SIZE > _maxBufferSize) {
        DEBUG("Skipping read, buffer size exceeds max buffer size");
        _lastReadTime = std::chrono::steady_clock::now();
        return -1;
    }

    char buffer[READ_BUFFER_SIZE];
    ssize_t bytesRead = ::read(_fd, buffer, sizeof(buffer));
    DEBUG("Bytes read: " << bytesRead << " from fd: " << _fd);

    if (bytesRead > 0) {
        _readBuffer.append(buffer, bytesRead);
        _totalReadBytes += bytesRead;
        _lastReadTime = std::chrono::steady_clock::now();
        DEBUG("Read " << bytesRead << " bytes from fd: " << _fd);
    }
    else if (bytesRead == 0)
        FDReader::_state = FDState::Closed;
    else if (bytesRead < 0)
        FDReader::_state = FDState::Awaiting;

    return (bytesRead);
}

ssize_t FDReader::getTotalReadBytes() const {
    return (_totalReadBytes);
}

ssize_t FDReader::getTotalBodyBytes() const {
    return (_totalBodyBytes);
}

size_t FDReader::getReadBufferSize() const {
    return (_readBuffer.size());
}

void FDReader::clearReadBuffer() {
    _readBuffer.clear();
}

void FDReader::setReaderFDState(FDState state) {
    FDReader::_state = state;
}

bool FDReader::wouldReadExceedMaxBufferSize() const {
    return (_readBuffer.size() + READ_BUFFER_SIZE > _maxBufferSize);
}

bool FDReader::isFinalChunkRead() const {
    return (_isLastChunkRead);
}

FDState FDReader::getReaderFDState() const {
    return (FDReader::_state);
}

const std::string &FDReader::peekReadBuffer() const {
    return (_readBuffer);
}

std::string FDReader::extractHeadersFromReadBuffer() {
    size_t pos = _readBuffer.find("\r\n\r\n");
    if (pos != std::string::npos) {
        std::string headerStr = _readBuffer.substr(0, pos);
        _readBuffer.erase(0, pos + 4);
        return (headerStr);
    }

    return (std::string());
}

FDReader::HTTPChunk FDReader::extractHTTPChunkFromReadBuffer() {
    size_t sizeSepPos = _readBuffer.find("\r\n");
    if (sizeSepPos != std::string::npos) {
        ssize_t chunkSize = 0;
        try { chunkSize = std::stoul(_readBuffer.substr(0, sizeSepPos), nullptr, 16); }
        catch (...) { throw std::runtime_error("Invalid chunk size format in buffer"); }
        size_t minBuffLen = sizeSepPos + 4 + chunkSize;
        if (_readBuffer.size() >= minBuffLen) {
            _readBuffer.erase(0, sizeSepPos + 2);
            std::string chunkData = extractChunkFromReadBuffer(chunkSize);
            _readBuffer.erase(0, 2);
            if (chunkSize == 0)
                _isLastChunkRead = true;
            return HTTPChunk(std::move(chunkData), chunkSize);
        }
    }
    return HTTPChunk("", HTTPChunk::noChunk);
}

FDReader::HTTPChunkStatus FDReader::returnHTTPChunkStatus() const {
    DEBUG("Call the chunk checker; len remaining buff: " << _readBuffer.size());
    size_t sizePos = _readBuffer.find("\r\n");
    ssize_t chunkSize = 0;

    if (_readBuffer.empty())
        return (HTTPChunkStatus::Ok);

    try {   size_t chunkStartPos = 0;
            chunkSize = std::stoul(_readBuffer.substr(chunkStartPos, sizePos), nullptr, 16); }
    catch (...) { return (HTTPChunkStatus::Error); }

    if (chunkSize > MAX_ACCEPT_CHUNK_SIZE)
        return (HTTPChunkStatus::TooLarge);

    size_t minBuffLen = sizePos + 4 + chunkSize;
    if (_readBuffer.size() < minBuffLen)
        return (HTTPChunkStatus::Ok);

    if (_readBuffer.compare(sizePos + 2 + chunkSize, 2, "\r\n") != 0)
        return (HTTPChunkStatus::Error);

    return (HTTPChunkStatus::Ok);
}

std::string FDReader::extractChunkFromReadBuffer(size_t chunkSize) {
    if (chunkSize > _readBuffer.size())
        chunkSize = _readBuffer.size();

    std::string chunk = _readBuffer.substr(0, chunkSize);
    _readBuffer.erase(0, chunkSize);
    _totalBodyBytes += chunkSize;
    DEBUG("Extracted chunk of size: " << chunkSize << " from buffer");
    return (chunk);
}

std::string FDReader::extractFullBuffer() {
    std::string fullBuffer = std::move(_readBuffer);
    _totalReadBytes += fullBuffer.size();
    _readBuffer = std::string();
    return (fullBuffer);
}

ReadableFD ReadableFD::file(int fd, int maxBufferSize) {
    return (ReadableFD(fd, maxBufferSize, FDState::Ready));
}

ReadableFD ReadableFD::pipe(int *fd, int maxBufferSize) {
    ::close(fd[1]);
    return (ReadableFD(fd[0], maxBufferSize, FDState::Awaiting));
}

std::chrono::steady_clock::time_point FDReader::getLastReadTime() const {
    return (_lastReadTime);
}

void FDReader::resetCounter() {
    _totalReadBytes = 0;
    _totalBodyBytes = 0;
    _isLastChunkRead = false;
}




FDWriter::FDWriter() : _fd(-1), _state(FDState::Invalid) {}

FDWriter::FDWriter(int fd, FDState state) : _fd(fd), _state(state) {}

ssize_t FDWriter::writeAsString(const std::string &data) {
    if (_fd < 0) {
        ERROR("Trying to write to an invalid file descriptor");
        return -1;
    }
    ssize_t bytesWritten = ::write(_fd, data.c_str(), data.size());
    if (bytesWritten < 0)
        FDWriter::_state = FDState::Awaiting;
    if (bytesWritten == 0)
        FDWriter::_state = FDState::Closed;

    DEBUG("Wrote " << bytesWritten << " bytes to fd: " << _fd);
    return (bytesWritten);
}

ssize_t FDWriter::writeAsChunk(const std::string &data) {
    if (_fd < 0) {
        ERROR("Trying to write to an invalid file descriptor");
        return -1;
    }

    std::stringstream ss;
    ss << std::hex << data.size();
    ss << "\r\n";
    ss << data;
    ss << "\r\n";

    return (writeAsString(ss.str()));
}

void FDWriter::setWriterFDState(FDState state) {
    FDWriter::_state = state;
    DEBUG("Set FDWriter state to: " << static_cast<int>(_state));
}

FDState FDWriter::getWriterFDState() const {
    return (FDWriter::_state);
}

WritableFD WritableFD::pipe(int *fd) {
    ::close(fd[0]);
    return WritableFD(fd[1], FDState::Awaiting);
}
