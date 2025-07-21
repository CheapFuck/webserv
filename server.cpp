#include "config/rules/rules.hpp"
#include "sessionManager.hpp"
#include "request.hpp"
#include "cookie.hpp"
#include "server.hpp"
#include "print.hpp"
#include "timer.hpp"
#include "Utils.hpp"
#include "fd.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <filesystem>
#include <exception>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <memory>

typedef struct sockaddr_in sockaddr_in;
typedef struct epoll_event epoll_event;

class ServerCreationException : public std::exception {
private:
    std::string _message;

public:
    ServerCreationException(const std::string &message) : _message(message) {}
    const char *what() const noexcept override {
        return _message.c_str();
    }
};

class ClientConnectionException : public std::exception {
private:
    std::string _message;

public:
    ClientConnectionException(const std::string &message) : _message(message) {}
    const char *what() const noexcept override {
        return _message.c_str();
    }
};

Server::Server(const std::map<int, std::vector<ServerConfig>> &configs) :
    _portToConfigs(),
    _sessionManager("sessions"),
    _server_fd(-1),
    _epoll_fd(-1),
    _timer(),
    _clientDescriptors(),
    _serverAddress(),
    _serverExecutablePath(std::filesystem::current_path().string())
{
    try {
        for (const auto &pair : configs)
            this->_setupSocket(pair.first, pair.second);
        this->_setupEpoll();
    } catch (const ServerCreationException &e) {
        for (const auto &pair : _portToConfigs) {
            close(pair.first);
        }
        throw;
    }

    _timer.addEvent(std::chrono::seconds(SESSION_CLEANUP_INTERVAL), [this]() {
        _sessionManager.cleanUpExpiredSessions();
    }, true);
}

Server::Server(const Server &other) :
    _portToConfigs(other._portToConfigs),
    _sessionManager(other._sessionManager),
    _server_fd(other._server_fd),
    _epoll_fd(other._epoll_fd),
    _timer(other._timer),
    _clientDescriptors(other._clientDescriptors),
    _serverAddress(other._serverAddress),
    _serverExecutablePath(other._serverExecutablePath) {}

Server &Server::operator=(const Server &other) {
    if (this != &other) {
        _portToConfigs = other._portToConfigs;
        _sessionManager = other._sessionManager;
        _server_fd = other._server_fd;
        _epoll_fd = other._epoll_fd;
        _timer = other._timer;
        _clientDescriptors = other._clientDescriptors;
        _serverAddress = other._serverAddress;
        _serverExecutablePath = other._serverExecutablePath;
    }
    return *this;
}

Server::~Server() {
    DEBUG("Server destructor called, cleaning up resources");
    _timer.clear();
}

/// @brief Clean up the server resources, closing sockets and cleaning up sessions.
void Server::cleanUp() {
    _clientDescriptors.clear();
    _readableDescriptors.clear();
    _writableDescriptors.clear();

    for (const auto &[serverFd, configs] : _portToConfigs) {
        if (serverFd == -1) continue;
        close(serverFd);
    }

    if (_epoll_fd != -1)
        close(_epoll_fd);

    _sessionManager.shutdown();
}

/// @brief Set up the server socket
/// @throws ServerCreationException if socket creation, binding, or listening fails
void Server::_setupSocket(int listenPort, const std::vector<ServerConfig> &configs) {
	int serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (serverFd == -1)
		throw ServerCreationException("Failed to create socket");

	int enable = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		throw ServerCreationException("Failed to set socket options");

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(listenPort);

	if (bind(serverFd, (sockaddr *)&address, sizeof(address)) == -1)
		throw ServerCreationException("Failed to bind socket");

	if (listen(serverFd, SOMAXCONN) == -1)
		throw ServerCreationException("Failed to listen on socket");

    _portToConfigs[serverFd] = configs;
    _serverAddress = inet_ntoa(address.sin_addr);

    PRINT("Server " << configs[0].serverName.getServerName() << " is listening on port " << listenPort);
}

/// @brief Set up the epoll instance and add the server socket to it
/// @throws ServerCreationException if epoll creation or adding the server socket fails
void Server::_setupEpoll() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
		throw ServerCreationException("Failed to create epoll instance");

    for (const auto &[serverFd, configs] : _portToConfigs) {
		epoll_event event{};
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = serverFd;

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, serverFd, &event) == -1) {
			perror("epoll_ctl");
			throw ServerCreationException("Failed to add socket to epoll");
		}
    }
}

