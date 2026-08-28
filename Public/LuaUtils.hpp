//
// Created by ymod1 on 02/06/2026.
//

#ifndef YMODECS_LUAUTILS_HPP
#define YMODECS_LUAUTILS_HPP

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <iostream>

#include "Environments.hpp"

namespace LuaUtils {
    lua_State* InitLua();
    void CloseLua(lua_State* in_lua_state);

    template <typename T>
    void GetLuaVariable(lua_State* in_lua_state, const char* in_variable_name, T& out_variable) {

        if (!in_lua_state) {
            std::cerr << "[Lua Error] GetLuaVariable: INVALID Lua state!" << std::endl;
            return;
        }

        lua_getglobal(in_lua_state, in_variable_name);

        if constexpr (std::is_same_v<T, std::string>) {
            if (lua_isstring(in_lua_state, -1)) {

                out_variable = lua_tostring(in_lua_state, -1);

                if (env::display_lua_debug_messages) {
                    std::cout << "[Lua] GetLuaVariable: " << in_variable_name << "(string): "<< out_variable  << std::endl;
                }
            }

            else
                std::cerr << "[Lua] GetLuaVariable: " << in_variable_name << " is not a stringa\n";
        }
        else if constexpr (std::is_same_v<T, int>|| std::is_same_v<T, Uint8>) {
            if (lua_isinteger(in_lua_state, -1)) {

                out_variable = static_cast<int>(lua_tointeger(in_lua_state, -1));

                if (env::display_lua_debug_messages) {
                    std::cout << "[Lua] GetLuaVariable: " << in_variable_name << "(int): "<< std::to_string(out_variable) << std::endl;
                }
            }
            else
                std::cerr << "[Lua] GetLuaVariable: " << in_variable_name << " is not an integer\n";
        }
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            if (lua_isnumber(in_lua_state, -1)) {

                out_variable = static_cast<T>(lua_tonumber(in_lua_state, -1));

                if (env::display_lua_debug_messages) {
                    std::cout << "[Lua] GetLuaVariable: " << in_variable_name << "(float/double): "<< out_variable  << std::endl;
                }
            }
            else
                std::cerr << "[Lua] GetLuaVariable: " << in_variable_name << " is not a double/float\n";
        }
        else if constexpr (std::is_same_v<T, bool>) {
            if (lua_isboolean(in_lua_state, -1)) {

                out_variable = lua_toboolean(in_lua_state, -1);

                if (env::display_lua_debug_messages) {
                    std::cout << "[Lua] GetLuaVariable: " << in_variable_name << "(bool): "<< (out_variable ? "TRUE" : "FALSE")  << std::endl;
                }

            }
            else
                std::cerr << "[Lua] GetLuaVariable: " << in_variable_name << " is not a boolean\n";
        }
        else {
            std::cerr << "[Lua] GetLuaVariable: Type '" << type_name<T>() << "' not supported for '" << in_variable_name << "'\n";
            static_assert(false, "[Lua] GetLuaVariable: type not supported!'");
        }

        lua_pop(in_lua_state, 1); // clears the stack removing the top element
    };

    void LoadLuaConfig(lua_State* in_lua_state);
}
#endif //YMODECS_LUAUTILS_HPP
