//
// Created by Marco Stulic on 1/30/26.
//

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "models/BoxEntry.hpp"
#include "models/ShelfEntry.hpp"

class ShelfDatabase {
public:
    [[nodiscard]] static BoxEntry* get_box_entry(int box_id);
    [[nodiscard]] static BoxEntry* get_box_entry_shelf(std::string shelf_id);
    [[nodiscard]] static bool is_shelf_occupied(const std::string& shelf_id);
    [[nodiscard]] static ShelfEntry* get_empty_shelf_entry();
    static void set_shelf_box_is_on(const std::string& shelf_id, int box_id);
    static void put_box_entry(const BoxEntry& entry);

    [[nodiscard]] static std::vector<BoxEntry*> get_all_entries();

    static int get_box_currently_being_processed();
    static void set_box_currently_being_processed(int box_id);

    static void pop_box_entry(int box_id);
    static void push_shelf_entry(ShelfEntry &entry);
private:
    static int box_currently_being_processed;
    static std::unordered_map<int,std::shared_ptr<BoxEntry>> box_db;
    static std::unordered_map<std::string, ShelfEntry*> shelf_db;
    static std::recursive_mutex shelf_db_mutex;
};
