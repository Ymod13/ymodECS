//
// Created by ymod1 on 12/05/2026.
//

#ifndef YMODECS_ENVIRONMENTS_HPP
#define YMODECS_ENVIRONMENTS_HPP

#include <string>
#include <SDL3/SDL_render.h>
#include <unordered_map>

#include "lua.h"
#include "MathUtils.hpp"
#include "RmlRenderInterface.hpp"
#include "RmlSystemInterface.hpp"
#include "RmlUi/Core/Context.h"

using namespace MathUtils;
namespace ecs {
    using EntityID    = std::uint32_t;
}

namespace env
{
    inline std::string sprites_folder = "Sprites/";
    inline std::string scripts_folder = "Scripts/";

    inline std::string window_title = "Ymod ECS";
    inline int screen_width = 1600;
    inline int screen_height = 1200;
    inline bool is_fullscreen = false;

    inline Vector2D map_size = Vector2D(10000, 10000);
    inline Vector2D camera_start_pos = Vector2D(0, 0); // screen represents camera view. this pos represents top-left corner of the camera view
    inline float map_movement_boundaries = 0.5f; // percentage of the screen used to move the map.
    inline float camera_deceleration = 0.25f;
    inline float camera_max_speed = 0.5f;
    inline float camera_acceleration = 2000.0f;

    inline bool is_text_debug = false;
    inline bool is_input_text_debug = false;
    inline bool display_lua_debug_messages = false;
    inline bool display_stats = true;
    inline float stats_display_interval = 0.5f;
    inline bool show_imgui = false;

    inline Vector2D player_pos;
    inline Vector2D player_screen_pos;
    inline float player_max_speed;
    inline float player_acceleration;
    inline float player_deceleration;
    inline ecs::EntityID player_id;

    struct Camera {
        Vector2D pos;
        Vector2D old_pos;
        Vector2D delta_pos;
        Vector2D current_pos;
        float movement_bounds = 100.0f;

        Vector2D x_axis_bounds;
        Vector2D y_axis_bounds;
        float acceleration = 2000.0f;
        float deceleration = 2000.0f;
        float max_speed = 1800.0f;

        Vector2D follow_velocity;
    };

    struct Stats {
        float stats_timer = 0.0f;
        float fps = 0.0f;
        Vector2D mouse_screen_pos;
        Vector2D camera_position;

        void UpdateStats( SDL_Renderer* renderer, const float& dt) {
            if (env::display_stats) {

                stats_timer +=dt;
                if (stats_timer>=env::stats_display_interval) {
                    stats_timer = 0.0f;
                    fps = 1.0f / dt;
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_SetRenderScale(renderer, 2.0f, 2.0f);
                SDL_RenderDebugTextFormat(renderer, 5.0f, 5.0f, "FPS: %.1f", fps);
                SDL_RenderDebugTextFormat(renderer, 5.0f, 15.0f, "Mouse: x: %.0f; y: %.0f", mouse_screen_pos.x,  mouse_screen_pos.y);
                SDL_RenderDebugTextFormat(renderer, 5.0f, 25.0f, "Camera: x: %.0f; y: %.0f", camera_position.x,  camera_position.y);
                SDL_SetRenderScale(renderer, 1.0f, 1.0f);
            }
        }
    };

    struct LUAContext {
        lua_State* lua_state;
    };


    struct SDLContext {
        SDL_Window*   window   = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Gamepad* gamepad = nullptr;

        int num_gamepads = 0;
        SDL_JoystickID* gamapads_ids = nullptr;

        SDLContext(SDL_Window* w, SDL_Renderer* r)
            : window(w), renderer(r) {}

        SDLContext(SDLContext&&) = default;
        SDLContext& operator=(SDLContext&&) = default;
        SDLContext(const SDLContext&) = delete;
        SDLContext& operator=(const SDLContext&) = delete;

        void InitGamepad(SDL_Gamepad* new_gamepad) {
            gamepad = new_gamepad;
        }
    };

    struct RmlUIContext {
        Rml::Context* context = nullptr;
        RmlRenderInterface* renderInterface = nullptr;
        RmlSystemInterface* systemInterface = nullptr;
    };

    struct InputState {
        const bool* keys = nullptr;   // SDL keyboard state pointer
        std::unordered_map<SDL_Keycode, double> pressed_keys;
        bool quit        = false;
        SDL_Event event;

        bool is_down(SDL_Scancode key) const {
            return keys && keys[key];
        }
    };

    enum BulletType {
        NONE,
        PISTOL,
        SHOTGUN,
        ROCKET,
        GRENADE
    };


}

namespace Collisions {

    inline Uint8 cell_size = 32;
    inline Uint8 alpha_threshold = 20;

    enum CollisionType {
        NONE,
        RADIUS,
        RECTANGLE,
        MULTI_CIRCLE
    };

    // A collision circle expressed in LOCAL coordinates relative to the sprite
    struct LocalCircle {
        Vector2D local_center; // distance from sprite center
        float radius;
    };


    struct WorldCircle {
        Vector2D  center;
        float radius;
    };
}

namespace UserInterface {

    inline ecs::EntityID MouseCursorId=ecs::EntityID();

    enum LayerType : uint16_t {
        NONE = 0,
        BACKGROUND = 100,
        WORLD_STATIC = 200,
        WORLD_DYNAMIC = 300,
        FOREGROUND = 400,
        UI = 500
    };
}

#endif //YMODECS_ENVIRONMENTS_HPP
