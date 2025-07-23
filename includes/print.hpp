#pragma once

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

#define TERM_COLOR_RESET "\033[0m"
#define TERM_COLOR_RED "\033[31m"
#define TERM_COLOR_GREEN "\033[32m"
#define TERM_COLOR_YELLOW "\033[33m"
#define TERM_COLOR_BLUE "\033[34m"
#define TERM_COLOR_MAGENTA "\033[35m"
#define TERM_COLOR_CYAN "\033[36m"

#define TERM_BOLD "\033[1m"
#define TERM_UNDERLINE "\033[4m"

inline std::ostream& print_time(std::ostream& os) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return os << TERM_COLOR_CYAN << "[" << std::put_time(&tm, "%H:%M:%S") << "] ";
}

#define PRINT(x) do { \
    print_time(std::cout); \
    std::cout << __FILE__ << ":" << __LINE__ << " " TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
} while (0)
#define PRINT_IF(cond, x) do { \
    if (cond) { \
        print_time(std::cout); \
        std::cout << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
    } \
} while (0)
#define PRINT_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        print_time(std::cout); \
        std::cout << TERM_COLOR_YELLOW << "[COND: !(" #cond ")] " << TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
    } \
} while (0)
#define ERROR(x) do { \
    print_time(std::cerr); \
    std::cerr << TERM_COLOR_RED << "[ERROR " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_RESET << x << std::endl; \
} while (0)
#define ERROR_IF(cond, x) do { \
    if (cond) { \
        print_time(std::cerr); \
        std::cerr << TERM_COLOR_RED << "[ERROR " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
#define ERROR_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        print_time(std::cerr); \
        std::cerr << TERM_COLOR_RED << "[ERROR " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: !(" #cond ")] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
#define ERROR_RET_IF(cond, x) do { \
    if (cond) { \
        print_time(std::cerr); \
        std::cerr << TERM_COLOR_RED << "[ERROR " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
        return; \
    } \
} while (0)
#define ERROR_RET_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        print_time(std::cerr); \
        std::cerr << TERM_COLOR_RED << "[ERROR " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: !(" #cond ")] " << TERM_COLOR_RESET << x << std::endl; \
        return; \
    } \
} while (0)

#ifdef DEBUG_MODE
# define DEBUG(x) do { \
    print_time(std::cout); \
    std::cout << TERM_COLOR_GREEN << "[DEBUG " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_RESET << x << std::endl; \
} while (0)
# define DEBUG_IF(cond, x) do { \
    if (cond) { \
        print_time(std::cout); \
        std::cout << TERM_COLOR_GREEN << "[DEBUG " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
# define DEBUG_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        print_time(std::cout); \
        std::cout << TERM_COLOR_GREEN << "[DEBUG " << __FILE__ << ":" << __LINE__ << "] " << TERM_COLOR_YELLOW << "[COND: !" #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
#else
# define DEBUG(x) do {} while (0)
# define DEBUG_IF(cond, x) do {} while (0)
# define DEBUG_IF_NOT(cond, x) do {} while (0)
#endif