#pragma once

#include <stdexcept>
#include <string>

class QuoromError : public std::runtime_error {
public:
    explicit QuoromError(const std::string& msg)
        : std::runtime_error(msg) {}
};
