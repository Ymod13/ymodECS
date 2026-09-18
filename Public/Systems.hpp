//
// Created by ymod1 on 07/05/2026.
//

#ifndef YMODECS_FRAMEWORK_HPP
#define YMODECS_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <SDL3_image/SDL_image.h>

#include "ComponetsDefinitions.hpp"
#include "AccessSystem.hpp"
#include "EcsInspector.hpp"
#include "LuaUtils.hpp"
#include "Scheduler.hpp"
#include "UsdWrapper.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "RmlInputBridge.hpp"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Core/ElementDocument.h"

#ifdef YMODECS_RMLUI_DEBUGGER
#include "RmlUi/Debugger/Debugger.h"
#endif


// Systems declarations

using InputAccess = ecs::AccessMode<>
    ::With<NoMultithreading>;

using PlayerMovementAccess = ecs::AccessMode<>
    ::Read<Velocity>
    ::Write<Position>
    ::With<Player>;

using EnemiesMovementAccess = ecs::AccessMode<>
    ::Read<Velocity>
    ::Write<Position>
    ::With<Enemy>;

using BulletsMovementAccess = ecs::AccessMode<>
    ::Read<Velocity>
    ::Write<Position>
    ::With<Bullet>;

using CollisionDetectionAccess = ecs::AccessMode<>
    ::Read<Velocity>
    ::Read<Size>
    ::Read<Sprite>
    ::Write<Position>;

using RenderAccess = ecs::AccessMode<>
    ::Read<Position>
    ::Read<Name>;


inline void Usd_Parser_System(ecs::World& world, float dt) {
}
//------------------------------------------------------------------------------------------------------------------------

inline bool Init_Systems(ecs::World& world) {

    // Init Lua
    //-----------
    lua_State* lua_state=LuaUtils::InitLua();
    LuaUtils::LoadLuaConfig(lua_state);

    world.add_resource(env::LUAContext{lua_state});

    // Init SDL
    //----------
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Init_Systems: SDL Error: %s", SDL_GetError());
        return false;
    }
    else
    {
        std::cout << "Init_Systems: SDL initialized!\n";
    }

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer(env::window_title.c_str(), env::screen_width, env::screen_height, 0, &window, &renderer))
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER,"Init_Systems: Error creating Windows/Renderer: %s", SDL_GetError());
        return false;
    }

    SDL_StopTextInput(window);

    //ImGUI init
    //-----------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // RmlUi init
    //-----------
    auto* rmlRenderInterface = new RmlRenderInterface(renderer);
    auto* rmlSystemInterface = new RmlSystemInterface();

    Rml::SetRenderInterface(rmlRenderInterface);
    Rml::SetSystemInterface(rmlSystemInterface);

    if (!Rml::Initialise())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init_Systems: RmlUi initialisation failed");
        return false;
    }

    Rml::LoadFontFace("Resources/Fonts/Roboto-Regular.ttf", true);
    Rml::LoadFontFace("Resources/Fonts/Roboto-Italic.ttf");
    Rml::LoadFontFace("Resources/Fonts/Roboto-Bold.ttf");

    Rml::Context* rmlContext = Rml::CreateContext("main", Rml::Vector2i(env::screen_width, env::screen_height));
    if (!rmlContext)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init_Systems: Failed to create RmlUi context");
        return false;
    }

#ifdef YMODECS_RMLUI_DEBUGGER
    Rml::Debugger::Initialise(rmlContext);
