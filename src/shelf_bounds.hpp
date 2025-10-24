#pragma once

#include "code.hpp"
#include <mutex>
#include <string>
#include <vector>

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

class ShelfBounds {
    public:
        ShelfBounds(std::string shelf_tag, std::array<std::unique_ptr<ArucoMarker>,4> markers);
        ~ShelfBounds();
        std::array<std::unique_ptr<ArucoMarker>,4> markers;
        bool check_square_inside_square(Rect rect1, Rect rect2) const;

    protected:
        std::string shelf_tag;

        bool is_square_inside_square(Rect rect1, Rect rect2) const;

        mutable std::mutex mutex;
};