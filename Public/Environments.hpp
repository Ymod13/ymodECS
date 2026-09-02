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

    inline bool is_text_debug = false;
    inline bool is_input_text_debug = false;
    inline bool display_lua_debug_messages = false;
    inline bool display_stats = true;
    inline float stats_display_interval = 0.5f;

    inline Vector2D player_pos;
    inline ecs::EntityID player_id;


    struct Stats {
        float stats_timer = 0.0f;
        float fps = 0.0f;
        Vector2D mouse_screen_pos;

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


#endif //YMODECS_ENVIRONMENTS_HPP
