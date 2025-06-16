#include "config/rules/rules.hpp"
#include "request.hpp"
#include "session.hpp"
#include "cookie.hpp"
#include "client.hpp"
#include "server.hpp"
#include "print.hpp"
#include "timer.hpp"
#include "CGI.hpp"
#include "Utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
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

Server::Server(const std::vector<ServerConfig> &configs) :
    _configs(configs),
    _sessionManager("sessions" + std::to_string(configs[0].port.get())),
    _server_fd(-1),
    _epoll_fd(-1),
    _timer(),
    _clients()
{
    this->_setupSocket();
    this->_setupEpoll();

    _timer.addEvent(std::chrono::steady_clock::duration(SESSION_CLEANUP_INTERVAL), [this]() {
        _sessionManager.cleanUpExpiredSessions();
    });

    PRINT("Server " << _configs[0].serverName.get() << " is running on port " << _configs[0].port.get());
}

Server::Server(const Server &other) :
    _configs(other._configs),
    _sessionManager(other._sessionManager),
    _server_fd(other._server_fd),
    _epoll_fd(other._epoll_fd),
    _timer(other._timer),
    _clients(other._clients) {}

Server &Server::operator=(const Server &other) {
    if (this != &other) {
        _configs = other._configs;
        _sessionManager = other._sessionManager;
        _server_fd = other._server_fd;
        _epoll_fd = other._epoll_fd;
        _timer = other._timer;
        _clients = other._clients;
    }
    return *this;
}

Server::~Server() {
    _timer.clear();
}

/// @brief Clean up the server resources, closing sockets and cleaning up sessions.
void Server::cleanUp() {
    if (_server_fd != -1)
        close(_server_fd);
    if (_epoll_fd != -1)
        close(_epoll_fd);
    _sessionManager.shutdown();
}

/// @brief Set up the server socket
/// @throws ServerCreationException if socket creation, binding, or listening fails
void Server::_setupSocket() {
	_server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_server_fd == -1)
		throw ServerCreationException("Failed to create socket");

	int enable = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		throw ServerCreationException("Failed to set socket options");

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(_configs[0].port.get());

	if (bind(_server_fd, (sockaddr *)&address, sizeof(address)) == -1)
		throw ServerCreationException("Failed to bind socket");

	if (listen(_server_fd, SOMAXCONN) == -1)
		throw ServerCreationException("Failed to listen on socket");
}

/// @brief Set up the epoll instance and add the server socket to it
/// @throws ServerCreationException if epoll creation or adding the server socket fails
void Server::_setupEpoll() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
		throw ServerCreationException("Failed to create epoll instance");

	epoll_event event{};
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = _server_fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &event) == -1) {
		perror("epoll_ctl");
		throw ServerCreationException("Failed to add socket to epoll");
	}
}

/// @brief Handle a new client connection by accepting it and adding it to the epoll instance.
/// @throws ClientConnectionException if accepting the connection or setting it to non-blocking fails
void Server::_handleNewConnection() {
    sockaddr_in client_address{};
    socklen_t client_len = sizeof(client_address);
    int client_fd = accept(_server_fd, (sockaddr *)&client_address, &client_len);
    if (client_fd == -1)
        throw ClientConnectionException("Failed to accept new connection");
    
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
        close(client_fd);
        throw ClientConnectionException("Failed to set client socket to non-blocking");
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        close(client_fd);
        throw ClientConnectionException("Failed to add client socket to epoll");
    }

    _clients.emplace(client_fd, Client());
    DEBUG("New client connected: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port));
}

/// @brief Modify the epoll events for a given file descriptor.
/// @param fd The file descriptor to modify
/// @param operation The operation to perform (EPOLL_CTL_ADD, EPOLL_CTL_MOD, or EPOLL_CTL_DEL)
/// @param events The events to set for the file descriptor (EPOLLIN, EPOLLOUT, etc.)
/// @return True if the operation was successful, false otherwise
bool Server::_epollExecute(int fd, uint32_t operation, uint32_t events) {
	epoll_event event{};
	event.events = events;
	event.data.fd = fd;
	int result = epoll_ctl(_epoll_fd, operation, fd, &event);
	if (result == -1) {
        if (errno == EBADF) {
            DEBUG("epoll_ctl: fd " << fd << " is already closed (EBADF) during operation " << operation << ", events: " << events);
        } else {
            ERROR("Failed to modify epoll events for fd: " << fd << ", operation: " << operation << ", events: " << events << ", errno: " << errno << " (" << strerror(errno) << ")");
        }
		return false;
	}
	DEBUG("epoll_ctl success for fd: " << fd << ", operation: " << operation << ", events: " << events);
	return true;
}

