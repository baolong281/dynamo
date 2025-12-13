#include "hash_ring/quorom.h"
#include "hash_ring/node.h"
#include "logging/logger.h"
#include <thread>
#include <atomic>
#include <mutex>

ValueList Quorom::get(const std::string& key) {
    auto preference_list = ring_->getNextNodes(key, N_);
    std::atomic<int> counter{0};
    std::atomic<bool> timeout{false};
    ValueList values{};
    std::mutex mu{};

    for (auto node : preference_list) {

        if(node->getId() == curr_node_->getId()) {
            continue;
        }

        // function will exit and values will be destroyed, resulting in segfault
        std::thread([node, &key, &counter, &values, &mu] {
            try {
                std::lock_guard<std::mutex> lock(mu);
                auto result = node->replicate_get(key);
                for(auto &v : result) {
                    values.push_back(v);
                }
                counter.fetch_add(1, std::memory_order_relaxed);
            } catch (std::runtime_error e) {
                Logger::instance().error(e.what());
            }
        }).detach(); 
    }

    std::thread([&timeout] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timeout = true;
    }).detach();

    while (counter < R_  - 1 && !timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if(counter < R_ - 1) {
        throw std::runtime_error("Did not recieve enough read responses!" + std::to_string(counter) + "<" + std::to_string(R_ - 1));
    }

    return values;
}

bool Quorom::put(const std::string& key, const Value& value) {
        auto preference_list = ring_->getNextNodes(key, N_);
        std::atomic<int> counter{0};
        std::atomic<bool> timeout{false};

        for (auto node : preference_list) {

            if(node->getId() == curr_node_->getId()) {
                continue;
            }

            std::thread([node, &key, &value, &counter] {
                // put rpcs are constructed over and over again in a loop
                // rewrite? 
                // TODO
                auto result = node->replicate_put(key, value);
                if (result) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                } else {
                    Logger::instance().error("Put replication request for key '" + key + "' to node" + node->getId() + " failed!");
                }
            }).detach(); 
        }

        std::thread([&timeout] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timeout = true;
        }).detach();

        while (counter < W_  - 1 && !timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return counter >= W_ - 1;
}

int Quorom::getN() {
    return N_;
}

std::shared_ptr<Node> Quorom::getCurrNode() {
    return curr_node_;
}
