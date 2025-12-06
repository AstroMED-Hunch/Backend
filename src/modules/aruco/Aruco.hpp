//
// Created by user on 12/6/25.
//

#pragma once
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_dictionary.hpp>

#include "Module.hpp"
#include "Module_Registry.hpp"

class Aruco : public Module {
public:
    virtual ~Aruco() = default;

    [[nodiscard]] std::string get_module_name() const override;
    void initialize() override;
    void run() override;
    void shutdown() override;
protected:
    cv::VideoCapture* cap;
    cv::Ptr<cv::aruco::DetectorParameters> params;
    cv::Ptr<cv::aruco::Dictionary> dictionary = nullptr;

};

MAKE_MODULE(Aruco, "aruco");