#pragma once

#include "httplib.h"
#include <memory>
#include <string>

class Node {
    public:
        Node(std::string id) : id_{id} {};

        Node(std::string addr, int port) : 
            id_(addr+":"+std::to_string(port)), 
            addr_(addr), 
            port_(port), 
            client_(std::make_unique<httplib::Client>(addr, port)) {};

        void send_put(const std::string& key, const std::string& data) {
            httplib::Headers headers{
                {"Key", key},
                {"Content-Type", "application/octet-stream"}
            };

            client_ -> Post("/replication/put", headers, data, "application/octet-stream");
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