#pragma once

#include "config/rules/rules.hpp"
#include "client.hpp"

#include <map>

class Server {
private:
	ServerConfig _config;

	int _server_fd;
	int _epoll_fd;

	std::map<int, Client> _clients;

	void _setup_socket();
	void _setup_epoll();

	void _handle_new_connection();
	void _handle_client_io(int fd, short revents);
	void _remove_client(Client &client);
	void _remove_client(int fd);

	void _handle_client_input(int fd, Client &client);
	void _handle_client_output(int fd, Client &client);


public:
	Server(const ServerConfig &config);
	~Server();

	void run_once();
};
