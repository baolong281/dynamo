#pragma once

#include "logging/logger.h"
#include "storage_engine.h"
#include "leveldb/db.h"
#include <cassert>

const std::string DB_PATH{"/tmp/dynamo"};

class DiskEngine : public StorageEngine<DiskEngine> {
    public:
        DiskEngine(std::string id) {
            leveldb::Options options;
            options.create_if_missing = true;
            leveldb::Status status = leveldb::DB::Open(options,  DB_PATH + id, &db_);
            assert(status.ok());
        }

        ~DiskEngine() {
            delete db_;
        }

        DiskEngine(DiskEngine&& other) noexcept : db_(other.db_) {
            other.db_ = nullptr;
        }

        DiskEngine& operator=(DiskEngine&& other) noexcept {
            if(this != &other) {
                delete db_;
                this->db_ = other.db_;
                other.db_ = nullptr;
            }
            return *this;
        }

        ByteString get(const std::string &key) {
            std::string data{};
            leveldb::Status s = db_ -> Get(leveldb::ReadOptions(), key, &data);
            if(!s.ok()) {
                throw std::runtime_error("Error fetching key");
            }
            return data;
        }

        void put(const std::string &key, const ByteString value) {
            std::string data{};
            leveldb::Status s = db_ -> Put(leveldb::WriteOptions(), key, value);
            if(!s.ok()) {
                throw std::runtime_error("Error writing key");
            }
        }

        // TODO
        // this does two reads, make better later
        bool contains(const std::string &key) {
            std::string data{};
            leveldb::Status s = db_ -> Get(leveldb::ReadOptions(), key, &data);
            return !s.IsNotFound();
        }
    
    private: 
        leveldb::DB* db_;
};