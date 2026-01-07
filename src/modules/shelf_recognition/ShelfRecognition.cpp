//
// Created by Marco Stulic on 1/7/26.
//

#include "ShelfRecognition.hpp"

#include <iostream>
#include <opencv2/core/utility.hpp>

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
    double current_time = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();

    for (const auto& code_group : global_layout.code_groups) {
        if (code_group->interpreter == MLayout::Interpreter::INTERPRETER_SHELF) {
            std::vector<Vec2> shelf_polygon;
            for (const auto& shelf_marker : code_group->markers) {
                if (shelf_marker->is_visible) {
                    shelf_polygon.push_back(shelf_marker->position);
                }
            }
            for (const auto& active_marker : code_group->bound_to.at(0)->markers) {
                ArucoMarker* marker = active_marker.get();
                bool currently_on_shelf = false;

                if (marker->is_visible && is_point_in_polygon(marker->position, shelf_polygon)) {
                    currently_on_shelf = true;
                    marker_last_found_time[marker->get_code_id()] = current_time;

                    // handle entry
                    if (!marker_shelf_mappings.contains(marker->get_code_id()) || marker_shelf_mappings[marker->get_code_id()] != code_group->tag) {
                        marker_shelf_mappings[marker->get_code_id()] = code_group->tag;
                        std::cout << "new marker:" << marker->get_code_id() << std::endl;
                        KioskEventHandler::get()->on_box_entered_shelf(marker->get_code_id(), code_group);
                    }
                }
                if (!currently_on_shelf) {
                    if (marker_shelf_mappings.contains(marker->get_code_id())) {
                        if (current_time < marker_last_found_time[marker->get_code_id()] + EXPIRATION_TIME) {
                            continue;
                        }

                        marker_shelf_mappings.erase(marker->get_code_id());
                        std::cout << "Marker " << marker->get_code_id() << " removed from shelf" << std::endl;
                        KioskEventHandler::get()->on_box_exited_shelf(marker->get_code_id(), code_group);
                    }
                }
            }
        }
    }
}

void ShelfRecognition::shutdown() {

}