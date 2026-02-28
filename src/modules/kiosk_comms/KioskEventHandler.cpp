//
// Created by Marco Stulic on 1/6/26.
//

#include "KioskEventHandler.hpp"
#include "json.hpp"

#include <iostream>

#include "main.hpp"
#include "ShelfDatabase.hpp"
#include "models/ShelfEntry.hpp"

KioskEventHandler* KioskEventHandler::instance = nullptr;

std::string KioskEventHandler::get_module_name() const {
    return "KioskEventHandler";
}

void KioskEventHandler::on_msg(const std::string& msg_type) {
    if (msg_type == "registerBox") {
        int box_being_registered = last_box_sent;
        auto empty_shelf = ShelfDatabase::get_empty_shelf_entry();

        if (empty_shelf == nullptr) {
            std::cerr << "[KioskEventHandler] No empty shelf available for box registration." << std::endl;
            change_status(KioskStatus::SHELVES_FULL);
            return;
        }
        nlohmann::json event_msg;
        event_msg["type"] = "boxLocation";
        event_msg["msg"] = empty_shelf->get_shelf_id();
        ws_socket->send(event_msg.dump());

        ShelfDatabase::set_shelf_box_is_on(empty_shelf->get_shelf_id(), box_being_registered);

        std::cout << "Picked the shelf to place box on: " << empty_shelf->get_shelf_id() << std::endl;
    }
}

void KioskEventHandler::initialize() {
    instance = this;
    ws_socket = new ix::WebSocket();

    std::string ws_url = layout->get_config_value("kiosk_event_ws_uri");
    int timeout_secs = std::stoi(layout->get_config_value("kiosk_event_ws_timeout_secs"));

    ws_socket->setUrl(ws_url);
    std::cout << "[KioskEventHandler] Starting up websocket to " << ws_url << std::endl;

    ws_socket->setOnMessageCallback(
        [](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Message) {
                std::cout << "[KioskEventHandler] Received message: " << msg->str << std::endl;
                nlohmann::json received_json = nlohmann::json::parse(msg->str, nullptr, false);
                if (received_json.is_discarded()) {
                    std::cerr << "[KioskEventHandler] Error parsing JSON message." << std::endl;
                    return;
                }

                std::string message = received_json.value("type", "");
                KioskEventHandler::get()->on_msg(message);
            }
            else if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[KioskEventHandler] WebSocket connection opened." << std::endl;
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[KioskEventHandler] WebSocket connection closed." << std::endl;
            }
            else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "[KioskEventHandler] WebSocket error: " << msg->errorInfo.reason << std::endl;
            }
        }
    );

    ws_socket->enableAutomaticReconnection();
    ws_socket->setPingInterval(45);
    //auto result = ws_socket->connect(timeout_secs);
    ws_socket->start();
}

void KioskEventHandler::run(cv::Mat cap) {

}

void KioskEventHandler::shutdown() {
    ws_socket->close();
}

void KioskEventHandler::change_status(KioskStatus new_status) {
    status = new_status;

    nlohmann::json status_msg;
    status_msg["type"] = "statusUpdate";
    status_msg["status"] = status_to_str(new_status);
    ws_socket->send(status_msg.dump());
}

KioskStatus KioskEventHandler::get_status() const {
    return status;
}

std::string KioskEventHandler::status_to_str(KioskStatus set_status) const {
    auto it = kiosk_to_str.find(status);
    if (it != kiosk_to_str.end()) {
        return it->second;
    }
    return "unknown";
}

void KioskEventHandler::internal_ws_thread_run() {

}

KioskEventHandler* KioskEventHandler::get() {
    return instance;
}

void KioskEventHandler::on_box_entered_shelf(int box_code_id, MLayout::CodeGroup* shelf_code_group) {
    // nlohmann::json event_msg;
    // event_msg["type"] = "boxEnteredShelf";
    // event_msg["box_code_id"] = box_code_id;
    // event_msg["shelf_code_group"] = shelf_code_group->tag;
    // ws_socket->send(event_msg.dump());
}

void KioskEventHandler::on_box_exited_shelf(int box_code_id, MLayout::CodeGroup* shelf_code_group) {
    // nlohmann::json event_msg;
    // event_msg["type"] = "boxExitedShelf";
    // event_msg["box_code_id"] = box_code_id;
    // event_msg["shelf_code_group"] = shelf_code_group->tag;
    // ws_socket->send(event_msg.dump());
}

void KioskEventHandler::on_box_entered(int box_code_id) {
    last_box_sent = box_code_id;

    nlohmann::json event_msg;
    event_msg["type"] = "boxEntered";
    event_msg["box_code_id"] = box_code_id;
    ws_socket->send(event_msg.dump());
}

void KioskEventHandler::on_box_exited(int box_code_id) {
    nlohmann::json event_msg;
    event_msg["type"] = "boxExited";
    event_msg["box_code_id"] = box_code_id;
    ws_socket->send(event_msg.dump());
}