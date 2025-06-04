#ifndef SERVER_HPP
#define SERVER_HPP

#include "config/rules/rules.hpp"
#include "session.hpp"
#include "client.hpp"
#include "CGI.hpp"
#include "thread-pool.hpp"
#include <vector>
#include <map>
#include <memory>

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
    std::map<int, Client> _clients;
    std::map<int, CGI*> _activeCGIs;  // Map client fd to CGI instance
    
    // Thread pool for handling blocking operations
    std::unique_ptr<ThreadPool> _threadPool;

    // Socket and epoll setup
    void _setupSocket();
    void _setupEpoll();
    bool _epollExecute(int fd, uint32_t operation, uint32_t events);

    // Connection handling
    void _handleNewConnection();
    void _removeClient(int fd);

    // Request processing
    void _prepareRequestProcessing(Client &client);
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
    
    // Asynchronous request processing
    void _processRequestAsync(int fd, Client &client);
};

#endif // SERVER_HPP