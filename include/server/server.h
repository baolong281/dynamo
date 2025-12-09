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
        explicit Server(Engine&& engine) : engine_(std::move(engine)) {
            svr_.Get("/get", [this](const httplib::Request & req, httplib::Response &res) {
                this->handleGet(req, res);
            });
            svr_.Post("/put", [this](const httplib::Request & req, httplib::Response &res) {
                this->handlePut(req, res);
            });
            Logger::instance().info(
                "endpoints /get /put registered!"
            );
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
            ByteString binary_data = engine_.get(key);
            mu_.unlock();

            Logger::instance().debug(
                "Fetching key: " + key + " Body: "
                + binary_data
            );

            res.set_header("Content-Type", "application/octet-stream");
            res.set_content(binary_data, "application/octet-stream"); 

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

            std::string key{req.get_header_value("Key")};

            Logger::instance().debug(
                "Inserting key: " + key + " Body: "
                + data
            );

            mu_.lock();
            engine_.put(key, data);
            mu_.unlock();

        }
};