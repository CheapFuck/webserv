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
    std::string getMimeType(const std::string& path) {
        if (path.ends_with(".html")) return "text/html";
        if (path.ends_with(".css")) return "text/css";
        if (path.ends_with(".js")) return "application/javascript";
        if (path.ends_with(".png")) return "image/png";
        if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
        if (path.ends_with(".gif")) return "image/gif";
        return "application/octet-stream";
    }
    
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
