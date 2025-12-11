#pragma once

#include "storage/value.h"
#include <sstream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

class Serializer {
    public:
        static ByteString toBinary(const ValueList& valueList) {
            std::stringstream ss;

            {
                cereal::BinaryOutputArchive oarchive(ss);
                oarchive(valueList);
            }

            return ss.str();
        }

        static ByteString toJson(const ValueList& valueList) {
            std::stringstream ss;

            {
                cereal::JSONOutputArchive oarchive(ss);
                oarchive(valueList);
            }

            return ss.str();
        }

        static ValueList fromBinary(const std::string& binary) {
            std::istringstream ss(binary);
            cereal::BinaryInputArchive iarchive({ss});

            ValueList output{};
            iarchive(output);
            return output;
        }
};