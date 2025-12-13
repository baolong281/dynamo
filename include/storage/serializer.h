#pragma once

#include "storage/value.h"
#include <sstream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

class Serializer {
    public:
        template <typename Serializable>
        static ByteString toBinary(const Serializable& obj) {
            std::stringstream ss;

            {
                cereal::BinaryOutputArchive oarchive(ss);
                oarchive(obj);
            }

            return ss.str();
        }

        template <typename Serializable>
        static Serializable fromJson(const std::string& json)  {
            Serializable input{};
            std::stringstream ss(json);
            {
                cereal::JSONInputArchive archive(ss);
                archive(input);
            }

            return input;
        }

        // if string is empty we just return the default constructed object
        template <typename Serializable>
        static Serializable fromBinary(const std::string& binary) {
            Serializable output{};

            if(binary.size() == 0) {
                return output;
            }

            {
                std::istringstream ss(binary);
                cereal::BinaryInputArchive iarchive(ss);

                iarchive(output); 
            }
            return output;
        }

        template <typename Serializable>
        static ByteString toJson(const Serializable& obj) {
            std::stringstream ss;

            {
                cereal::JSONOutputArchive oarchive(ss);
                oarchive(obj);
            }

            return ss.str();
        }


};