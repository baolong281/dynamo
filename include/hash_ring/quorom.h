#pragma once

#include "hash_ring/hash_ring.h"
#include <memory>
#include <string>

class Quorom {
    public:
        Quorom(int N, int R, int W, std::shared_ptr<Node> node) : N_(N), R_(R), W_(W), curr_node_(node) {};
        void replicateReads(const std::string& key) {}
        void replicatePuts(const std::string& key, const std::string& value) {}
        int getN() {return N_;}
        std::shared_ptr<Node> getCurrNode() {return curr_node_;}
    private:
        std::shared_ptr<Node> curr_node_;
        std::shared_ptr<HashRing> ring_;
        int N_;
        int R_;
        int W_;
};