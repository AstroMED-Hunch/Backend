//
// Created by Marco Stulic on 2/5/26.
//

#pragma once
#include <string>

class ShelfEntry {
public:
    explicit ShelfEntry(std::string shelf_id);

    std::string get_shelf_id();
    void set_shelf_id(const std::string& id);
protected:
    std::string shelf_id;
};
