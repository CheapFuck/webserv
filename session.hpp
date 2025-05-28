#pragma once

#include <time.h>
#include <string>
#include <map>

#define SESSION_ID_LENGTH 32
#define SESSION_COOKIE_NAME "sessionId"

#define SESSION_COOKIE_MAX_AGE 3600

class UserSession {
private:
time_t _lastAccessTime;
std::string _sessionId;
std::map<std::string, std::string> _data;

public:
	int personal_counter = 0;

	UserSession() = default;
	UserSession(const std::string &sessionId);
	UserSession(const UserSession &other) = default;
	UserSession &operator=(const UserSession &other) = default;
	~UserSession() = default;

	inline const std::string &getSessionId() const { return _sessionId; }
	inline const std::map<std::string, std::string> &getData() const { return _data; }
	const std::string &getData(const std::string &key, const std::string &defaultValue = "") const;
	inline void setData(const std::string &key, const std::string &value) { _data[key] = value; }
	inline void updateLastAccessTime() { _lastAccessTime = time(nullptr); }
	inline time_t getLastAccessTime() const { return _lastAccessTime; }

	static UserSession defaultUserSession;
};

class UserSessionManager {
private:
	std::map<std::string, UserSession*> _sessions;

public:
	UserSessionManager() = default;
	UserSessionManager(const UserSessionManager &other) = default;
	UserSessionManager &operator=(const UserSessionManager &other) = default;
	~UserSessionManager() = default;

	UserSession *getOrCreateSession(const std::string &sessionId);
	UserSession *createNewSession();
	void cleanUpExpiredSessions();
	void fullCleanup();
};