#endif


    // ECS Inspector
    //----------------
    EcsInspector ecsInspector;
    world.add_resource(EcsInspector{ecsInspector});

    // World resources
    world.add_resource(env::SDLContext{window, renderer});
    world.add_resource(env::RmlUIContext{rmlContext, rmlRenderInterface, rmlSystemInterface});
    world.add_resource(env::InputState{});
    world.add_resource(env::Stats{});

    // init all sprite textures components
    auto& ctx = world.get_resource<env::SDLContext>();

    // init gamepad
    //--------------
    int num_gamepads = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&num_gamepads);
    if (num_gamepads > 0 && gamepads != nullptr) {
        ctx.gamepad = SDL_OpenGamepad(gamepads[0]);
        if (ctx.gamepad) {
            SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"\n - Gamepad found: %s \n", SDL_GetGamepadName(ctx.gamepad));
        }
        else {
            SDL_LogError(SDL_LOG_CATEGORY_INPUT, "Gamepad not initialized correctly!");
        }

        SDL_free(gamepads);
    }
    else {
        SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"Gamepads not found!");
    }

    // Load Assets
    //--------------
    UsdWrapper::LoadUsdFile("Scenes/SceneTemplates.usda", world);
    UsdWrapper::LoadUsdFile("Scenes/SceneBackgrounds.usda", world);
    UsdWrapper::LoadUsdFile("Scenes/Scene1.usda", world);
    UsdWrapper::LoadUsdFile("Scenes/SceneUI.usda", world);

    // Load RmlUi UI Assets
    //----------------------
    Rml::ElementDocument* testDocument = rmlContext->LoadDocument("Resources/UI/test.rml");
    if (testDocument) {
        testDocument->Show();
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init_Systems (RmlUi): Failed to load test.rml");
    }

    world.each<Sprite, Size, Position>([&](ecs::EntityID, Sprite& sprite, Size& size, Position &pos)
    {
        FunctionsLib::LoadSprite(ctx.renderer, size, pos, sprite);
    },  ecs::World::Exclude<Template>{});

    std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>> renderables_by_layer;

    world.each<Actor, Name, Size, Sprite, Visibility, Position>([&](ecs::EntityID id, Actor &actor, Name& name, Size &size, Sprite &sprite, Visibility &visibility, const Position &pos)
    {
        if (world.has<ZOrder>(id)) {
            auto &z_order = world.get<ZOrder>(id);
            if (z_order.layer == UserInterface::LayerType::WORLD_DYNAMIC) {
                z_order.depth = pos.pos.y;
                std::cout << " updated Depth: " << z_order.depth << std::endl;
            }
            renderables_by_layer[z_order.layer].push_back({id, z_order.layer, z_order.depth});
        }

    },  ecs::World::Exclude<Template>{});

    // renderables map first ordering
    for (auto& [layer, entries] : renderables_by_layer) {
        FunctionsLib::DynamicZOrdering(world, entries, true);
    }

    world.add_resource(std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>{renderables_by_layer});


    return true;
}
//------------------------------------------------------------------------------------------------------------------------

inline bool Quit_Systems(ecs::World& world) {

    // Cleanup (SDL3 handles memory better, but it's good to be explicit)

    /*
    world.each<Sprite>([](ecs::EntityID, Sprite& sprite)
    {
        if (sprite.texture) {
            SDL_DestroyTexture(sprite.texture.get());
        }
    });
    */

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // RmlUi shutdown
    //-----------------
    auto& rmlCtx = world.get_resource<env::RmlUIContext>();
    Rml::Shutdown();
    delete rmlCtx.renderInterface;
    delete rmlCtx.systemInterface;

    auto& ctx = world.get_resource<env::SDLContext>();
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);

    SDL_Quit();

    auto& lua_context = world.get_resource<env::LUAContext>();
    LuaUtils::CloseLua(lua_context.lua_state);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------

