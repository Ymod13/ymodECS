cmake_minimum_required(VERSION 3.30)

set(CMAKE_TOOLCHAIN_FILE "D:/Stuff/Projects/vcpkg/vcpkg-master/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(ymodECS CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(USD_ROOT "D:/Stuff/Projects/USD_Debug")
else()
    set(USD_ROOT "D:/Stuff/Projects/USD_Release")
endif()

set(pxr_DIR "${USD_ROOT}")

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(pxr REQUIRED)

add_executable(ymodECS main.cpp
        Public/ecs.hpp
        Public/Systems.hpp
        Public/ComponetsDefinitions.hpp
        Public/AccessSystem.hpp
        Public/Environments.hpp
        Public/Scheduler.hpp
        Public/Utilities.hpp
        Private/Utilities.cpp
        Public/UsdWrapper.hpp
        Private/UsdWrapper.cpp
        Public/MathUtils.hpp
        Public/LuaUtils.hpp
        Private/LuaUtils.cpp
        Public/EventsListener.hpp
        Private/EventsListener.cpp)

target_include_directories(ymodECS SYSTEM PRIVATE
        ${PXR_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/include"
)

target_include_directories(ymodECS PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        "D:/Stuff/Projects/sol2-develop/include"
)

target_compile_definitions(ymodECS PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        __TBB_NO_IMPLICIT_LINKAGE=1
        __TBBMALLOC_NO_IMPLICIT_LINKAGE=1
        TBB_SUPPRESS_DEPRECATED_MESSAGES=1
)

target_link_libraries(ymodECS PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        Boost::system
        Boost::filesystem
        ${USD_ROOT}/lib/usd_tf.lib
        ${USD_ROOT}/lib/usd_gf.lib
        ${USD_ROOT}/lib/usd_usd.lib
        ${USD_ROOT}/lib/usd_usdGeom.lib
        ${USD_ROOT}/lib/usd_sdf.lib
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/lib/lua54.lib"
)

# Copia DLL e plugin accanto all'eseguibile
if(WIN32)
    add_custom_command(TARGET ymodECS POST_BUILD
            # DLL from bin\ (TBB, MaterialX)
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/bin"
            $<TARGET_FILE_DIR:ymodECS>

            # DLL from lib\ (usd_*.dll)
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/lib"
            $<TARGET_FILE_DIR:ymodECS>

            # USD plugin (necessaries at runtime)
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/plugin/usd"
            "$<TARGET_FILE_DIR:ymodECS>/usd"

            # Resources directories copy
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Scripts
            $<TARGET_FILE_DIR:ymodECS>/Scripts
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Scenes
            $<TARGET_FILE_DIR:ymodECS>/Scenes
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Sprites
            $<TARGET_FILE_DIR:ymodECS>/Sprites
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Resources
            $<TARGET_FILE_DIR:ymodECS>/Resources
    )
endif()