#pragma once

#include <iostream>

#define TERM_COLOR_RESET "\033[0m"
#define TERM_COLOR_RED "\033[31m"
#define TERM_COLOR_GREEN "\033[32m"
#define TERM_COLOR_YELLOW "\033[33m"
#define TERM_COLOR_BLUE "\033[34m"
#define TERM_COLOR_MAGENTA "\033[35m"
#define TERM_COLOR_CYAN "\033[36m"

#define PRINT(x) do { \
    std::cout << TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
} while (0)
#define PRINT_IF(cond, x) do { \
    if (cond) { \
        std::cout << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
    } \
} while (0)
#define PRINT_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        std::cout << TERM_COLOR_YELLOW << "[COND: !" #cond "] " TERM_COLOR_BLUE << x << TERM_COLOR_RESET << std::endl; \
    } \
} while (0)
#define ERROR(x) do { \
    std::cerr << TERM_COLOR_RED << "[ERROR] " << TERM_COLOR_RESET << x << std::endl; \
} while (0)
#define ERROR_IF(cond, x) do { \
    if (cond) { \
        std::cerr << TERM_COLOR_RED << "[ERROR] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
#define ERROR_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        std::cerr << TERM_COLOR_RED << "[ERROR] " << TERM_COLOR_YELLOW << "[COND: !" #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)

#ifdef DEBUG_MODE
# define DEBUG(x) do { \
    std::cout << TERM_COLOR_GREEN << "[DEBUG] " << TERM_COLOR_RESET << x << std::endl; \
} while (0)
# define DEBUG_IF(cond, x) do { \
    if (cond) { \
        std::cout << TERM_COLOR_GREEN << "[DEBUG] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
# define DEBUG_IF_NOT(cond, x) do { \
    if (!(cond)) { \
        std::cout << TERM_COLOR_GREEN << "[DEBUG] " << TERM_COLOR_YELLOW << "[COND: " #cond "] " << TERM_COLOR_RESET << x << std::endl; \
    } \
} while (0)
#else
# define DEBUG(x) do {} while (0)
# define DEBUG_IF(cond, x) do {} while (0)
# define DEBUG_IF_NOT(cond, x) do {} while (0)
#endif