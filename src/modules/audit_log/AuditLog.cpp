//
// Created by user on 3/1/26.
//

#include "AuditLog.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

AuditLog* AuditLog::instance = nullptr;

std::string AuditLog::get_module_name() const {
    return "Audit Log";
}

void AuditLog::initialize() {
    instance = this;
}

void AuditLog::run(cv::Mat cap) {
}

void AuditLog::shutdown() {
    instance = nullptr;
}

std::string AuditLog::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string AuditLog::get_log() const {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::string full_log;
    for (const auto& entry : log_entries) {
        full_log += entry + "\n";
    }
    return full_log;
}

void AuditLog::add_log_entry(const std::string& entry) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::string timestamped_entry = "[" + get_timestamp() + "] " + entry;
    log_entries.push_back(timestamped_entry);
    std::cout << "Audit Log Entry Added: " << timestamped_entry << std::endl;
}

AuditLog* AuditLog::get() {
    return instance;
}