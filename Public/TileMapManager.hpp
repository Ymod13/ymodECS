//
// Created by ymod1 on 08/10/2026.
//

#ifndef YMODECS_TILEMAPMANAGER_HPP
#define YMODECS_TILEMAPMANAGER_HPP

#include <string>
#include <vector>
#include <SDL3/SDL_render.h>

namespace ecs {
    class World;
}

namespace TileMapUtils {

    struct TileMapData {
        static constexpr uint16_t kEmpty = 0xFFFF;
        int width = 0, height = 0;
        int atlasCols = 9;
        std::vector<uint16_t> cells;   // id = riga * atlasCols + colonna
    };

    bool LoadAsciiMap(const std::string& path, int atlasCols, TileMapData& out);
    bool SpawnTileMap(ecs::World& world, SDL_Renderer* renderer, const std::string& atlas_filename, const TileMapData& map, float tile_px, SDL_FPoint scale);
}

#endif //YMODECS_TILEMAPMANAGER_HPP
