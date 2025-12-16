#include "hash_ring/node.h"
#include "hash_ring/rpc.h"
#include "storage/serializer.h"
#include "logging/logger.h"
#include "storage/value.h"
 
// this constructor is only ever used for testing
Node::Node(std::string id, size_t tokens) : 
    id_{id}, 
    active_(true), 
    tokens_(tokens) {
    }

Node::Node(std::string addr, int port, size_t tokens) : 
    id_(addr+":"+std::to_string(port)), 
    addr_(addr), 
    port_(port), 
    client_(std::make_unique<httplib::Client>(addr, port)),
    tokens_(tokens),
    active_(true) {
        client_ -> set_connection_timeout(5);  
        client_ -> set_read_timeout(5);       
        client_ -> set_write_timeout(5);  
    }

bool Node::checkHealth() {
    auto res = client_ -> Get("/admin/health");

    if(res) {
        return res->status == httplib::StatusCode::OK_200;
    } else {
        return false;
    }
}

bool Node::send(const std::string& endpoint, const ByteString& data) {
    auto res = client_ -> Post(endpoint, data, "application/octet-stream");

    if(res) {
        return res->status == httplib::StatusCode::OK_200;
    } else {
        return false;
    }
}

bool Node::replicate_put(const std::string& key, const Value& value) {
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

ValueList Node::replicate_get(const std::string& key) {
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

std::string Node::getFullAddress() {
    return addr_ + ":" + std::to_string(port_);
}

std::string Node::getId() {
    return id_;
}
