#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "Client.hpp"
#include <vector>
#include <map>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

class Server {
private:
    Config _config;
    std::vector<int> _serverSockets;
    std::map<int, Client> _clients;
    std::vector<struct pollfd> _pollfds;
    bool _running;
    
    // Private methods
    bool setupServerSockets();
    void acceptNewConnections(int serverSocket);
    void handleClientIO(size_t pollIndex);
    void removeClient(int clientFd);
    
public:
    Server(const Config& config);
    ~Server();
    
    void run();
    void shutdown();
};

#endif // SERVER_HPP