/// @brief Handle a new client connection by accepting it and adding it to the epoll instance.
/// @throws ClientConnectionException if accepting the connection or setting it to non-blocking fails
void Server::_handleNewConnection(int sourceFd) {
    sockaddr_in client_address{};
    socklen_t client_len = sizeof(client_address);

    while (true) {
        int fd = accept(sourceFd, (sockaddr *)&client_address, &client_len);
        SocketFD clientFD(fd, DEFAULT_MAX_BUFFER_SIZE);
        if (!clientFD)
        return ;
        
        if (clientFD.setNonBlocking() == -1) {
            clientFD.close();
            return ;
        }
        
        if (clientFD.connectToEpoll(_epoll_fd, DEFAULT_EPOLLIN_EVENTS) == -1) {
            clientFD.close();
            return ;
        }

        std::shared_ptr<Client> client = std::make_shared<Client>(*this, sourceFd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        auto it = _clientDescriptors.emplace(clientFD, ServerClientInfo(std::move(clientFD), client));
        if (it.second == false) {
            clientFD.close();
            return ;
        }

        DEBUG("New client connected: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port));
        DEBUG("Client FD: " << it.first->second.fd.get() << ", Server FD: " << sourceFd);
    }
}

/// @brief Fetch the server configuration for a request based on the Host header
/// @param request The Request object containing the headers
/// @return A reference to the ServerConfig object that matches the Host header | 
/// or the first config if no direct match was found
ServerConfig &Server::loadRequestConfig(Request &request, int serverFd) {
    ServerConfig *defaultConfig = nullptr;

	for (ServerConfig &config : _portToConfigs[serverFd]) {
		if (config.serverName.getServerName() == request.headers.getHeader(HeaderKey::Host, "")) {
			DEBUG("Found matching server for request: " << config.serverName.getServerName());
			return (config);
		}
        if (config.port.isDefault())
            defaultConfig = &config;
	}

    if (defaultConfig) {
        DEBUG("No matching server found for request, using default: " << defaultConfig->serverName.getServerName());
        return *defaultConfig;
    }

	DEBUG("No matching server found for request, using default: " << _portToConfigs[serverFd][0].serverName.getServerName());
	return _portToConfigs[serverFd][0];
}

/// @brief Fetch the user session for a request.
/// @param request The Request object containing the session cookie
/// @param response The Response object to set the session cookie if a new session is created
/// @return A shared pointer to the SessionMetaData object associated with the user session.
std::shared_ptr<SessionMetaData> Server::fetchUserSession(Request &request, Response &response) {
    const Cookie *sessionCookie = Cookie::getCookie(request, SESSION_COOKIE_NAME);
    if (!sessionCookie) {
        DEBUG("No session cookie found in request, creating new session");
        request.session = _sessionManager.createNewSession();
        if (!request.session) {
            ERROR("Failed to create new session, returning null");
            return (request.session);
        }
        response.headers.add(HeaderKey::SetCookie, Cookie::createSessionCookie(request.session->sessionId).getHeaderInitializationString());
        return (request.session);
    } else {
        DEBUG("Session cookie found: " << sessionCookie->getValue());
        request.session = _sessionManager.getOrCreateNewSession(sessionCookie->getValue());
        return (request.session);
    }
}

void Server::trackCallbackFD(ReadableFD &fd, std::function<void(ReadableFD&, short)> callback) {
    _readableDescriptors[fd.get()] = FDEvent<ReadableFD>{fd, callback};
    DEBUG("Tracking ReadableFD: " << fd.get());
}

void Server::trackCallbackFD(WritableFD &fd, std::function<void(WritableFD&, short)> callback) {
    _writableDescriptors[fd.get()] = FDEvent<WritableFD>{fd, callback};
    DEBUG("Tracking WritableFD: " << fd.get());
}

void Server::untrackCallbackFD(int fd) {
    auto readableIt = _readableDescriptors.find(fd);
    if (readableIt != _readableDescriptors.end()) {
        _readableDescriptors.erase(readableIt);
        DEBUG("Untracked ReadableFD: " << fd);
        return ;
    }

    auto writableIt = _writableDescriptors.find(fd);
    if (writableIt != _writableDescriptors.end()) {
        _writableDescriptors.erase(writableIt);
        DEBUG("Untracked WritableFD: " << fd);
        return ;
    }

    ERROR("Failed to untrack FD: " << fd << ", not found in tracked descriptors");
}

/// @brief Untrack and close a file descriptor.
/// @details This function removes the file descriptor from the server's descriptor map and closes it.
/// @param fd The file descriptor to untrack and close.
void Server::untrackClient(int fd) {
    auto it = _clientDescriptors.find(fd);
    if (it != _clientDescriptors.end()) {
        it->second.fd.close();
        _clientDescriptors.erase(it);
        DEBUG("Untracked and closed descriptor for fd: " << fd);
    }
    else
        DEBUG("No descriptor found for fd: " << fd << ", nothing to untrack");
}

void Server::_handleClientFD(ServerClientInfo &clientInfo, short revents) {
    DEBUG("Handling client FD: " << clientInfo.fd.get());

    if (revents & (EPOLLERR | EPOLLHUP)) {
        DEBUG("Client disconnected or error occurred, closing FD: " << clientInfo.fd.get());
        clientInfo.fd.disconnectFromEpoll();
        _clientDescriptors.erase(clientInfo.fd.get());
        return ;
    }

    if (revents & EPOLLIN) {
        clientInfo.fd.setWriterFDState(FDState::OtherFunctionality);
        ssize_t readerRet = clientInfo.fd.read();

        if (clientInfo.fd.getReaderFDState() == FDState::Closed) {
            DEBUG("Client disconnected: " << clientInfo.fd.get());
            clientInfo.fd.disconnectFromEpoll();
            _clientDescriptors.erase(clientInfo.fd);
            return ;
        }

        clientInfo.client.get()->handleRead(clientInfo.fd, readerRet);
        return ;
    }

    if (revents & EPOLLOUT) {
        clientInfo.client.get()->handleWrite(clientInfo.fd);
    }
}

/// @brief Entry point for the server, running it's event loop once.
void Server::runOnce() {
    epoll_event events[EPOLL_MAX_EVENTS];

    int event_count = epoll_wait(_epoll_fd, events, EPOLL_MAX_EVENTS, std::min(1000, _timer.getNextEventTimeoutMS()));
    if (event_count == -1) {
        ERROR("epoll_wait failed: " << strerror(errno));
        return ;
    }

    for (int i = 0; i < event_count; ++i) {
        int fd = events[i].data.fd;
        DEBUG_IF(events[i].events & EPOLLIN, "EPOLLIN event detected for fd: " << fd);
        DEBUG_IF(events[i].events & EPOLLOUT, "EPOLLOUT event detected for fd: " << fd);
        DEBUG_IF(events[i].events & EPOLLERR, "EPOLLERR event detected for fd: " << fd);
        DEBUG_IF(events[i].events & EPOLLHUP, "EPOLLHUP event detected for fd: " << fd);
        DEBUG_IF_NOT(events[i].events & (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP), "Unexpected event for fd: " << fd);

        auto it = _portToConfigs.find(fd);
        if (it != _portToConfigs.end()) {
            DEBUG("New connection on listening socket fd: " << fd);
            _handleNewConnection(it->first);
            continue ;
        }

        auto clientIt = _clientDescriptors.find(fd);
        if (clientIt != _clientDescriptors.end()) {
            _handleClientFD(clientIt->second, events[i].events);
            continue ;
        }

        auto readableIt = _readableDescriptors.find(fd);
        if (readableIt != _readableDescriptors.end()) {
            DEBUG("Handling ReadableFD: " << fd);
            readableIt->second.callback(readableIt->second.fd, events[i].events);
            continue ;
        }

        auto writableIt = _writableDescriptors.find(fd);
        if (writableIt != _writableDescriptors.end()) {
            DEBUG("Handling WritableFD: " << fd);
            writableIt->second.callback(writableIt->second.fd, events[i].events);
            continue ;
        }

        ERROR("No handler found for fd: " << fd << ", ignoring event (should never happen though :eyes:)");
    }

    _timer.processEvents();
}

// Helper to check if a file descriptor is valid (open)
bool is_fd_valid(int fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}