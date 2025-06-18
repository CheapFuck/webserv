#ifndef SERVER_HPP
#define SERVER_HPP

#include "config/rules/rules.hpp"
#include "sessionManager.hpp"
#include "session.hpp"
#include "client.hpp"
#include "timer.hpp"
#include "CGI.hpp"
#include "fd.hpp"

#include <vector>
#include <memory>
#include <map>

#define EPOLL_MAX_EVENTS 64

// Free function to check if a file descriptor is valid (open)
bool is_fd_valid(int fd);

class Server {
public:
    Server(const std::map<int, std::vector<ServerConfig>> &configs);
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    void cleanUp();
    void runOnce();

    // Request processing
    ServerConfig &loadRequestConfig(Request &request, int serverFd);

private:
    std::map<int, std::vector<ServerConfig>> _portToConfigs;
    UserSessionManager _sessionManager;
    int _server_fd;
    int _epoll_fd;
    Timer _timer;
    std::map<int, FD> _descriptors;

    // Socket and epoll setup
    void _setupEpoll();
    void _setupSocket(int listenPort, const std::vector<ServerConfig> &configs);
    bool _epollExecute(int fd, uint32_t operation, uint32_t events);

    // Connection handling
    void _handleNewConnection(int sourceFd);
    void _removeDescriptor(int fd);

    // I/O handling
    void _handleFDIO(FD &fd, short revents);
};

#endif // SERVER_HPP