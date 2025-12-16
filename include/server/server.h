#pragma once

#include "error/storage_error.h"
#include "hash_ring/hash_ring.h"
#include "hash_ring/quorom.h"
#include "hash_ring/rpc.h"
#include "logging/logger.h"
#include "membership/gossip.h"
#include "storage/serializer.h"
#include "httplib.h"
#include "storage/value.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <mutex>
#include "storage/base64.hpp"
#include <boost/functional/hash.hpp>


using json = nlohmann::json;

template<typename Engine>
class Server {
    public:
        explicit Server(std::shared_ptr<Engine> engine, std::shared_ptr<HashRing> ring, std::shared_ptr<Quorom> quorom, std::shared_ptr<Gossip> gossip) : 
        engine_(engine), 
        ring_(ring), 
        quorom_(quorom) ,
        gossip_(gossip)        
        {

            svr_.Options("/(.*)",
			[&](const httplib::Request & /*req*/, httplib::Response &res) {
                res.set_header("Access-Control-Allow-Methods", " POST, GET, OPTIONS");
                res.set_header("Content-Type", "application/json");
                res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Key");
                res.set_header("Access-Control-Allow-Origin", "*");
            });

            svr_.set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
                    try {
                        std::rethrow_exception(ep);
                    } 
                    catch (const StorageError& e) {
                        Logger::instance().error(std::string("STORAGE ERROR: ") + e.what());
                        res.set_content(e.what(), "text/plain");
                        res.status = httplib::StatusCode::InternalServerError_500;
                    }
                    catch (const cereal::Exception& e) {
                        Logger::instance().error(std::string("CEREAL SERDE ERROR: ") + e.what());
                        res.set_content(e.what(), "text/plain");
                        res.status = httplib::StatusCode::InternalServerError_500;
                    }
                    catch (std::exception &e) {
                        Logger::instance().error(e.what());
                        res.set_content(e.what(), "text/html");
                    } catch (...) { 
                        Logger::instance().error("Unknown exception occured in router handler!");
                    }
                    res.status = httplib::StatusCode::InternalServerError_500;
            });

            svr_.Post("/get", [this](const httplib::Request & req, httplib::Response &res) {
                this -> handleGet(req, res);
            });

            svr_.Post("/put", [this](const httplib::Request & req, httplib::Response &res) {
                this -> handlePut(req, res);
            });

            svr_.Post("/replication/put", [this](const httplib::Request & req, httplib::Response &res) {
                this -> handleReplicationPut(req, res);
            });

            svr_.Post("/replication/get", [this](const httplib::Request & req, httplib::Response &res) {
                this->handleReplicationGet(req, res);
            });

            // should be logically seperated?
            svr_.Post("/admin/gossip", [this](const httplib::Request & req, httplib::Response &res) {
                auto serialized = Serializer::fromBinary<ClusterState>(req.body);
                this -> gossip_ -> onRecieve(serialized);
                res.status = 200;
            });

            svr_.Post("/admin/membership", [this](const httplib::Request & req, httplib::Response &res) {
                this -> setCORS(req, res);
                json j = gossip_->getState();
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            });

            svr_.Post("/admin/ring", [this](const httplib::Request & req, httplib::Response &res) {
                this -> setCORS(req, res);
                json j = ring_->getVirtualNodes();
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            });

            svr_.Get("/admin/health", [this](const httplib::Request & req, httplib::Response &res) {
                res.status = 200;
            });

            Logger::instance().info(
                "endpoints successfully registered!"
            );
        }
    
    // start in new thread (?)
    void start(const std::string& ip, int port) {
        Logger::instance().info(
            "starting http server on port: "  + std::to_string(port) + " with ip: " + ip
        );
        svr_.listen(ip, port);
    }

    void stop() {
        Logger::instance().debug("Stopping server...");
        svr_.stop();
    }

    private:
        std::shared_ptr<Engine> engine_;
        std::shared_ptr<HashRing> ring_;
        std::shared_ptr<Quorom> quorom_;
        std::shared_ptr<Gossip> gossip_;
        httplib::Server svr_;
        std::mutex mu_;

        void setCORS(const httplib::Request &req, httplib::Response &res) { 
            res.set_header("Access-Control-Allow-Origin", "*"); 
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Key");
        }

        void handleReplicationGet(const httplib::Request &req, httplib::Response &res) { 
            Logger::instance().debug("Running replication get request for key: " + req.body);
            {
                std::lock_guard<std::mutex> guard(mu_);
                res.body = engine_ -> get(req.body);
            }

            res.status = 200;
        }

        void handleReplicationPut(const httplib::Request &req, httplib::Response &res) { 
            PutRpc rpc = Serializer::fromBinary<PutRpc>(req.body);
            Logger::instance().debug("Running replication put request for key: " + rpc.key_);

            {
                std::lock_guard<std::mutex> guard(mu_);
                ValueList values = Serializer::fromBinary<ValueList>(engine_ -> get(rpc.key_));

                bool current_clock_lt = std::any_of(values.begin(), values.end(), [&rpc](Value &v) {
                    return rpc.data_.clock_ < v.clock_;
                });

                if(current_clock_lt) {
                    throw std::runtime_error("RPC PUT CLOCK OUTDATED FOR KEY: " + rpc.key_);
                    return;
                }

                values.erase(
                    std::remove_if(values.begin(), values.end(), [&rpc](auto &v) {
                        return v.clock_ < rpc.data_.clock_;
                    }),
                    values.end()
                );

                values.push_back(rpc.data_);
                engine_ -> put(rpc.key_, Serializer::toBinary(values));
            }

            res.status = 200;
        }

        void handlePut(const httplib::Request &req, httplib::Response &res) { 
            setCORS(req, res);
            if(handleRedirect(req, res, "/put")) {
                return;
            }

            auto body = json::parse(req.body);

            std::string key{body["key"]};
            std::string data_b64{body["data"]};
            std::string context_b64{body["context"]};

            Logger::instance().debug("Running PUT for key: " + key);

            auto data = base64::from_base64(data_b64);

            VectorClock clock{};
            if(context_b64.size() != 0) {
                clock = Serializer::fromBinary<VectorClock>(base64::from_base64(context_b64));
            }

            clock.increment(quorom_->getCurrNode()->getId());

            Value val{data, clock};

            // TODO put this into a function?
            {
                std::lock_guard<std::mutex> guard(mu_);
                ValueList values = Serializer::fromBinary<ValueList>(engine_ -> get(key));

                // if the clock is less than any, then we cannot put
                bool current_clock_lt = std::any_of(values.begin(), values.end(), [&clock](Value &v) {
                    return clock < v.clock_;
                });

                if(current_clock_lt) {
                    res.set_content("Outdated clock specified. Re-run a get operation to get the updated clock.", "text/plain");
                    res.status = 400;
                    return;
                }

                // remove clock values less than ours
                // anything remaining must be a sibling
                values.erase(
                    std::remove_if(values.begin(), values.end(), [&clock](auto &v) {
                        return v.clock_ < clock;
                    }),
                    values.end()
                );

                values.push_back(val);
                engine_ -> put(key, Serializer::toBinary(values));
            }

            bool success = quorom_->put(key, val);

            if(success) {
                res.status = 200;
            } else {
                res.status = 500;
                res.set_content("Failed to replicate to enough nodes", "text/plain");
            }
        }

        void handleGet(const httplib::Request &req, httplib::Response &res) { 
            setCORS(req, res);
            if(handleRedirect(req, res, "/get")) {
                return;
            }

            auto body = json::parse(req.body);
            ValueList values{};
            {
                std::lock_guard<std::mutex> guard(mu_);
                values = Serializer::fromBinary<ValueList>(engine_ -> get(body["key"]));
            }

            std::string key = body["key"];
            Logger::instance().debug("Running GET for key: " + key);

            try {
                ValueList replica_values = quorom_ -> get(key);

                for(auto &v : replica_values) {
                    values.push_back(v);
                }

                GetResponse resp{values.size()};

                for(int i = 0; i < values.size(); i++) {
                    resp.values[i].context = base64::to_base64(Serializer::toBinary(values[i].clock_));
                    resp.values[i].data = base64::to_base64(values[i].data_);
                }

                GetResponse uniqueResp = filterDuplicates(resp);

                json j = uniqueResp;
                res.set_header("Content-Type", "application/json");
                res.set_content(j.dump(), "application/json"); 
                res.status = 200;
            } catch(std::runtime_error e) {
                Logger::instance().error("Error fetching key: " + key);
                Logger::instance().error(e.what());
                res.status = 500;
                res.set_content(e.what(), "text/plain");
                return;
            }
        }

        // returns true if we are in the wrong partition and need to redirect
        bool handleRedirect(const httplib::Request &req, httplib::Response &res, std::string endpoint) { 
            auto body = json::parse(req.body);
            std::string key{body["key"]};

            auto current_node = quorom_->getCurrNode();
            auto coordination_node = ring_->findNode(key);

            if(current_node->getId() != coordination_node->getId()) {
                std::string node_url{"http://" + coordination_node -> getFullAddress() + endpoint};
                Logger::instance().debug("Redirecting request for key: " + key + " to node: " + node_url);
                res.status = 307; 
                res.set_header("Location", node_url);
                return true;
            }

            return false;
        }

        GetResponse filterDuplicates(const GetResponse& resp) {
            GetResponse out{0};
            std::unordered_set<std::pair<std::string, std::string>, 
                            boost::hash<std::pair<std::string, std::string>>> seen;

            for (const auto& v : resp.values) {
                auto key = std::make_pair(v.data, v.context);
                if (seen.insert(key).second) { // true if not seen before
                    out.values.push_back(v);
                }
            }

            return out;
        }

};