#include "config/rules/rules.hpp"
#include "request.hpp"
#include "session.hpp"
#include "cookie.hpp"
#include "client.hpp"
#include "server.hpp"
#include "print.hpp"
#include "CGI.hpp"
#include "Utils.hpp"
#include "thread-pool.hpp"
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
    _sessionManager(),
    _server_fd(-1),
    _epoll_fd(-1),
    _clients(),
    _activeCGIs(),
    _threadPool(new ThreadPool(std::thread::hardware_concurrency())) {  // Use hardware concurrency for thread count

    this->_setupSocket();
    this->_setupEpoll();
    _sessionManager.loadFromPort(_configs[0].port.get());
    PRINT("Server " << _configs[0].server_name.get() << " is running on port " << _configs[0].port.get());
}

Server::Server(const Server &other) :
    _configs(other._configs),
    _sessionManager(other._sessionManager),
    _server_fd(other._server_fd),
    _epoll_fd(other._epoll_fd),
    _clients(other._clients),
    _activeCGIs(other._activeCGIs),
    _threadPool(new ThreadPool(std::thread::hardware_concurrency())) {}

Server &Server::operator=(const Server &other) {
    if (this != &other) {
        _configs = other._configs;
        _sessionManager = other._sessionManager;
        _server_fd = other._server_fd;
        _epoll_fd = other._epoll_fd;
        _clients = other._clients;
        _activeCGIs = other._activeCGIs;
        _threadPool.reset(new ThreadPool(std::thread::hardware_concurrency()));
    }
    return *this;
}

Server::~Server() {
    // Clean up active CGI processes
    for (auto& pair : _activeCGIs) {
        delete pair.second;
    }
}

/// @brief Clean up the server resources, closing sockets and cleaning up sessions.
void Server::cleanUp() {
    // Clean up CGI processes first
    for (auto& pair : _activeCGIs) {
        delete pair.second;
    }
    _activeCGIs.clear();
    
    if (_server_fd != -1)
        close(_server_fd);
    if (_epoll_fd != -1)
        close(_epoll_fd);
    _sessionManager.fullCleanup(_configs[0].port.get());
}


/// @brief Handle client input by reading from the socket and processing the request.
/// @param fd The file descriptor of the client socket
/// @param client The Client object associated with the socket
// void Server::_handleClientInput(int fd, Client &client) {
//     if (!client.read(fd)) {
//         _removeClient(fd);
//         return;
//     }

//     if (client.requestIsComplete()) {
//         // Process the request asynchronously
//         _processRequestAsync(fd, client);
//     }
// }

/// @brief Process a client request asynchronously using the thread pool
/// @param fd The client file descriptor
/// @param client The client object
// void Server::_processRequestAsync(int fd, Client &client) {
// 	(void)client;
//     // Capture necessary data for the lambda
//     auto processTask = [this, fd]() {
//         auto clientIt = this->_clients.find(fd);
//         if (clientIt == this->_clients.end()) {
//             return;
//         }
        
//         Client& client = clientIt->second;
        
//         this->_prepareRequestProcessing(client);
//         ServerConfig& config = this->_loadRequestConfig(client.request);
        
//         // Check if this request should be handled by CGI
//         const LocationRule* route = config.routes.find(client.request.metadata.getPath());
//         if (!route) {
//             client.response.setStatusCode(HttpStatusCode::NotFound);
//         } else if (this->shouldUseCGI(client.request, *route)) {
//             // Start CGI execution
//             if (!this->_startCGIExecution(fd, client, *route, config)) {
//                 client.response.setStatusCode(HttpStatusCode::InternalServerError);
//                 if (!this->_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
//                     this->_removeClient(fd);
//                 }
//             }
//             // CGI is now running, response will be handled when CGI completes
//         } else {
//             // Handle normal request processing
//             client.processRequest(config);
//             if (!this->_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
//                 ERROR("Failed to modify epoll event for client: " << fd);
//                 this->_removeClient(fd);
//                 return;
//             }
//         }
//     };
    
//     // Enqueue the task to the thread pool
//     _threadPool->enqueue(processTask);
// }



