#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <print>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "mlayout/parser.hpp"

#include "code.hpp"

constexpr int DEFAULT_CAM_INDEX = 1;

int main(int argc, char** argv) {
    int cam_index = DEFAULT_CAM_INDEX;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--camera" && i + 1 < argc) {
            cam_index = std::stoi(argv[++i]);
        }
    }

    init_cv(cam_index);

    auto layout = MLayout::parse_file("../extern/example.mlayout");

    for (const auto& code_group : layout->code_groups) {
        std::printf("Code Group: %s\n", code_group->tag.c_str());


        std::printf("%zu", code_group->markers.size());
        for (const auto& code : code_group->markers) {
            std::printf("  Code: %d\n", code->get_code_id());
        }

        for (const auto& layout : code_group->bound_to) {
            std::printf("  Bound to layout: %s\n", layout->tag.c_str());
        }
    }
    while (true) {
        identify_and_set_positions();

        if (cv::waitKey(30) >= 0) break;
    }

    return 0;
}