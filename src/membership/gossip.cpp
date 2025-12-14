#include "membership/gossip.h"
#include <thread>
#include <chrono>
#include "storage/serializer.h"
#include "logging/logger.h"

void Gossip::start() {
    if (t_.joinable()) return;
    running = true;

    // bootstrap first before starting thread
    ByteString serialized = Serializer::toBinary<ClusterState>(state_);

    for(auto &[ip, port] : bootstrap_servers_) {
        Node node{ip, port};
        node.send("/admin/gossip", serialized);
    }

    t_ = std::thread([this]() {
        std::random_device rd;
        std::mt19937 gen(rd());  

        while (running) {
            auto nodes = this->ring_->getNodes();

            std::ostringstream oss;
            oss << "ring state: ";
            bool first = true;
            for (auto& n : nodes) {
                if (!first) oss << ", ";
                first = false;
                oss << n->getFullAddress();
            }
            Logger::instance().debug(oss.str());


            std::vector<int> pool(nodes.size());
            std::iota(pool.begin(), pool.end(), 0);

            std::shuffle(pool.begin(), pool.end(), gen);
            std::vector<int> selected(pool.begin(), pool.begin() + fanout_);

            // does not guarantee we send to fanout_ nodes (can include ourselves)
            // fix later

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
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    });
    Logger::instance().info("starting gossip background process...");
}


void Gossip::onRecieve(std::unordered_map<std::string, NodeState> &other_state) {
    for(auto &[k, v] : other_state) {
        auto it = state_.find(k);
        if(it == state_.end()) {
            addState(v);
        }  else if(v.incarnation_ > state_[k].incarnation_) {
            state_[k] = v;
        }
    }
}

void Gossip::addState(NodeState state) {
    Logger::instance().debug("Discovered new node. adding state: " + state.id_);

    state_[state.id_] = state;
    
    std::shared_ptr<Node> new_node = std::make_shared<Node>(
        state.address_, state.port_
    );

    ring_->addNode(new_node);
}

