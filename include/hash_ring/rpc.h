#pragma once

#include "storage/value.h"
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// binary serialized

struct PutRpc {

    PutRpc() {};

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

// json serialized

struct GetBody {
    std::string key;
};

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