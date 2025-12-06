//
// Created by user on 12/6/25.
//

#include "Aruco.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

#include "Code.hpp"
#include "Vec.hpp"

std::string Aruco::get_module_name() const {
    return "Aruco";
}

void Aruco::initialize() {
    const std::string camera_index_str = layout->get_config_value("camera_index");
    const int camera_index = std::stoi(camera_index_str);

    cap = new cv::VideoCapture(camera_index);
    if (!cap->isOpened()) {
        throw std::runtime_error("Could not open video capture device: " + std::to_string(camera_index));
    }

    params = cv::makePtr<cv::aruco::DetectorParameters>();
    dictionary = cv::makePtr<cv::aruco::Dictionary>(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_7X7_1000));
    cv::namedWindow("NASA Hunch Debug Window");
}

void Aruco::run() {
    cv::Mat frame;
    cap->read(frame);

    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;

    cv::aruco::ArucoDetector detector(*dictionary, *params);
    detector.detectMarkers(frame, marker_corners, marker_ids);

    for (int marker_id : marker_ids) {
        std::cout << "Detected marker ID: " << marker_id << std::endl;
    }

    for (auto& marker : ArucoMarker::registered_markers) {
    auto index = std::find(marker_ids.begin(), marker_ids.end(), marker.first);
    if (index != marker_ids.end()) {
            size_t idx = std::distance(marker_ids.begin(), index);
            std::vector<cv::Point2f> corners = marker_corners[idx];
            float center_x = 0.0f;
            float center_y = 0.0f;
            for (const auto& corner : corners) {
                center_x += corner.x;
                center_y += corner.y;
            }
            center_x /= static_cast<float>(corners.size());
            center_y /= static_cast<float>(corners.size());
            marker.second->position = Vec2(center_x, center_y);
            marker.second->is_visible = true;

            cv::circle(frame, cv::Point(static_cast<int>(center_x), static_cast<int>(center_y)), 5, cv::Scalar(0, 255, 0), 20);

            std::printf("Marker %d detected at position (%.2f, %.2f)\n", marker.first, center_x, center_y);
        } else {
            marker.second->is_visible = false;
        }
    }

    cv::imshow("NASA Hunch Debug Window", frame);
}

void Aruco::shutdown() {
}
