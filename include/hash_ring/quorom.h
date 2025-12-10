#pragma once

#include "hash_ring/hash_ring.h"
#include "logging/logger.h"
#include "storage/storage_engine.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

class Quorom {
    public:
        Quorom(int N, int R, int W, std::shared_ptr<Node> node, std::shared_ptr<HashRing> ring) : 
                curr_node_(node),
                ring_(ring),
                N_(N), 
                R_(R), 
                W_(W) {};

        std::vector<ByteString> get(const std::string& key) {
            auto preference_list = ring_->getNextNodes(key, N_);
            std::atomic<int> counter{0};
            std::atomic<bool> timeout{false};
            std::vector<ByteString> values{};
            std::mutex mu{};

            for (auto node : preference_list) {

                if(node->getId() == curr_node_->getId()) {
                    continue;
                }

                std::thread([node, &key, &counter, &values, &mu] {
                    try {
                        std::lock_guard<std::mutex> lock(mu);
                        auto result = node->replicate_get(key);
                        values.push_back(result);
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

        bool put(const std::string& key, const ByteString& value) {
                auto preference_list = ring_->getNextNodes(key, N_);
                std::atomic<int> counter{0};
                std::atomic<bool> timeout{false};

                for (auto node : preference_list) {

                    if(node->getId() == curr_node_->getId()) {
                        continue;
                    }

                    std::thread([node, &key, &value, &counter] {
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

        int getN() {
            return N_;
        }

        std::shared_ptr<Node> getCurrNode() {
            return curr_node_;
        }

    private:
        std::shared_ptr<Node> curr_node_;
        std::shared_ptr<HashRing> ring_;
        int N_;
        int R_;
        int W_;
};