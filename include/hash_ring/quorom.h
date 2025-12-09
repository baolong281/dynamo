#pragma once

#include "hash_ring/hash_ring.h"
#include <memory>
#include <string>

class Quorom {
    public:
        void replicateReads(const std::string& key);
        void replicatePuts(const std::string& key, const std::string& value);
    private:
        std::shared_ptr<Node> curr_node_;
        std::shared_ptr<HashRing> ring_;
        int R_;
        int W_;
        int N_;
};