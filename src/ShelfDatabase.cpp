//
// Created by Marco Stulic on 1/30/26.
//

#include "ShelfDatabase.hpp"
#include <iostream>

#include <ranges>

std::unordered_map<int, BoxEntry> ShelfDatabase::shelf_db;
int ShelfDatabase::box_currently_being_processed = -1;

BoxEntry* ShelfDatabase::get_box_entry(const int box_id) {
    if (shelf_db.contains(box_id)) {
        return &shelf_db.at(box_id);;
    }
    return nullptr;
}

BoxEntry* ShelfDatabase::get_box_entry_shelf(int shelf_id) {
    for (const auto& [key, entry] : shelf_db) {
        if (entry.get_shelf_id() == shelf_id) {
            return &shelf_db.at(key);
        }
    }
    return nullptr;
}

void ShelfDatabase::put_box_entry(const BoxEntry &entry) {
    shelf_db[entry.get_box_id()] = entry;
}

std::vector<BoxEntry*> ShelfDatabase::get_all_entries() {
    std::vector<BoxEntry*> entries;
    entries.reserve(shelf_db.size());
    for (BoxEntry& entry: shelf_db | std::views::values) {
        entries.push_back(&entry);
    }
    entries.shrink_to_fit();
    return entries;
}

void ShelfDatabase::pop_box_entry(const int box_id) {
    shelf_db.erase(box_id);
}

int ShelfDatabase::get_box_currently_being_processed() {
    return box_currently_being_processed;
}

void ShelfDatabase::set_box_currently_being_processed(int box_id) {
    box_currently_being_processed = box_id;
}