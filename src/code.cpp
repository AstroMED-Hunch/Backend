#include "code.hpp"
#include <algorithm>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/opencv.hpp>
#include <unordered_map>

cv::VideoCapture* cap;
cv::Ptr<cv::aruco::DetectorParameters> params;
cv::Ptr<cv::aruco::Dictionary> dictionary = nullptr;
std::unordered_map<int, ArucoMarker*> ArucoMarker::registered_markers;

void init_cv(int camera_index) {
    cap = new cv::VideoCapture(camera_index);
    if (!cap->isOpened()) {
        throw std::runtime_error("Could not open video capture device: " + std::to_string(camera_index));
    }

    params = cv::makePtr<cv::aruco::DetectorParameters>();
    dictionary = cv::makePtr<cv::aruco::Dictionary>(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_7X7_1000));
    cv::namedWindow("NASA Hunch Debug Window");
}

void identify_and_set_positions() {
    cv::Mat frame;
    cap->read(frame);

    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;

    cv::aruco::ArucoDetector detector(*dictionary, *params);
    detector.detectMarkers(frame, marker_corners, marker_ids);

    for (int i = 0; i < marker_ids.size(); ++i) {
        std::println("Detected marker ID: {}", marker_ids[i]);
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
            center_x /= corners.size();
            center_y /= corners.size();
            marker.second->position = Vec2(center_x, center_y);
            marker.second->is_visible = true;

            cv::circle(frame, cv::Point(center_x, center_y), 5, cv::Scalar(0, 255, 0), 20);

            std::printf("Marker %d detected at position (%.2f, %.2f)\n", marker.first, center_x, center_y);
        } else {
            marker.second->is_visible = false;
        }
    }

    cv::imshow("NASA Hunch Debug Window", frame);
}


void ArucoMarker::set_code_id(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    code_id = id;
}

ArucoMarker::ArucoMarker(int new_id) : code_id(new_id) {
    registered_markers[code_id] = this;
    std::println("Registered marker ID: {}", code_id);
    
}

ArucoMarker::~ArucoMarker() {
    registered_markers.erase(code_id);
}

int ArucoMarker::get_code_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return code_id;
}

std::shared_ptr<CodeGroup> ArucoMarker::get_code_group() const {
    return code_group.lock();
}
