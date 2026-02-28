//
// Created by Marco Stulic on 2/5/26.
//

#include "ShelfEntry.hpp"


std::string ShelfEntry::get_shelf_id() {
    return shelf_id;
}

void ShelfEntry::set_shelf_id(const std::string& id) {
    shelf_id = id;
}

ShelfEntry::ShelfEntry(std::string shelf_id) : shelf_id(std::move(shelf_id)) {
    // null implimentation
}
