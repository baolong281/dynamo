#include "storage/disk_engine.h"
#include "leveldb/db.h"
#include <stdexcept>
#include <cassert>

const std::string DB_PATH{"/tmp/dynamo"};

DiskEngine::DiskEngine(std::string id) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options,  DB_PATH + id, &db_);
    assert(status.ok());
}

DiskEngine::~DiskEngine() {
    delete db_;
}

DiskEngine::DiskEngine(DiskEngine&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

DiskEngine& DiskEngine::operator=(DiskEngine&& other) noexcept {
    if(this != &other) {
        delete db_;
        this->db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

ByteString DiskEngine::get(const std::string &key) {
    std::string data{};
    leveldb::Status s = db_ -> Get(leveldb::ReadOptions(), key, &data);
    // we do not consider not found to be an error
    if(!s.ok() && !s.IsNotFound()) {
        throw std::runtime_error("Error fetching key: " + s.ToString());
    }
    return data;
}

void DiskEngine::put(const std::string &key, const ByteString value) {
    leveldb::Status s = db_ -> Put(leveldb::WriteOptions(), key, value);
    if(!s.ok()) {
        throw StorageError("Error writing key: " + s.ToString());
    }
}

// TODO
// this does two reads, make better later
bool DiskEngine::contains(const std::string &key) {
    std::string data{};
    leveldb::Status s = db_ -> Get(leveldb::ReadOptions(), key, &data);
    return !s.IsNotFound();
}
