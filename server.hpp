#pragma once

#include "config/rules/rules.hpp"
#include "request.hpp"
#include "session.hpp"
#include "client.hpp"

#include <map>

class Server {
private:
	std::vector<ServerConfig> _configs;
	UserSessionManager _sessionManager;

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
	void _prepareRequestProcessing(Client &client);
	ServerConfig& _loadRequestConfig(Request &request);

public:
	Server(const std::vector<ServerConfig> &configs);
	Server(const Server &other);
	Server &operator=(const Server &other);
	~Server();

	void cleanUp();
	void runOnce();
};
