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

#include "config/rules/ruleTemplates/httpRule.hpp"
#include "config/config.hpp"

bool g_quit = false;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_quit = true;
    }
}

void signalPipeShit(int signum) {
    ERROR("Broken pipe shit " << signum);
}

int main(int argc, const char* const argv[]) {
    if (argc > 2) {
        ERROR("Usage: " << argv[0] << " [configuration file]");
        return 1;
    }

    std::string configPath = "default.conf";
    if (argc == 2) configPath = argv[1];

    ConfigurationParser *parser = new ConfigurationParser();
    parser->parseFile(configPath);
    HTTPRule configs = parser->getResult(configPath);
    delete parser;

    if (configs.servers.empty())
        return (1);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGPIPE, signalPipeShit);

    PRINT("Configuration loaded successfully from " << configPath);
	try{
    	Server server(configs);
		while (!g_quit)
        	server.runOnce();
    	server.cleanUp();
	}
	catch (const std::exception &e)
	{
		std::cout << e.what() << std::endl;
		return (1);
	}

    PRINT("Server shutting down gracefully - how nice ^^");
    return (0);
}
