#pragma once

#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>
#include <map>

#define SESSION_ID_LENGTH 32
#define SESSION_MAX_CACHE_AGE 60 // 5 minutes
#define SESSION_MAX_STORAGE_AGE 120 // 7 days
#define SESSION_CLEANUP_INTERVAL 10 // seconds
#define SESSION_COOKIE_NAME "webservSessionId"

#define BUFFER_INSERT(buffer, data) \
	buffer.insert(buffer.end(), reinterpret_cast<const char *>(&data), reinterpret_cast<const char *>(&data) + sizeof(data))
#define BUFFER_READ(buffer, offset, type) \
	*reinterpret_cast<const type *>(buffer.data() + offset); \
	offset += sizeof(type)

constexpr char MAGIC[] = "2r66XJwfcQSTkl";
constexpr uint32_t VERSION = 1;
constexpr size_t MAX_SESSION_KEY_SIZE = 1024;
constexpr size_t MAX_SESSION_VALUE_SIZE = 1024 * 1024;

class UserSession {
private:
    time_t _lastAccessTime;
    std::string _sessionId;
    std::map<std::string, std::string> _data;

public:
    UserSession() = default;
    UserSession(const std::string &sessionId);
    UserSession(time_t lastAccessTime, const std::string &sessionId, const std::map<std::string, std::string> &data);
    UserSession(const UserSession &other) = default;
    UserSession &operator=(const UserSession &other) = default;
    ~UserSession() = default;

    inline const std::string &getSessionId() const { return _sessionId; }
    inline const std::map<std::string, std::string> &getData() const { return _data; }
    const std::string &getData(const std::string &key, const std::string &defaultValue = "") const;
    inline void setData(const std::string &key, const std::string &value) { _data[key] = value; }
    inline void updateLastAccessTime() { _lastAccessTime = time(nullptr); }
    inline time_t getLastAccessTime() const { return _lastAccessTime; }

	bool store(const std::string &storageFile) const;
	static std::shared_ptr<UserSession> load(const std::string &sessionId, const std::string &sourceFile);
};
