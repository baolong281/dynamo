#pragma once

#include "hash_ring/hash_ring.h"
#include "hash_ring/quorom.h"
#include "logging/logger.h"
#include "storage/storage_engine.h"
#include "httplib.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <mutex>


template<typename Engine>
class Server {
    public:
        explicit Server(std::shared_ptr<Engine> engine, std::shared_ptr<HashRing> ring, std::shared_ptr<Quorom> quorom) : engine_(engine), ring_(ring), quorom_(quorom) {
            svr_.Options("/(.*)",
			[&](const httplib::Request & /*req*/, httplib::Response &res) {
			res.set_header("Access-Control-Allow-Methods", " POST, GET, OPTIONS");
			res.set_header("Content-Type", "application/octet-stream");
			res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Key");
			res.set_header("Access-Control-Allow-Origin", "*");
		});
            svr_.Get("/get", [this](const httplib::Request & req, httplib::Response &res) {
                res.set_header("Access-Control-Allow-Origin", "*"); 
                res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Key");

                if(!req.has_header("Key")) {
                    res.status = 400;
                    res.set_content("Invalid request scheme. Must be binary data and include 'Key' header", "text/plain");
                    return;
                }

                if(this -> handleRedirect(req, res, "/get")) {
                    return;
                }

                Logger::instance().debug("Running get operation on key: " + req.get_header_value("Key"));
                this->handleGet(req, res);
            });

            svr_.Post("/put", [this](const httplib::Request & req, httplib::Response &res) {
                res.set_header("Access-Control-Allow-Origin", "*"); 
                res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Key");
                if(handleRedirect(req, res, "/put")) {
                    return;
                }
                Logger::instance().debug("Running put operation on key: " + req.get_header_value("Key"));
                this->handlePut(req, res);
            });

            svr_.Post("/replication/put", [this](const httplib::Request & req, httplib::Response &res) {
                Logger::instance().debug("Replication call for key: " + req.get_header_value("Key"));
                this->handlePut(req, res);
            });

            Logger::instance().info(
                "endpoints /get /put registered!"
            );
        }
    
    // start in new thread (?)
    void start(const std::string& ip, int port) {
        Logger::instance().info(
            "starting http server on port: "  + std::to_string(port) + " with ip: " + ip
        );
        svr_.listen(ip, port);
    }

    private:
        std::shared_ptr<Engine> engine_;
        std::shared_ptr<HashRing> ring_;
        std::shared_ptr<Quorom> quorom_;
        httplib::Server svr_;
        std::mutex mu_;

        // returns true if we are in the wrong partition and need to redirect
        bool handleRedirect(const httplib::Request &req, httplib::Response &res, std::string endpoint) { 
            std::string key{req.get_header_value("Key")};
            auto preference_list = ring_->getNextNodes(key, quorom_->getN());
            auto current_node = quorom_->getCurrNode();

            for(auto n : preference_list) {
                Logger::instance().info("node: " + n ->getId());
            }

            if(std::all_of(preference_list.begin(), preference_list.end(), 
                [this, current_node](auto node) {
                    return node -> getId() != current_node->getId();
                })) {
                    // need to put http beacuse cors or something
                    std::string node_url{"http://" + preference_list.front() -> getFullAddress() + endpoint};
                    Logger::instance().debug("Redirecting request for key: " + key + " to node: " + node_url);
                    res.status = 307; 
                    res.set_header("Location", node_url);
                    return true;
                }

            return false;
        }

        void handleGet(const httplib::Request &req, httplib::Response &res){ 
            std::string key{req.get_header_value("Key")};

            if(!(engine_ -> contains(key))) {
                res.status = 404;
                res.set_content("Missing key: " + key, "text/plain");
                return;
            }

            mu_.lock();
            ByteString binary_data = engine_ -> get(key);
            mu_.unlock();

            res.set_header("Content-Type", "application/octet-stream");
            res.set_content(binary_data, "application/octet-stream"); 

            res.status = 200;
        }

        void handlePut(const httplib::Request &req, httplib::Response &res){ 
            if(req.get_header_value("Content-Type") != "application/octet-stream" || !req.has_header("Key")
            ) {
                res.status = 400;
                res.set_content("Invalid request scheme. Must be binary data and include 'Key' header", "text/plain");
                return;
            } 

            auto data = req.body;
            std::vector<std::byte> bytes;

            std::string key{req.get_header_value("Key")};

            mu_.lock();
            engine_ -> put(key, data);
            mu_.unlock();

        }
};