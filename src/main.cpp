#include "error/error_detector.h"
#include "hash_ring/hash_ring.h"
#include "hash_ring/quorom.h"
#include "membership/gossip.h"
#include "storage/disk_engine.h"
#include "server/server.h"
#include <CLI/CLI.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <csignal>

std::atomic<bool> stop{false};
void on_sigint(int) {
    stop.store(true, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {

    CLI::App app{"Dynamo"};

    int port = 8080;
    int tokens = 1000;
    std::string address = "localhost";
    std::vector<std::string> bootstrap_servers_raw{};

    app.add_option("-p,--port", port, "Port to bind to");
    app.add_option("-a,--address", address, "Address to bind to");
    app.add_option("-b,--bootstrap-servers", bootstrap_servers_raw, "List of bootstrap servers (addr:port)");
    app.add_option("-t,--tokens", tokens, "Number of tokens to allocate for node");

    CLI11_PARSE(app, argc, argv);

    std::vector<std::pair<std::string, int>> bootstrap_servers{bootstrap_servers_raw.size()};

    for(auto &s : bootstrap_servers_raw) {
        Logger::instance().info("Bootstrap node: " + s);
        auto delimiter_pos = s.find(':');
        if (delimiter_pos != std::string::npos) {
             std::string ip = s.substr(0, delimiter_pos);
             int p = std::stoi(s.substr(delimiter_pos + 1));
             bootstrap_servers.push_back({ip, p});
        } else {
             std::cerr << "Invalid bootstrap node format: " << s << "\n";
        }
    }
    std::shared_ptr<Node> parent = std::make_shared<Node>(address, port, tokens);

    // making main services
    auto ring = std::make_shared<HashRing>();
    auto err_detector = std::make_shared<ErrorDetector>(ring, 3);
    auto quorom = std::make_shared<Quorom>(2, 2, 2, parent, ring, err_detector);
    auto gossip = std::make_shared<Gossip>(ring, 2, parent, bootstrap_servers, err_detector);
    auto db = std::make_shared<DiskEngine>(std::to_string(port));

    Server service{db, ring, quorom, gossip};

    std::thread killer([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        service.stop();
        gossip->stop();
    });

    std::signal(SIGINT, on_sigint);

    gossip->start();
    err_detector->start();
    service.start("0.0.0.0", port);

    killer.join();
    return 0;
}