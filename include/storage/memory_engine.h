#pragma once

#include "storage_engine.h"
#include <boost/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <iostream>

class MemoryEngine : public StorageEngine<MemoryEngine> {
    public:
        MemoryEngine() = default;

        Value get(const std::string &key) {
            auto it = map_.find(key);
            if(it != map_.end()) {
                return it -> second;
            }
            throw std::runtime_error("Key not found");
        }

        void put(const std::string &key, const Value value) {
            map_.insert(std::pair{key, value});
        }
    
    private: 
        boost::unordered_flat_map<std::string, Value>  map_;
};