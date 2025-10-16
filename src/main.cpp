#include <print>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>

constexpr int NUM_MARKERS = 99;

int main() {
    std::filesystem::create_directory("markers");
    for (int i = 0; i < NUM_MARKERS; ++i) {
        cv::Mat markerImage;
        cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
        cv::aruco::generateImageMarker(dictionary, i, 200, markerImage, 1);
        std::string filename = "markers/marker" + std::to_string(i) + ".png";
        cv::imwrite(filename, markerImage);
    }
    return 0;
}