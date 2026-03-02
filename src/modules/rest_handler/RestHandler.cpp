//
// Created by user on 2/28/26.
//

#include "RestHandler.hpp"
#include "httplib.h"
#include "json.hpp"
#include "ShelfDatabase.hpp"
#include "modules/kiosk_comms/KioskEventHandler.hpp"
#include "modules/audit_log/AuditLog.hpp"
#include <sstream>

RestHandler* RestHandler::instance = nullptr;

void RestHandler::start_server() {
    server->set_pre_routing_handler([this](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");

        std::cout << "Responded to cors" << std::endl;

        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    server->Get("/getShelves", [](const httplib::Request &req, httplib::Response &res) {
        RestHandler::get()->handle_shelf_request(req, res);
    });

    server->Get("/getAuditLog", [](const httplib::Request &req, httplib::Response &res) {
        RestHandler::get()->handle_audit_log_request(req, res);
    });

    server->Get("/systemHealth", [](const httplib::Request &req, httplib::Response &res) {
        nlohmann::json json;
        json["status"] = "ok"; // always return ok so it knows we are connected
        res.set_header("Content-Type", "application/json");
        res.set_content(json.dump(), "application/json");
    });

    server->Delete("/clearShelf", [](const httplib::Request &req, httplib::Response &res) {
        std::string shelf_id = req.get_param_value("shelf_id");
        ShelfDatabase::set_shelf_box_is_on(shelf_id, -1, "");
        nlohmann::json json;
        json["status"] = "ok";
        res.set_header("Content-Type", "application/json");
        res.set_content(json.dump(), "application/json");
        KioskEventHandler::get()->tell_client_box_update();
    });

    std::string host = layout->get_config_value("rest_host");
    int port = std::stoi(layout->get_config_value("rest_port"));
    std::cout << "[RestHandler] Starting REST server on " << host << ":" << port << std::endl;
    server->listen(host, port);
}

void RestHandler::initialize() {
    instance = this;
    server = std::make_unique<httplib::Server>();
    std::thread server_thread(&RestHandler::start_server, this);
    server_thread.detach(); // go do your own thing

}

// this will run multithreaded so we need mutex locking
void RestHandler::handle_shelf_request(const httplib::Request& req, httplib::Response& res) {
    std::cout << "yapping shelf request" << std::endl;
    nlohmann::json json;
    std::vector<MLayout::CodeGroup> shelves;
    shelves.reserve(layout->code_groups.size());
    for (const auto& code_group : layout->code_groups) {
        if (code_group->interpreter == MLayout::Interpreter::INTERPRETER_SHELF) {
            shelves.push_back(*code_group);
        }
    }
    shelves.shrink_to_fit();

    std::vector<nlohmann::json> shelves_json;
    shelves_json.reserve(shelves.size());
    for (const auto& shelf : shelves) {
        nlohmann::json shelf_json;
        shelf_json["tag"] = shelf.tag;
        auto occupant = ShelfDatabase::get_box_entry_shelf(shelf.tag);
        std::cout << "Shelf " << shelf.tag << " is occupied: " << occupant << std::endl;
        shelf_json["box_id"] = occupant != nullptr ? occupant->get_box_id() : -1;
        shelf_json["registrant"] = occupant != nullptr ? occupant->get_user_placed() : "";

        int occupant_id = occupant != nullptr ? occupant->get_box_id() : -1;
        std::string pretty_name_box = layout->marker_id_to_pretty_name[occupant_id];
        shelf_json["box_pretty_name"] = pretty_name_box;

        shelves_json.push_back(shelf_json);
    }
    shelves_json.shrink_to_fit();

    json["shelves"] = shelves_json;

    res.set_header("Content-Type", "application/json");
    res.set_content(json.dump(), "application/json");
}

void RestHandler::run(cv::Mat cap) {

}

void RestHandler::shutdown() {

}

RestHandler* RestHandler::get() {
    return instance;
}

void RestHandler::handle_audit_log_request(const httplib::Request& req, httplib::Response& res) {
    nlohmann::json json;

    if (AuditLog::get() != nullptr) {
        std::string log_content = AuditLog::get()->get_log();
        std::vector<std::string> log_entries;

        std::stringstream ss(log_content);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty()) {
                log_entries.push_back(line);
            }
        }

        json["entries"] = log_entries;
        json["total_entries"] = log_entries.size();
    } else {
        json["error"] = "Audit log module not initialized";
        json["entries"] = nlohmann::json::array();
        json["total_entries"] = 0;
    }

    res.set_header("Content-Type", "application/json");
    res.set_content(json.dump(), "application/json");
}

