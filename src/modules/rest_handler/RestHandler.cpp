//
// Created by user on 2/28/26.
//

#include "RestHandler.hpp"
#include "httplib.h"
#include "json.hpp"

RestHandler* RestHandler::instance = nullptr;

void RestHandler::initialize() {
    instance = this;
    server = std::make_unique<httplib::Server>();

    server->Get("/getShelves", [](const httplib::Request &, httplib::Response &res) {
        nlohmann::json json;

    });

}

void RestHandler::run(cv::Mat cap) {

}

void RestHandler::shutdown() {

}

RestHandler* RestHandler::get() {
    return instance;
}