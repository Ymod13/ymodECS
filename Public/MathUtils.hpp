//
// Created by ymod1 on 01/06/2026.
//

#ifndef YMODECS_MATHUTILS_HPP
#define YMODECS_MATHUTILS_HPP

#include <pxr/base/gf/vec2f.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MathUtils {
    struct Vector2D : public SDL_FPoint {
        // Constructors
        Vector2D(float _x = 0.0f, float _y = 0.0f) { x = _x; y = _y; }
        Vector2D(GfVec2f in_vect) { x = in_vect[0]; y = in_vect[1]; }

        // Sum
        Vector2D operator+(const Vector2D& v) const { return Vector2D(x + v.x, y + v.y); }
        Vector2D& operator+=(const Vector2D& v) { x += v.x; y += v.y; return *this; }

        Vector2D operator-(const Vector2D& v) const { return Vector2D(x - v.x, y - v.y); }
        Vector2D& operator-=(const Vector2D& v) { x -= v.x; y -= v.y; return *this; }

        bool operator!=(const Vector2D& v) { return (x!=v.x || y!=v.y); }

        // Scalar Mul
        Vector2D operator*(float scalar) const { return Vector2D(x * scalar, y * scalar); }

        Vector2D& operator=(const GfVec2f& in_vect) {
            x = in_vect[0];
            y = in_vect[1];
            return *this;
        }

        // Vector lenght
        float length() const { return std::sqrt(x * x + y * y); }

        void normalize() {
            float len = length();
            if (len>0.0f) {
                x /= len;
                y /= len;
            }
            else {
                x = 0.0f;
                y = 0.0f;
            }

        };
    };

    static Vector2D SmoothMoveTowards(const Vector2D& current, const Vector2D& target, float smoothing, float deltaTime)
    {
        float t = 1.0f - std::exp(-smoothing * deltaTime);

        return {
            current.x + (target.x - current.x) * t,
            current.y + (target.y - current.y) * t
        };
    }

    static Vector2D AccelerateTowards (
        const Vector2D& current,
        const Vector2D& target,
        Vector2D& velocity,        // persistent state
        float acceleration,        // ex. 2000.0f
        float deceleration,        // ex. 2500.0f
        float max_speed,           // ex. 1800.0f
        float deltaTime)
    {
        Vector2D delta = target - current;
        float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        if (distance < 0.5f) {
            velocity = Vector2D{0.0f, 0.0f};
            return target;
        }

        Vector2D direction = { delta.x / distance, delta.y / distance };

        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

        // distance needed to stop at current speed, given deceleration
        float stopping_distance = (speed * speed) / (2.0f * deceleration);

        if (distance <= stopping_distance) {
            // deceleration stage: slows down until it reaches the target
            speed = std::max(0.0f, speed - deceleration * deltaTime);
        } else {
            // acceleration/constant speed stage
            speed = std::min(max_speed, speed + acceleration * deltaTime);
        }

        velocity.x = direction.x * speed;
        velocity.y = direction.y * speed;

        float step = speed * deltaTime;
        if (step >= distance) {
            velocity = Vector2D{0.0f, 0.0f};
            return target;
        }

        return {
            current.x + velocity.x * deltaTime,
            current.y + velocity.y * deltaTime
        };
    }
}

#endif //YMODECS_MATHUTILS_HPP