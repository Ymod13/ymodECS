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
}

#endif //YMODECS_MATHUTILS_HPP