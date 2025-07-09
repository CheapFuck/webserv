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

#include "config/rules/ruleTemplates/serverconfigRule.hpp"
#include "config/config.hpp"

bool g_quit = false;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_quit = true;
    }
}

std::map<int, std::vector<ServerConfig>> translate_config_vec_to_map(const std::vector<ServerConfig> &configs) {
    std::map<int, std::vector<ServerConfig>> portToConfigs;

    for (const auto &config : configs) {
        if (config.port.isSet()) {
            portToConfigs[config.port.getPort()].push_back(config);
        } else {
            ERROR("Server configuration missing port: " << config.serverName.getServerName());
        }
    }

    return portToConfigs;
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        ERROR("Usage: " << argv[0] << " [configuration file]");
        return 1;
    }

    std::string configPath = "default.conf";
    if (argc == 2) configPath = argv[1];

    ConfigurationParser *parser = new ConfigurationParser();
    parser->parseFile(configPath);
    std::vector<ServerConfig> configs = parser->getResult(configPath);
    delete parser;

    if (configs.empty())
        return 1;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);

    PRINT("Configuration loaded successfully from " << configPath);
    Server server(translate_config_vec_to_map(configs));
    while (!g_quit)
        server.runOnce();
    server.cleanUp();

    PRINT("Server shutting down gracefully - how nice ^^");
    return (0);
}
