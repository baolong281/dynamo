#pragma once

#include "httplib.h"
#include "storage/value.h"
#include <memory>
#include <string>

class Node {
    public:
        Node(std::string id);
        Node(std::string addr, int port);
        bool send(const std::string& endpoint, const  ByteString& data);
        bool replicate_put(const std::string& key, const Value& value);
        ValueList replicate_get(const std::string& key);
        std::string getFullAddress();
        std::string getId();

        std::string getAddr() { return addr_; }
        int getPort() { return port_; }

    private:
        std::string id_;
        std::string addr_;
        std::shared_ptr<httplib::Client> client_;
        bool active_;
        int port_;
};