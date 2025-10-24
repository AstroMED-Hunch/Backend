#include "box.hpp"

Box::Box(std::string box_tag, int id)
    : box_tag(std::move(box_tag)), box_id(id) {}

Box::~Box() = default;

void Box::compute_position(std::array<Vec2, 4> code_positions) {
    std::lock_guard<std::mutex> lock(mutex);
    position = (code_positions[0] + code_positions[1] + code_positions[2] + code_positions[3]) / 4.0f;
}

