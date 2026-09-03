//
// Created by ymod1 on 02/06/2026.
//

#include "LuaUtils.hpp"

lua_State* LuaUtils::InitLua() {
    std::cout << "[C++] LUA virtual machine init..." << std::endl;

    lua_State* lua_state = luaL_newstate();

    if (!lua_state) {
        std::cerr << "[Error] Impossible to create new Lua state!" << std::endl;
        return nullptr;
    }

    luaL_openlibs(lua_state);

    // Wrappa lo stato raw con sol2 per poter registrare tipi/funzioni
    sol::state_view lua(lua_state);
    RegisterVector2D(lua);   // vedi sotto

    return lua_state;
}
//------------------------------------------------------------------------------------------------------------------------

void LuaUtils::CloseLua(lua_State* in_lua_state) {

    if (!in_lua_state) {
        std::cerr << "[Lua Error] INVALID Lua state!" << std::endl;
        return;
    }

    lua_close(in_lua_state);

    std::cout << "[C++] Lua Closed." << std::endl;
}
//------------------------------------------------------------------------------------------------------------------------

void LuaUtils::RegisterVector2D(sol::state_view& lua) {
    lua.new_usertype<Vector2D>("Vector2D",
        sol::constructors<Vector2D(), Vector2D(float, float)>(),

        "x", &Vector2D::x,
        "y", &Vector2D::y,

        "length", &Vector2D::length,
        "normalize", &Vector2D::normalize,

        sol::meta_function::addition, &Vector2D::operator+,
        sol::meta_function::subtraction, [](const Vector2D& a, const Vector2D& b) { return a - b; },
        sol::meta_function::multiplication, [](const Vector2D& v, float scalar) { return v * scalar; },

        sol::meta_function::to_string, [](const Vector2D& v) {
            return "Vector2D(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
        }
    );
}
//------------------------------------------------------------------------------------------------------------------------

void LuaUtils::LoadLuaConfig(lua_State* in_lua_state) {

    if (!in_lua_state) {
        std::cerr << "[Lua Error] INVALID Lua state!" << std::endl;
        return;
    }

    std::string config_filename = env::scripts_folder + "Config.lua";

    if (luaL_dofile(in_lua_state, config_filename.c_str()) != LUA_OK) {
        std::cerr << "[Lua Error] Impossible to load file: "
                  << lua_tostring(in_lua_state, -1) << std::endl;
        lua_close(in_lua_state);
        return;
    }

    // DEBUG
    GetLuaVariable<bool>(in_lua_state, "display_lua_debug_messages", env::display_lua_debug_messages);
    GetLuaVariable<bool>(in_lua_state, "is_text_debug", env::is_text_debug);
    GetLuaVariable<bool>(in_lua_state, "is_input_text_debug", env::is_input_text_debug);
    GetLuaVariable<bool>(in_lua_state, "display_stats", env::display_stats);
    GetLuaVariable<float>(in_lua_state, "stats_display_interval", env::stats_display_interval);

    // WINDOW
    GetLuaVariable<std::string>(in_lua_state, "window_title", env::window_title);
    GetLuaVariable<int>(in_lua_state, "screen_width", env::screen_width);
    GetLuaVariable<int>(in_lua_state, "screen_height", env::screen_height);
    GetLuaVariable<bool>(in_lua_state, "is_fullscreen", env::is_fullscreen);

    // CAMERA
    GetLuaVariable<Vector2D>(in_lua_state, "map_size", env::map_size);
    GetLuaVariable<Vector2D>(in_lua_state, "camera_pos", env::camera_pos);

    // FOLDERS
    GetLuaVariable<std::string>(in_lua_state, "sprites_folder", env::sprites_folder);

    // COLLISIONS
    GetLuaVariable<Uint8>(in_lua_state, "cell_size", Collisions::cell_size);
    GetLuaVariable<Uint8>(in_lua_state, "alpha_threshold", Collisions::alpha_threshold);

}
//------------------------------------------------------------------------------------------------------------------------