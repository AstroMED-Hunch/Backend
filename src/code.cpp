#include "code.hpp"

std::vector<std::shared_ptr<ArucoMarker>> Markers::registered_markers = std::vector<std::shared_ptr<ArucoMarker>>();

void ArucoMarker::set_code_id(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    code_id = id;
}

ArucoMarker::ArucoMarker(std::weak_ptr<CodeGroup> group) : code_group(group) {
    Markers::registered_markers.push_back(shared_from_this());
}

int ArucoMarker::get_code_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return code_id;
}
