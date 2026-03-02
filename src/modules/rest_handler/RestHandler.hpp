//
// Created by user on 2/28/26.
//

#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"

namespace httplib
{
    class Server; // Forward declaration
    class Request; // Forward declaration
    class Response; // Forward declaration
}

class RestHandler : public Module {
public:
    static RestHandler* get();
    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;
    void start_server();

    void handle_shelf_request(const httplib::Request& req, httplib::Response& res);
    void handle_audit_log_request(const httplib::Request& req, httplib::Response& res);
protected:
    static RestHandler* instance;
    std::unique_ptr<httplib::Server> server{};
};

MAKE_MODULE(RestHandler, "rest_handler");