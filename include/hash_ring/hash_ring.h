#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include "node.h"

struct VirtualNode {
    std::string id_;
    uint64_t position_;
    std::shared_ptr<Node> parent_;
};

struct VirtualNodeCmp {
    bool operator()(const VirtualNode& lhs, const VirtualNode& rhs) const {
        return lhs.position_ < rhs.position_;
    }
};

uint64_t md5_hash_64(const std::string& key);

class HashRing {
public:
    explicit HashRing(int n_vnodes = 1000);
    ~HashRing();

    std::shared_ptr<Node> findNode(const std::string& key);
    void addNode(Node node);
    void removeNode(const std::string& node_id);

private:
    uint64_t ring_size_;
    std::multiset<VirtualNode, VirtualNodeCmp> node_ring_;
    std::set<std::shared_ptr<Node>> nodes_;
    int n_vnodes_;
};