inline void Handle_Input(ecs::World& world, float dt) {

    auto& input = world.get_resource<env::InputState>();
    auto& stats = world.get_resource<env::Stats>();
    auto& rmlCtx = world.get_resource<env::RmlUIContext>();

    Vector2D mouse_pos;
    Vector2D mouse_rel;
    bool is_mouse_moved = false;

    while (SDL_PollEvent(&input.event)) {
        //std::cout << "input: event type = " << input.event.type << "\n";

        ImGui_ImplSDL3_ProcessEvent(&input.event);

        RmlInputBridge::ProcessEvent(rmlCtx, &input.event);

        switch (input.event.type) {
            case SDL_EVENT_QUIT:
                input.quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (input.event.key.repeat) {
                    break; // ignore automatic key repetition
                }
                switch (input.event.key.key) {
                    case SDLK_ESCAPE:
                        input.quit = true;
                        break;

                    case SDLK_F1:
                        env::show_imgui = !env::show_imgui;
                        break;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                mouse_pos = Vector2D(input.event.motion.x, input.event.motion.y);
                mouse_rel = Vector2D(input.event.motion.xrel, input.event.motion.yrel);
                is_mouse_moved = true;
                break;
        }

        if (!env::show_imgui) {

            // game logic input handling
            //---------------------------
            switch (input.event.type) {

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (input.event.button.button == SDL_BUTTON_LEFT) {
                        // left click
                        Vector2D dest_pos = Vector2D(input.event.motion.x, input.event.motion.y);
                        FunctionsLib::SpawnBullet(world, env::player_id, env::PISTOL, env::player_pos, dest_pos);
                    }
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (input.event.key.repeat) {
                        break; // ignore automatic key repetition
                    }
                    switch (input.event.key.key) {
                        case SDLK_UP:
                        case SDLK_W:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"UP pressed");
                            }
                            break;

                        case SDLK_DOWN:
                        case SDLK_S:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"DOWN pressed");
                            }
                            break;

                        case SDLK_LEFT:
                        case SDLK_A:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"LEFT pressed");
                            }
                            break;

                        case SDLK_RIGHT:
                        case SDLK_D:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"RIGHT pressed");
                            }
                            break;

                        case SDLK_SPACE:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "SPACE pressed");
                            }
                            break;
                    }
                    break;

                case SDL_EVENT_KEY_UP:
                    switch (input.event.key.key) {
                        case SDLK_UP:
                        case SDLK_W:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"UP released");
                            }
                            break;

                        case SDLK_DOWN:
                        case SDLK_S:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"DOWN released");
                            }
                            break;

                        case SDLK_LEFT:
                        case SDLK_A:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"LEFT released");
                            }
                            break;

                        case SDLK_RIGHT:
                        case SDLK_D:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"RIGHT released");
                            }
                            break;

                        case SDLK_SPACE:
                            if (env::is_input_text_debug) {
                                SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,"SPACE released");
                            }
                            break;
                    }
                    break;

                    /*
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    switch (event.gbutton.button) {
                    case SDL_GAMEPAD_BUTTON_SOUTH: // Equivalent to A on Xbox or Cross on PS
                            SDL_Log("Bottom button (A/Cross) pressed");
                            break;
                    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                            SDL_Log("D-Pad Left pressed");
                            break;
                    }
                    break;

                case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
                    const int DEADZONE = 8000;
                    // In SDL3 the axis macros use the SDL_GAMEPAD_AXIS_ prefix
                    if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                        if (event.gaxis.value < -DEADZONE) {
                            SDL_Log("Left stick: WEST");
                        } else if (event.gaxis.value > DEADZONE) {
                            SDL_Log("Left stick: EAST");
                        }
                    }
                    break;
                }

                    // 5. HOT-PLUG HANDLING IN SDL3
                case SDL_EVENT_GAMEPAD_ADDED:

                    if (!gGamepad) {
                        // e.gdevice.which directly contains the correct instance ID
                        gGamepad = SDL_OpenGamepad(e.gdevice.which);
                        if (gGamepad) {
                            SDL_Log("Gamepad connected: %s", SDL_GetGamepadName(gGamepad));
                        }
                    }

                    break;

                case SDL_EVENT_GAMEPAD_REMOVED:

                    if (gGamepad && e.gdevice.which == SDL_GetGamepadID(gGamepad)) {
                        SDL_CloseGamepad(gGamepad);
                        gGamepad = NULL;
                        SDL_Log("Gamepad disconnected!");
                    }
                    break;
                    */
            }
        }

    }

    world.each<UI, GameCursor, Position, Sprite, Size>([&, dt](ecs::EntityID e, UI& ui, GameCursor &game_cursor, Position& pos, Sprite &sprite, Size &size) {
       if (is_mouse_moved) {
           FunctionsLib::UpdatePosition(mouse_pos, pos, sprite, size.scale);
           stats.mouse_screen_pos = mouse_pos;;
           UserInterface::MouseCursorId = e;
       }

    },  ecs::World::Exclude<Template>{});


    input.keys = SDL_GetKeyboardState(nullptr);
}