/// @brief Remove a client from the server and the epoll instance.
/// @param fd The file descriptor of the client to remove
void Server::_removeClient(int fd) {
    DEBUG("[REMOVECLIENT] Called for fd: " << fd << " (about to close)");
    // Only try to remove from epoll and close if fd is valid
    if (fd != -1) {
        _epollExecute(fd, EPOLL_CTL_DEL, 0);
        DEBUG("[REMOVECLIENT] Actually closing fd: " << fd);
        close(fd);
    }
    auto it = _clients.find(fd);
    if (it == _clients.end()) {
        ERROR("Client not found in _clients map: " << fd);
        return ;
    }
    _clients.erase(it); // shared_ptr will clean up if no other references
    DEBUG("Client disconnected: " << fd);
}

/// @brief Fetch the server configuration for a request based on the Host header
/// @param request The Request object containing the headers
/// @return A reference to the ServerConfig object that matches the Host header | 
/// or the first config if no direct match was found
ServerConfig &Server::_loadRequestConfig(Request &request) {
	for (ServerConfig &config : _configs) {
		if (config.serverName.get() == request.headers.getHeader(HeaderKey::Host, "")) {
			DEBUG("Found matching server for request: " << config.serverName.get());
			return (config);
		}
	}
	DEBUG("No matching server found for request, using default: " << _configs[0].serverName.get());
	return _configs[0];
}

/// @brief Handle client input by reading from the socket and processing the request.
/// @param fd The file descriptor of the client socket
/// @param client The Client object associated with the socket
void Server::_handleClientInput(int fd, Client &client) {
    if (!client.read(fd)) {
        _removeClient(fd);
        return;
    }

    if (client.requestIsComplete()) {
        DEBUG("Request complete, starting async processing for fd: " << fd);
        client.processRequest(_loadRequestConfig(client.request));
        if (!_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
            ERROR("Failed to modify epoll event for client: " << fd);
            _removeClient(fd);
            return;
        }
    }
}

/// @brief Handle client output by sending the response back to the client.
/// @param fd The file descriptor of the client socket
/// @param client The Client object associated with the socket
void Server::_handleClientOutput(int fd, Client &client) {
    if (!client.sendResponse(fd, _loadRequestConfig(client.request))) {
        ERROR("Failed to send response for client: " << fd);
        _removeClient(fd);
        return ;
    }

    if (!_epollExecute(fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLET)) {
        ERROR("Failed to modify epoll event for client: " << fd);
        _removeClient(fd);
        return ;
    }

    if (client.request.session) {
        _sessionManager.storeSession(*client.request.session);
    }

    client.reset();
}

/// @brief Handle I/O events for a client socket.
/// @details This function processes the events for a client socket based on the revents flags.
/// @param fd The file descriptor of the client socket.
/// @param revents The epoll events for the client socket.
void Server::_handleClientIo(int fd, short revents) {
    if (revents & (EPOLLERR | EPOLLHUP)) {
        DEBUG("[CLIENTIO] EPOLLERR or EPOLLHUP for client fd: " << fd << ". Client likely disconnected.");
    }

    auto it = _clients.find(fd);
    if (it == _clients.end()) {
        ERROR("Client not found in _clients map: " << fd);
        return ;
    }
    Client &client = it->second;

    DEBUG_IF(revents & EPOLLIN, "EPOLLIN event detected for fd: " << fd);
    DEBUG_IF(revents & EPOLLOUT, "EPOLLOUT event detected for fd: " << fd);
    DEBUG_IF(revents & (EPOLLERR | EPOLLHUP), "EPOLLERR or EPOLLHUP event detected for fd: " << fd);
    DEBUG_IF_NOT(revents & (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP), "Unexpected event for fd: " << fd);

    if (revents & EPOLLIN)
        _handleClientInput(fd, client);
    else if (revents & EPOLLOUT)
        _handleClientOutput(fd, client);
    else if (revents & (EPOLLERR | EPOLLHUP)) {
        ERROR("Error or hangup on client socket: " << fd);
        _removeClient(fd);
    }
}

/// @brief Entry point for the server, running it's event loop once.
void Server::runOnce() {
    epoll_event events[64];

    int event_count = epoll_wait(_epoll_fd, events, 64, 0);
    if (event_count == -1)
        throw ServerCreationException("Failed to wait for epoll events");
        
    for (int i = 0; i < event_count; ++i) {
        int fd = events[i].data.fd;
        
        if (fd == _server_fd) {
            _handleNewConnection();
        } else if (_clients.find(fd) != _clients.end()) {
            // Client socket event
            _handleClientIo(fd, events[i].events);
        }
    }

    _timer.processEvents();
}

// Helper to check if a file descriptor is valid (open)
bool is_fd_valid(int fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}