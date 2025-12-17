#include "storage/memory_engine.h"
#include "error/storage_error.h"

ByteString MemoryEngine::get(const std::string &key) {
    auto it = map_.find(key);
    if(it != map_.end()) {
        return it -> second;
    }
    throw StorageError("Key not found");
}

bool MemoryEngine::contains(const std::string &key) {
    return map_.contains(key);
}

void MemoryEngine::put(const std::string &key, const ByteString value) {
    map_[key] = value;
}

void MemoryEngine::remove(const std::string &key) {
    map_.erase(key);
}
