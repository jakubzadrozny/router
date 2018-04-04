// Jakub Zadrozny 290920
#include "handler.h"

#include <cstdlib>
#include <cerrno>

#include <iostream>
#include <string>

void handle_error(const std::string &msg, bool is_errno) {
    std::cerr << msg << "\n";
    if (is_errno) {
        std::cerr << std::strerror(errno) << "\n";
    }
    exit(EXIT_FAILURE);
}
