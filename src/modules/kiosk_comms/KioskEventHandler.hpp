//
// Created by Marco Stulic on 1/6/26.
//

#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"
#include <thread>

#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>


class KioskEventHandler : public Module {
public:
    std::string get_module_name() const override;

    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;
protected:

    void internal_ws_thread_run();
    ix::WebSocket* ws_socket = nullptr;
};

MAKE_MODULE(KioskEventHandler, "kiosk_events");
