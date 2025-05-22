#include "server.hpp"
#include "config/rules/rules.hpp"

#include <iostream>
#include <exception>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct epoll_event epoll_event;

class ServerCreationException : public std::exception {
private:
	std::string _message;

public:
	ServerCreationException(const std::string &message) : _message(message) {}
	const char *what() const noexcept override {
		return _message.c_str();
	}
};

class ClientConnectionException : public std::exception {
private:
	std::string _message;

public:
	ClientConnectionException(const std::string &message) : _message(message) {}
	const char *what() const noexcept override {
		return _message.c_str();
	}
};

Server::Server(const ServerConfig &config) :
	_config(config),
	_server_fd(-1),
	_epoll_fd(-1) {

	this->_setup_socket();
	this->_setup_epoll();
	std::cout << "Server " << _config.server_name.get() << " is running on port " << _config.port.get() << std::endl;
}

Server::~Server() {
	if (_server_fd != -1)
		close(_server_fd);
	if (_epoll_fd != -1)
		close(_epoll_fd);
}

void Server::_setup_socket() {
	_server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_server_fd == -1)
		throw ServerCreationException("Failed to create socket");

	int enable = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		throw ServerCreationException("Failed to set socket options");

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(_config.port.get());

	if (bind(_server_fd, (sockaddr *)&address, sizeof(address)) == -1)
		throw ServerCreationException("Failed to bind socket");

	if (listen(_server_fd, SOMAXCONN) == -1)
		throw ServerCreationException("Failed to listen on socket");
}

void Server::_setup_epoll() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
		throw ServerCreationException("Failed to create epoll instance");

	epoll_event event{};
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = _server_fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &event) == -1) {
		perror("epoll_ctl");
		throw ServerCreationException("Failed to add socket to epoll");
	}
}

void Server::_handle_new_connection() {
	sockaddr_in client_address{};
	socklen_t client_len = sizeof(client_address);
	int client_fd = accept(_server_fd, (sockaddr *)&client_address, &client_len);
	if (client_fd == -1)
		throw ClientConnectionException("Failed to accept new connection");
	
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
		close(client_fd);
		throw ClientConnectionException("Failed to set client socket to non-blocking");
	}

	epoll_event event{};
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = client_fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
		close(client_fd);
		throw ClientConnectionException("Failed to add client socket to epoll");
	}

	_clients.emplace(client_fd, Client(client_fd));
	std::cout << "New client connected: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << std::endl;
}

void Server::_remove_client(int fd) {
	epoll_event event{};
	event.data.fd = fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &event) == -1) {
		std::cerr << "Failed to remove client from epoll: " << fd << std::endl;
	}
	auto it = _clients.find(fd);
	if (it == _clients.end()) {
		std::cerr << "Client not found: " << fd << std::endl;
		return ;
	}
	Client &client = it->second;
	client.cleanup();
	_clients.erase(it);
	std::cout << "Client disconnected: " << fd << std::endl;
}

void Server::_handle_client_input(int fd, Client &client) {
	if (!client.read_request()) {
		std::cerr << "Failed to read request from client: " << fd << std::endl;
		_remove_client(client.get_socket());
		return ;
	}

	if (client.request.is_complete())
	{
		epoll_event event{};
		event.events = EPOLLOUT | EPOLLET;
		event.data.fd = fd;
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
			std::cerr << "Failed to modify epoll event for client: " << fd << std::endl;
			_remove_client(client.get_socket());
			return ;
		}
		client.process_request(_config);
	}
}

void Server::_handle_client_output(int fd, Client &client) {
	if (!client.send_response()) {
		std::cerr << "Failed to send response to client: " << fd << std::endl;
		_remove_client(client.get_socket());
		return ;
	}

	if (client.is_response_complete()) {
		epoll_event event{};
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = fd;
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
			std::cerr << "Failed to modify epoll event for client: " << fd << std::endl;
			_remove_client(client.get_socket());
			return ;
		}
		client.reset();
	}
}

void Server::_handle_client_io(int fd, short revents) {
	auto it = _clients.find(fd);
	if (it == _clients.end()) {
		std::cerr << "Client not found: " << fd << std::endl;
		return ;
	}
	Client &client = it->second;

	std::cout << "Handling client IO: " << fd << std::endl;
	std::cout << "Events: " << revents << std::endl;
	std::cout << (revents & EPOLLIN) << std::endl;
	std::cout << (revents & EPOLLOUT) << std::endl;
	std::cout << (revents & (EPOLLERR | EPOLLHUP)) << std::endl;

	if (revents & EPOLLIN)
		_handle_client_input(fd, client);
	else if (revents & EPOLLOUT)
		_handle_client_output(fd, client);
	else if (revents & (EPOLLERR | EPOLLHUP)) {
		std::cerr << "Error on client socket: " << fd << std::endl;
		_remove_client(fd);
	}
}

void Server::run_once() {
	epoll_event events[64];

	int event_count = epoll_wait(_epoll_fd, events, 64, 0);
	if (event_count == -1)
		throw ServerCreationException("Failed to wait for epoll events");
	for (int i = 0; i < event_count; ++i) {
		std::cout << "Event: " << events[i].events << std::endl;
		if (events[i].data.fd == _server_fd) {
			_handle_new_connection();
		}
		else {
			_handle_client_io(events[i].data.fd, events[i].events);
		}
	}
}
