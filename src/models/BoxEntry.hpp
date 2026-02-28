//
// Created by Marco Stulic on 1/30/26.
//

#pragma once
#include <mutex>
#include <optional>
#include <string>


class BoxEntry {
public:
    using sid = std::optional<std::string>;
    BoxEntry();
    BoxEntry(const std::string& user, int64_t ts, int id, sid shelf);
    BoxEntry(const BoxEntry& entry);

    void set_user_placed(const std::string& user);
    void set_timestamp(int64_t ts);
    void set_box_id(int id);
    void set_shelf_id(sid shelf);

    void set_timestamp_current();
    void put(const std::string& user, sid shelf);

    [[nodiscard]] std::string get_user_placed() const;
    [[nodiscard]] int64_t get_timestamp() const;
    [[nodiscard]] int get_box_id() const;
    [[nodiscard]] sid get_shelf_id() const;
private:
    std::string user_placed;
    int64_t timestamp;
    sid shelf_id;
    int box_id;
    mutable std::recursive_mutex mutex;
};
