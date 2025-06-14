#include <sys/socket.h>
#include <filesystem>
#include <exception>
#include <iostream>
#include <signal.h>
#include <vector>
#include <map>
#include <unistd.h>

#include "print.hpp"
#include "server.hpp"
#include "config/config.hpp"
#include "config/rules/rules.hpp"

bool g_quit = false;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_quit = true;
    }
}

/// @brief Fetch server configurations
/// @param configPath The file path to the configuration file
/// @return A vector of ServerConfig objects
std::vector<ServerConfig> load_server_configs(const std::string& configPath) {
    try {
        std::vector<Token> tokens = tokenize(configPath);
        Object raw_data = lexer(tokens);
        return fetchServerConfigs(raw_data);
    }
    catch (const std::exception& e) {
        ERROR("Failed to load server configurations: " << e.what());
        return std::vector<ServerConfig>();
    }
}

/// @brief Free up resources for running servers to freely exit the program
/// @param servers A vector of Server objects to clean up
void handleServersCleanup(std::vector<Server> &servers) {
    for (Server& server : servers)
        server.cleanUp();
}

/// @brief Spawn and run servers based on the provided configurations
/// @param serverConfigs A vector of ServerConfig objects containing the server configurations
/// @return 1 if either one of the servers failed to start, 0 otherwise
int run_servers(const std::vector<ServerConfig>& serverConfigs) {
    std::map<int, std::vector<ServerConfig>> portToCOnfigs;
    for (const ServerConfig& config : serverConfigs) {
        if (portToCOnfigs.find(config.port.get()) == portToCOnfigs.end())
            portToCOnfigs[config.port.get()] = std::vector<ServerConfig>();
        portToCOnfigs[config.port.get()].push_back(config);
    }

    std::vector<Server> servers;
    for (const std::pair<int, std::vector<ServerConfig>> entry : portToCOnfigs) {
        try {
            servers.emplace_back(entry.second);
        } catch (const std::exception& e) {
            ERROR("Error creating server for port " << entry.first << ": " << e.what());
            handleServersCleanup(servers);
            return (1);
        }
    }

    while (!g_quit) {
        for (Server& server : servers)
            server.runOnce();
        // Optionally sleep for a short time to avoid busy-waiting
        usleep(1000); // 1ms
    }

    handleServersCleanup(servers);
    return (0);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        ERROR("Usage: " << argv[0] << " [configuration file]");
        return 1;
    }

    std::string configPath = "default.conf";
    if (argc == 2) configPath = argv[1];

    std::vector<ServerConfig> serverConfigs = load_server_configs(configPath);
    if (serverConfigs.empty()) {
        ERROR("No valid server configurations found in " << configPath);
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (run_servers(serverConfigs) != 0) {
        ERROR("Error running servers.");
        return 1;
    }
    return (0);
}
