//
// Created by ymod1 on 13/05/2026.
//

#ifndef YMODECS_UTILITIES_HPP
#define YMODECS_UTILITIES_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <mutex>
#include <pxr/base/gf/vec2f.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>

#include "Environments.hpp"


PXR_NAMESPACE_USING_DIRECTIVE


namespace ecs {
    class World;
    using EntityID    = std::uint32_t;
    struct RenderableEntry;
}

namespace MathUtils {
    struct Vector2D;
}

using namespace MathUtils;

struct Sprite;
struct Size;
struct Position;
struct Name;
struct Visibility;
struct BackgroundTile;

namespace Utils {

    class FunctionsLib {
    public:
        // position and map
        static void UpdatePosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_map_limits = false);
        static void UpdateScreenPosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_screen = false);
        static void RestoreOldPosition(Position &pos, Sprite &sprite, const Vector2D &scale);
        static void Keyboard_vel_axis_movement(const SDL_Scancode dir_1_key, const SDL_Scancode dir_2_key, const bool* keys, float &vel, const float &acceleration,  const float &deceleration, const float &max_vel, const float &dt);
        static bool clamp_position_map(Vector2D &pos, const Vector2D &scale, const SDL_Texture *SpriteTexture);
        static Vector2D MapToWindow(const Vector2D &map_pos, const Vector2D &cam_pos);

        // Collisions
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

        // Z-Order
        static float ComputeDepth(const Position& position, const Sprite& sprite);
        static void InsertionSortByDepth(std::vector<ecs::RenderableEntry>& entries);
        static void DynamicZOrdering(ecs::World &world, std::vector<ecs::RenderableEntry>& entries, const bool &is_first_ordering);

        static bool LoadSprite(SDL_Renderer* renderer, const Size& size, const Position &pos, Sprite& out_sprite);
        static bool LoadBackgroundSprite(ecs::World &world, const ecs::EntityID &id, SDL_Renderer* renderer);

        static void SpawnBullet(ecs::World &world, const std::uint32_t owner_id, const env::BulletType bullet_type, const Vector2D &start_pos, const Vector2D &end_pos);

        static void DrawCircle(SDL_Renderer* renderer, const Vector2D &center, float radius, Uint8 r=255, Uint8 g=0, Uint8 b=0, Uint8 a=255);
        static void DrawRectangle(SDL_Renderer *renderer, const SDL_FRect &rect, Uint8 r=0, Uint8 g=255, Uint8 b=0, Uint8 a=255);
        static void DrawCirclesCluster(SDL_Renderer *renderer, const Sprite& obj, Uint8 r=0, Uint8 g=0, Uint8 b=255, Uint8 a=255);

        static void DrawSprite(SDL_Renderer* renderer, const Sprite& sprite, const Name* name);
        static bool IsSpriteOnScreen(const Sprite& sprite);

        template<typename EnumType>
        static std::string EnumToString(EnumType value) {
            if constexpr (std::is_same_v<EnumType, env::BulletType>) {
                if (value == env::PISTOL) return "PISTOL";
                if (value == env::SHOTGUN) return "SHOTGUN";
                if (value == env::ROCKET) return "ROCKET";
                if (value == env::GRENADE) return "GRENADE";
            }
            else if constexpr (std::is_same_v<EnumType, Collisions::CollisionType>) {
                if (value == Collisions::RADIUS) return "RADIUS";
                if (value == Collisions::RECTANGLE) return "RECTANGLE";
                if (value == Collisions::MULTI_CIRCLE) return "MULTI_CIRCLE";
                if (value == Collisions::NONE) return "NONE";
            }
            else if constexpr (std::is_same_v<EnumType, UserInterface::LayerType>) {
                if (value == UserInterface::NONE) return "NONE";
                if (value == UserInterface::BACKGROUND) return "BACKGROUND";
                if (value == UserInterface::WORLD_STATIC) return "WORLD_STATIC";
                if (value == UserInterface::WORLD_DYNAMIC) return "WORLD_DYNAMIC";
                if (value == UserInterface::FOREGROUND) return "FOREGROUND";
                if (value == UserInterface::UI) return "UI";
            }
            else if constexpr (std::is_same_v<EnumType, UserInterface::TilingType>) {
                if (value == UserInterface::REPEAT) return "REPEAT";
                if (value == UserInterface::FILL) return "FILL";
                if (value == UserInterface::TILESET) return "TILESET";
                if (value == UserInterface::SINGLE) return "SINGLE";
            }

            return "NOT_RECOGNIZED";
        }
    };

    struct FrameProfiler {
        struct Entry { double total_ms = 0.0; int count = 0; };
        std::map<std::string, Entry> entries;
        std::mutex mutex;
        Uint64 last_print = SDL_GetPerformanceCounter();

        static FrameProfiler& Get() {
            static FrameProfiler instance;   // una sola istanza per tutto il programma
            return instance;
        }

        void Add(const char* name, Uint64 start) {
            const double ms = (SDL_GetPerformanceCounter() - start) * 1000.0 / SDL_GetPerformanceFrequency();
            std::lock_guard<std::mutex> lock(mutex);
            auto& e = entries[name];
            e.total_ms += ms;
            ++e.count;
        }

        void PrintEverySecond() {
            std::lock_guard<std::mutex> lock(mutex);
            const double elapsed = (SDL_GetPerformanceCounter() - last_print) / (double)SDL_GetPerformanceFrequency();
            if (elapsed < 1.0) return;
            for (auto& [name, e] : entries)
                std::cout << name << ": " << e.total_ms / e.count << " ms (x" << e.count << ")\n";
            std::cout << "---- entries: " << entries.size() << "\n";
            entries.clear();
            last_print = SDL_GetPerformanceCounter();
        }
    };

    struct ScopedTimer {
        const char* name;
        Uint64 start;
        bool enabled;

        explicit ScopedTimer(const char* n, bool en = true)
            : name(n), start(en ? SDL_GetPerformanceCounter() : 0), enabled(en) {}

        ~ScopedTimer() {
            if (enabled) FrameProfiler::Get().Add(name, start);
        }
    };

    #define PROFILE_CONCAT_(a, b) a##b
    #define PROFILE_CONCAT(a, b)  PROFILE_CONCAT_(a, b)
    #define PROFILE_SCOPE(name) \
    Utils::ScopedTimer PROFILE_CONCAT(_prof_, __LINE__)(name, env::use_frame_profiler)
}

//void y_axis_movement();

#endif //YMODECS_UTILITIES_HPP
