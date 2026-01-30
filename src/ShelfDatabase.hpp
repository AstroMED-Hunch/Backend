//
// Created by Marco Stulic on 1/30/26.
//

#include <unordered_map>
#include <vector>

#include "models/BoxEntry.hpp"

class ShelfDatabase {
public:
    [[nodiscard]] static BoxEntry* get_box_entry(int box_id);
    [[nodiscard]] static BoxEntry* get_box_entry_shelf(int shelf_id);
    static void put_box_entry(const BoxEntry& entry);

    [[nodiscard]] static std::vector<BoxEntry*> get_all_entries();

    static int get_box_currently_being_processed();
    static void set_box_currently_being_processed(int box_id);

    static void pop_box_entry(int box_id);
private:
    static int box_currently_being_processed;
    static std::unordered_map<int,BoxEntry> shelf_db;
};
