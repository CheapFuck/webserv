#ifndef SERVER_HPP
#define SERVER_HPP

#include "config/rules/rules.hpp"
#include "sessionManager.hpp"
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
private:
    std::map<int, std::vector<ServerConfig>> _portToConfigs;
    UserSessionManager _sessionManager;
    int _server_fd;
    int _epoll_fd;
    Timer _timer;
    std::map<int, FD> _descriptors;

    std::string _serverAddress;
    std::string _serverPort;
    std::string _serverExecutablePath;

    // Socket and epoll setup
    void _setupEpoll();
    void _setupSocket(int listenPort, const std::vector<ServerConfig> &configs);
    bool _epollExecute(int fd, uint32_t operation, uint32_t events);

    // Connection handling
    void _handleNewConnection(int sourceFd);
    void _removeDescriptor(int fd);

    // I/O handling
    void _handleFDIO(FD &fd, short revents);

public:
    Server(const std::map<int, std::vector<ServerConfig>> &configs);
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    void cleanUp();
    void runOnce();

    // Request processing
    ServerConfig &loadRequestConfig(Request &request, int serverFd);
    std::shared_ptr<SessionMetaData> fetchUserSession(Request &request, Response &response);

    void untrackDescriptor(int fd);

    inline Timer &getTimer() { return _timer; }
    inline int getEpollFd() const { return _epoll_fd; }
    inline void trackDescriptor(const FD &fd) { _descriptors[fd.get()] = fd; }
    inline std::string getServerAddress() { return _serverAddress; }
    inline std::string getServerPort() { return _serverPort; }
    inline std::string getServerExecutablePath() { return _serverExecutablePath; }
};

#endif // SERVER_HPP