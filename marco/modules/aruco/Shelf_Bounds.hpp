#pragma once

#include "Code.hpp"
#include <mutex>
#include <string>
#include <array>
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
        static bool check_square_inside_square(Rect rect1, Rect rect2);

    protected:
        std::string shelf_tag;

        mutable std::mutex mutex;
};