#pragma once

#include <ostream>
#include <cstdint>
#include <limits>
#include <string>

class LocationRule;

struct StatusCode {
    int value;

    inline explicit StatusCode(int v) : value(v) {
        if ((v < 100 || v > 599) && v != -1)
            throw std::out_of_range("Status code must be between 100 and 599");
    }

    inline static StatusCode Wildcard() {
        return StatusCode(-1);
    }

    operator int() const { return (value); }
};

struct PortNumber {
    int value;

    inline explicit PortNumber(int v) : value(v) {
        if (v < 0 || v > static_cast<int>(std::numeric_limits<uint16_t>::max()))
            throw std::out_of_range("Port number must be between 0 and 65535");
    }

    operator int() const { return (value); }
};

struct DefaultVal {
    bool value;

    inline explicit DefaultVal(bool v) : value(v) {}

    operator bool() const { return (value); }
};

class Size {
private:
	size_t _size;

public:
Size() = default;
    Size(size_t size);
	Size(const std::string &str);
	Size(const Size &other) = default;
	Size &operator=(const Size &other) = default;
    ~Size() = default;

	size_t get() const;
};

class Timespan {
private:
    double _seconds;

public:
    Timespan() = default;
    Timespan(std::string str);
    Timespan(double seconds);
    Timespan(const Timespan &other) = default;
    Timespan &operator=(const Timespan &other) = default;
    ~Timespan() = default;

    double getSeconds() const;
};

class Path {
private:
	std::string _path;
	bool _is_set;

public:
    Path();
	Path(const std::string &str);
	Path(const Path &other);
	Path &operator=(const Path &other);

	static Path createFromUrl(const std::string &url, const LocationRule &route);
	static Path createDummy();

	Path &pop();
	Path &append(const std::string &str);
	Path &updateFromUrl(const std::string &route, const std::string &root);

	std::string getFilename() const;
	const std::string &str() const;
	bool isValid() const;
};

std::ostream &operator<<(std::ostream &os, const Size &size);
std::ostream &operator<<(std::ostream &os, const Path &path);