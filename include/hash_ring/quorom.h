#pragma once

#include "hash_ring/hash_ring.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>

class Quorom {
    public:
        Quorom(int N, int R, int W, std::shared_ptr<Node> node, std::shared_ptr<HashRing> ring) : N_(N), R_(R), W_(W), curr_node_(node), ring_(ring) {};
        void replicateReads(const std::string& key) {}

        bool replicatePuts(const std::string& key, const std::string& value) {
                auto preference_list = ring_->getNextNodes(key, N_);
                std::atomic<int> counter{0};
                std::atomic<bool> timeout{false};

                // this may not work
                for (auto node : preference_list) {

                    if(node->getId() == curr_node_->getId()) {
                        continue;
                    }

                    std::thread([node, &key, &value, &counter] {
                        auto result = node->replicate_put(key, value);
                        if (result) {
                            counter.fetch_add(1, std::memory_order_relaxed);
                        }
                    }).detach(); 
                }

                // TODO: configurable timeout
                std::thread([&timeout] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    timeout = true;
                }).detach();

                // busy spin later(?)
                while (counter < W_  - 1 && !timeout) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                return counter >= W_ - 1;
        }

        int getN() {return N_;}
        std::shared_ptr<Node> getCurrNode() {return curr_node_;}
    private:
        std::shared_ptr<Node> curr_node_;
        std::shared_ptr<HashRing> ring_;
        int N_;
        int R_;
        int W_;
};