#include "hash_ring/node.h"
#include "hash_ring/rpc.h"
#include "storage/serializer.h"
#include "logging/logger.h"
#include "storage/value.h"
#include <httplib.h>
#include <optional>
 
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
        client_->set_connection_timeout(std::chrono::milliseconds(50));
        client_->set_read_timeout(std::chrono::milliseconds(50));
        client_->set_write_timeout(std::chrono::milliseconds(50));
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

// should we early return like this here?
bool Node::replicatePut(const std::string& key, const Value& value) {
    if(!this->isActive()) {
        return false;
    }

    PutRpc data{key, value};
    auto serialized = Serializer::toBinary(data);
    auto res = client_ -> Post("/replication/put", serialized, "application/octet-stream");

    if(res) {
        return res->status == httplib::StatusCode::OK_200 || res->status == httplib::StatusCode::BadRequest_400;
    } else {
        return false;
    }
}

bool Node::replicateHandoff(const std::string& key, const Value& value, const std::string& node_id) {
    Logger::instance().debug("Hinted handoff for key: " + key + " to node " + getId());

    if(!this->isActive()) {
        return false;
    }

    HandoffRpc data{key, value, node_id};
    auto serialized = Serializer::toBinary(data);
    auto res = client_ -> Post("/replication/handoff", serialized, "application/octet-stream");

    if(res) {
        return res->status == httplib::StatusCode::OK_200 || res->status == httplib::StatusCode::BadRequest_400;
    } else {
        return false;
    }
}

std::optional<ValueList> Node::replicateGet(const std::string& key) {
    httplib::Headers headers{
        {"Content-Type", "application/octet-stream"}
    };
    auto res = client_ -> Post("/replication/get", key, "application/octet-stream");
    if(res) {
        return Serializer::fromBinary<ValueList>(res->body);
    } else {
        return std::nullopt;
    }
}

std::string Node::getFullAddress() {
    return addr_ + ":" + std::to_string(port_);
}

std::string Node::getId() {
    return id_;
}
