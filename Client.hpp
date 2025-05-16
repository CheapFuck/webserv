#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include <string>
#include <netinet/in.h>

class Client {
private:
    int _socket;
    struct sockaddr_in _addr;
    std::string _buffer;
    Request _request;
    Response _response;
    bool _requestComplete;
    bool _responseComplete;
    
public:
    Client();
    Client(int socket, const struct sockaddr_in& addr);
    ~Client();
    
    bool readRequest();
    bool isRequestComplete() const;
    void processRequest(const Config& config);
    bool sendResponse();
    bool isResponseComplete() const;
    void reset();
};

#endif // CLIENT_HPP
