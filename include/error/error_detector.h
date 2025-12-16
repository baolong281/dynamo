#pragma once

#include "hash_ring/hash_ring.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
class ErrorDetector {
    public:
        ErrorDetector(std::shared_ptr<HashRing> ring, int threshhold) :
            ring_(ring),
            threshhold_(threshhold) {}
        ~ErrorDetector();
        void markError(const std::string& key);
        void markSuccess(const std::string& key);
        void start();

    private:
        std::unordered_map<std::string, std::atomic<int>> err_counts_;
        std::shared_ptr<HashRing> ring_;
        std::queue<std::shared_ptr<Node>> q_;
        std::unordered_set<std::string> processing_;
        std::thread t_;
        std::mutex mu_;
        std::condition_variable cv_;
        std::atomic<bool> running_;
        int threshhold_;
};