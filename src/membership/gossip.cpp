#include "membership/gossip.h"
#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include "hash_ring/node.h"
#include "storage/serializer.h"
#include "logging/logger.h"

void Gossip::transmitRandom(std::mt19937 &gen) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f); 
    auto nodes = this->ring_->getNodes();

    nodes.erase(
        std::remove_if(
            nodes.begin(), nodes.end(), [&](auto &n) {
                return state_[n -> getId()].status_ == NodeState::Status::KILLED && n -> getId() != curr_node_ -> getId();
            }
        ),
        nodes.end()
    );

    std::vector<int> pool(nodes.size());
    std::iota(pool.begin(), pool.end(), 0);

    std::shuffle(pool.begin(), pool.end(), gen);
    std::vector<int> selected(pool.begin(), pool.begin() + fanout_);

    float r = dist(gen);


    ByteString serialized = Serializer::toBinary<ClusterState>(state_);
    for(auto idx : selected) {
        std::shared_ptr<Node> other = nodes.at(idx);
        if(other->getId() == curr_node_->getId()) {
            continue;
        }
        bool success = nodes.at(idx)->send("/admin/gossip", serialized);
        if(!success) {
            Logger::instance().error("gossip request failing to node: " + nodes.at(idx)->getId());
        }
    }

    // randomly send with low probability to seed server
    // this may be bad but fixes a scenario in which one one is killed then restarted
    if(r < 0.05) {
        Logger::instance().debug("random firing!");
        for(auto &[ip, port] : bootstrap_servers_) {
            // we are setting tokens to one, but does not matter since we only use this node as a handle
            Node node{ip, port, 1};
            node.send("/admin/gossip", serialized);
        }
    }
}

void Gossip::start() {
    if (t_.joinable()) return;
    running = true;

    // bootstrap first before starting thread
    ByteString serialized = Serializer::toBinary<ClusterState>(state_);

    // fire requests to bootstrap nodes until one is good
    // can maybe be more robust
    bool success = false;
    while(!success && bootstrap_servers_.size() > 0) {
        for(auto &[ip, port] : bootstrap_servers_) {
            Node node{ip, port, 1};
            success = success | node.send("/admin/gossip", serialized);
        }
    }

    t_ = std::thread([this]() {
        std::random_device rd;
        std::mt19937 gen(rd());  

        while (running) {
            transmitRandom(gen);
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    });
    Logger::instance().info("starting gossip background process...");
}


// theres an edge case where number of tokens is changed and is not synchronized
// should not happen though unless there is a shutdown, which will mean our changes are reflected
void Gossip::onRecieve(std::unordered_map<std::string, NodeState> &other_state) {
    for(auto &[k, v] : other_state) {
        auto it = state_.find(k);
        if(it == state_.end()) {
            addState(v);
        }  else if(v.incarnation_ > state_[k].incarnation_) {
            // we do a kill here
            if(v.status_ == NodeState::Status::KILLED && state_[k].status_ == NodeState::Status::ACTIVE) {
                Logger::instance().debug("Recieved node shutdown for: " + v.id_);
                ring_ -> removeNode(v.id_);
            } else if (v.status_ == NodeState::Status::ACTIVE && state_[k].status_ == NodeState::Status::KILLED) {
                // we have to make the new node and re-insert it again
                std::shared_ptr<Node> new_node = std::make_shared<Node>(
                    v.address_, v.port_, v.tokens_
                );

                ring_->addNode(new_node);
            }
            state_[k] = v;
        }
    }
}

void Gossip::addState(NodeState state) {
    Logger::instance().debug("Discovered new node: " + state.id_);

    state_[state.id_] = state;
    
    std::shared_ptr<Node> new_node = std::make_shared<Node>(
        state.address_, state.port_, state.tokens_
    );

    ring_->addNode(new_node);
}

// there is a race condition here but i don think it matters right now
// TODO: FIX
void Gossip::stop() {
    Logger::instance().info("Changing node status and killing gossip...");
    std::string node_id = curr_node_->getId();
    state_[node_id].incarnation_ += 1;
    state_[node_id].status_ = NodeState::KILLED;

    std::random_device rd;
    std::mt19937 gen(rd());  
    Logger::instance().info("Sending out kill requests over gossip...");
    transmitRandom(gen);

    Logger::instance().info("Writing gossip number to disk...");
    std::string path{
        "/tmp/" + curr_node_->getAddr() + ":" + std::to_string(curr_node_->getPort()) + "-gossip"
    };
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("Failed to open file for gossip number!");
    out << state_[node_id].incarnation_;
}