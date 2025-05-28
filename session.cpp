#include "session.hpp"
#include "print.hpp"

#include <iostream>
#include <random>

/// @brief Defines a dummy object which can be used as a default session.
UserSession UserSession::defaultUserSession = UserSession("default_session");

/// @brief Generates a random session ID consisting of alphanumeric characters.
/// @return A string representing the session ID, which is 32 characters long.
static std::string generateSessionId() {
	std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::string sessionId;
	sessionId.reserve(SESSION_ID_LENGTH);

	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<> distribution(0, chars.size() - 1);

	for (size_t i = 0; i < SESSION_ID_LENGTH; ++i)
		sessionId += chars[distribution(generator)];

	return sessionId;
}

UserSession::UserSession(const std::string &sessionId) : _sessionId(sessionId) {}

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

/// @brief Creates a new UserSession with a unique session ID.
/// @return A UserSession object initialized with a unique session ID.
/// @throws std::runtime_error if a unique session ID cannot be generated after multiple attempts.
UserSession *UserSessionManager::createNewSession() {
	size_t attempts = 0;

	do {
		std::string sessionId = generateSessionId();
		if (_sessions.find(sessionId) == _sessions.end())
			return new UserSession(sessionId);
	} while (++attempts < 100);

	return (nullptr);
}

/// @brief Gets or creates a UserSession based on the provided session ID.
/// @param sessionId The session ID to look for or create a new session with.
/// @return A reference to the UserSession associated with the given session ID.
UserSession *UserSessionManager::getOrCreateSession(const std::string &sessionId) {
	auto it = _sessions.find(sessionId);
	if (it != _sessions.end())
		return _sessions[sessionId];

	_sessions[sessionId] = new UserSession(sessionId);
	return _sessions[sessionId];
}

/// @brief Cleans up expired sessions from the session manager.
void UserSessionManager::cleanUpExpiredSessions() {
	time_t currentTime = time(nullptr);

	for (auto it = _sessions.begin(); it != _sessions.end(); ++it) {
		if (it->second->getLastAccessTime() + SESSION_COOKIE_MAX_AGE < currentTime) {
			delete it->second;
			_sessions.erase(it);
		}
	}
}

/// @brief Cleans up all sessions in the session manager.
void UserSessionManager::fullCleanup() {
	for (auto &pair : _sessions) {
		delete pair.second;
	}
	_sessions.clear();
}
