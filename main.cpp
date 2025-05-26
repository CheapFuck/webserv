#include "Server.hpp"
#include "Config.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <signal.h>

Server* g_server = NULL; // Global pointer for signal handling

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (g_server)
            g_server->shutdown();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    try {
        std::string configPath = "default.conf";
        
        if (argc > 2) {
            std::cerr << "Usage: " << argv[0] << " [configuration file]" << std::endl;
            return 1;
        } else if (argc == 2) {
            configPath = argv[1];
        }
        
        // Set up signal handling
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        // Parse configuration
        Config config;
        if (!config.parseFile(configPath)) {
            std::cerr << "Failed to parse configuration file: " << configPath << std::endl;
            return 1;
        }
        
        // Initialize and run server
        Server server(config);
        g_server = &server;
        
        std::cout << "Server starting..." << std::endl;
        server.run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
