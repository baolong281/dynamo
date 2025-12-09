#pragma once

#include "storage_engine.h"
#include <boost/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

class MemoryEngine : public StorageEngine<MemoryEngine> {
    public:
        MemoryEngine() = default;

        ByteString get(const std::string &key) {
            auto it = map_.find(key);
            if(it != map_.end()) {
                return it -> second;
            }
            throw std::runtime_error("Key not found");
        }

        bool contains(const std::string &key) {
            return map_.contains(key);
        }

        void put(const std::string &key, const ByteString value) {
            map_[key] = value;
        }
    
    private: 
        boost::unordered_flat_map<std::string, ByteString>  map_;
};