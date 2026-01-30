//
// Created by Marco Stulic on 1/30/26.
//

#pragma once
#include <string>


class BoxEntry {
public:
    BoxEntry();
    BoxEntry(const std::string& user, int64_t ts, int id, int shelf);

    void set_user_placed(const std::string& user);
    void set_timestamp(int64_t ts);
    void set_box_id(int id);
    void set_shelf_id(int shelf);

    void set_timestamp_current();
    void put(const std::string& user, int shelf);

    [[nodiscard]] std::string get_user_placed() const;
    [[nodiscard]] int64_t get_timestamp() const;
    [[nodiscard]] int get_box_id() const;
    [[nodiscard]] int get_shelf_id() const;
private:
    std::string user_placed;
    int64_t timestamp;
    int shelf_id;
    int box_id;
};
