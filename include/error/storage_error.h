#pragma once

#include <stdexcept>
#include <string>

class StorageError : public std::runtime_error {
public:
    explicit StorageError(const std::string& msg)
        : std::runtime_error(msg) {}
};
