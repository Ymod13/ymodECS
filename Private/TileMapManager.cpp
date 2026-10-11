//
// Created by ymod1 on 08/10/2026.
//

#include "TileMapManager.hpp"

#include <fstream>
#include <iosfwd>
#include <iostream>
#include <string>
#include <unordered_map>
#include <SDL3_image/SDL_image.h>

#include "ComponetsDefinitions.hpp"
#include "Environments.hpp"

using namespace TileMapUtils;

bool TileMapUtils::LoadAsciiMap(const std::string& path, int atlasCols, TileMapData& out)
{
    std::ifstream file_stream(path);
    if (!file_stream) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }

    std::unordered_map<char, uint16_t> legend;
    std::vector<std::string> rows;
    enum { None, Legend, Map } section = None;

    for (std::string line; std::getline(file_stream, line);)
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();  // file CRLF
        }

        if (line.rfind("[legend]", 0) == 0)
        {
            section = Legend;
            continue;
        }

        if (line.rfind("[map]", 0) == 0)
        {
            section = Map;
            continue;
        }

        if (section == Legend)
        {
            if (line.empty() || line[0] == ';')
            {
                continue;
            }
            char c; int col, row;

            if (std::sscanf(line.c_str(), "%c = %d,%d", &c, &col, &row) == 3) {
                legend[c] = static_cast<uint16_t>(row * atlasCols + col);
            }
        }
        else if (section == Map && !line.empty())
        {
            rows.push_back(line);
        }
    }

    out.atlasCols = atlasCols;
    out.height = static_cast<int>(rows.size());
    out.width = 0;
    for (auto& r : rows) out.width = std::max(out.width, (int)r.size());
    out.cells.assign(out.width * out.height, TileMapData::kEmpty);

    for (int y = 0; y < out.height; ++y)
    {
        for (int x = 0; x < (int)rows[y].size(); ++x)
        {
            char c = rows[y][x];
            if (c == '.' || c == ' ')
            {
                continue;
            }

            auto it = legend.find(c);

            if (it == legend.end())
            {
                std::cerr << "Map: char '" << c << "' not present in the legend (row " << y << ", col " << x << ")" << std::endl;
            }
            else
            {
                out.cells[y * out.width + x] = it->second;
            }

        }
    }

    std::cout << "LoadiAsciiMap: Map " << path << " loaded successfully!" << std::endl;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------
bool TileMapUtils::SpawnTileMap(ecs::World& world, SDL_Renderer* renderer, const std::string& atlas_filename, const TileMapData& map, float tile_px, SDL_FPoint scale)
{
    const std::string filename = env::sprites_folder + atlas_filename;

    SDL_Texture* tex = IMG_LoadTexture(renderer, filename.c_str());
    if (!tex)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "SpawnTileMap: %s (%s)", SDL_GetError(), filename.c_str());
        return false;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);   // pixel art nitida
    std::shared_ptr<SDL_Texture> shared_texture(tex, Sprite::SDLTextureDeleter{});

    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            const uint16_t id = map.cells[y * map.width + x];

            if (id == TileMapUtils::TileMapData::kEmpty)
            {
                continue;   // niente entità per il vuoto
            }

            ecs::EntityID e = world.create();

            Name name;
            name.name = "tile_" + std::to_string(x) + "_" + std::to_string(y);

            Size size;
            size.scale.x = scale.x;
            size.scale.y = scale.y;

            Position pos;
            pos.pos.x = x * tile_px * scale.x;
            pos.pos.y = y * tile_px * scale.y;

            Sprite spr;
            spr.filename = atlas_filename;
            spr.texture  = shared_texture;
            spr.is_tileset = true;

            spr.rect.w = tile_px;
            spr.rect.h = tile_px;
            spr.scaled_rect.w = tile_px * scale.x;
            spr.scaled_rect.h = tile_px * scale.y;

            spr.texture_rect.w = tile_px;
            spr.texture_rect.h = tile_px;
            spr.texture_rect.x = (id % map.atlasCols) * tile_px;
            spr.texture_rect.y = (id / map.atlasCols) * tile_px;

            spr.rect.x = spr.scaled_rect.x = pos.pos.x;
            spr.rect.y = spr.scaled_rect.y = pos.pos.y;

            spr.center.x = pos.pos.x + spr.scaled_rect.w / 2;
            spr.center.y = pos.pos.y + spr.scaled_rect.h / 2;

            spr.bounding_radius = FunctionsLib::CalculateRectRadius(spr.scaled_rect);

            ZOrder z;
            z.layer = UserInterface::LayerType::BACKGROUND;
            Visibility vis;
            vis.is_visible = true;

            BackgroundTile bg_tile;
            bg_tile.tile_coords.x = x;
            bg_tile.tile_coords.y = y;
            bg_tile.tiling_type = UserInterface::TILESET;

            world.add(e, Actor{});
            world.add(e, Background{});
            world.add(e, std::move(name));
            world.add(e, std::move(pos));
            world.add(e, std::move(size));
            world.add(e, std::move(vis));
            world.add(e, std::move(spr));
            world.add(e, std::move(z));
            world.add(e, std::move(bg_tile));
        }
    }
    return true;
}
