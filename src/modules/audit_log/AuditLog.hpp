//
// Created by user on 3/1/26.
//

#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"
#include <mutex>
#include <chrono>

class AuditLog : public Module {
public:
    std::string get_module_name() const override;

    void initialize() override;

    void run(cv::Mat cap) override;

    void shutdown() override;

    std::string get_log() const;
    void add_log_entry(const std::string& entry);

    static AuditLog* get();
protected:
    std::vector<std::string> log_entries;
    mutable std::mutex log_mutex;
    static AuditLog* instance;

    std::string get_timestamp() const;
};

MAKE_MODULE(AuditLog, "audit_log");
