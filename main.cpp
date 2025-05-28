#include <sys/socket.h>
#include <filesystem>
#include <exception>
#include <iostream>
#include <signal.h>

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

/// @brief Spawn and run servers based on the provided configurations
/// @param serverConfigs A vector of ServerConfig objects containing the server configurations
/// @return 1 if either one of the servers failed to start, 0 otherwise
int run_servers(const std::vector<ServerConfig>& serverConfigs) {
    std::vector<Server> servers;
    servers.reserve(serverConfigs.size());

    for (const ServerConfig& config : serverConfigs) {
        try {
            servers.emplace_back(config);
        } catch (const std::exception& e) {
            ERROR("Error creating server: " << e.what());
            return 1;
        }
    }

    while (!servers.empty() && !g_quit) {
        for (size_t i = 0; i < servers.size(); ++i) {
            try {
                servers[i].run_once();
            } catch (const std::exception& e) {
                ERROR("Error running server: " << e.what());
                servers.erase(servers.begin() + i);
                break;
            }
        }
    }

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
