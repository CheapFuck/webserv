#include "session.hpp"
#include "Utils.hpp"
#include "print.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <string>

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


/// @brief Generates a storable string representation of the UserSession.
std::string UserSession::generateStorableString() const {
	std::ostringstream oss;
	oss << _sessionId << "|" << _lastAccessTime;

	for (const auto &pair : _data) {
		oss << "|" << pair.first.length() << ";" << pair.first << pair.second.length() << ";" << pair.second;
	}

	return oss.str();
}

/// @brief Creates a UserSession from a storable string representation.
UserSession *UserSession::fromStorableString(const std::string &data) {
	std::istringstream iss(data);
	std::string sessionId;
	time_t lastAccessTime;
	std::map<std::string, std::string> sessionData;

	try {
		if (!std::getline(iss, sessionId, '|'))
			throw std::runtime_error("Invalid storable string format");
		std::string lastAccessTimeStr;
		if (!std::getline(iss, lastAccessTimeStr, '|'))
			throw std::runtime_error("Invalid storable string format");
		lastAccessTime = static_cast<time_t>(std::stoul(lastAccessTimeStr));

		std::string keyValuePair;
		while (std::getline(iss, keyValuePair, '|') && !keyValuePair.empty()) {
			size_t semicolonKeyPos = keyValuePair.find(';');
			if (semicolonKeyPos == std::string::npos)
				throw std::runtime_error("Invalid session format");

			size_t keyLen = std::stoul(keyValuePair);
			size_t keyEndIdx = semicolonKeyPos + keyLen;
			size_t valueLen = std::stoul(keyValuePair.c_str() + keyEndIdx + 1);
			size_t semicolonValuePos = keyValuePair.find(';', keyEndIdx + 1);
			if (semicolonValuePos == std::string::npos || keyValuePair.length() != semicolonValuePos + valueLen + 1)
				throw std::runtime_error("Invalid session format");
			
			std::string key = keyValuePair.substr(semicolonKeyPos + 1, keyLen);
			std::string value = keyValuePair.substr(semicolonValuePos + 1, valueLen);
			sessionData[key] = value;
		}
	} catch (...) {
		return (nullptr);
	}

	return new UserSession(lastAccessTime, sessionId, sessionData);
}

/// @brief Creates a new UserSession with a unique session ID.
/// @return A UserSession object initialized with a unique session ID.
/// @throws std::runtime_error if a unique session ID cannot be generated after multiple attempts.
UserSession *UserSessionManager::createNewSession() {
	return (nullptr);
	size_t attempts = 0;

	do {
		std::string sessionId = generateSessionId();
		if (_sessions.find(sessionId) == _sessions.end()) {
			_sessions[sessionId] = new UserSession(sessionId);
			return _sessions[sessionId];
		}
	} while (++attempts < 100);

	return (nullptr);
}

/// @brief Gets or creates a UserSession based on the provided session ID.
/// @param sessionId The session ID to look for or create a new session with.
/// @return A reference to the UserSession associated with the given session ID.
UserSession *UserSessionManager::getOrCreateSession(const std::string &sessionId) {
	return (nullptr);
	auto it = _sessions.find(sessionId);
	if (it != _sessions.end())
		return _sessions[sessionId];

	_sessions[sessionId] = new UserSession(sessionId);
	return _sessions[sessionId];
}

/// @brief Cleans up expired sessions from the session manager.
void UserSessionManager::cleanUpExpiredSessions() {
    time_t currentTime = time(nullptr);

    for (auto it = _sessions.begin(); it != _sessions.end(); ) {
        if (it->second->getLastAccessTime() + SESSION_COOKIE_MAX_AGE < currentTime) {
            delete it->second;
            it = _sessions.erase(it);  // erase returns the next valid iterator
        } else {
            ++it;
        }
    }
}

/// @brief Cleans up all sessions in the session manager and saves them to a file for the specified port.
void UserSessionManager::fullCleanup(int port) {
	std::ofstream sessionFile(SESSION_STORAGE_FOLDER + std::to_string(port));
	if (!sessionFile.is_open()) {
		ERROR("Failed to open session storage file for port " << port);
		ERROR("No sessions will be stored for this port");
	}

	for (auto &pair : _sessions) {
		if (sessionFile.is_open()) {
			std::string storableString = pair.second->generateStorableString();
			sessionFile << storableString << std::endl;
		}
		delete pair.second;
	}
	_sessions.clear();
	sessionFile.close();
	DEBUG("Cleaned up all sessions and saved to port " << port);
}

/// @brief Loads sessions from a file for the specified port.
void UserSessionManager::loadFromPort(int port) {
	std::ifstream sessionFile(SESSION_STORAGE_FOLDER + std::to_string(port));
	if (!sessionFile.is_open()) {
		DEBUG("Failed to open session storage file for port " << port);
		DEBUG("Defaulting with a clean session manager");
		return;
	}

	std::string line;
	while (std::getline(sessionFile, line)) {
		UserSession *session = UserSession::fromStorableString(line);
		if (session)
			_sessions[session->getSessionId()] = session;
		else
			ERROR("Failed to parse session from line: " << line);
	}
	sessionFile.close();
	DEBUG("Loaded " << _sessions.size() << " sessions from port " << port);
}
