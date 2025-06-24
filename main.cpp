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

int main(int argc, char* argv[]) {
    if (argc > 2) {
        ERROR("Usage: " << argv[0] << " [configuration file]");
        return 1;
    }

    std::string configPath = "default.conf";
    if (argc == 2) configPath = argv[1];

    ConfigurationParser parser(configPath);
    if (!parser.fetchConfiguration())
        return (1);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);

    PRINT("Configuration loaded successfully from " << configPath);
    Server server(parser.getResult());
    while (!g_quit)
        server.runOnce();
    server.cleanUp();

    PRINT("Server shutting down gracefully - how nice ^^");
    return (0);
}
