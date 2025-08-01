#include "fd.hpp"

#include <functional>

#ifndef DEFAULT_CHUNK_SIZE
# define DEFAULT_CHUNK_SIZE 1024 * 8
#endif

template <typename From, typename To>
class BodyWriter {
    static_assert(std::is_base_of<FDReader, From>::value, "From must inherit from ReadableFD");
    static_assert(std::is_base_of<FDWriter, To>::value, "To must inherit from WritableFD");

private:
    std::string _failedBuffer;

    ssize_t _safeWriteAsHTTPChunk(To &to, const std::string &data) {
        std::stringstream chunkStream;

        chunkStream << std::hex << data.size() << "\r\n";
        chunkStream << data << "\r\n";

        return (_safeWrite(to, chunkStream.str()));
    }

    ssize_t _safeWrite(To &to, const std::string &data) {
        if (data.empty())
            return (0);

        ssize_t bytesWritten = to.writeAsString(data);
        if (bytesWritten < 0) {
            _failedBuffer = data;
            return -1;
        }

        if (static_cast<size_t>(bytesWritten) < data.size()) {
            _failedBuffer = data.substr(bytesWritten);
			amountOfBytesWritten += bytesWritten;
            return (bytesWritten);
        }

        _failedBuffer.clear();
		amountOfBytesWritten += bytesWritten;
        return (bytesWritten);
    }

public:
    BodyWriter() = default;
    BodyWriter(const BodyWriter &other) = default;
    BodyWriter& operator=(const BodyWriter &other) = default;
    ~BodyWriter() = default;

	ssize_t amountOfBytesWritten;

    ssize_t sendBodyAsHTTPChunk(From &from, To &to) {
        if (!_failedBuffer.empty())
            return (_safeWrite(to, _failedBuffer));
        
        size_t bytesToSend = std::min(from.getReadBufferSize(), static_cast<size_t>(DEFAULT_CHUNK_SIZE));
        if (bytesToSend == 0)
            return (0);

        return (_safeWriteAsHTTPChunk(to, from.extractChunkFromReadBuffer(bytesToSend)));
    }

    ssize_t sendBodyAsString(From &from, To &to) {
        if (!_failedBuffer.empty())
            return (_safeWrite(to, _failedBuffer));

        size_t bytesToSend = std::min(from.getReadBufferSize(), static_cast<size_t>(DEFAULT_CHUNK_SIZE));
        if (bytesToSend == 0)
            return (0);

        return (_safeWrite(to, from.extractChunkFromReadBuffer(bytesToSend)));
    }

    ssize_t sendBodyAsHTTPChunk(std::string &data, To &to) {
        if (!_failedBuffer.empty())
            return (_safeWrite(to, _failedBuffer));

        size_t bytesToSend = std::min(data.size(), static_cast<size_t>(DEFAULT_CHUNK_SIZE));
        if (bytesToSend == 0)
            return (0);

        std::string toSend = data.substr(0, bytesToSend);
        data.erase(0, bytesToSend);

        return (_safeWriteAsHTTPChunk(to, toSend));
    }

    ssize_t sendBodyAsString(std::string &data, To &to) {
        if (!_failedBuffer.empty())
            return (_safeWrite(to, _failedBuffer));

        size_t bytesToSend = std::min(data.size(), static_cast<size_t>(DEFAULT_CHUNK_SIZE));
        if (bytesToSend == 0)
            return (0);

        std::string toSend = data.substr(0, bytesToSend);
        data.erase(0, bytesToSend);

        return (_safeWrite(to, toSend));
    }

    ssize_t tick(To &to) {
        if (!_failedBuffer.empty())
            return (_safeWrite(to, _failedBuffer));
        return (0);
    }

    bool isEmpty() const {
        return _failedBuffer.empty();
    }
};
