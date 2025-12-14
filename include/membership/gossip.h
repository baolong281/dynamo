#pragma once

#include "hash_ring/hash_ring.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <string>

struct NodeState {
    enum Status {
        ACTIVE,
        KILLED
    };

    std::string id_;
    std::string address_;
    int port_;
    Status status_;
    uint64_t incarnation_;

    template <class Archive>
    void serialize(Archive & archive) {
        archive(id_, address_, port_, status_, incarnation_);
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "NodeState{"
            << "id=" << id_
            << ", address=" << address_
            << ", port=" << port_
            << ", status=" << (status_ == ACTIVE ? "ACTIVE" : "KILLED")
            << ", incarnation=" << incarnation_
            << "}";
        return oss.str();
    }
};

using ClusterState = std::unordered_map<std::string, NodeState>;

class Gossip {
    public:
        void start();
        Gossip(std::shared_ptr<HashRing> ring, int fanout, std::shared_ptr<Node> curr, std::vector<std::pair<std::string, int>> bootstrap_servers) : 
            ring_(ring),
            fanout_(fanout),
            curr_node_(curr),
            bootstrap_servers_(bootstrap_servers) {
                NodeState initial{
                        curr->getId(),
                        curr->getAddr(),
                        curr->getPort(),
                        NodeState::Status::ACTIVE,
                        1
                    };
                addState(initial);
            }

        ~Gossip() {
            running = false;
            if (t_.joinable()) t_.join();
        }

        void addState(NodeState state); 
        void onRecieve(ClusterState &other_state);

    private:
        std::shared_ptr<HashRing> ring_;
        std::shared_ptr<Node> curr_node_;
        ClusterState state_;
        std::thread t_;
        std::atomic<bool> running{false};
        std::vector<std::pair<std::string, int>> bootstrap_servers_;
        int fanout_;
};