#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <print>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include "mlayout/lexer.hpp"

#include "code.hpp"

constexpr int NUM_MARKERS = 1000;

void read_markers() {
    for (auto& marker: Markers::registered_markers) {
        std::printf("%d\n", marker->get_code_id());
    }
}

int main() {
    read_markers();
    MLayout::lex_string("example input");
    return 0;
}