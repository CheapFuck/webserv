#pragma once

#include "session.hpp"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>
#include <map>

constexpr char SESSION_MANAGER_FILE[] = "session_manager.sm";

struct SessionMetaData {
	time_t							lastAccessTime;
	std::shared_ptr<UserSession>	sessionPtr;
};

class UserSessionManager {
private:
	time_t _lastCleanupTime;
    std::string _storagePath;
	std::map<std::string, SessionMetaData> _currentSessions;

	std::shared_ptr<UserSession> _loadSession(const std::string &sessionId) const;
	bool _deleteSession(std::map<std::string, SessionMetaData>::iterator it);

public:
    UserSessionManager(const std::string &storagePath);
    UserSessionManager(const UserSessionManager &other) = default;
    UserSessionManager &operator=(const UserSessionManager &other) = default;
    ~UserSessionManager() = default;

	std::shared_ptr<UserSession> createNewSession();
	std::shared_ptr<UserSession> getOrCreateNewSession(const std::string &sessionId);
	
	bool storeSession(const UserSession &session) const;
	
	bool sessionHasCurrentReferences(const std::string &sessionId) const;
	void cleanUpExpiredSessions();
	void shutdown();
};