/// @brief Start CGI execution for a client request
/// @param fd The client file descriptor
/// @param client The client object
/// @param route The location rule that matched
/// @param config The server configuration
/// @return True if CGI was started successfully, false otherwise
bool Server::_startCGIExecution(int fd, Client &client, const LocationRule &route, const ServerConfig &config) {
    DEBUG("Starting CGI execution for client: " << fd);
    
    CGI* cgi = new CGI();
    
    if (cgi->startExecution(client.request, route, config)) {
        _activeCGIs[fd] = cgi;
        
        // Add CGI file descriptors to epoll
        std::vector<int> cgiFds = cgi->getFileDescriptors();
        for (int cgiFd : cgiFds) {
            epoll_event event{};
            event.events = EPOLLIN | EPOLLOUT | EPOLLET;
            event.data.fd = cgiFd;
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, cgiFd, &event) == -1) {
                ERROR("Failed to add CGI fd to epoll: " << cgiFd);
            } else {
                DEBUG("Added CGI fd to epoll: " << cgiFd);
            }
        }
        
        DEBUG("Started CGI for client: " << fd);
        return true;
    } else {
        delete cgi;
        ERROR("Failed to start CGI for client: " << fd);
        return false;
    }
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
	if (epoll_ctl(_epoll_fd, operation, fd, &event) == -1) {
		ERROR("Failed to modify epoll events for fd: " << fd);
		return (false);
	}
	return (true);
}

/// @brief Remove a client from the server and the epoll instance.
/// @param fd The file descriptor of the client to remove
void Server::_removeClient(int fd) {
	_epollExecute(fd, EPOLL_CTL_DEL, 0);
	auto it = _clients.find(fd);
	if (it == _clients.end()) {
		ERROR("Client not found in _clients map: " << fd);
		return ;
	}
	_clients.erase(it);
	DEBUG("Client disconnected: " << fd);
}

/// @brief Prepare the request processing for a client by attaching a session to the request.
/// @param client The Client object associated with the request
/// @details If no session cookie is found, a new session is created. If a session cookie is found,
/// the session is retrieved or created based on the cookie value.
void Server::_prepareRequestProcessing(Client &client) {
	const Cookie *sessionCookie = Cookie::getCookie(client.request, SESSION_COOKIE_NAME);
	if (!sessionCookie) {
		DEBUG("No session cookie found for client; creating a new session");
		client.request.session = !client.request.session ? _sessionManager.createNewSession() : client.request.session;
		if (!client.request.session)
			ERROR("Failed to create a new session");
		client.response.headers.add(HeaderKey::SetCookie,
			Cookie::createSessionCookie(client.request.session->getSessionId())
			.getHeaderInitializationString());
		client.request.session->setData("Some testvalue", client.request.session->getSessionId());
	} else
		client.request.session = _sessionManager.getOrCreateSession(sessionCookie->getValue());
	DEBUG("Session storage thingy with value " << client.request.session->getData("Some testvalue"));

	DEBUG("Object prt session: " << client.request.session);
	DEBUG("Session ID for client: " << client.request.session->getSessionId());
	client.request.session->updateLastAccessTime();
}

/// @brief Fetch the server configuration for a request based on the Host header
/// @param request The Request object containing the headers
/// @return A reference to the ServerConfig object that matches the Host header | 
/// or the first config if no direct match was found
ServerConfig &Server::_loadRequestConfig(Request &request) {
	for (ServerConfig &config : _configs) {
		if (config.server_name.get() == request.headers.getHeader(HeaderKey::Host, "")) {
			DEBUG("Found matching server for request: " << config.server_name.get());
			return (config);
		}
	}
	DEBUG("No matching server found for request, using default: " << _configs[0].server_name.get());
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
        // Process the request asynchronously
        _processRequestAsync(fd, client);
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

	client.reset();
}