// entities movements routines
inline void Update_Player_Movement(ecs::World& world, float dt)
{
    auto& input = world.get_resource<env::InputState>();

    world.each<Player, Position, Velocity, Sprite, Size>([input, dt](ecs::EntityID, Player& player, Position& pos, Velocity& vel, Sprite &sprite, Size &size)
    {
        if (!env::show_imgui) {
            // y axis movement
            FunctionsLib::Keyboard_vel_axis_movement(SDL_SCANCODE_W, SDL_SCANCODE_S, input.keys, vel.vel.y, vel.acceleration.y, vel.max_vel.y, dt);

            // x axis movement
            FunctionsLib::Keyboard_vel_axis_movement(SDL_SCANCODE_A, SDL_SCANCODE_D, input.keys, vel.vel.x, vel.acceleration.x, vel.max_vel.x, dt);

            // Update position
            FunctionsLib::UpdatePosition(pos.pos + vel.vel * dt, pos, sprite, size.scale, true);
        }

        env::player_pos = sprite.center;

    },  ecs::World::Exclude<Template>{});
}
//------------------------------------------------------------------------------------------------------------------------

inline void Update_Enemies_Movement(ecs::World& world, float dt)
{
    world.each<Enemy, Position, Velocity, Sprite, Size, Visibility>([dt](ecs::EntityID, Enemy& en, Position& pos, Velocity& vel,Sprite &sprite, Size &size, Visibility &visibility)
    {
        if (visibility.is_visible) {
            FunctionsLib::UpdatePosition(pos.pos + vel.vel * dt, pos, sprite, size.scale);
        }

    },  ecs::World::Exclude<Template>{});
}
//------------------------------------------------------------------------------------------------------------------------

inline void Update_Bullets_Movement(ecs::World& world, float dt)
{
    std::vector<ecs::EntityID> to_destroy;

    world.each<Bullet, Position, Velocity, Sprite, Size, Visibility>([dt, &to_destroy](ecs::EntityID id, Bullet& bullet, Position& pos, Velocity& vel,Sprite &sprite, Size &size, Visibility &visibility)
    {
        if (visibility.is_visible) {
            bullet.lifetimer+=dt;
            if (bullet.lifetimer >= bullet.lifespan) {
                // Destroy the bullet
                to_destroy.push_back(id);
            }
            else {
                FunctionsLib::UpdatePosition(pos.pos + vel.vel * dt, pos, sprite, size.scale);
            }
        }

    }, ecs::World::Exclude<Template>{});

    for (ecs::EntityID id : to_destroy) {
        world.destroy(id);
    }
}
//------------------------------------------------------------------------------------------------------------------------

