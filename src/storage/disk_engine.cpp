#include "storage/disk_engine.h"
#include "leveldb/db.h"
#include "logging/logger.h"
#include "error/storage_error.h"

const std::string DB_PATH{"/tmp/dynamo"};

DiskEngine::DiskEngine(std::string id, const std::string& postfix) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options,  DB_PATH + id + postfix, &db_);
    if(!status.ok()) {
        Logger::instance().error(status.ToString());
        throw StorageError("Error instantiating leveldb engine...");
    }
}

DiskEngine::~DiskEngine() {
    Logger::instance().debug("Killing disk engine...");
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
        throw StorageError("Error fetching key: " + s.ToString());
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

void DiskEngine::remove(const std::string &key) {
    leveldb::Status s = db_->Delete(leveldb::WriteOptions(), key);
    if(!s.ok()) {
        throw StorageError("Error remove key key: " + key + ". " + s.ToString());
    }
}