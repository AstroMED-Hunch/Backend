#include "code.hpp"

void ArucoMarker::set_code_id(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    code_id = id;
}

ArucoMarker::ArucoMarker() {
    
}

int ArucoMarker::get_code_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return code_id;
}

std::shared_ptr<CodeGroup> ArucoMarker::get_code_group() const {
    return code_group.lock();
}
