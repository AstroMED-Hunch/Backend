//
// Created by Marco Stulic on 1/7/26.
//

#include "GeomUtils.hpp"

#include <vector>

#include "Vec.hpp"

bool is_point_in_polygon(const Vec2& point, const std::vector<Vec2>& polygon) {
    int n = polygon.size();
    if (n < 3) return false; // must be 2d

    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((polygon[i].y > point.y) != (polygon[j].y > point.y)) &&
            (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}
