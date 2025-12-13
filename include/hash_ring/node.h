#pragma once

#include "hash_ring/rpc.h"
#include "httplib.h"
#include "logging/logger.h"
#include "storage/serializer.h"
#include "storage/value.h"
#include <memory>
#include <stdexcept>
#include <string>

class Node {
    public:
        Node(std::string id) : id_{id} {};

        Node(std::string addr, int port) : 
            id_(addr+":"+std::to_string(port)), 
            addr_(addr), 
            port_(port), 
            client_(std::make_unique<httplib::Client>(addr, port)) {};

        bool replicate_put(const std::string& key, const Value& value) {
            Logger::instance().debug("Replicating key: " + key + " to node " + getId());

            PutRpc data{key, value};
            auto serialized = Serializer::toBinary(data);
            auto res = client_ -> Post("/replication/put", serialized, "application/octet-stream");

            if(res) {
                return res->status == httplib::StatusCode::OK_200;
            } else {
                return false;
            }
        }

        ValueList replicate_get(const std::string& key) {
            httplib::Headers headers{
                {"Content-Type", "application/octet-stream"}
            };

            Logger::instance().debug("Retrieving key: " + key + " from replcia node " + getId());
            auto res = client_ -> Post("/replication/get", key, "application/octet-stream");
            if(res) {
                return Serializer::fromBinary<ValueList>(res->body);
            } else {
                throw std::runtime_error("Request to replicate node: " + getId() + " failed!");
            }
        }

        std::string getFullAddress() {
            return addr_ + ":" + std::to_string(port_);
        }

        std::string getId() {
            return id_;
        }

    private:
        std::string id_;
        std::string addr_;
        std::shared_ptr<httplib::Client> client_;
        int port_;
};