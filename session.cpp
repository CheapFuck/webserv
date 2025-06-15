#include "session.hpp"
#include "print.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>
#include <random>
#include <string>
#include <memory>

/// @brief Calculates the CRC32 checksum of a given buffer.
/// This function uses the zlib library to compute the CRC32 checksum.
/// @param buffer The input buffer for which the CRC32 checksum is to be calculated.
/// @note The buffer should not be empty, and the function assumes that the zlib library is properly linked.
/// @return The CRC32 checksum of the input buffer as a uLong value.
static uLong calculateCRC(const std::vector<char> &buffer) {
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()), buffer.size());
	return (crc);
}

UserSession::UserSession(const std::string &sessionId) : _sessionId(sessionId) {}

UserSession::UserSession(time_t lastAccessTime, const std::string &sessionId, const std::map<std::string, std::string> &data)
	: _lastAccessTime(lastAccessTime), _sessionId(sessionId), _data(data) {}

/// @brief Returns the value associated with the given key in the session data.
/// @param key The key for which the value is requested.
/// @param defaultValue The value to return if the key does not exist in the session data.
/// @return The value associated with the key if it exists, otherwise returns the default value.
const std::string &UserSession::getData(const std::string &key, const std::string &defaultValue) const {
	auto it = _data.find(key);
	if (it != _data.end())
		return it->second;
	return defaultValue;
}

/// @brief Stores the session data to a file with the specified filename.
/// @return True if the session data was successfully stored, false otherwise.
/// @warning This method should not be called directly for automatic file management. Use the SessionManager class API instead.
bool UserSession::store(const std::string &storageFile) const {
	std::ofstream outputStream(storageFile, std::ios::binary);
	if (!outputStream || !outputStream.is_open()) {
		ERROR("Failed to open session file for writing: " << storageFile);
		return false;
	}

	std::vector<char> buffer;
	BUFFER_INSERT(buffer, MAGIC);
	uint32_t version = VERSION;
	BUFFER_INSERT(buffer, version);

	size_t mapSize = _data.size();
	BUFFER_INSERT(buffer, mapSize);
	BUFFER_INSERT(buffer, _lastAccessTime);

	for (const auto & [key, value] : _data) {
		size_t keyLen = key.size();
		size_t valueLen = value.size();
		if (keyLen > MAX_SESSION_KEY_SIZE || valueLen > MAX_SESSION_VALUE_SIZE) {
			ERROR("Key or value size exceeds maximum allowed size");
			continue;
		}

		BUFFER_INSERT(buffer, keyLen);
		BUFFER_INSERT(buffer, valueLen);
		buffer.insert(buffer.end(), key.begin(), key.end());
		buffer.insert(buffer.end(), value.begin(), value.end());
	}

	uLong crc = calculateCRC(buffer);
	DEBUG("Calculated CRC: " << crc);

	outputStream.write(buffer.data(), buffer.size());
	outputStream.write(reinterpret_cast<const char *>(&crc), sizeof(crc));

	if (!outputStream) {
		ERROR("Failed to write session data to file: " << storageFile);
		return false;
	}

	outputStream.close();
	DEBUG("Session data for " << _sessionId << " written successfully");
	return true;
}

/// @brief Loads a UserSession from a file with the given session ID.
/// @param sessionId The session ID to load the session from.
/// @return A pointer to the UserSession object if successful, or nullptr if loading failed.
/// @warning This method should not be called directly for automatic file management. Use the SessionManager class API instead.
std::shared_ptr<UserSession> UserSession::load(const std::string &sessionId, const std::string &sourceFile) {
	std::ifstream inputStream(sourceFile, std::ios::binary | std::ios::ate);
	if (!inputStream || !inputStream.is_open()) {
		ERROR("Failed to open session file for reading: " << sourceFile);
		return nullptr;
	}

	std::streamsize fileSize = inputStream.tellg();
	inputStream.seekg(0, std::ios::beg);

	if (static_cast<unsigned long>(fileSize) < sizeof(MAGIC) + sizeof(VERSION) + sizeof(size_t) + sizeof(time_t)) {
		ERROR("Session file is too small to be valid");
		return nullptr;
	}

	std::vector<char> buffer(fileSize - sizeof(uLong));
	inputStream.read(buffer.data(), buffer.size());

	uLong readCrc;
	inputStream.read(reinterpret_cast<char *>(&readCrc), sizeof(readCrc));
	inputStream.close();

	uLong crc = calculateCRC(buffer);

	if (crc != readCrc) {
		ERROR("CRC mismatch in session data for " << sessionId);
		return (nullptr);
	}

	size_t offset = 0;
	if (std::string(buffer.data(), sizeof(MAGIC) - 1) != MAGIC) {		
		ERROR("Invalid magic number in session data for " << sessionId);
		return (nullptr);
	}

	offset += sizeof(MAGIC);

	uint32_t version;
	BUFFER_READ(buffer, offset, uint32_t, version);
	if (version != VERSION) {
		ERROR("Unsupported session version: " << version << " in session data for " << sessionId);
		return (nullptr);
	}

	size_t mapSize;
	BUFFER_READ(buffer, offset, size_t, mapSize);
	time_t lastAccessTime;
	BUFFER_READ(buffer, offset, time_t, lastAccessTime);

	std::map<std::string, std::string> data;

	for (size_t i = 0; i < mapSize; ++i) {
		if (offset + sizeof(size_t) * 2 > buffer.size()) {
			ERROR("Buffer overflow while reading session data for " << sessionId);
			return (nullptr);
		}

		size_t keyLen;
		BUFFER_READ(buffer, offset, size_t, keyLen);
		size_t valueLen;
		BUFFER_READ(buffer, offset, size_t, valueLen);
		if (offset + keyLen + valueLen > buffer.size()) {
			ERROR("Buffer overflow while reading session data for " << sessionId);
			return (nullptr);
		}

		std::string key(buffer.data() + offset, keyLen);
		offset += keyLen;
		std::string value(buffer.data() + offset, valueLen);
		offset += valueLen;

		data[key] = value;
	}

	DEBUG("Session data for " << sessionId << " loaded successfully");
	return std::make_shared<UserSession>(lastAccessTime, sessionId, data);
}
