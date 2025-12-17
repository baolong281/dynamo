#pragma once

#include "storage_engine.h"
#include <string>
#include "leveldb/db.h"

namespace leveldb {
    class DB;
}

class DiskEngine : public StorageEngine<DiskEngine> {
    public:
        DiskEngine(std::string id, const std::string& postfix);
        ~DiskEngine();
        DiskEngine(DiskEngine&& other) noexcept;
        DiskEngine& operator=(DiskEngine&& other) noexcept;
        ByteString get(const std::string &key);
        // the caller must not delete this pointer
        leveldb::DB* getDB() {
            return db_;
        }

        void put(const std::string &key, const ByteString value);
        void remove(const std::string &key);

        // TODO
        // this does two reads, make better later
        bool contains(const std::string &key);
    
    private: 
        leveldb::DB* db_;
};