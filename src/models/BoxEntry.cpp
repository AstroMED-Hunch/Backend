//
// Created by Marco Stulic on 1/30/26.
//

#include "BoxEntry.hpp"
#include <chrono>

void BoxEntry::set_timestamp_current() {
    auto dur = std::chrono::high_resolution_clock::now().time_since_epoch();
    timestamp = dur.count();
}

BoxEntry::BoxEntry() : box_id(-1) {
    set_timestamp_current();
}

BoxEntry::BoxEntry(const std::string& user, int64_t ts, int id, sid shelf)
    : user_placed(user), timestamp(ts), box_id(id), shelf_id(shelf) {}

void BoxEntry::set_user_placed(const std::string& user) {
    user_placed = user;
}

void BoxEntry::set_shelf_id(sid shelf) {
    shelf_id = shelf;
}

void BoxEntry::set_timestamp(int64_t ts) {
    timestamp = ts;
}

void BoxEntry::set_box_id(int id) {
    box_id = id;
}

void BoxEntry::put(const std::string& user, sid shelf) {
    set_user_placed(user);
    set_timestamp_current();
}

std::string BoxEntry::get_user_placed() const {
    return user_placed;
}

int64_t BoxEntry::get_timestamp() const {
    return timestamp;
}

int BoxEntry::get_box_id() const {
    return box_id;
}

BoxEntry::sid BoxEntry::get_shelf_id() const {
    return shelf_id;
}