//
// Created by user on 2/28/26.
//

#pragma once
#include "Module.hpp"

namespace httplib
{
    class Server; // Forward declaration
}

class RestHandler : public Module {
public:
    static RestHandler* get();
    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;
protected:
    static RestHandler* instance;
    std::unique_ptr<httplib::Server> server{};
};
