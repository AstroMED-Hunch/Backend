//
// Created by Marco Stulic on 1/30/26.
//

#include "ShelfDatabase.hpp"
#include <iostream>

#include <ranges>
#include <opencv2/core/utility.hpp>

std::unordered_map<int, std::shared_ptr<BoxEntry>> ShelfDatabase::box_db;
std::recursive_mutex ShelfDatabase::shelf_db_mutex;
std::unordered_map<std::string, ShelfEntry*> ShelfDatabase::shelf_db;

int ShelfDatabase::box_currently_being_processed = -1;

BoxEntry* ShelfDatabase::get_box_entry(const int box_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    if (box_db.contains(box_id)) {
        return box_db[box_id].get();
    }
    return nullptr;
}

BoxEntry* ShelfDatabase::get_box_entry_shelf(std::string shelf_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    for (const auto& [key, entry] : box_db) {
        if (entry->get_shelf_id() == shelf_id) {
            return entry.get();
        }
    }
    return nullptr;
}

void ShelfDatabase::put_box_entry(const BoxEntry &entry) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    const std::shared_ptr<BoxEntry> new_entry = std::make_shared<BoxEntry>(entry);
    box_db[entry.get_box_id()] = new_entry;
}

std::vector<BoxEntry*> ShelfDatabase::get_all_entries() {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    std::vector<BoxEntry*> entries;
    entries.reserve(box_db.size());
    for (std::shared_ptr<BoxEntry> entry: box_db | std::views::values) {
        entries.push_back(entry.get());
    }
    entries.shrink_to_fit();
    return entries;
}

void ShelfDatabase::pop_box_entry(const int box_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    box_db.erase(box_id);
}

int ShelfDatabase::get_box_currently_being_processed() {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    return box_currently_being_processed;
}

void ShelfDatabase::set_box_currently_being_processed(int box_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    box_currently_being_processed = box_id;
}

bool ShelfDatabase::is_shelf_occupied(const std::string& shelf_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    for (const auto& [key, entry] : box_db) {
        if (entry->get_shelf_id() == shelf_id) {
            return true;
        }
    }
    return false;
}

ShelfEntry* ShelfDatabase::get_empty_shelf_entry() {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    for (auto& [key, entry] : shelf_db) {
        if (!is_shelf_occupied(key)) {
            return entry;
        }
    }
    return nullptr;
}

void ShelfDatabase::push_shelf_entry(ShelfEntry &entry) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    shelf_db[entry.get_shelf_id()] = &entry;
}

void ShelfDatabase::set_shelf_box_is_on(const std::string& shelf_id, int box_id) {
    std::lock_guard<std::recursive_mutex> lock(shelf_db_mutex);
    for (auto& [id, entry] : box_db) {
        if (entry->get_shelf_id() == shelf_id) {
            entry->set_shelf_id(std::nullopt);
        }
    }

    if (box_db.contains(box_id)) {
        box_db.at(box_id)->set_shelf_id(shelf_id);
        std::cout << "Updated box entry for box " << box_id << " to be on shelf " << shelf_id << std::endl;
    } else {
        const auto new_entry = std::make_shared<BoxEntry>("",cv::getTickCount(), box_id, shelf_id);
        box_db[box_id] = new_entry;
        std::cout << "Created new box entry for box " << box_id << " on shelf " << shelf_id << std::endl;
    }
}