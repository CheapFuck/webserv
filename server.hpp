#pragma once

#include "config/rules/rules.hpp"
#include "request.hpp"
#include "client.hpp"

#include <map>

class Server {
private:
	ServerConfig _config;

	int _server_fd;
	int _epoll_fd;

	std::map<int, Client> _clients;

	void _setupSocket();
	void _setupEpoll();

	bool _epollExecute(int fd, uint32_t operation, uint32_t events);

	void _handleNewConnection();
	void _handleClientIo(int fd, short revents);
	void _removeClient(int fd);

	void _handleClientInput(int fd, Client &client);
	void _handleClientOutput(int fd, Client &client);

public:
	Server(const ServerConfig &config);
	~Server();

	void run_once();
};
