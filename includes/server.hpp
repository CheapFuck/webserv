#ifndef SERVER_HPP
#define SERVER_HPP

#include "config/rules/rules.hpp"
#include "sessionManager.hpp"
#include "response.hpp"
#include "client.hpp"
#include "timer.hpp"
#include "fd.hpp"

#include <concepts>
#include <vector>
#include <memory>
#include <map>

#define EPOLL_MAX_EVENTS 64

class Client;
class Response;
class Request;
class CGIResponse;

// Free function to check if a file descriptor is valid (open)
bool is_fd_valid(int fd);

struct ServerClientInfo {
    SocketFD fd;
    Client *client;

    ServerClientInfo(SocketFD fd, Client *client);
};

template <typename T>
struct FDEvent {
    T &fd;
    std::function<void(T&, short)> callback;
};

class Server {
private:
    std::map<int, std::vector<ServerConfig>> _portToConfigs;
    UserSessionManager _sessionManager;
    int _server_fd;
    int _epoll_fd;
    Timer _timer;
    HTTPRule &_httpRule;
    std::map<int, ServerClientInfo> _clientDescriptors;
    std::map<int, FDEvent<ReadableFD&>> _readableDescriptors;
    std::map<int, FDEvent<WritableFD&>> _writableDescriptors;

    std::string _serverAddress;
    std::string _serverExecutablePath;

    // Socket and epoll setup
    void _setupEpoll();
    void _setupSocket(int listenPort, const std::vector<ServerConfig> &configs);
    bool _epollExecute(int fd, uint32_t operation, uint32_t events);

    // Connection handling
    void _handleNewConnection(int sourceFd);
    void _removeDescriptor(int fd);

    // I/O handling
    void _handleClientFD(ServerClientInfo &clientInfo, short revents);
    void _checkHangingConnections();

public:
    Server(HTTPRule &http);
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    void cleanUp();
    void runOnce();

    // Request processing
    ServerConfig &loadRequestConfig(const Request &request, int serverFd);
    std::shared_ptr<SessionMetaData> fetchUserSession(Request &request, Response &response);

    void untrackClient(int fd);

    void trackCallbackFD(ReadableFD &fd, std::function<void(ReadableFD&, short)> callback);
    void trackCallbackFD(WritableFD &fd, std::function<void(WritableFD&, short)> callback);

    void untrackCallbackFD(int fd);

    inline Timer &getTimer() { return _timer; }
    inline int getEpollFd() const { return _epoll_fd; }
    inline std::string getServerAddress() { return _serverAddress; }
    inline std::string getServerExecutablePath() { return _serverExecutablePath; }
};

#endif // SERVER_HPP