/// @brief Handle I/O events for a client socket.
/// @details This function processes the events for a client socket based on the revents flags.
/// @param fd The file descriptor of the client socket.
/// @param revents The epoll events for the client socket.
void Server::_handleClientIo(int fd, short revents) {
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

/// @brief Handle CGI file descriptor events
/// @param fd The CGI file descriptor
/// @param revents The epoll events
void Server::_handleCGIIo(int fd, short revents) {
    DEBUG("CGI I/O event on fd: " << fd << " events: " << revents);
    DEBUG_IF(revents & EPOLLIN, "CGI EPOLLIN event on fd: " << fd);
    DEBUG_IF(revents & EPOLLOUT, "CGI EPOLLOUT event on fd: " << fd);
    DEBUG_IF(revents & (EPOLLERR | EPOLLHUP), "CGI EPOLLERR or EPOLLHUP event on fd: " << fd);
    
    // Since CGI file descriptors are non-blocking and managed by the CGI class,
    // we don't need to do much here. The updateCGIProcesses() method will
    // handle the actual reading/writing when it's called in the main loop.
    
    // However, if there's an error or hangup, we should log it
    if (revents & EPOLLERR) {
        ERROR("CGI file descriptor error on fd: " << fd);
    }
    
    if (revents & EPOLLHUP) {
        DEBUG("CGI file descriptor hangup on fd: " << fd << " (this might be normal)");
    }
    
    // The actual CGI I/O processing happens in _updateCGIProcesses()
    // which is called in the main event loop
}

// /// @brief Determine if a request should use CGI based on the location rule
// /// @param request The client request
// /// @param route The location rule
// /// @return True if CGI should be used
// bool Server::shouldUseCGI(const Request& request, const LocationRule& route) {
//     DEBUG("Checking if request should use CGI: " << request.metadata.getPath());
    
//     if (!route.cgi_paths.isSet()) {
//         DEBUG("CGI paths not set for route");
//         return false;
//     }
    
//     // Check if the file extension matches a CGI interpreter
//     std::string path = request.metadata.getPath();
//     std::string extension = Utils::getFileExtension(path);
//     DEBUG("File extension: " << extension);
    
//     if (!route.cgi_paths.exists(extension)) {
//         DEBUG("No CGI interpreter found for extension: " << extension);
//         return false;
//     }
    
//     // Verify the file exists and is executable
//     Path requestPath = Path::createFromUrl(request.metadata.getPath(), route);
//     if (!requestPath.isValid()) {
//         DEBUG("Invalid request path for CGI: " << requestPath.str());
//         return false;
//     }
    
//     struct stat statBuf;
//     if (stat(requestPath.str().c_str(), &statBuf) != 0) {
//         DEBUG("CGI: Script not found: " << requestPath.str());
//         return false;
//     }
    
//     // Check if it's a regular file (not a directory)
//     if (!S_ISREG(statBuf.st_mode)) {
//         DEBUG("CGI: Path is not a regular file: " << requestPath.str());
//         return false;
//     }
    
//     // Check if it's executable
//     if (!(statBuf.st_mode & S_IXUSR)) {
//         DEBUG("CGI: Script not executable: " << requestPath.str());
//         return false;
//     }
    
//     DEBUG("CGI: Request SHOULD use CGI - " << path << " with extension " << extension);
//     return true;
// }

/// @brief Process a client request asynchronously using the thread pool
/// @param fd The client file descriptor
/// @param client The client object
void Server::_processRequestAsync(int fd, Client &client) {
	(void)client;  // Suppress unused variable warning
    // Capture necessary data for the lambda
    auto processTask = [this, fd]() {
        auto clientIt = this->_clients.find(fd);
        if (clientIt == this->_clients.end()) {
            return;
        }
        
        Client& client = clientIt->second;
        
        this->_prepareRequestProcessing(client);
        ServerConfig& config = this->_loadRequestConfig(client.request);
        
        // Find the matching route
        const LocationRule* route = config.routes.find(client.request.metadata.getPath());
        if (!route) {
            client.response.setStatusCode(HttpStatusCode::NotFound);
            if (!this->_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                this->_removeClient(fd);
            }
            return;
        }
        
        // Check if this request should be handled by CGI BEFORE processing the request
        if (this->shouldUseCGI(client.request, *route)) {
            DEBUG("Starting CGI execution for request: " << client.request.metadata.getPath());
            // Start CGI execution
            if (!this->_startCGIExecution(fd, client, *route, config)) {
                ERROR("Failed to start CGI execution");
                client.response.setStatusCode(HttpStatusCode::InternalServerError);
                if (!this->_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                    this->_removeClient(fd);
                }
            }
            // CGI is now running, response will be handled when CGI completes
            // Don't call processRequest() for CGI requests
            return;
        }
        
        // Handle normal request processing (non-CGI)
        DEBUG("Processing non-CGI request: " << client.request.metadata.getPath());
        client.processRequest(config);
        if (!this->_epollExecute(fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
            ERROR("Failed to modify epoll event for client: " << fd);
            this->_removeClient(fd);
            return;
        }
    };
    
    // Enqueue the task to the thread pool
    _threadPool->enqueue(processTask);
}

// /// @brief Determine if a request should use CGI based on the location rule
// /// @param request The client request
// /// @param route The location rule
// /// @return True if CGI should be used
// bool Server::shouldUseCGI(const Request& request, const LocationRule& route) {
//     DEBUG("Checking if request should use CGI: " << request.metadata.getPath());
    
//     if (!route.cgi_paths.isSet()) {
//         DEBUG("CGI paths not set for route");
//         return false;
//     }
    
//     // Check if the file extension matches a CGI interpreter
//     std::string path = request.metadata.getPath();
//     std::string extension = Utils::getFileExtension(path);
//     DEBUG("File extension: " << extension);
    
//     if (!route.cgi_paths.exists(extension)) {
//         DEBUG("No CGI interpreter found for extension: " << extension);
//         return false;
//     }
    
//     DEBUG("Found CGI interpreter for extension: " << extension << " -> " << route.cgi_paths.getPath(extension));
    
//     // Verify the file exists and is executable
//     Path requestPath = Path::createFromUrl(request.metadata.getPath(), route);
//     if (!requestPath.isValid()) {
//         DEBUG("Invalid request path for CGI: " << requestPath.str());
//         return false;
//     }
    
//     DEBUG("CGI script path: " << requestPath.str());
    
//     struct stat statBuf;
//     if (stat(requestPath.str().c_str(), &statBuf) != 0) {
//         DEBUG("CGI: Script not found: " << requestPath.str());
//         return false;
//     }
    
//     // Check if it's a regular file (not a directory)
//     if (!S_ISREG(statBuf.st_mode)) {
//         DEBUG("CGI: Path is not a regular file: " << requestPath.str());
//         return false;
//     }
    
//     // Check if it's executable
//     if (!(statBuf.st_mode & S_IXUSR)) {
//         DEBUG("CGI: Script not executable: " << requestPath.str());
//         return false;
//     }
    
//     DEBUG("CGI: Request SHOULD use CGI - " << path << " with extension " << extension);
//     return true;
// }



/// @brief Determine if a request should use CGI based on the location rule
/// @param request The client request
/// @param route The location rule
/// @return True if CGI should be used
bool Server::shouldUseCGI(const Request& request, const LocationRule& route) {
    DEBUG("Checking if request should use CGI: " << request.metadata.getPath());
    
    if (!route.cgi_paths.isSet()) {
        DEBUG("CGI paths not set for route");
        return false;
    }
    
    // Check if the file extension matches a CGI interpreter
    std::string path = request.metadata.getPath();
    std::string extension = Utils::getFileExtension(path);
    DEBUG("File extension: " << extension);
    
    if (!route.cgi_paths.exists(extension)) {
        DEBUG("No CGI interpreter found for extension: " << extension);
        return false;
    }
    
    DEBUG("Found CGI interpreter for extension: " << extension << " -> " << route.cgi_paths.getPath(extension));
    
    // Verify the file exists and is executable
    Path requestPath = Path::createFromUrl(request.metadata.getPath(), route);
    if (!requestPath.isValid()) {
        DEBUG("Invalid request path for CGI: " << requestPath.str());
        return false;
    }
    
    DEBUG("CGI script path: " << requestPath.str());
    
    struct stat statBuf;
    if (stat(requestPath.str().c_str(), &statBuf) != 0) {
        DEBUG("CGI: Script not found: " << requestPath.str());
        return false;
    }
    
    // Check if it's a regular file (not a directory)
    if (!S_ISREG(statBuf.st_mode)) {
        DEBUG("CGI: Path is not a regular file: " << requestPath.str());
        return false;
    }
    
    // Check if it's executable
    if (!(statBuf.st_mode & S_IXUSR)) {
        DEBUG("CGI: Script not executable: " << requestPath.str());
        return false;
    }
    
    DEBUG("CGI: Request SHOULD use CGI - " << path << " with extension " << extension);
    return true;
}


/// @brief Update all active CGI processes and handle completed ones
void Server::_updateCGIProcesses() {
    std::vector<int> toRemove;
    
    // DEBUG("Updating " << _activeCGIs.size() << " active CGI processes");
    
    for (auto& pair : _activeCGIs) {
        int clientFd = pair.first;
        CGI* cgi = pair.second;
        
        // DEBUG("Checking CGI status for client: " << clientFd);
        CGI::Status status = cgi->updateExecution();
        
        switch (status) {
            case CGI::Status::FINISHED: {
                DEBUG("CGI finished for client: " << clientFd);
                auto clientIt = _clients.find(clientFd);
                if (clientIt != _clients.end()) {
                    Client& client = clientIt->second;
                    
                    if (cgi->getResponse(client.response)) {
                        DEBUG("CGI completed successfully for client: " << clientFd);
                        DEBUG("CGI response status: " << static_cast<int>(client.response.getStatusCode()));
                        DEBUG("CGI response body length: " << client.response.getBody().length());
                        
                        // Switch client to output mode
                        if (!_epollExecute(clientFd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                            ERROR("Failed to modify epoll event for client: " << clientFd);
                            _removeClient(clientFd);
                        }
                    } else {
                        ERROR("Failed to get CGI response for client: " << clientFd);
                        client.response.setStatusCode(HttpStatusCode::InternalServerError);
                        if (!_epollExecute(clientFd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                            _removeClient(clientFd);
                        }
                    }
                } else {
                    ERROR("Client not found for finished CGI: " << clientFd);
                }
                toRemove.push_back(clientFd);
                break;
            }
            case CGI::Status::ERROR: {
                ERROR("CGI error for client: " << clientFd);
                auto clientIt = _clients.find(clientFd);
                if (clientIt != _clients.end()) {
                    Client& client = clientIt->second;
                    client.response.setStatusCode(HttpStatusCode::InternalServerError);
                    client.response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI script execution failed.</p></body></html>");
                    client.response.headers.replace(HeaderKey::ContentType, "text/html");
                    if (!_epollExecute(clientFd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                        _removeClient(clientFd);
                    }
                }
                toRemove.push_back(clientFd);
                break;
            }
            case CGI::Status::TIMEOUT: {
                ERROR("CGI timeout for client: " << clientFd);
                auto clientIt = _clients.find(clientFd);
                if (clientIt != _clients.end()) {
                    Client& client = clientIt->second;
                    client.response.setStatusCode(HttpStatusCode::RequestTimeout);
                    client.response.setBody("<html><body><h1>408 Request Timeout</h1><p>CGI script execution timed out.</p></body></html>");
                    client.response.headers.replace(HeaderKey::ContentType, "text/html");
                    if (!_epollExecute(clientFd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET)) {
                        _removeClient(clientFd);
                    }
                }
                toRemove.push_back(clientFd);
                break;
            }
            case CGI::Status::RUNNING:
                // Continue running
                // DEBUG("CGI still running for client: " << clientFd);
                break;
        }
    }
    
    // Clean up finished CGIs
    for (int clientFd : toRemove) {
        DEBUG("Cleaning up CGI for client: " << clientFd);
        auto cgiIt = _activeCGIs.find(clientFd);
        if (cgiIt != _activeCGIs.end()) {
            // Remove CGI file descriptors from epoll
            std::vector<int> cgiFds = cgiIt->second->getFileDescriptors();
            for (int cgiFd : cgiFds) {
                _epollExecute(cgiFd, EPOLL_CTL_DEL, 0);
                DEBUG("Removed CGI fd from epoll: " << cgiFd);
            }
            
            delete cgiIt->second;
            _activeCGIs.erase(cgiIt);
            DEBUG("CGI cleanup completed for client: " << clientFd);
        }
    }
    
    if (!toRemove.empty()) {
        DEBUG("Cleaned up " << toRemove.size() << " CGI processes");
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
        } else {
            // Might be a CGI file descriptor
            _handleCGIIo(fd, events[i].events);
        }
    }

    // Update CGI processes (this handles the actual CGI I/O)
    _updateCGIProcesses();
    
    _sessionManager.cleanUpExpiredSessions();
}