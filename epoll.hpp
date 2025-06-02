#pragma once

#include <sys/epoll.h>

#define EPOLL_INPUT_EVENTS (EPOLLIN | EPOLLET)
#define EPOLL_OUTPUT_EVENTS (EPOLLOUT | EPOLLET)

class Epoll {
private:
	int _epoll_fd;

public:
	Epoll();
	Epoll(const Epoll &other) = default;
	Epoll &operator=(const Epoll &other) = default;
	~Epoll() = default;

	void create();
	void destroy();

	bool add(int fd, uint32_t events);
	bool modify(int fd, uint32_t events);
	bool remove(int fd);

	inline const int getEpollFd() const { return _epoll_fd; }
};
