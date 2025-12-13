#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

// NEED TO INCLUDE THESE FOR CEREAL TO WORK!!!!!!!!!!!
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

using ByteString = std::string;

// client
// get (json) key -> json key + context formatted
// put (json) key, data, context (binary) -> OK 

// rpcs
// get_replica binary key in body -> binary valuelist
// put_replace binary value in body -> OK 


class VectorClock  {
    public:
        VectorClock() = default;

        template <class Archive>
        void serialize(Archive & archive) {
            archive(times_);
        }


        uint64_t get(const std::string& key) const {
            auto it = times_.find(key);
            return it != times_.end() ? it->second : 0;
        }

        std::unordered_map<std::string, uint64_t> getTimes() {
            return times_;
        }

        bool isSibling(const VectorClock& other) const {
            return !( *this < other ) && !( other < *this );
        }

        bool operator<(const VectorClock& other) const {
            return std::all_of(
                times_.begin(), times_.end(),
                [&other](const auto& kv) {
                        return kv.second <= other.get(kv.first);
                    }
                );
        }

        void increment(const std::string& key) {
            times_[key]++;
        }

        std::string toString() const {
            std::ostringstream oss;
            oss << "{";
            bool first = true;
            for (const auto& [k, v] : times_) {
                if (!first) oss << ", ";
                oss << k << ": " << v;
                first = false;
            }
            oss << "}";
            return oss.str();
        }


    private:
        std::unordered_map<std::string, uint64_t> times_;
};

struct Value {
    ByteString data_;
    VectorClock clock_;

    template <class Archive>
    void serialize(Archive & archive) {
        archive( 
            data_,
            clock_
        );
    }
};

using ValueList = std::vector<Value>; 
