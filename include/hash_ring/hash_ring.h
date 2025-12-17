#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "node.h"
#include <shared_mutex>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct VirtualNode {
    std::string id_;
    uint64_t position_;
    std::shared_ptr<Node> parent_;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VirtualNode, id_, position_, parent_ -> getId(), parent_ -> isActive());

struct VirtualNodeCmp {
    bool operator()(const VirtualNode& lhs, const VirtualNode& rhs) const {
        return lhs.position_ < rhs.position_;
    }
};

uint64_t md5_hash_64(const std::string& key);


// the hash ring is guaranteed to be thread safe for reads / writes
// we allow single read but concurrent rights
class HashRing {
public:
    explicit HashRing();
    ~HashRing();
    std::shared_ptr<Node> findNode(const std::string& key);
    std::vector<std::shared_ptr<Node>> getNextNodes(const std::string& key, size_t n);
    void addNode(std::shared_ptr<Node> node);
    void removeNode(const std::string& node_id);

    std::shared_ptr<Node> getNode(const std::string& id);

    std::vector<std::shared_ptr<Node>> getNodes() {
        return nodes_;
    }
    std::vector<VirtualNode> getVirtualNodes() {
        return std::vector<VirtualNode>(node_ring_.begin(), node_ring_.end());
    }

private:
    uint64_t ring_size_;
    std::multiset<VirtualNode, VirtualNodeCmp> node_ring_;
    std::vector<std::shared_ptr<Node>> nodes_;
    std::shared_mutex rwlock_;
};
