#pragma once

#include "code.hpp"
#include <mutex>
#include <string>
#include <vector>
#include "vec.hpp"

class Box {
    public:
        Box(std::string box_tag, int id);
        ~Box();

        const int box_id;

        void compute_position(std::array<Vec2, 4> code_positions);

    protected:
        std::string box_tag;

        Vec2 position;

        mutable std::mutex mutex;
};