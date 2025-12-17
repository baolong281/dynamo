#pragma once

#include <string>

using ByteString = std::string;

template <typename EngineImpl>
class StorageEngine {
    public:
        ByteString get(const std::string &key) {
            return static_cast<EngineImpl*>(this) -> get(key);
        }

        void put(const std::string &key, const ByteString value) {
            static_cast<EngineImpl*>(this) -> put(key, value);
        }

        bool contains(const std::string &key) {
            return static_cast<EngineImpl*>(this) -> contains(key);
        }

        void remove(const std::string &key) {
            return static_cast<EngineImpl*>(this) -> remove(key);
        }
};