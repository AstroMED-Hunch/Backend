//
// Created by Marco Stulic on 1/7/26.
//

#pragma once
#include <vector>

#include "Vec.hpp"

bool is_point_in_polygon(const Vec2& point, const std::vector<Vec2>& polygon);
