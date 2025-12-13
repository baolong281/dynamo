#include "storage/memory_engine.h"

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
