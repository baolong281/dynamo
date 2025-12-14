#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "node.h"
#include <shared_mutex>

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

//TODO: add iterator in the future over next ring nodes?
class HashRing {
public:
    explicit HashRing(int n_vnodes = 1000);
    ~HashRing();
    // acquire a shared lock for concurrent reads
    std::shared_ptr<Node> findNode(const std::string& key);
    std::vector<std::shared_ptr<Node>> getNextNodes(const std::string& key, size_t n);
    // acquire unique lock to have only one writer at a time
    void addNode(std::shared_ptr<Node> node);
    void removeNode(const std::string& node_id);
    std::vector<std::shared_ptr<Node>> getNodes() {
        return nodes_;
    }

private:
    uint64_t ring_size_;
    std::multiset<VirtualNode, VirtualNodeCmp> node_ring_;
    std::vector<std::shared_ptr<Node>> nodes_;
    std::shared_mutex rwlock_;
    int n_vnodes_;
};
