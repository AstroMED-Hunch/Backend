//
// Created by Marco Stulic on 1/7/26.
//

#pragma once
#include <map>

#include "Module.hpp"
#include "Module_Registry.hpp"

class ShelfRecognition : public Module {
public:
    std::string get_module_name() const override;

    void initialize() override;

    void run(cv::Mat cap) override;

    void shutdown() override;
protected:
    std::unordered_map<int, std::string> marker_shelf_mappings; // what shelf each box is on
};

MAKE_MODULE(ShelfRecognition, "shelf_recognition");