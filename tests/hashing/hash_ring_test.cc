#include <cstdlib>
#include <memory>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "hash_ring/hash_ring.h"

TEST(HashRingTest, SingleNodeWorks) {
    HashRing ring{};
    auto node = std::make_shared<Node>("node-1", 1);

    ring.addNode(node);
    EXPECT_EQ(ring.findNode("hello") -> getId(), node->getId());
}

TEST(HashRingTest, TwoNodeSplit) {
    HashRing ring{};
    auto node1 = std::make_shared<Node>("node-1");
    auto node2 = std::make_shared<Node>("node-2");

    ring.addNode(node1);
    ring.addNode(node2);

    std::vector<int> counts{0, 0};

    for(int i = 0; i < 1000; i++) {
        auto n = ring.findNode(std::to_string(i));

        if(n->getId() == "node-1") {
            counts[0]++;
        } else {
            counts[1]++;
        }
    }

    float total = counts[0] + counts[1];
    float freq1 = static_cast<float>(counts[0]) / total;
    float freq2 = static_cast<float>(counts[1]) / total;
    EXPECT_LT(freq1, 0.7) << "keys not evenly distributed " << freq1;
    EXPECT_LT(freq2, 0.7) << "keys not evenly distributed " << freq2;
}

TEST(HashRingTest, NodeRemovalWorks) {
    HashRing ring{};
    auto node1 = std::make_shared<Node>("node-1");
    auto node2 = std::make_shared<Node>("node-2");

    ring.addNode(node1);
    ring.addNode(node2);

    std::vector<int> counts{0, 0};

    for(int i = 0; i < 1000; i++) {
        auto n = ring.findNode(std::to_string(i));

        if(n->getId() == "node-1") {
            counts[0]++;
        } else {
            counts[1]++;
        }
    }

    EXPECT_GT(counts[0], 100) << "should have some keys, count: " << counts[0];
    EXPECT_GT(counts[1], 100) << "should have some keys, count: " << counts[1];

    ring.removeNode("node-1");

    counts = {0, 0};
    for(int i = 0; i < 1000; i++) {
        auto n = ring.findNode(std::to_string(i));

        if(n->getId() == "node-1") {
            counts[0]++;
        } else {
            counts[1]++;
        }
    }

    EXPECT_EQ(counts[0], 0) << "should have no keys mapping to node 1: " << counts[0];
}

TEST(HashRingTest, FiveNodeSplit) {
    HashRing ring{};

    for(int i = 0; i < 5; i++) {
        ring.addNode(std::make_shared<Node>(std::to_string(i)));
    }

    std::vector<int> counts{0, 0, 0, 0, 0};

    const int keys = 1000;
    for(int i = 0; i < keys; i++) {
        auto n = ring.findNode(std::to_string(i));

        counts[std::stoi(n->getId())]++;
    }

    const int tolerance = 20;
    for(auto count : counts) {
        EXPECT_LT(std::abs(count - keys / 5), tolerance) << "count: " << count << "tolerance: " << tolerance;
    }
}

TEST(HashRingTest, getNextNodes) {
    HashRing ring{};

    for(int i = 0; i < 2; i++) {
        ring.addNode(std::make_shared<Node>(std::to_string(i), 1));
    }

    auto node_id = ring.findNode("hello") ->getId();
    std::vector<std::string> order{node_id, node_id == "0" ? "1" : "0"};
    auto output = ring.getNextNodes("hello", 2);

    for(int i = 0; i < 1; i++ ){
        EXPECT_EQ(output[i] -> getId() , order[i]) << "Order does not match: " << output[i] -> getId() << ", " << order[i];
    }
}

TEST(HashRingTest, getNextNodesFiveUnique) {
    HashRing ring{};

    for(int i = 0; i < 5; i++) {
        ring.addNode(std::make_shared<Node>(std::to_string(i), 1));
    }

    auto output = ring.getNextNodes("hello", 5);

    EXPECT_EQ(output.size(), 5);

    std::unordered_set<std::string> ids;
    for (auto &n : output) {
        ids.insert(n->getId());
    }

    EXPECT_EQ(ids.size(), 5);
}

TEST(HashRingTest, getNextNodesVnodes) {
    HashRing ring{};

    for(int i = 0; i < 50; i++) {
        ring.addNode(std::make_shared<Node>(std::to_string(i)));
    }

    auto output = ring.getNextNodes("hello", 10);

    for(auto &node : output){
        std::cout << node -> getId() << " ";
    }

    EXPECT_EQ(output.size(), 10);

    std::unordered_set<std::string> ids;
    for (auto &n : output) {
        ids.insert(n->getId());
    }

    EXPECT_EQ(ids.size(), 10);
}

TEST(HashRingTest, getNextNodesVnodesBig) {
    HashRing ring{};

    for(int i = 0; i < 3; i++) {
        ring.addNode(std::make_shared<Node>(std::to_string(i)));
    }

    auto output = ring.getNextNodes("hello", 10);

    for(auto &node : output){
        std::cout << node -> getId() << " ";
    }

    EXPECT_EQ(output.size(), 3);

    std::unordered_set<std::string> ids;
    for (auto &n : output) {
        ids.insert(n->getId());
    }

    EXPECT_EQ(ids.size(), 3);
}