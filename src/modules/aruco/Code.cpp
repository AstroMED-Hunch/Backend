#include "Code.hpp"
#include <algorithm>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <ostream>


std::unordered_map<int, ArucoMarker*> ArucoMarker::registered_markers;

void ArucoMarker::set_code_id(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    code_id = id;
}

ArucoMarker::ArucoMarker(int new_id) : code_id(new_id) {
    registered_markers[code_id] = this;
    std::cout << "Registered marker with ID: " << code_id << std::endl;
    
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
