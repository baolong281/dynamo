#pragma once

#include "httplib.h"
#include "storage/value.h"
#include <memory>
#include <string>

class Node {
    public:
        Node(std::string id, size_t tokens = 1000);
        Node(std::string addr, int port, size_t tokens = 1000);
        bool send(const std::string& endpoint, const ByteString& data);
        bool replicate_put(const std::string& key, const Value& value);
        ValueList replicate_get(const std::string& key);
        std::string getFullAddress();
        std::string getId();

        std::string getAddr() { return addr_; }
        int getPort() { return port_; }
        void setInactive() {
            active_ = false;
        }
        void setActive() {
            active_ = true;
        }

        int getTokens() {
            return tokens_;
        }

    private:
        std::string id_;
        std::string addr_;
        std::shared_ptr<httplib::Client> client_;
        size_t tokens_;
        bool active_;
        int port_;
};