//
// Created by Marco Stulic on 1/6/26.
//

#include "KioskEventHandler.hpp"
#include "json.hpp"

#include <iostream>

#include "main.hpp"
#include "ShelfDatabase.hpp"
#include "models/ShelfEntry.hpp"
#include "modules/audit_log/AuditLog.hpp"
#include "modules/pill_detect/PillDetector.hpp"

KioskEventHandler* KioskEventHandler::instance = nullptr;

std::string KioskEventHandler::get_module_name() const {
    return "KioskEventHandler";
}

void KioskEventHandler::on_msg(const std::string& msg_type, std::string content) {
    if (msg_type == "registerBox") {
        if (status == KioskStatus::IDLE) {
            change_status(KioskStatus::FACE_RECOGNITION_BOXENTRY);
        } else if (status == KioskStatus::FACE_RECOGNITION_BOXENTRY) {
            registering_identity = last_seen_identity; // capture

            pending_pill_result.clear();
            if (PillDetector::get() != nullptr) {
                PillDetector::get()->get_result([this](PillResult result) {
                    pending_pill_result = std::move(result);
                    std::cout << "[KioskEventHandler] Pill scan captured " << pending_pill_result.size() << " pill type(s)" << std::endl;
                });
            }

            change_status(KioskStatus::PILL_CHECKUP);
        } else {
            int box_being_registered = last_box_sent;
            auto empty_shelf = ShelfDatabase::get_empty_shelf_entry();

            if (empty_shelf == nullptr) {
                std::cerr << "[KioskEventHandler] No empty shelf available for box registration." << std::endl;
                if (AuditLog::get() != nullptr) {
                    AuditLog::get()->add_log_entry("ERROR: No empty shelf available for box registration");
                }
                change_status(KioskStatus::SHELVES_FULL);
                return;
            }

            // send it as json
            nlohmann::json pills_json = nlohmann::json::array();
            for (const auto& pill : pending_pill_result) {
                nlohmann::json pill_obj;
                pill_obj["pill_type"] = pill.pill_type;
                pill_obj["quantity"] = pill.quantity;
                pills_json.push_back(pill_obj);
            }

            nlohmann::json event_msg;
            event_msg["type"] = "boxLocation";
            event_msg["msg"] = empty_shelf->get_shelf_id();
            event_msg["pills"] = pills_json;
            ws_socket->send(event_msg.dump());

            ShelfDatabase::set_shelf_box_is_on(empty_shelf->get_shelf_id(), box_being_registered, registering_identity);

            // commit
            BoxEntry* box_entry = ShelfDatabase::get_box_entry(box_being_registered);
            if (box_entry != nullptr) {
                box_entry->set_pills(pending_pill_result);
            }
            pending_pill_result.clear();

            std::cout << "Picked the shelf to place box on: " << empty_shelf->get_shelf_id() << std::endl;
            if (AuditLog::get() != nullptr) {
                AuditLog::get()->add_log_entry("Box " + std::to_string(box_being_registered) + " registered by " + registering_identity + " on shelf " + empty_shelf->get_shelf_id());
            }

            tell_client_box_update();
            status = KioskStatus::IDLE; // do not inform
        }
    } else if (msg_type == "registerBoxExit") {
        if (status == KioskStatus::IDLE) {
            change_status(KioskStatus::FACE_RECOGNITION_BOXEXIT);
        } else {
            try {
                std::string shelf_being_checked_out = content;
                auto shelf_entry = ShelfDatabase::get_shelf_entry(shelf_being_checked_out);
                if (shelf_entry == nullptr) {
                    std::cerr << "[KioskEventHandler] Shelf not found for box exit: " << shelf_being_checked_out << std::endl;
                    if (AuditLog::get() != nullptr) {
                        AuditLog::get()->add_log_entry("ERROR: Shelf not found for box exit: " + shelf_being_checked_out);
                    }
                    change_status(KioskStatus::IDLE);
                    return;
                }
                BoxEntry* box_on_shelf = ShelfDatabase::get_box_on_shelf(shelf_being_checked_out);
                if (box_on_shelf == nullptr) {
                    std::cerr << "[KioskEventHandler] No box found on shelf for box exit: " << shelf_being_checked_out << std::endl;
                    if (AuditLog::get() != nullptr) {
                        AuditLog::get()->add_log_entry("ERROR: No box found on shelf for box exit: " + shelf_being_checked_out);
                    }
                    change_status(KioskStatus::IDLE);
                    return;
                }
                int box_being_registered = box_on_shelf->get_box_id();
                ShelfDatabase::set_shelf_box_is_on("", box_being_registered, "");

                std::cout << "Box exited, removed from shelf: " << box_being_registered << std::endl;
                if (AuditLog::get() != nullptr) {
                    AuditLog::get()->add_log_entry("Box " + std::to_string(box_being_registered) + " checked out by " + last_seen_identity + " from shelf " + shelf_being_checked_out);
                }

                tell_client_box_update();
                change_status(KioskStatus::IDLE); // do not inform
            } catch (const std::exception& e) {
                change_status(KioskStatus::IDLE);
                std::cerr << "[KioskEventHandler] Error handling registerBoxExit message: " << e.what() << std::endl;
                if (AuditLog::get() != nullptr) {
                    AuditLog::get()->add_log_entry("ERROR: Exception in registerBoxExit: " + std::string(e.what()));
                }
            }

        }
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
                const nlohmann::json received_json = nlohmann::json::parse(msg->str, nullptr, false);
                if (received_json.is_discarded()) {
                    std::cerr << "[KioskEventHandler] Error parsing JSON message." << std::endl;
                    return;
                }

                std::string message = received_json.value("type", "");
                std::string content = received_json.value("message", "");
                KioskEventHandler::get()->on_msg(message, content);
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

    if (AuditLog::get() != nullptr) {
        AuditLog::get()->add_log_entry("Status changed to: " + status_to_str(new_status));
    }
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
    event_msg["message"] = box_code_id;
    ws_socket->send(event_msg.dump());

    if (AuditLog::get() != nullptr) {
        AuditLog::get()->add_log_entry("Box " + std::to_string(box_code_id) + " entered the system");
    }
}

void KioskEventHandler::tell_client_box_update() {
    nlohmann::json event_msg;
    event_msg["type"] = "boxUpdate";
    event_msg["box_code_id"] = last_box_sent;
    ws_socket->send(event_msg.dump());
}

void KioskEventHandler::on_box_exited(int box_code_id) const {
    nlohmann::json event_msg;
    event_msg["type"] = "boxExited";
    event_msg["message"] = box_code_id;
    ws_socket->send(event_msg.dump());

    if (AuditLog::get() != nullptr) {
        AuditLog::get()->add_log_entry("Box " + std::to_string(box_code_id) + " exited the system");
    }
}

void KioskEventHandler::send_face_recogniton_update(const std::string& identity) {
    nlohmann::json event_msg;
    event_msg["type"] = "faceRecognitionUpdate";
    event_msg["message"] = identity;
    ws_socket->send(event_msg.dump());
    last_seen_identity = identity;
}
