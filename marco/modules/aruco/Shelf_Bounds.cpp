#include "Shelf_Bounds.hpp"

ShelfBounds::ShelfBounds(std::string shelf_tag, std::array<std::unique_ptr<ArucoMarker>,4> markers) : shelf_tag(std::move(shelf_tag)), markers(std::move(markers)) {}

ShelfBounds::~ShelfBounds() = default;

bool ShelfBounds::check_square_inside_square(Rect rect1, Rect rect2) {
    return (rect1.x >= rect2.x) &&
           (rect1.y >= rect2.y) &&
           (rect1.x + rect1.width <= rect2.x + rect2.width) &&
           (rect1.y + rect1.height <= rect2.y + rect2.height);
}