inline void Collision_detection(ecs::World& world, float dt) {

    struct EntityData {
        ecs::EntityID id = ecs::NULL_ENTITY;
        Sprite *sprite = nullptr;
        Bullet *bullet = nullptr;
        Size *size;
    };

    std::vector<EntityData> coll_entities;

    // collect all collision entities from world
    world.each<Sprite, Size, Visibility>([&](ecs::EntityID id, Sprite &sprite, Size &size, Visibility &visibility)
        {
            if (visibility.is_visible) {
                if (sprite.collision_type == Collisions::RADIUS || sprite.collision_type == Collisions::RECTANGLE || sprite.collision_type == Collisions::MULTI_CIRCLE) {
                    sprite.possible_collision_entities.clear();
                    coll_entities.push_back({id, &sprite, nullptr, &size});
                }
            }
        },  ecs::World::Exclude<Template>{});

    // populate sprite possible collision entities
    for (std::size_t i = 0; i < coll_entities.size(); ++i) {
        for (std::size_t j = i + 1; j < coll_entities.size(); ++j) {
            auto& object_a = coll_entities[i];
            auto& object_b = coll_entities[j];

            float dist = (object_a.sprite->center - object_b.sprite->center).length();

            if (dist < (object_a.sprite->bounding_radius + object_b.sprite->bounding_radius)) {
                object_a.sprite->possible_collision_entities.push_back(object_b.id);
                object_b.sprite->possible_collision_entities.push_back(object_a.id);
            }
        }
    }

    world.each<Position, Velocity, Sprite, Size, Name, Visibility>([&](ecs::EntityID id, Position& pos, Velocity& vel, Sprite &sprite, Size &size, Name &name, Visibility &visibility)
        {
            if (visibility.is_visible) {

                Bullet *bullet=nullptr;
                if (world.has<Bullet>(id)) {
                    bullet = &world.get<Bullet>(id);
                }

                for (ecs::EntityID coll_id : sprite.possible_collision_entities) {
                    Position &pos_2 = world.get<Position>(coll_id);
                    Sprite &sprite_2 = world.get<Sprite>(coll_id);
                    Name &name_2 = world.get<Name>(coll_id);
                    Bullet *bullet_2=nullptr;

                    if (world.has<Bullet>(coll_id)) {
                        bullet_2 = &world.get<Bullet>(coll_id);
                    }

                    bool collision_detected = false;
                    Vector2D push_vector;

                    // sprite will be pushed by sprite_2 is function is present
                    switch (sprite.collision_type) {
                        case Collisions::RADIUS:
                            if (sprite_2.collision_type == Collisions::RADIUS) {
                                collision_detected = Utils::FunctionsLib::check_radius_collision(sprite.bounding_radius, sprite_2.bounding_radius, sprite.center, sprite_2.center, push_vector);
                            } else if (sprite_2.collision_type == Collisions::RECTANGLE) {
                                collision_detected = Utils::FunctionsLib::check_radius_rectangle_collision(sprite.bounding_radius, sprite.center, sprite_2, push_vector);
                            } else if (sprite_2.collision_type == Collisions::MULTI_CIRCLE) {
                                collision_detected = Utils::FunctionsLib::check_radius_multicircle_collision(sprite.bounding_radius, sprite.center, FunctionsLib::GetWorldColliders(sprite_2), push_vector);
                            }
                            break;

                        case Collisions::MULTI_CIRCLE:
                            if (sprite_2.collision_type == Collisions::RECTANGLE) {
                                collision_detected = Utils::FunctionsLib::check_multicircle_rectangle_collision(FunctionsLib::GetWorldColliders(sprite), sprite_2, push_vector);
                            } else if (sprite_2.collision_type == Collisions::MULTI_CIRCLE) {
                                collision_detected = Utils::FunctionsLib::check_multicircle_collision(FunctionsLib::GetWorldColliders(sprite), FunctionsLib::GetWorldColliders(sprite_2), push_vector);
                            }
                            break;

                        case Collisions::RECTANGLE:
                            if (sprite_2.collision_type == Collisions::RECTANGLE) {
                                collision_detected = Utils::FunctionsLib::check_rectangle_collision(sprite, sprite_2, push_vector);
                            }
                            break;
                    }

                    if (collision_detected) {
                        std::erase(sprite_2.possible_collision_entities, id);

                        if (sprite.overlaps_only || sprite_2.overlaps_only) {
                            // Collisions overlaps only

                            // Bullets handling
                            if (bullet) {
                                if (coll_id==bullet->owner_id) {
                                    // OWNER-Collision detected

                                }
                                else {
                                    std::cout << "-> BULLET 1 Collision detected between " << name.name << " and " << name_2.name << "\n";
                                }
                            } else if (bullet_2) {
                                if (id==bullet_2->owner_id) {
                                    // OWNER-Collision detected

                                } else {
                                    std::cout << "-> BULLET 2 Collision detected between " << name.name << " and " << name_2.name << "\n";
                                }
                            }
                            else {
                                // Non-bullets collision handling with overlaps_only objects
                                std::cout << "Collision detected between " << name.name << " and " << name_2.name << "\n";
                            }

                            // TODO: fire an event

                        } else {
                            //  Collisions influenced movement
                            // TODO: if PLAYER, update map position too
                            if (sprite.is_static_obstacle) {
                                FunctionsLib::UpdatePosition(pos_2.pos - push_vector , pos_2, sprite_2, size.scale);
                            }  else {
                                if (sprite_2.is_static_obstacle) {
                                    FunctionsLib::UpdatePosition(pos.pos + push_vector , pos, sprite, size.scale);
                                }
                                else {
                                    if (sprite.can_push && !sprite_2.can_push) {
                                        // move sprite_2 only
                                        FunctionsLib::UpdatePosition(pos_2.pos - push_vector , pos_2, sprite_2, size.scale);
                                    }
                                    else if (!sprite.can_push && sprite_2.can_push) {
                                        FunctionsLib::UpdatePosition(pos.pos + push_vector , pos, sprite, size.scale);
                                    }
                                    else if (sprite.can_push && sprite_2.can_push) {
                                        FunctionsLib::UpdatePosition(pos.pos + push_vector , pos, sprite, size.scale);
                                        FunctionsLib::UpdatePosition(pos_2.pos - push_vector, pos_2, sprite_2, size.scale);
                                    }
                                    else if (!sprite.can_push && !sprite_2.can_push){
                                        // no sprite can push
                                    }
                                }
                            }
                        }
                    }
                }
            }

        },  ecs::World::Exclude<Template>{});
}
//------------------------------------------------------------------------------------------------------------------------

