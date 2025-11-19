#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <print>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>

#include "mlayout/parser.hpp"

#include "code.hpp"

constexpr int NUM_MARKERS = 1000;

int main(int argc, char** argv) {
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
    return 0;
}