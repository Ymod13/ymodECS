cmake_minimum_required(VERSION 3.30)

set(CMAKE_TOOLCHAIN_FILE "D:/Stuff/Projects/vcpkg/vcpkg-master/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Disattiva la copia automatica delle DLL vcpkg accanto all'exe: la gestiamo noi manualmente in bin/
set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)

project(ymodECS CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VCPKG_TARGET_TRIPLET "x64-windows") # Vcpkg capirà da solo di pescare da /debug/lib
    set(USD_ROOT "D:/Stuff/Projects/USD_Debug")
    set(VCPKG_BIN_DIR "D:/Stuff/Projects/vcpkg/vcpkg-master/installed/x64-windows/debug/bin")
    set(LUA_LIB "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/lib/Debug/lua54d.lib")

else()

    set(USD_ROOT "D:/Stuff/Projects/USD_Release")
    set(VCPKG_BIN_DIR "D:/Stuff/Projects/vcpkg/vcpkg-master/installed/x64-windows/bin")
    set(LUA_LIB "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/lib/Release/lua54.lib")

endif()

set(pxr_DIR "${USD_ROOT}")

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(pxr REQUIRED)

# --- Dear ImGui (sorgenti diretti, no vcpkg) ---
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui")

add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3)

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
        Private/EventsListener.cpp
        Public/EcsInspector.hpp)

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
        ${USD_ROOT}/lib/usd_plug.lib
        ${LUA_LIB}
        imgui
)

if(WIN32)
    set(RUNTIME_DIR $<TARGET_FILE_DIR:ymodECS>)
    set(BIN_DIR ${RUNTIME_DIR}/bin)
    set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)

    file(GLOB USD_BIN_DLLS "${USD_ROOT}/bin/*.dll")
    file(GLOB USD_LIB_DLLS "${USD_ROOT}/lib/*.dll")
    file(GLOB VCPKG_DLLS   "${VCPKG_BIN_DIR}/*.dll")

    set(ALL_DLLS ${USD_BIN_DLLS} ${USD_LIB_DLLS} ${VCPKG_DLLS})

    add_custom_command(TARGET ymodECS POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    )

    foreach(dll ${ALL_DLLS})
        add_custom_command(TARGET ymodECS POST_BUILD
                # Copy in bin/ (for distribution)
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "${BIN_DIR}"
        )
    endforeach()

    add_custom_command(TARGET ymodECS POST_BUILD
            # Plugin USD Plugins (necessary at runtime, separated path from dll's)
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/plugin/usd"
            "${RUNTIME_DIR}/usd"

            # Asset
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Scripts ${RUNTIME_DIR}/Scripts
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Scenes ${RUNTIME_DIR}/Scenes
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Sprites ${RUNTIME_DIR}/Sprites
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Resources ${RUNTIME_DIR}/Resources
    )

    # Separated Target : make/update dist/ into repo's root, ONLY if launched explicitly
    add_custom_target(package
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ymodECS> ${DIST_DIR}/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${BIN_DIR} ${DIST_DIR}/bin
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RUNTIME_DIR}/usd ${DIST_DIR}/usd
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RUNTIME_DIR}/Resources ${DIST_DIR}/Resources
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RUNTIME_DIR}/Scenes ${DIST_DIR}/Scenes
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RUNTIME_DIR}/Scripts ${DIST_DIR}/Scripts
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${RUNTIME_DIR}/Sprites ${DIST_DIR}/Sprites
            DEPENDS ymodECS
    )
endif()