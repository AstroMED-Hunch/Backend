//
// Created by Marco Stulic on 1/6/26.
//

#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"
#include <thread>

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>

enum class KioskStatus {
    IDLE,
    MULTIPLE_BOXES,
    SHELVES_FULL,
    FACE_RECOGNITION_BOXENTRY
};

const std::map<KioskStatus, std::string> kiosk_to_str = {
    {KioskStatus::IDLE, "idle"},
    {KioskStatus::MULTIPLE_BOXES, "multiple_boxes"},
    {KioskStatus::FACE_RECOGNITION_BOXENTRY, "face_recognition_boxentry"},
    {KioskStatus::SHELVES_FULL, "shelves_full"},
};

class KioskEventHandler : public Module {
public:
    std::string get_module_name() const override;

    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;

    void change_status(KioskStatus new_status);
    static KioskEventHandler* get();
    KioskStatus get_status() const;
    void on_box_entered_shelf(int box_code_id, MLayout::CodeGroup* shelf_code_group);
    void on_box_exited_shelf(int box_code_id, MLayout::CodeGroup* shelf_code_group);

    void tell_client_box_update();

    void on_box_entered(int box_code_id);
    void on_box_exited(int box_code_id) const;

    void on_msg(const std::string& msg_type);

    void send_face_recogniton_update(const std::string& identity);
protected:

    KioskStatus status = KioskStatus::IDLE;
    std::string status_to_str(KioskStatus set_status) const;
    void internal_ws_thread_run();
    ix::WebSocket* ws_socket = nullptr;
    static KioskEventHandler* instance;
    int last_box_sent = -1;
    std::string last_seen_identity = "err_nodetect";
};

MAKE_MODULE(KioskEventHandler, "kiosk_events");
