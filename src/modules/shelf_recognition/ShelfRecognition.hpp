//
// Created by Marco Stulic on 1/7/26.
//

#pragma once
#include <map>
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
    std::unordered_map<int, std::string> marker_shelf_mappings; // what shelf each box is on
    std::unordered_map<int, double> marker_last_found_time;
};

MAKE_MODULE(ShelfRecognition, "shelf_recognition");