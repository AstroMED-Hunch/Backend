//
// Created by Marco Stulic on 1/7/26.
//

#pragma once
#include <limits>
#include <map>
#include <unordered_set>
#include <opencv2/core/utility.hpp>

#include "Module.hpp"
#include "Module_Registry.hpp"

constexpr double EXPIRATION_TIME = 2.0;

class ShelfRecognition : public Module {
public:
    std::string get_module_name() const override;

    void initialize() override;

    void run(cv::Mat cap) override;

    void shutdown() override;
protected:
    std::unordered_map<int, std::string> marker_shelf_mappings;
    std::unordered_map<int, double> marker_last_found_time;
    double currently_processing_last_seen_time = std::numeric_limits<double>::max();
    std::unordered_set<int> seen_box_ids;
};

MAKE_MODULE(ShelfRecognition, "shelf_recognition");