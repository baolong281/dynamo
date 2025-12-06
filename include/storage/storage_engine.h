#pragma once

#include <cstddef>
#include <vector>

struct Value {
    std::vector<std::byte> data;
};

template <typename EngineImpl>
class StorageEngine {
    public:
        Value get(const std::string &key) {
            static_cast<EngineImpl*>(this) -> get(key);
        }

        void put(const std::string &key, const Value value) {
            static_cast<EngineImpl*>(this) -> put(key, value);
        }
};