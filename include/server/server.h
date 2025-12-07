#pragma once

#include "logging/logger.h"
#include "storage/storage_engine.h"
#include "httplib.h"
#include <cstddef>
#include <vector>
#include <mutex>


template<typename Engine>
class Server {
    public:
        explicit Server(Engine engine) : engine_(std::move(engine)) {
            svr_.Get("/get", [this](const httplib::Request & req, httplib::Response &res) {
                this->handleGet(req, res);
            });
            svr_.Post("/put", [this](const httplib::Request & req, httplib::Response &res) {
                this->handlePut(req, res);
            });
        }
    
    // start in new thread (?)
    void start(const std::string& ip, int port) {
        Logger::instance().info(
            "starting http server on port: "  + std::to_string(port) + " with ip: " + ip
        );
        svr_.listen(ip, port);
    }

    private:
        Engine engine_;
        httplib::Server svr_;
        std::mutex mu_;

        void handleGet(const httplib::Request &req, httplib::Response &res){ 
            if(!req.has_header("Key")) {
                res.status = 400;
                res.set_content("Invalid request scheme. Must be binary data and include 'Key' header", "text/plain");
                return;
            }

            std::string key{req.get_header_value("Key")};


            mu_.lock();
            auto binary_data = engine_.get(key).data;
            mu_.unlock();

            std::string data_body(
                reinterpret_cast<const char*>(binary_data.data()), 
                binary_data.size()
            );

            Logger::instance().debug(
                "Fetching key: " + key + " Body: "
                + data_body
            );

            res.set_header("Content-Type", "application/octet-stream");
            res.set_content(data_body, "application/octet-stream"); // The second argument is for completeness but the header is already set

            res.status = 200;
        }

        void handlePut(const httplib::Request &req, httplib::Response &res){ 
            if(req.get_header_value("Content-Type") != "application/octet-stream" || 
            !req.has_header("Content-Length") || !req.has_header("Key")
            ) {
                res.status = 400;
                res.set_content("Invalid request scheme. Must be binary data and include 'Key' header", "text/plain");
                return;
            } 

            auto data = req.body;
            std::vector<std::byte> bytes;


            // make better
            for(auto c : data) {
                bytes.push_back(static_cast<std::byte>(c));
            }


            std::string key{req.get_header_value("Key")};
            Value value{bytes};

            Logger::instance().debug(
                "Inserting key: " + key + " Body: "
                + data
            );

            mu_.lock();
            engine_.put(key, value);
            mu_.unlock();

        }
};