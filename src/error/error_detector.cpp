#include "error/error_detector.h"
#include "logging/logger.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

void ErrorDetector::markSuccess(const std::string& nodeId) {
    err_counts_[nodeId] = 0;
}

void ErrorDetector::markError(const std::string& nodeId) {
    std::shared_ptr<Node> node;
    {
        auto& cnt = err_counts_[nodeId];
        cnt++;

        if (cnt < threshhold_) return;

        // add to queue to check if active periodically
        // set node as inactive too
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (processing_.contains(nodeId)) return;
            node = ring_->getNode(nodeId);
            node->setInactive();
            q_.push(node);
            processing_.insert(nodeId);

        }

        Logger::instance().info("Marking node: " + node->getId() + " in error state.");
        cv_.notify_one();
    }

}

void ErrorDetector::start() {
    running_.store(true);
    t_ = std::thread([this]() {
        std::unique_lock<std::mutex> lk(mu_);
        while (running_.load(std::memory_order_relaxed)) {
            cv_.wait(lk, [&] {
                return !running_.load() || !q_.empty();
            });

            if (!running_.load()) break;

            auto node = q_.front();
            q_.pop();
            lk.unlock();

            // can take a while if node is unreachable
            // do we to set a shorter timeout
            bool healthy = node->checkHealth();

            lk.lock();
            if (!healthy) {
                q_.push(node);
            } else {
                processing_.erase(node->getId());
                node->setActive();
                Logger::instance().info("Marking node: " + node->getId() + " as recovered.");
            }
            lk.unlock();

            // do we even need this sleep?
            std::this_thread::sleep_for(std::chrono::seconds(1));
            lk.lock();
        }
    });
    Logger::instance().info("Starting error detection service in backgruond thread...");
}

ErrorDetector::~ErrorDetector() {
    running_.store(false);
    cv_.notify_all();
    if (t_.joinable()) t_.join();
}