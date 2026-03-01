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

    std::vector<ArucoMarker*> visible_unregistered_markers;

    for (const auto& marker : ArucoMarker::registered_markers | std::views::values) {
        if (marker->is_visible) {
            if (ShelfDatabase::get_box_entry(marker->get_code_id()) == nullptr) {
                visible_unregistered_markers.push_back(marker);
            }

            if (marker->get_code_id() == ShelfDatabase::get_box_currently_being_processed()) {
                currently_processing_last_seen_time = current_time;
            }
        }
    }

    // find the new markers
    std::vector<ArucoMarker*> new_markers;
    for (const auto& marker : visible_unregistered_markers) {
        if (!seen_box_ids.contains(marker->get_code_id())) {
            new_markers.push_back(marker);
        }
    }

    // check positions
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

                if (marker->is_visible && is_point_in_polygon(marker->position, shelf_polygon)) {
                    marker_last_found_time[marker->get_code_id()] = current_time;

                    // handle entry
                    if (!marker_shelf_mappings.contains(marker->get_code_id()) || marker_shelf_mappings[marker->get_code_id()] != code_group->tag) {
                        marker_shelf_mappings[marker->get_code_id()] = code_group->tag;
                        std::cout << "new marker on shelf:" << marker->get_code_id() << std::endl;
                        KioskEventHandler::get()->on_box_entered_shelf(marker->get_code_id(), code_group);
                    }
                }
            }
        }
    }

    int visible_unregistered_count = static_cast<int>(visible_unregistered_markers.size());

    if (visible_unregistered_count > 1) {
        if (KioskEventHandler::get()->get_status() != KioskStatus::MULTIPLE_BOXES) {
            KioskEventHandler::get()->change_status(KioskStatus::MULTIPLE_BOXES);
            std::cout << "Multiple boxes detected, changing status to MULTIPLE_BOXES" << std::endl;
        }

        int processing_id = ShelfDatabase::get_box_currently_being_processed();
        if (processing_id != -1) {
            std::cout << "Box " << processing_id << " processing canceled due to multiple boxes detected" << std::endl;
            KioskEventHandler::get()->on_box_exited(processing_id);
            seen_box_ids.erase(processing_id);
            ShelfDatabase::set_box_currently_being_processed(-1);
            currently_processing_last_seen_time = std::numeric_limits<double>::max();
        }

        for (const auto& marker : visible_unregistered_markers) {
            seen_box_ids.insert(marker->get_code_id());
        }

    } else if (visible_unregistered_count == 1) {
        ArucoMarker* candidate = visible_unregistered_markers[0];
        int candidate_id = candidate->get_code_id();

        if (KioskEventHandler::get()->get_status() == KioskStatus::MULTIPLE_BOXES) {
            KioskEventHandler::get()->change_status(KioskStatus::IDLE);
            std::cout << "Back to single box, status IDLE" << std::endl;
            seen_box_ids.erase(candidate_id);
        }

        if (!seen_box_ids.contains(candidate_id) && ShelfDatabase::get_box_currently_being_processed() == -1) {
            std::cout << "Single new box detected: " << candidate_id << std::endl;
            seen_box_ids.insert(candidate_id);
            ShelfDatabase::set_box_currently_being_processed(candidate_id);
            KioskEventHandler::get()->on_box_entered(candidate_id);
            currently_processing_last_seen_time = current_time;
        }

    } else {
        // no unregistered boxes visible
        if (KioskEventHandler::get()->get_status() == KioskStatus::MULTIPLE_BOXES) {
            std::cout << "No boxes detected, changing status to IDLE" << std::endl;
            KioskEventHandler::get()->change_status(KioskStatus::IDLE);
        }
        // clear seen id so new box is detected next time
        seen_box_ids.clear();
    }

    // expiration check
    if (currently_processing_last_seen_time + EXPIRATION_TIME < current_time &&
        ShelfDatabase::get_box_currently_being_processed() != -1) {
        int expired_box_id = ShelfDatabase::get_box_currently_being_processed();
        std::cout << "Box " << expired_box_id << " processing expired due to timeout" << std::endl;
        KioskEventHandler::get()->on_box_exited(expired_box_id);
        seen_box_ids.erase(expired_box_id);
        ShelfDatabase::set_box_currently_being_processed(-1);
        currently_processing_last_seen_time = std::numeric_limits<double>::max();
    }
}

void ShelfRecognition::shutdown() {

}