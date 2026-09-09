
#include <SDL3/SDL.h>
#include <iostream>
#include <string>

#include <pxr/base/plug/registry.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/tf/debug.h>
#include <filesystem>

#include "Systems.hpp"

void InitUsdSubsystem()
{
#if defined(_DEBUG)
    std::cout << "[C++] Registering USD plugins (Debug)..." << std::endl;
    const std::vector<std::string> pluginPaths = {
        "D:/Stuff/Projects/USD_Debug/plugin/usd",
        "D:/Stuff/Projects/USD_Debug/lib/usd"
    };
#else
    std::cout << "[C++] Registering USD plugins (Release)..." << std::endl;
    const std::vector<std::string> pluginPaths = {
        "D:/Stuff/Projects/USD_Release/plugin/usd",
        "D:/Stuff/Projects/USD_Release/lib/usd"
    };
#endif

    auto& registry = PlugRegistry::GetInstance();
    auto newPlugins = registry.RegisterPlugins(pluginPaths);

    std::cout << "[C++] Registered USD plugins: " << newPlugins.size() << std::endl;
    for (const auto& plug : newPlugins) {
        std::cout << "  - " << plug->GetName() << std::endl;
    }
}

int main()
{
    InitUsdSubsystem();

    std::cout << "Starting...\n";

    ecs::World game_world;
    ecs::Scheduler scheduler;

    scheduler.add<InputAccess> ("InputSystem", Handle_Input);
    scheduler.add<PlayerMovementAccess> ("PlayerMovementSystem", Update_Player_Movement);
    scheduler.add<EnemiesMovementAccess> ("EnemiesMovementSystem", Update_Enemies_Movement);
    scheduler.add<BulletsMovementAccess> ("BulletsMovementSystem", Update_Bullets_Movement);
    scheduler.add<CollisionDetectionAccess> ("CollisionDetectionSystem", Collision_detection);
    scheduler.add<RenderAccess> ("RenderSystem", Render_Scene);

    ecs::EntityID single_tread_entity = game_world.create();
    game_world.add(single_tread_entity, NoMultithreading{});

    if (!Init_Systems(game_world)) {
        std::cout << "   -> Systems initialization failed!";
        return -1;
    }

    bool running = true;

    using Clock = std::chrono::high_resolution_clock;
    using Seconds = std::chrono::duration<float>;

    auto last = Clock::now();

    // 5. Game Loop
    while (running)
    {
        auto now = Clock::now();
        float dt = std::chrono::duration_cast<Seconds>(now - last).count();
        last = now;

        auto& input = game_world.get_resource<env::InputState>();

        scheduler.tick(game_world, dt);

        running = !input.quit;
    }

    std::cout << "Quitting...\n";

    Quit_Systems(game_world);

    std::cout << "Exiting...\n";

    return 0;
}
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.