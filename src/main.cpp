#include "hash_ring/hash_ring.h"
#include "hash_ring/quorom.h"
#include "storage/disk_engine.h"
#include "server/server.h"
#include <memory>
#include <string>

int main(int argc, char* argv[]) {

    int port = 8080;  

    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[1] << "\n";
            return 1;
        }
    }

    auto db = std::make_shared<DiskEngine>(std::to_string(port));
    auto ring = std::make_shared<HashRing>(1000);

    Node parent{"localhost", port};
    ring -> addNode(parent);

    std::vector<int> ports{8080, 8081, 8082};
    ports.erase(std::remove(ports.begin(), ports.end(), port));

    for(auto p : ports) {
        ring -> addNode({"localhost", p});
    }

    auto quorom = std::make_shared<Quorom>(3, 2, 2, std::make_shared<Node>(parent), ring);

    Server service{db, ring, quorom};

    service.start("0.0.0.0", port);
}