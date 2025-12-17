#pragma once

#include "error/error_detector.h"
#include "hash_ring/hash_ring.h"
#include "storage/value.h"
#include <httplib.h>
#include <memory>
#include <string>

class Quorom {
    public:
        Quorom(int N, int R, int W, std::shared_ptr<Node> node, std::shared_ptr<HashRing> ring, std::shared_ptr<ErrorDetector> err_detector) : 
                curr_node_(node),
                ring_(ring),
                N_(N), 
                R_(R), 
                W_(W),
                err_detector_(err_detector) {};

        // TODO: FIX SEGFAULT RACE CONDITION
        ValueList get(const std::string& key);

        bool put(const std::string& key, const Value& value);

        int getN();

        std::shared_ptr<Node> getCurrNode();

    private:
        std::shared_ptr<Node> curr_node_;
        std::shared_ptr<HashRing> ring_;
        std::shared_ptr<ErrorDetector> err_detector_;
        int N_;
        int R_;
        int W_;
};