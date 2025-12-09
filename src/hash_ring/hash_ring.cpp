#include "hash_ring/hash_ring.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <openssl/md5.h>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#define RING_SIZE ULLONG_MAX

uint64_t md5_hash_64(const std::string& key) {
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5(reinterpret_cast<const unsigned char*>(key.data()),
        key.size(),
        digest);

    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value = (value << 8) | digest[i];
    }

    return value;
}

HashRing::HashRing(int n_vnodes)
    : ring_size_(RING_SIZE),
      n_vnodes_(n_vnodes) {}

HashRing::~HashRing() = default;

std::shared_ptr<Node> HashRing::findNode(const std::string& key) {
    auto position_idx = md5_hash_64(key);

    auto it = std::upper_bound(
        node_ring_.begin(),
        node_ring_.end(),
        position_idx,
        [](uint64_t pos, const VirtualNode& vn) {
            return pos < vn.position_;
        }
    );

    if (it == node_ring_.end()) {
        // wrap-around: return the first vnode
        return node_ring_.begin()->parent_;
    }

    return it->parent_;
}

void HashRing::addNode(Node node) {
    auto node_ptr = std::make_shared<Node>(node);

    for (int i = 0; i < n_vnodes_; i++) {
        std::string vnode_id = node.getId() + "-" + std::to_string(i);
        uint64_t pos = md5_hash_64(vnode_id);

        node_ring_.insert(VirtualNode{vnode_id, pos, node_ptr});
    }

    nodes_.insert(node_ptr);
}


void HashRing::removeNode(const std::string& node_id) {
    for (auto it = node_ring_.begin(); it != node_ring_.end();) {
        if (it->parent_->getId() == node_id) {
            it = node_ring_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = nodes_.begin(); it != nodes_.end();) {
        if ((*it)->getId() == node_id) {
            it = nodes_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::shared_ptr<Node>> HashRing::getNextNodes(const std::string& key, size_t n) {
    // make n minimum of available nodes
    // does this make sense to do
    n = std::min(n, nodes_.size());

    auto position_idx = md5_hash_64(key);

    auto it = std::upper_bound(
        node_ring_.begin(),
        node_ring_.end(),
        position_idx,
        [](uint64_t pos, const VirtualNode& vn) {
            return pos < vn.position_;
        }
    );

    std::vector<std::shared_ptr<Node>> top_nodes{};
    std::unordered_set<std::string> seen{};

    int counter{0};

    while(n > 0) {
        // wrap around
        if(counter > node_ring_.size()) {
            throw std::runtime_error("Infinite looping, something wrong with getting next nodes. Possibly all nodes are the same? ");
        }
        if(it == node_ring_.end()) {
            it = node_ring_.begin();
        }
        if(seen.find(it ->parent_ -> getId()) == seen.end()) {
            seen.insert(it->parent_->getId());
            top_nodes.push_back(it->parent_);
            n--;
        }
        it++;
        counter++;
    }

    return top_nodes;
}