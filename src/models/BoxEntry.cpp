//
// Created by Marco Stulic on 1/30/26.
//

#include "BoxEntry.hpp"
#include <chrono>

void BoxEntry::set_timestamp_current() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto dur = std::chrono::high_resolution_clock::now().time_since_epoch();
    timestamp = dur.count();
}

BoxEntry::BoxEntry() : box_id(-1) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    set_timestamp_current();
}

BoxEntry::BoxEntry(const BoxEntry& entry) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    user_placed = entry.user_placed;
    timestamp = entry.timestamp;
    shelf_id = entry.shelf_id;
    box_id = entry.box_id;
}

BoxEntry::BoxEntry(const std::string& user, int64_t ts, int id, sid shelf)
    : user_placed(user), timestamp(ts), box_id(id), shelf_id(shelf) {}

void BoxEntry::set_user_placed(const std::string& user) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    user_placed = user;
}

void BoxEntry::set_shelf_id(sid shelf) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    shelf_id = shelf;
}

void BoxEntry::set_timestamp(int64_t ts) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    timestamp = ts;
}

void BoxEntry::set_box_id(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    box_id = id;
}

void BoxEntry::put(const std::string& user, sid shelf) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    set_user_placed(user);
    set_timestamp_current();
}

std::string BoxEntry::get_user_placed() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return user_placed;
}

int64_t BoxEntry::get_timestamp() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return timestamp;
}

int BoxEntry::get_box_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return box_id;
}

BoxEntry::sid BoxEntry::get_shelf_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return shelf_id;
}