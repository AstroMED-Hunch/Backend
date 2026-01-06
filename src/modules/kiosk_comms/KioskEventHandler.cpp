//
// Created by Marco Stulic on 1/6/26.
//

#include "KioskEventHandler.hpp"

#include <iostream>


std::string KioskEventHandler::get_module_name() const {
    return "KioskEventHandler";
}

void KioskEventHandler::initialize() {
    ws_socket = new ix::WebSocket();

    std::string ws_url = layout->get_config_value("kiosk_event_ws_uri");
    int timeout_secs = std::stoi(layout->get_config_value("kiosk_event_ws_timeout_secs"));

    ws_socket->setUrl(ws_url);
    std::cout << "[KioskEventHandler] Starting up websocket to " << ws_url << std::endl;

    ws_socket->setOnMessageCallback(
        [](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Message) {
                std::cout << "[KioskEventHandler] Received message: " << msg->str << std::endl;
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
    auto result = ws_socket->connect(timeout_secs);

    if (!result.success) {
        std::cerr << "[KioskEventHandler] WebSocket connection failed: " << result.errorStr << std::endl;
        return;
    }

    std::cout << "[KioskEventHandler] WebSocket initialized and connecting to " << ws_url << std::endl;
}

void KioskEventHandler::run(cv::Mat cap) {

}

void KioskEventHandler::shutdown() {
    ws_socket->close();
}