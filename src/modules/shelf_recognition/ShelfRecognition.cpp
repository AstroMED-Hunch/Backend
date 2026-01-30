//
// Created by Marco Stulic on 1/7/26.
//

#include "ShelfRecognition.hpp"

#include <iostream>
#include <ranges>
#include <opencv2/core/utility.hpp>

#include "GeomUtils.hpp"
#include "main.hpp"
#include "ShelfDatabase.hpp"
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

    std::vector<ArucoMarker*> new_markers;

    for (const auto& marker : ArucoMarker::registered_markers | std::views::values) {
        if (marker->is_visible) {
            if (ShelfDatabase::get_box_entry(marker->get_code_id()) == nullptr && ShelfDatabase::get_box_currently_being_processed() != marker->get_code_id()) { // if its a new code
                new_markers.push_back(marker);
            }

            if (marker->get_code_id() == ShelfDatabase::get_box_currently_being_processed()) {
                currently_processing_last_seen_time = current_time;
            }
        }
    }

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
                // if (!currently_on_shelf) {
                //     if (marker_shelf_mappings.contains(marker->get_code_id())) {
                //         if (current_time < marker_last_found_time[marker->get_code_id()] + EXPIRATION_TIME) {
                //             continue;
                //         }
                //
                //         marker_shelf_mappings.erase(marker->get_code_id());
                //         std::cout << "Marker " << marker->get_code_id() << " removed from shelf" << std::endl;
                //         KioskEventHandler::get()->on_box_exited_shelf(marker->get_code_id(), code_group);
                //     }
                // }
            }
        }
    }

    if (new_markers.size() > 1) {
        if (KioskEventHandler::get()->get_status() == KioskStatus::IDLE) {
            KioskEventHandler::get()->change_status(KioskStatus::MULTIPLE_BOXES);
            std::cout << "Multiple boxes detected, changing status to MULTIPLE_BOXES" << std::endl;
        }

        if (ShelfDatabase::get_box_currently_being_processed() != -1) {
            int exited_box_id = ShelfDatabase::get_box_currently_being_processed();
            std::cout << "Box " << exited_box_id << " processing canceled due to multiple boxes detected" << std::endl;
            KioskEventHandler::get()->on_box_exited(exited_box_id);
        }
        ShelfDatabase::set_box_currently_being_processed(-1);
    } else if (new_markers.size() == 1) {
        std::cout << "Single box detected" << std::endl;
        if (KioskEventHandler::get()->get_status() == KioskStatus::MULTIPLE_BOXES) {
            KioskEventHandler::get()->change_status(KioskStatus::IDLE);
        }

        int new_box_id = new_markers[0]->get_code_id();
        ShelfDatabase::set_box_currently_being_processed(new_box_id);
        KioskEventHandler::get()->on_box_entered(new_box_id);
        currently_processing_last_seen_time = current_time;
    } else {
        if (KioskEventHandler::get()->get_status() == KioskStatus::MULTIPLE_BOXES) {
            std::cout << "No boxes detected, changing status to IDLE" << std::endl;
            KioskEventHandler::get()->change_status(KioskStatus::IDLE);
        }
    }

    if (currently_processing_last_seen_time + EXPIRATION_TIME < current_time && ShelfDatabase::get_box_currently_being_processed() != -1) {
        int expired_box_id = ShelfDatabase::get_box_currently_being_processed();
        std::cout << "Box " << expired_box_id << " processing expired due to timeout" << std::endl;
        KioskEventHandler::get()->on_box_exited(expired_box_id);
        ShelfDatabase::set_box_currently_being_processed(-1);
    }
}

void ShelfRecognition::shutdown() {

}