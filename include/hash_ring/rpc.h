#pragma once

#include "storage/value.h"
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// binary serialized
struct PutRpc {

    PutRpc() = default;

    PutRpc(const std::string key, const Value data) : 
        key_(key), 
        data_(data) 
        {};

    template <class Archive>
    void serialize(Archive & archive) {
        archive( 
            key_,
            data_
        );
    }

    std::string key_;
    Value data_;
};

struct HandoffRpc : PutRpc {

    HandoffRpc() = default;

    HandoffRpc(const std::string& key,
           const Value& data,
           const std::string& target_node_id)
        : PutRpc(key, data),
          target_node_id_(target_node_id) {}

    template <class Archive>
    void serialize(Archive & archive) {
        archive( 
            cereal::base_class<PutRpc>(this),
            target_node_id_
        );
    }

    std::string target_node_id_;
};

// json serialized
struct PutBody {
    std::string key;
    std::string data;
    std::string context;
};

struct GetResponse {
    explicit GetResponse(size_t size) : values(size) {}
    struct ResponseValue {
        std::string data;
        std::string context;
    };
    std::vector<ResponseValue> values;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GetResponse::ResponseValue, data, context)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GetResponse, values)