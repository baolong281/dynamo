#include "hash_ring/quorom.h"
#include "hash_ring/node.h"
#include "logging/logger.h"
#include <future>
#include <thread>
#include <atomic>
#include <mutex>

ValueList Quorom::get(const std::string& key) {
    auto nodes = ring_->getNextNodes(key, N_);

    ValueList values;
    values.reserve(R_);

    std::atomic<int> received{0};
    std::mutex m;

    std::vector<std::future<void>> futures;

    for (auto& node : nodes) {
        if (node->getId() == curr_node_->getId()) continue;

        futures.push_back(std::async(std::launch::async,
            [&, node] {
                auto result = node->replicate_get(key);
                {
                    std::lock_guard lk(m);
                    for(auto &v : result) {
                        values.push_back(std::move(v));
                    }
                }
                received++;
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
        auto preference_list = ring_->getNextNodes(key, N_);
        std::atomic<int> received{0};

        std::vector<std::future<void>> futures;

        for (auto node : preference_list) {

            if(node->getId() == curr_node_->getId()) {
                continue;
            }

            futures.push_back(std::async(std::launch::async, [&, node] {
                auto result = node->replicate_put(key, value);
                if (result) {
                    received.fetch_add(1, std::memory_order_relaxed);
                } else {
                    Logger::instance().error("Put replication request for key '" + key + "' to node" + node->getId() + " failed!");
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
