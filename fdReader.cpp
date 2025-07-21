#include "print.hpp"
#include "fd.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

FDReader::HTTPChunk::HTTPChunk(std::string data, size_t size)
    : data(std::move(data)), size(size) {
    DEBUG("HTTPChunk created with size: " << size);
}

FDReader::FDReader() 
    : _fd(-1), _maxBufferSize(DEFAULT_MAX_BUFFER_SIZE), _totalReadBytes(0), _readBuffer(), _state(FDState::Invalid) {
    DEBUG("FDReader created with default parameters");
}

FDReader::FDReader(int fd, int maxBufferSize, FDState state)
    : _fd(fd), _maxBufferSize(maxBufferSize), _totalReadBytes(0), _readBuffer(), _state(state) {
    _readBuffer.reserve(_maxBufferSize);
    DEBUG("FDReader created with fd: " << _fd);
}

ssize_t FDReader::read() {
    if (_fd < 0) {
        ERROR("Trying to read from an invalid file descriptor");
        return -1;
    }

    if (_readBuffer.size() + READ_BUFFER_SIZE > _maxBufferSize) {
        DEBUG("Skipping read, buffer size exceeds max buffer size");
        return -1;
    }

    char buffer[READ_BUFFER_SIZE];
    ssize_t bytesRead = ::read(_fd, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        _readBuffer.append(buffer, bytesRead);
        _totalReadBytes += bytesRead;
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

size_t FDReader::getReadBufferSize() const {
    return (_readBuffer.size());
}

void FDReader::clearReadBuffer() {
    DEBUG("Clearing read buffer, previous size: " << _readBuffer.size());
    _readBuffer.clear();
}

void FDReader::setReaderFDState(FDState state) {
    FDReader::_state = state;
    DEBUG("Set FDReader state to: " << static_cast<int>(_state));
}

bool FDReader::wouldReadExceedMaxBufferSize() const {
    return (_readBuffer.size() + READ_BUFFER_SIZE > _maxBufferSize);
}

FDState FDReader::getReaderFDState() const {
    return (FDReader::_state);
}

std::string FDReader::extractHeadersFromReadBuffer() {
    size_t pos = _readBuffer.find("\r\n\r\n");
    if (pos != std::string::npos) {
        std::string headerStr = extractChunkFromReadBuffer(pos);
        extractChunkFromReadBuffer(4);
        return (headerStr);
    }

    return (std::string());
}

FDReader::HTTPChunk FDReader::extractHTTPChunkFromReadBuffer() {
    size_t sizeSepPos = _readBuffer.find("\r\n");
    ssize_t chunkSize = 0;

    if (sizeSepPos != std::string::npos) {
        try { chunkSize = std::stoul(_readBuffer.substr(0, sizeSepPos), nullptr, 16); }
        catch (...) { throw std::runtime_error("Invalid chunk size format in buffer"); }

        size_t minBuffLen = sizeSepPos + 4 + chunkSize;
        if (_readBuffer.size() >= minBuffLen) {
            extractChunkFromReadBuffer(sizeSepPos + 2);
            std::string chunkData = extractChunkFromReadBuffer(chunkSize);
            extractChunkFromReadBuffer(2);

            return HTTPChunk(std::move(chunkData), chunkSize);
        }
    }

    return HTTPChunk("", HTTPChunk::noChunk);
}

std::string FDReader::extractChunkFromReadBuffer(size_t chunkSize) {
    if (chunkSize > _readBuffer.size()) {
        ERROR("Requested chunk size exceeds buffer size");
        return (std::string());
    }

    std::string chunk = _readBuffer.substr(0, chunkSize);
    _readBuffer.erase(0, chunkSize);
    DEBUG("Extracted chunk of size: " << chunkSize << " from buffer");
    return (chunk);
}

std::string FDReader::extractFullBuffer() {
    std::string fullBuffer = std::move(_readBuffer);
    _readBuffer = std::string();
    return (fullBuffer);
}

ReadableFD ReadableFD::file(int fd, int maxBufferSize) {
    return (ReadableFD(fd, maxBufferSize, FDState::Ready));
}

ReadableFD ReadableFD::pipe(int *fd, int maxBufferSize) {
    return (ReadableFD(fd[0], maxBufferSize, FDState::Awaiting));
}




FDWriter::FDWriter() : _fd(-1), _state(FDState::Invalid) {
    DEBUG("FDWriter created with default parameters");
}

FDWriter::FDWriter(int fd, FDState state) : _fd(fd), _state(state) {
    DEBUG("FDWriter created with fd: " << _fd);
}

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
    return WritableFD(fd[1], FDState::Awaiting);
}
