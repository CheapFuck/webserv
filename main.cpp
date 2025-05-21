// #include "Server.hpp"
// #include "Config.hpp"
#include <exception>
#include <iostream>
// #include <string>
// #include <cstdlib>
#include <signal.h>

#include "config/config.hpp"
#include "config/rules/rules.hpp"

#include "server.hpp"

#include <sys/socket.h>

bool g_quit = false;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_quit = true;
    }
}

std::vector<ServerConfig> load_server_configs(const std::string& configPath) {
    try {
        std::vector<Token> tokens = tokenize(configPath);
        Object raw_data = lexer(tokens);
        return fetch_server_configs(raw_data);
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing configuration: " << e.what() << std::endl;
        return std::vector<ServerConfig>();
    }
}

int run_servers(const std::vector<ServerConfig>& serverConfigs) {
    std::vector<Server> servers;
    servers.reserve(serverConfigs.size());

    for (const ServerConfig& config : serverConfigs) {
        try {
            servers.emplace_back(config);
        } catch (const std::exception& e) {
            std::cerr << "Error creating server: " << e.what() << std::endl;
            return 1;
        }
    }

    while (!servers.empty() && !g_quit) {
        for (size_t i = 0; i < servers.size(); ++i) {
            try {
                servers[i].run_once();
            } catch (const std::exception& e) {
                std::cerr << "Error running server: " << e.what() << std::endl;
                servers.erase(servers.begin() + i);
                break;
            }
        }
    }

    return (0);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [configuration file]" << std::endl;
        return 1;
    }

    std::string configPath = "default.conf";
    if (argc == 2) configPath = argv[1];

    std::vector<ServerConfig> serverConfigs = load_server_configs(configPath);
    if (serverConfigs.empty()) {
        std::cerr << "No valid server configurations found." << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (run_servers(serverConfigs) != 0) {
        std::cerr << "Error running servers." << std::endl;
        return 1;
    }
    return (0);
}
