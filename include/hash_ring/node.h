#pragma once

#include "httplib.h"
#include "storage/value.h"
#include <atomic>
#include <memory>
#include <string>

class Node {
    public:
        Node(std::string id, size_t tokens = 1000);
        Node(std::string addr, int port, size_t tokens = 1000);
        bool send(const std::string& endpoint, const ByteString& data);
        bool replicatePut(const std::string& key, const Value& value);
        bool replicateHandoff(const std::string& key, const Value& value, const std::string& node_id);
        std::optional<ValueList> replicateGet(const std::string& key);
        bool checkHealth();
        std::string getFullAddress();
        std::string getId();
        std::string getAddr() { 
            return addr_; 
        }
        int getPort() { 
            return port_;
        }
        bool isActive() {
            return active_.load(std::memory_order_relaxed);
        }
        void setInactive() {
            active_.store(false, std::memory_order_relaxed);
        }
        void setActive() {
            active_.store(true, std::memory_order_relaxed);
        }

        int getTokens() {
            return tokens_;
        }

    private:
        std::string id_;
        std::string addr_;
        std::shared_ptr<httplib::Client> client_;
        size_t tokens_;
        std::atomic<bool> active_;
        int port_;
};