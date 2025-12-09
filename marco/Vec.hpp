#pragma once

class Vec3 {
    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
};

class Vec2 {
    public:
        float x = 0.0f;
        float y = 0.0f;

        Vec2 operator+(const Vec2& other) const {
            Vec2 result;
            result.x = this->x + other.x;
            result.y = this->y + other.y;
            return result;
        }

        Vec2 operator/(float scalar) const {
            Vec2 result;
            result.x = this->x / scalar;
            result.y = this->y / scalar;
            return result;
        }
};