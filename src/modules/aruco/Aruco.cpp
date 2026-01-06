//
// Created by user on 12/6/25.
//

#include "Aruco.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <algorithm>

#include "Code.hpp"
#include "Vec.hpp"

std::string Aruco::get_module_name() const {
    return "Aruco";
}

void Aruco::initialize() {
    params = cv::makePtr<cv::aruco::DetectorParameters>();
    dictionary = cv::makePtr<cv::aruco::Dictionary>(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_7X7_1000));
    cv::namedWindow("NASA Hunch Debug Window");
}

void Aruco::run(cv::Mat cap) {

    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;

    cv::aruco::ArucoDetector detector(*dictionary, *params);
    detector.detectMarkers(cap, marker_corners, marker_ids);

    cv::Mat display;
    if (cap.channels() == 1) {
        cv::cvtColor(cap, display, cv::COLOR_GRAY2BGR);
    } else {
        display = cap.clone();
    }

    if (!marker_ids.empty()) {
        cv::aruco::drawDetectedMarkers(display, marker_corners, marker_ids);
    }

    for (int marker_id : marker_ids) {
        std::cout << "Detected marker ID: " << marker_id << std::endl;
    }

    for (auto& marker : ArucoMarker::registered_markers) {
    auto index = std::find(marker_ids.begin(), marker_ids.end(), marker.first);
    if (index != marker_ids.end()) {
            size_t idx = std::distance(marker_ids.begin(), index);
            if (idx >= marker_corners.size()) {
                std::cerr << "Warning: marker index " << idx << " out of marker_corners range\n";
                continue;
            }
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

            if (center_x >= 0.0f && center_x < static_cast<float>(display.cols) && center_y >= 0.0f && center_y < static_cast<float>(display.rows)) {
                std::vector<cv::Point> poly;
                poly.reserve(corners.size());
                for (const auto& c : corners) poly.emplace_back(static_cast<int>(c.x), static_cast<int>(c.y));
                const std::vector<std::vector<cv::Point>> polys = { poly };
                cv::polylines(display, polys, true, cv::Scalar(255, 0, 0), 2);

                cv::circle(display, cv::Point(static_cast<int>(center_x), static_cast<int>(center_y)), 5, cv::Scalar(0, 255, 0), -1);
            } else {
                std::cerr << "Warning: computed marker center out of image bounds for marker " << marker.first << " (" << center_x << ", " << center_y << ")\n";
            }

            std::printf("Marker %d detected at position (%.2f, %.2f)\n", marker.first, center_x, center_y);
        } else {
            marker.second->is_visible = false;
        }
    }

    cv::imshow("NASA Hunch Debug Window", display);
    cv::waitKey(1);
}

void Aruco::shutdown() {
}
