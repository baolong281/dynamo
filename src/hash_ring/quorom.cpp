#include "hash_ring/quorom.h"
#include "hash_ring/node.h"
#include "logging/logger.h"
#include "storage/value.h"
#include "error/quorom_error.h"
#include <future>
#include <thread>
#include <atomic>
#include <mutex>

ValueList Quorom::get(const std::string& key) {
    auto nodes = ring_->getNextNodes(key, N_ * 2);

    if(nodes.size() < N_) {
        throw QuoromError("Replica size larger than current cluster size!");
    }

    ValueList values;
    values.reserve(R_);

    std::atomic<int> received{0};
    std::mutex m;

    std::vector<std::future<void>> futures;

    for(int i = 0; i < N_; i++) {

        std::shared_ptr<Node> node = nodes.at(i);
        if (node->getId() == curr_node_->getId()) continue;

        auto f = [&](std::shared_ptr<Node> node){
            std::optional<ValueList> result = node->replicateGet(key);
            if (result.has_value()) {
                err_detector_->markSuccess(node->getId());
                received.fetch_add(1, std::memory_order_relaxed);
                {
                    std::lock_guard lk(m);
                    for(auto &v : result.value()) {
                        values.push_back(std::move(v));
                    }
                }
            } else {
                err_detector_->markError(node->getId());
                Logger::instance().error("Get replication request for key '" + key + "' to node" + node->getId() + " failed!");
            }
            return result.has_value();
        };

        futures.push_back(std::async(std::launch::async,
            [&, i, node] {
                bool success = f(node);
                int idx = N_ + i - 1;
                if(!success && idx < nodes.size()) {
                    std::shared_ptr<Node> next_node = nodes.at(idx);
                    f(next_node);
                }
            }
        ));
    }

    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(100);

    while (received.load() < R_ - 1 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (received.load() < R_ - 1) {
        throw std::runtime_error("Not enough read responses");
    }

    return values;
}


bool Quorom::put(const std::string& key, const Value& value) {
        auto preference_list = ring_->getNextNodes(key, N_ * 2);

        if(preference_list.size() < N_) {
            throw QuoromError("Replica size larger than current cluster size!");
        }

        std::atomic<int> received{0};
        std::vector<std::future<void>> futures;

        // this may need to be rewritten
        // this does not seem like it works well
        // we send at most N_ * 2 requests, but we may need more in extreme situations
        for(int i = 0; i < N_; i++) {
            std::shared_ptr<Node> node = preference_list.at(i);

            if(node->getId() == curr_node_->getId()) {
                continue;
            }

            auto f = [&](std::shared_ptr<Node> node, bool handoff=false, const std::string node_id = ""){
                bool success;
                if(handoff) {
                    success = node->replicateHandoff(key, value, node_id);
                } else {
                    success = node->replicatePut(key, value);
                }

                if (success) {
                    err_detector_->markSuccess(node->getId());
                    received.fetch_add(1, std::memory_order_relaxed);
                } else {
                    err_detector_->markError(node->getId());
                    Logger::instance().error("Put replication request for key '" + key + "' to node" + node->getId() + " failed!");
                }
                return success;
            };

            futures.push_back(std::async(std::launch::async, [&, node, i] {
                bool success = f(node);
                // -1 necesscary ?
                int idx = N_ + i - 1;
                if(!success && idx < preference_list.size()) {
                    std::shared_ptr<Node> next_node = preference_list.at(idx);
                    f(next_node, true, node->getId());
                }
            }));

        }

        auto deadline = std::chrono::steady_clock::now()
                    + std::chrono::milliseconds(100);

        while (received.load() < W_ - 1 &&
            std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (received.load() < W_ - 1) {
            throw std::runtime_error("Not enough responses for put requet");
        }

        return received >= W_ - 1;
}

int Quorom::getN() {
    return N_;
}

std::shared_ptr<Node> Quorom::getCurrNode() {
    return curr_node_;
}
