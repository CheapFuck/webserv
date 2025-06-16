#ifndef SERVER_HPP
#define SERVER_HPP

#include "config/rules/rules.hpp"
#include "sessionManager.hpp"
#include "session.hpp"
#include "client.hpp"
#include "timer.hpp"
#include "CGI.hpp"

#include <vector>
#include <memory>
#include <map>

// Free function to check if a file descriptor is valid (open)
bool is_fd_valid(int fd);

class Server {
public:
    Server(const std::vector<ServerConfig> &configs);
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    void cleanUp();
    void runOnce();


private:
    std::vector<ServerConfig> _configs;
    UserSessionManager _sessionManager;
    int _server_fd;
    int _epoll_fd;
    Timer _timer;
    std::map<int, Client> _clients;

    // Socket and epoll setup
    void _setupSocket();
    void _setupEpoll();
    bool _epollExecute(int fd, uint32_t operation, uint32_t events);

    // Connection handling
    void _handleNewConnection();
    void _removeClient(int fd);

    // Request processing
    ServerConfig &_loadRequestConfig(Request &request);

    // I/O handling
    void _handleClientInput(int fd, Client &client);
    void _handleClientOutput(int fd, Client &client);
    void _handleClientIo(int fd, short revents);
    void _handleCGIIo(int fd, short revents);

    // CGI handling
    bool _startCGIExecution(int fd, Client &client, const LocationRule &route, const ServerConfig &config);
    void _updateCGIProcesses();
    bool _clientHasActiveCGI(int fd) const;
    bool shouldUseCGI(const Request& request, const LocationRule& route);
};

#endif // SERVER_HPP