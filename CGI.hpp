#include "config/rules/rules.hpp"
#include "baseHandlerObject.hpp"
#include "client.hpp"

struct ParsedUrl {
    std::string scriptPath;
    std::string pathInfo;
    std::string query;
    bool isValid;
};

class CGIClient {
private:
    Client &_client;
    bool _isRunning;
    int _pid;

    std::unordered_map<std::string, std::string> _environmentVariables;

    void _setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl);
    const char **_createEnvironmentArray() const;

public:
    CGIClient(Client &client);
    CGIClient(const CGIClient &other) = delete;
    CGIClient &operator=(const CGIClient &other) = delete;
    ~CGIClient() = default;

    void start(const ServerConfig &config, const LocationRule &route);


}