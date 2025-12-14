#pragma once

#include "error/storage_error.h"
#include "logging/logger.h"
#include "storage_engine.h"
#include <string>

namespace leveldb {
    class DB;
}

class DiskEngine : public StorageEngine<DiskEngine> {
    public:
        DiskEngine(std::string id);

        ~DiskEngine();

        DiskEngine(DiskEngine&& other) noexcept;

        DiskEngine& operator=(DiskEngine&& other) noexcept;

        ByteString get(const std::string &key);

        void put(const std::string &key, const ByteString value);

        // TODO
        // this does two reads, make better later
        bool contains(const std::string &key);
    
    private: 
        leveldb::DB* db_;
};