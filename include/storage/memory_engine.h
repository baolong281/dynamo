#pragma once

#include "storage_engine.h"
#include <boost/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

class MemoryEngine : public StorageEngine<MemoryEngine> {
    public:
        MemoryEngine() = default;

        ByteString get(const std::string &key);

        bool contains(const std::string &key);

        void put(const std::string &key, const ByteString value);
        void remove(const std::string &key);
    
    private: 
        boost::unordered_flat_map<std::string, ByteString>  map_;
};