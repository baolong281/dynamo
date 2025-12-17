#pragma once

#include "hash_ring/hash_ring.h"
#include "storage/disk_engine.h"
#include "storage/value.h"
#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include "storage/serializer.h"
#include "logging/logger.h"

struct HandoffData {
    template <class Archive>
    void serialize(Archive & archive) {
        archive( 
            targets_,
            data
        );
    }
    // vector of set since this is at most like N nodes
    std::vector<std::string> targets_;
    Value data;
};


template<typename T> 
bool contains(const std::vector<T>& vec, const T& item) {
    return std::find(vec.begin(), vec.end(), item) !=  vec.end();
}

// abstract underlying storage layer out later
// TODO: 
class Handoff {
    private:
        std::shared_ptr<DiskEngine> storage_;
        std::shared_ptr<HashRing> ring_;
        std::thread t_;
        std::atomic<bool> running_;

    public:
        Handoff(std::shared_ptr<DiskEngine> storage, std::shared_ptr<HashRing> ring) :
            ring_(ring),
            storage_(storage) {}

        void append(const std::string& key, const std::string& tag, const Value& val) {
            HandoffData handoff = Serializer::fromBinary<HandoffData>(storage_ -> get(key));

            // add to targets if not already present
            if (std::find(handoff.targets_.begin(), handoff.targets_.end(), tag) == handoff.targets_.end()) {
                handoff.targets_.push_back(tag);
            }

            handoff.data = val;

            storage_->put(key, Serializer::toBinary(handoff));
        }

        void start() {
            if (t_.joinable()) return;

            running_.store(true);

            t_ = std::thread([this] {
                while (running_.load()) {
                    leveldb::DB* db = storage_ -> getDB();
                    auto snapshot = db->GetSnapshot();

                    auto ro = leveldb::ReadOptions();
                    ro.snapshot = snapshot;

                    auto it = std::unique_ptr<leveldb::Iterator>(
                        db->NewIterator(ro)
                    );

                    for (it->SeekToFirst();
                        it->Valid() && running_.load();
                        it->Next()) {

                        std::string key(it->key().data(), it->key().size());
                        HandoffData data =
                            Serializer::fromBinary<HandoffData>(std::string(it->value().data(), it->value().size()));

                        std::vector<std::string> succeeded;
                        succeeded.reserve(data.targets_.size());

                        for (const auto& target : data.targets_) {
                            if (!running_.load()) break;

                            auto node = ring_->getNode(target);
                            
                            Logger::instance().debug("Attempting to replicate to: " + target);
                            if (node && node->replicatePut(key, data.data)) {
                                Logger::instance().debug("Handoff successful to: " + target);
                                succeeded.push_back(target);
                            }
                        }

                        data.targets_.erase(
                            std::remove_if(
                                data.targets_.begin(),
                                data.targets_.end(),
                                [&](const std::string& t) {
                                    return contains(succeeded, t);
                            }),
                            data.targets_.end()
                        );

                        if (data.targets_.empty()) {
                            storage_->remove(key);
                        } else {
                            storage_->put(key, Serializer::toBinary(data));
                        }
                    }

                    db -> ReleaseSnapshot(snapshot);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
        });

        Logger::instance().info("Starting handoff process in background thread...");
    }

    void stop() {
        Logger::instance().info("Stopping handoff process...");
        running_.store(false);
        if (t_.joinable()) {
            t_.join();
        }
    }
};