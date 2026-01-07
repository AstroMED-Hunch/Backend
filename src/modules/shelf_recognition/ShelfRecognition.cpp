//
// Created by Marco Stulic on 1/7/26.
//

#include "ShelfRecognition.hpp"

#include <iostream>

#include "GeomUtils.hpp"
#include "main.hpp"
#include "modules/kiosk_comms/KioskEventHandler.hpp"

std::string ShelfRecognition::get_module_name() const {
    return "Shelf Recognition";
}

void ShelfRecognition::initialize() {
    std::cout << "Shelf Recognition module initialized" << std::endl;
}

void ShelfRecognition::run(cv::Mat cap) {
    auto global_layout = get_global_layout();
    for (const auto& code_group : global_layout.code_groups) {
        if (code_group->interpreter == MLayout::Interpreter::INTERPRETER_SHELF) {
            std::vector<Vec2> shelf_polygon;
            for (const auto& shelf_marker : code_group->markers) {
                if (shelf_marker->is_visible) {
                    shelf_polygon.push_back(shelf_marker->position);
                }
            }

            for (const auto& marker_pair : ArucoMarker::registered_markers) {
                ArucoMarker* marker = marker_pair.second;
                if (marker->is_visible) {
                    if (is_point_in_polygon(marker->position, shelf_polygon)) {
                        std::cout << "Marker " << marker->get_code_id() << " found on shelf" << std::endl;
                        if (!marker_shelf_mappings.contains(marker->get_code_id()) || marker_shelf_mappings[marker->get_code_id()] != code_group->tag) {
                            marker_shelf_mappings[marker->get_code_id()] = code_group->tag;
                            std::cout << "new marker" << std::endl;
                            KioskEventHandler::get()->on_box_entered_shelf(marker->get_code_id(), code_group);
                        }
                    } else {
                        if (marker_shelf_mappings.contains(marker->get_code_id())) {
                            marker_shelf_mappings.erase(marker->get_code_id());
                            std::cout << "Marker " << marker->get_code_id() << " removed from shelf" << std::endl;
                            KioskEventHandler::get()->on_box_exited_shelf(marker->get_code_id(), code_group);
                        }
                    }
                }
            }
        }
    }
}

void ShelfRecognition::shutdown() {

}