#pragma once

#include "../config.hpp"

class Size {
private:
	size_t _size;

	void _load_size(const std::string &str);

public:
	Size(const std::string &str);
	Size();

	size_t get() const;
};

class Path {
private:
	std::string _path;
	bool _is_valid;
	
public:
	Path(const std::string &str);
	Path(const Path &other);
	Path();
	Path &operator=(const Path &other);

	static Path create_dummy();
	std::string get_path() const;
	bool is_valid() const;
};

std::ostream& operator<<(std::ostream& os, const Path& path);
std::ostream& operator<<(std::ostream& os, const Size& size);