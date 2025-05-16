#include "Server.hpp"
#include "Utils.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>

Server::Server(const Config& config) : _config(config), _running(false) {
}

Server::~Server() {
    shutdown();
}

bool Server::setupServerSockets() {
    const std::vector<ServerConfig>& servers = _config.getServers();
    
    for (size_t i = 0; i < servers.size(); ++i) {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Set socket to non-blocking
        if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) < 0) {
            std::cerr << "Failed to set socket to non-blocking: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        // Allow socket reuse
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        // Bind socket
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(servers[i].port);
        serverAddr.sin_addr.s_addr = inet_addr(servers[i].host.c_str());
        
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        // Listen on socket
        if (listen(serverSocket, 128) < 0) {
            std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        _serverSockets.push_back(serverSocket);
        
        std::cout << "Server listening on " << servers[i].host << ":" << servers[i].port << std::endl;
        
        // Add to poll array
        struct pollfd pfd;
        pfd.fd = serverSocket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
    
    return true;
}

void Server::acceptNewConnections(int serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    // Set client socket to non-blocking
    if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Failed to set client socket to non-blocking: " << strerror(errno) << std::endl;
        close(clientSocket);
        return;
    }
    
    // Add client to map and poll array
    _clients[clientSocket] = Client(clientSocket, clientAddr);
    
    struct pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);
    
    std::cout << "New client connected: " << inet_ntoa(clientAddr.sin_addr) << ":" 
              << ntohs(clientAddr.sin_port) << " (fd: " << clientSocket << ")" << std::endl;
}

void Server::handleClientIO(size_t pollIndex) {
    int fd = _pollfds[pollIndex].fd;
    short revents = _pollfds[pollIndex].revents;
    
    // Check if this is a server socket
    for (size_t i = 0; i < _serverSockets.size(); ++i) {
        if (fd == _serverSockets[i]) {
            if (revents & POLLIN) {
                acceptNewConnections(fd);
            }
            return;
        }
    }
    
    // This is a client socket
    if (_clients.find(fd) == _clients.end()) {
        std::cerr << "Client not found for fd: " << fd << std::endl;
        removeClient(fd);
        return;
    }
    
    Client& client = _clients[fd];
    
    // Handle read events
    if (revents & POLLIN) {
        if (!client.readRequest()) {
            removeClient(fd);
            return;
        }
        
        // If request is complete, process it and prepare response
        if (client.isRequestComplete()) {
            client.processRequest(_config);
            // Update poll events to include POLLOUT
            _pollfds[pollIndex].events |= POLLOUT;
        }
    }
    
    // Handle write events
    if (revents & POLLOUT) {
        if (!client.sendResponse()) {
            removeClient(fd);
            return;
        }
        
        // If response is fully sent, reset client for next request
        if (client.isResponseComplete()) {
            client.reset();
            // Update poll events to only include POLLIN
            _pollfds[pollIndex].events = POLLIN;
        }
    }
    
    // Handle errors
    if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
        std::cerr << "Poll error on client fd: " << fd << std::endl;
        removeClient(fd);
    }
}

void Server::removeClient(int clientFd) {
    // Remove from poll array
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd == clientFd) {
            _pollfds.erase(_pollfds.begin() + i);
            break;
        }
    }
    
    // Close socket and remove from clients map
    close(clientFd);
    _clients.erase(clientFd);
    
    std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
}

void Server::run() {
    if (!setupServerSockets()) {
        throw std::runtime_error("Failed to set up server sockets");
    }
    
    _running = true;
    
    std::cout << "Server running..." << std::endl;
    
    while (_running) {
        // Wait for events with poll
        int pollResult = poll(&_pollfds[0], _pollfds.size(), -1);
        
        if (pollResult < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, continue
                continue;
            }
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            break;
        }
        
        // Handle events
        for (size_t i = 0; i < _pollfds.size() && pollResult > 0; ++i) {
            if (_pollfds[i].revents != 0) {
                handleClientIO(i);
                pollResult--;
            }
        }
    }
}

void Server::shutdown() {
    _running = false;
    
    // Close all client connections
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
    }
    _clients.clear();
    
    // Close all server sockets
    for (size_t i = 0; i < _serverSockets.size(); ++i) {
        close(_serverSockets[i]);
    }
    _serverSockets.clear();
    
    _pollfds.clear();
    
    std::cout << "Server shutdown complete" << std::endl;
}
