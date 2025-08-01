#include "sessionManager.hpp"
#include "print.hpp"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <string>
#include <memory>

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

/// @brief Constructs a UserSessionManager with the specified storage path. Loads existing sessions from the session manager file if it exists.
UserSessionManager::UserSessionManager(const std::string &storagePath) : 
	_lastCleanupTime(0),
	_storagePath(storagePath),
	_absExecutablePath(std::filesystem::current_path().string()),
	_currentSessions()
{
	if (_storagePath.empty() || _storagePath.back() != '/')
		_storagePath += '/';
	_lastCleanupTime = 0;

	try {
		if (!std::filesystem::exists(_storagePath)) {
			std::filesystem::create_directories(_storagePath);
		} else if (!std::filesystem::is_directory(_storagePath)) {
			ERROR("Session storage path exists but is not a directory: " << _storagePath);
		}
	} catch (const std::filesystem::filesystem_error& e) {
		ERROR("Failed to create session storage directory: " << e.what());
	}

	DEBUG("UserSessionManager initialized with storage path: " << _storagePath);

	std::string managerFile = _storagePath + SESSION_MANAGER_FILE;
	std::ifstream inFile(managerFile, std::ios::binary);
	if (inFile.is_open()) {
		std::vector<char> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
		size_t offset = 0;
		while (offset + SESSION_ID_LENGTH + sizeof(time_t) <= buffer.size()) {
			std::string sessionId(buffer.data() + offset, SESSION_ID_LENGTH);
			offset += SESSION_ID_LENGTH;
			time_t lastAccessTime = *reinterpret_cast<const time_t *>(buffer.data() + offset);
			offset += sizeof(time_t);
			_currentSessions[sessionId] = std::make_shared<SessionMetaData>(lastAccessTime, sessionId, getAbsoluteStoragePath(sessionId));
		}
		DEBUG("Loaded " << _currentSessions.size() << " sessions from session manager file: " << managerFile);
		inFile.close();
	} else {
		DEBUG("No session manager file found at " << managerFile << " | starting with empty session map");
	}
}

/// @brief Creates a new UserSession with a unique session ID.
/// @return A UserSession object initialized with a unique session ID.
/// @throws std::runtime_error if a unique session ID cannot be generated after multiple attempts.
std::shared_ptr<SessionMetaData> UserSessionManager::createNewSession() {
	size_t attempts = 0;

	do {
		std::string sessionId = generateSessionId();
		if (_currentSessions.find(sessionId) == _currentSessions.end()) {
			std::shared_ptr<SessionMetaData> newSession = std::make_shared<SessionMetaData>(time(nullptr), sessionId, getAbsoluteStoragePath(sessionId));
			_currentSessions[sessionId] = newSession;
			return (newSession);
		}
	} while (++attempts < 100);

	return (nullptr);
}

/// @brief Gets or creates a UserSession based on the provided session ID.
/// @param sessionId The session ID to look for or create a new session with.
/// @return A reference to the UserSession associated with the given session ID.
std::shared_ptr<SessionMetaData> UserSessionManager::getOrCreateNewSession(const std::string &sessionId) {
	auto it = _currentSessions.find(sessionId);
	if (it != _currentSessions.end()) {
		it->second->lastAccessTime = time(nullptr);
		return (it->second);
	}

	std::shared_ptr<SessionMetaData> newSession = std::make_shared<SessionMetaData>(time(nullptr), sessionId, getAbsoluteStoragePath(sessionId));

	_currentSessions[sessionId] = newSession;
	return (newSession);
}

/// @brief Deletes a UserSession by its session ID.
bool UserSessionManager::_deleteSession(std::map<std::string, std::shared_ptr<SessionMetaData>>::iterator it) {
	if (it == _currentSessions.end()) {
		ERROR("Session " << it->first << " not found in current sessions | aborting deletion");
		return (false);
	}

	if (it->second.use_count() > 0) {
		ERROR("Cannot delete session " << it->first << " with current references");
		return (false);
	}

	std::string storageFile = getAbsoluteStoragePath(it->first);
	if (std::remove(storageFile.c_str()) != 0) {
		ERROR("Failed to delete session file: " << storageFile);
		return (false);
	}

	DEBUG("Session " << it->first << " deleted successfully");
	return (true);
}

/// @brief Checks if a session has any current references.
/// @param sessionId The session ID to check for current references.
/// @return True if the session has current references, false otherwise.
bool UserSessionManager::sessionHasCurrentReferences(const std::string &sessionId) const {
	auto it = _currentSessions.find(sessionId);
	if (it == _currentSessions.end())
		return (false);

	DEBUG("Session " << sessionId << " has " << it->second.use_count() << " current references");
	return (it->second.use_count() > 0);
}

/// @brief Cleans up expired sessions from the session manager.
void UserSessionManager::cleanUpExpiredSessions() {
	time_t currentTime = time(nullptr);
	_lastCleanupTime = currentTime;
	DEBUG("Cleaning up expired sessions");

    for (auto it = _currentSessions.begin(); it != _currentSessions.end(); ) {
		if (it->second.use_count() > 0) {
			++it;
			continue;
		}

        if (it->second->lastAccessTime + SESSION_MAX_STORAGE_AGE < currentTime) {
			DEBUG("Session " << it->first << " has expired and will be removed");
			DEBUG("Session last access time: " << it->second->lastAccessTime << ", current time: " << currentTime);
            DEBUG((it->second->lastAccessTime + SESSION_MAX_STORAGE_AGE) << " < " << currentTime);

            _deleteSession(it);
			it = _currentSessions.erase(it);
		} else {
			++it;
		}
	}
}

/// @brief Cleans up all sessions and releases resources. Stores session metadata to a file for future use.
void UserSessionManager::shutdown() {
	DEBUG("Shutting down session manager and cleaning up all sessions");

	std::string storagePath = _storagePath + SESSION_MANAGER_FILE;
	std::ofstream outFile(storagePath, std::ios::trunc | std::ios::binary);
	if (outFile.is_open()) {
		std::vector<char> buffer;

		for (const auto &pair : _currentSessions) {
			buffer.insert(buffer.end(), pair.first.begin(), pair.first.end());
			BUFFER_INSERT(buffer, pair.second->lastAccessTime);
		}
		outFile.write(buffer.data(), buffer.size());
		outFile.close();
	} else {
		ERROR("Failed to open session manager file for writing: " << storagePath);
	}

	_currentSessions.clear();
}

/// @brief Gets the absolute storage path for a session based on its session ID.
std::string UserSessionManager::getAbsoluteStoragePath(const std::string &sessionId) const {
	if (_storagePath.starts_with("/"))
  		return _storagePath + sessionId + ".json";
	return _absExecutablePath + "/" + _storagePath + sessionId + ".json";
}
