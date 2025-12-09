#include <cstdlib>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "hash_ring/hash_ring.h"

TEST(HashRingTest, SingleNodeWorks) {
    HashRing ring{1};
    Node node{"node-1"};

    ring.addNode(node);
    EXPECT_EQ(ring.findNode("hello") -> getId(), node.getId());
}

TEST(HashRingTest, TwoNodeSplit) {
    HashRing ring{1000};
    Node node1{"node-1"};
    Node node2{"node-2"};

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
    HashRing ring{1000};
    Node node1{"node-1"};
    Node node2{"node-2"};

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
    HashRing ring{1000};

    for(int i = 0; i < 5; i++) {
        ring.addNode({
            std::to_string(i)
        });
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