//
// Created by Marco Stulic on 1/30/26.
//

#include "ShelfDatabase.hpp"
#include <iostream>

#include <ranges>

std::unordered_map<int, BoxEntry> ShelfDatabase::box_db;
std::unordered_map<std::string, ShelfEntry*> ShelfDatabase::shelf_db;

int ShelfDatabase::box_currently_being_processed = -1;

BoxEntry* ShelfDatabase::get_box_entry(const int box_id) {
    if (box_db.contains(box_id)) {
        return &box_db.at(box_id);;
    }
    return nullptr;
}

BoxEntry* ShelfDatabase::get_box_entry_shelf(std::string shelf_id) {
    for (const auto& [key, entry] : box_db) {
        if (entry.get_shelf_id() == shelf_id) {
            return &box_db.at(key);
        }
    }
    return nullptr;
}

void ShelfDatabase::put_box_entry(const BoxEntry &entry) {
    box_db[entry.get_box_id()] = entry;
}

std::vector<BoxEntry*> ShelfDatabase::get_all_entries() {
    std::vector<BoxEntry*> entries;
    entries.reserve(box_db.size());
    for (BoxEntry& entry: box_db | std::views::values) {
        entries.push_back(&entry);
    }
    entries.shrink_to_fit();
    return entries;
}

void ShelfDatabase::pop_box_entry(const int box_id) {
    box_db.erase(box_id);
}

int ShelfDatabase::get_box_currently_being_processed() {
    return box_currently_being_processed;
}

void ShelfDatabase::set_box_currently_being_processed(int box_id) {
    box_currently_being_processed = box_id;
}

bool ShelfDatabase::is_shelf_occupied(const std::string& shelf_id) {
    for (const auto& [key, entry] : box_db) {
        if (entry.get_shelf_id() == shelf_id) {
            return true;
        }
    }
    return false;
}

ShelfEntry* ShelfDatabase::get_empty_shelf_entry() {
    for (auto& [key, entry] : shelf_db) {
        if (!is_shelf_occupied(key)) {
            return entry;
        }
    }
    return nullptr;
}

void ShelfDatabase::push_shelf_entry(ShelfEntry &entry) {
    shelf_db[entry.get_shelf_id()] = &entry;
}

void ShelfDatabase::set_shelf_box_is_on(const std::string& shelf_id, int box_id) {
    for (auto& [id, entry] : box_db) {
        if (entry.get_shelf_id() == shelf_id) {
            entry.set_shelf_id(std::nullopt);
        }
    }

    if (box_db.contains(box_id)) {
        box_db.at(box_id).set_shelf_id(shelf_id);
    } else {
        BoxEntry new_entry;
        new_entry.set_shelf_id(shelf_id);
        box_db.emplace(box_id, std::move(new_entry));
    }
}