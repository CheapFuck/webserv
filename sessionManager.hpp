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
#define SESSION_MAX_STORAGE_AGE 60 * 60 * 24 // 1 day
#define SESSION_CLEANUP_INTERVAL 60 // 1 minute

#define SESSION_COOKIE_NAME "webservSessionId"

#define BUFFER_INSERT(buffer, data) \
	buffer.insert(buffer.end(), reinterpret_cast<const char *>(&data), reinterpret_cast<const char *>(&data) + sizeof(data))

constexpr char SESSION_MANAGER_FILE[] = "session_manager.sm";

struct SessionMetaData {
	time_t		lastAccessTime;
	std::string	sessionId;
	std::string absoluteFilePath;
	std::string _storagePath;

	SessionMetaData(long t, const std::string& id, const std::string& path)
        : lastAccessTime(t), sessionId(id), _storagePath(path) {}
};

class UserSessionManager {
private:
	time_t _lastCleanupTime;
    std::string _storagePath;
	std::string _absExecutablePath;
	std::map<std::string, std::shared_ptr<SessionMetaData>> _currentSessions;

	bool _deleteSession(std::map<std::string, std::shared_ptr<SessionMetaData>>::iterator it);

public:
    UserSessionManager(const std::string &storagePath);
    UserSessionManager(const UserSessionManager &other) = default;
    UserSessionManager &operator=(const UserSessionManager &other) = default;
    ~UserSessionManager() = default;

	std::shared_ptr<SessionMetaData> createNewSession();
	std::shared_ptr<SessionMetaData> getOrCreateNewSession(const std::string &sessionId);
	
	bool sessionHasCurrentReferences(const std::string &sessionId) const;
	void cleanUpExpiredSessions();
	void shutdown();

	std::string getAbsoluteStoragePath(const std::string &sessionId) const;
};
