#include "hash_ring/hash_ring.h"
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

    Node one{"localhost", 8080};
    Node two{"localhost", 8081};
    Node three{"localhost", 8082};

    ring -> addNode(one);
    ring -> addNode(two);
    ring -> addNode(three);

    Server service{db, ring};

    service.start("0.0.0.0", port);
}