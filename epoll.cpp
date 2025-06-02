#include "epoll.hpp"

#include <sys/epoll.h>
#include <stdexcept>
#include <unistd.h>

Epoll::Epoll() {
	_epoll_fd = -1;
}

Epoll::~Epoll() {}

void Epoll::create() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
		throw std::runtime_error("Failed to create epoll instance");
}

void Epoll::destroy() {
	if (_epoll_fd != -1) {
		close(_epoll_fd);
		_epoll_fd = -1;
	}
}

bool Epoll::add(int fd, uint32_t events) {
	epoll_event event{};
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
		return (false);
	return (true);
}

bool Epoll::modify(int fd, uint32_t events) {
	epoll_event event{};
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1)
		return (false);
	return (true);
}

bool Epoll::remove(int fd) {
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		return (false);
	return (true);
}