inline void Render_Scene(ecs::World& world, float dt)
{
    auto& ctx = world.get_resource<env::SDLContext>();
    auto& stats = world.get_resource<env::Stats>();
    auto &ecsInspector = world.get_resource<EcsInspector>();
    auto &rmlCtx = world.get_resource<env::RmlUIContext>();

    // --- ImGui: frame start UI building ---
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // custom UI here (ex. ecsInspector.Draw(world))
    if (env::show_imgui) {
        ecsInspector.Draw(world);
    }

    auto& cursorVis = world.get<Visibility>(UserInterface::MouseCursorId);
    cursorVis.is_visible = !env::show_imgui;

    static bool was_showing_cursor = true;
    if (env::show_imgui != was_showing_cursor) {
        if (env::show_imgui) {
            SDL_ShowCursor();
        }
        else {
            SDL_HideCursor();
        }
        was_showing_cursor = env::show_imgui;
    }

    // --- RmlUi: update layout/state before rendering ---
    rmlCtx.context->Update();

    SDL_SetRenderDrawColor(ctx.renderer, 50, 0, 0, 255); // Black
    SDL_RenderClear(ctx.renderer);

    if (world.has_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>()) {
        auto& renderables_by_layer = world.get_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>();

        for (auto& [layer, renderable_entries] : renderables_by_layer) {
            if (layer==UserInterface::LayerType::WORLD_DYNAMIC) {
                FunctionsLib::DynamicZOrdering(world, renderable_entries, false);
            }

            for (auto& r : renderable_entries) {
                auto& sprite = world.get<Sprite>(r.entity);
                auto& name = world.get<Name>(r.entity);
                auto& visibility = world.get<Visibility>(r.entity);
                FunctionsLib::DrawSprite(ctx.renderer, sprite, name, visibility);
            }
        }
    }
    else
    {
        std::cerr << "Layer resource found, cannot render sprites" << std::endl;
        return;
    }

    stats.UpdateStats(ctx.renderer, dt);

    // --- RmlUi: draw HUD over the scene, under ImGui ---
    rmlCtx.context->Render();

    ImGui::Render();

    // --- ImGui: draw on top of the already rendered scene
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ctx.renderer);

    SDL_RenderPresent(ctx.renderer);
}

#endif //YMODECS_FRAMEWORK_HPP