//
// Created by ymod1 on 13/05/2026.
//

#ifndef YMODECS_UTILITIES_HPP
#define YMODECS_UTILITIES_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include <pxr/base/gf/vec2f.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>

#include "Environments.hpp"


PXR_NAMESPACE_USING_DIRECTIVE


namespace ecs {
    class World;
    using EntityID    = std::uint32_t;
}

namespace MathUtils {
    struct Vector2D;
}

using namespace MathUtils;

struct Sprite;
struct Size;
struct Position;

namespace Utils {

    class FunctionsLib {
    public:
        static void UpdatePosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_screen = true);
        static void RestoreOldPosition(Position &pos, Sprite &sprite, const Vector2D &scale);
        static void Keyboard_vel_axis_movement(const SDL_Scancode dir_1_key, const SDL_Scancode dir_2_key, const bool* keys, float &vel, const float &acceleration, const float &max_vel, const float &dt);
        static bool clamp_screen_position(Vector2D &pos, const Vector2D &scale, const SDL_Texture *SpriteTexture);
        static bool check_radius_collision(const float &radius_a, const float &radius_b, const Vector2D &center_a, const Vector2D &center_b, Vector2D &OutPushVector);
        static bool check_radius_rectangle_collision(const float &radius_a, const Vector2D &center_a, const Sprite& sprite_b_rect, Vector2D &OutPushVector);
        static bool check_radius_multicircle_collision(const float &radius_a, const Vector2D &center_a, const std::vector<Collisions::WorldCircle> &circles, Vector2D &OutPushVector);
        static bool check_multicircle_collision( const std::vector<Collisions::WorldCircle> &circles_a, const std::vector<Collisions::WorldCircle> &circles_b, Vector2D &OutPushVector);
        static bool check_multicircle_rectangle_collision(const std::vector<Collisions::WorldCircle> &circles, const Sprite& sprite_b_rect, Vector2D &OutPushVector);
        static bool check_rectangle_collision(const Sprite& sprite_a_rect, const Sprite& sprite_b_rect, Vector2D &OutPushVector);
        static bool CheckPixelPerfectCollision(const Sprite& objA, const Sprite& objB, Uint8 alphaThreshold = 0);
        static float CalculateRectRadius(const SDL_FRect& rect);
        static void GenerateCircleCluster(Sprite& obj, int cellSize = Collisions::cell_size, Uint8 alphaThreshold = Collisions::alpha_threshold);
        static std::vector<Collisions::WorldCircle> GetWorldColliders(const Sprite& obj);

        static bool LoadSprite(SDL_Renderer* renderer, const Size& size, const Position &pos, Sprite& out_sprite);
        static void SpawnBullet(ecs::World &world, const std::uint32_t owner_id, const env::BulletType bullet_type, const Vector2D &start_pos, const Vector2D &end_pos);

        static void DrawCircle(SDL_Renderer* renderer, Vector2D &center, float radius, Uint8 r=255, Uint8 g=0, Uint8 b=0, Uint8 a=255);
        static void DrawRectangle(SDL_Renderer *renderer, const SDL_FRect &rect, Uint8 r=0, Uint8 g=255, Uint8 b=0, Uint8 a=255);
        static void DrawCirclesCluster(SDL_Renderer *renderer, const Sprite& obj, Uint8 r=0, Uint8 g=0, Uint8 b=255, Uint8 a=255);


    };
}

//void y_axis_movement();

#endif //YMODECS_UTILITIES_HPP
