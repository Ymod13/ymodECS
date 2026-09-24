//
// Created by ymod1 on 20/05/2026.
//

#include "Utilities.hpp"
#include "../Public/MathUtils.hpp"

#include <algorithm>
#include <iostream>
#include <numbers>
#include <ostream>
#include <SDL3_image/SDL_image.h>

#include "ComponetsDefinitions.hpp"

using namespace Utils;

void FunctionsLib::UpdatePosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_map_limits) {
    pos.old_pos = pos.pos;
    pos.pos = new_pos;
    pos.delta_pos = pos.pos - pos.old_pos;

    // OPTIONAL: Clamp position to map limits
    if (clamp_to_map_limits) {
        clamp_position_map(pos.pos, scale, sprite.texture.get());
    }
}
//-------------------------------------------------------------------------------------------------------------

void FunctionsLib::UpdateScreenPosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_screen) {
    pos.screen_pos = new_pos;

    // this is the value used to render the sprite in the right location on screen
    sprite.rect.x = sprite.scaled_rect.x = pos.screen_pos.x;
    sprite.rect.y = sprite.scaled_rect.y = pos.screen_pos.y;

    sprite.scaled_rect.w = sprite.rect.w * scale.x;
    sprite.scaled_rect.h = sprite.rect.h * scale.y;

    sprite.center.x = pos.screen_pos.x + sprite.scaled_rect.w / 2;
    sprite.center.y = pos.screen_pos.y + sprite.scaled_rect.h / 2;

    sprite.bounding_radius = CalculateRectRadius(sprite.scaled_rect);
}
//-------------------------------------------------------------------------------------------------------------

void FunctionsLib::RestoreOldPosition(Position &pos, Sprite &sprite, const Vector2D &scale) {
    UpdatePosition(pos.old_pos, pos, sprite, scale, true);
}
//-------------------------------------------------------------------------------------------------------------

void FunctionsLib::Keyboard_vel_axis_movement(const SDL_Scancode dir_1_key, const SDL_Scancode dir_2_key, const bool* keys, float &vel, const float &acceleration, const float &deceleration, const float &max_vel, const float &dt)
{
    if (keys[dir_1_key] && !keys[dir_2_key]) {
        if (vel > -max_vel) {
            vel = std::clamp(vel -= acceleration*dt, -max_vel, 0.0f);
        }
    }

    if (keys[dir_2_key] && !keys[dir_1_key]) {
        if (vel < max_vel) {
            vel = std::clamp(vel += acceleration*dt, 0.0f, max_vel);
        }
    }

    if ((!keys[dir_2_key] && !keys[dir_1_key]) || (keys[dir_2_key] && keys[dir_1_key])) {
        if (vel < 0) {
            vel = std::clamp(vel += deceleration*dt, -max_vel, 0.0f);
        } else if (vel > 0) {
            vel = std::clamp(vel -= deceleration*dt, 0.0f, max_vel);
        }
        else {
            vel = 0;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::clamp_position_map(Vector2D &pos, const Vector2D &scale, const SDL_Texture *SpriteTexture) {
    bool clamped = false;

    if (pos.x < 0) {
        pos.x = 0;
        clamped = true;
    }

    if (pos.y < 0) {
        pos.y = 0;
        clamped = true;
    }

    if (SpriteTexture) {
        if (pos.x + SpriteTexture->w * scale.x > env::map_size.x) {
            pos.x = env::map_size.x - SpriteTexture->w * scale.x;
            clamped = true;
        }

        if (pos.y + SpriteTexture->h * scale.y > env::map_size.y) {
            pos.y = env::map_size.y - SpriteTexture->h * scale.y;
            clamped = true;
        }
    }

    return clamped;
}
//-------------------------------------------------------------------------------------------------------------

Vector2D FunctionsLib::MapToWindow(const Vector2D &map_pos, const Vector2D &cam_pos) {
    //return { map_pos.x - cam_pos.x, map_pos.y - cam_pos.y };
    Vector2D result;

    Vector2D top_left = {cam_pos.x - env::screen_width/2, cam_pos.y - env::screen_height/2};

    return result;

}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::check_radius_collision(const float &radius_a, const float &radius_b, const Vector2D &center_a, const Vector2D &center_b, Vector2D &OutPushVector)  {
    bool has_collided = false; //(radius_a + radius_b) >= (center_a - center_b).length();

    Vector2D centers_dist_v = center_a - center_b;

    float dist_sq  = centers_dist_v.x * centers_dist_v.x + centers_dist_v.y * centers_dist_v.y;
    float min_dist = radius_a + radius_b;

    if (dist_sq < min_dist * min_dist) {
        has_collided = true;

        float dist = std::sqrt(dist_sq);

        OutPushVector = centers_dist_v;
        OutPushVector.normalize();

        if (dist < 0.0001f) {
            // Circles are touching
            OutPushVector = OutPushVector * 10.0f;
        }
        else {
            float penetration = min_dist - dist;

            OutPushVector = OutPushVector * penetration;
        }
    }

    return has_collided;
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::check_radius_rectangle_collision(const float &radius_a, const Vector2D &center_a, const Sprite& sprite_b_rect, Vector2D &OutPushVector) {

    float rx = sprite_b_rect.rect.x;
    float ry = sprite_b_rect.rect.y;
    float rw = sprite_b_rect.scaled_rect.w;
    float rh = sprite_b_rect.scaled_rect.h;

    float cx = center_a.x;
    float cy = center_a.y;
    float cr = radius_a;

    // ── Closest point on the rectangle to the circle center ──
    // clamp = if the center is inside the rectangle, it stays where it is
    //         if it is outside, it is projected onto the edge
    float closest_x = std::clamp(cx, rx, rx + rw);
    float closest_y = std::clamp(cy, ry, ry + rh);

    float dx = cx - closest_x;
    float dy = cy - closest_y;
    float dist_sq = dx * dx + dy * dy;

    // ── Case 1: center OUTSIDE the rectangle ────────────────────
    if (dist_sq > 0.0001f) {
        if (dist_sq < cr * cr) {
            float dist = std::sqrt(dist_sq);
            float penetration = cr - dist;

            OutPushVector.x = dx / dist;
            OutPushVector.y = dy / dist;
            OutPushVector = OutPushVector*penetration;

            return true;
        }
    } else {
        // The center is inside, find the closest edge
        float overlap_left   =  (cx - rx);
        float overlap_right  =  (rx + rw - cx);
        float overlap_top    =  (cy - ry);
        float overlap_bottom =  (ry + rh - cy);

        float min_overlap = std::min({overlap_left, overlap_right,
                                      overlap_top,  overlap_bottom});

        float nx = 0, ny = 0;
        if      (min_overlap == overlap_left)   { nx = -1;  ny =  0; }
        else if (min_overlap == overlap_right)  { nx =  1;  ny =  0; }
        else if (min_overlap == overlap_top)    { nx =  0;  ny = -1; }
        else                                    { nx =  0;  ny =  1; }

        OutPushVector.x = nx;
        OutPushVector.y = ny;
        OutPushVector = OutPushVector*(cr + min_overlap);

        return true;
    }

    return false;
}

bool FunctionsLib::check_radius_multicircle_collision(const float &radius_a, const Vector2D &center_a,
    const std::vector<Collisions::WorldCircle> &circles, Vector2D &OutPushVector) {
    bool has_collided = false;
    for (auto& circle : circles) {
        Vector2D PushVector;
        if (check_radius_collision(radius_a, circle.radius, center_a, circle.center, PushVector)) {
            has_collided = true;
            OutPushVector += PushVector;
        }
    }

    return has_collided;
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::check_multicircle_collision(const std::vector<Collisions::WorldCircle> &circles_a,
    const std::vector<Collisions::WorldCircle> &circles_b, Vector2D &OutPushVector) {
    bool has_collided = false;
    for (auto& circle_a : circles_a) {
        for (auto& circle_b : circles_b) {
            Vector2D PushVector;
            if (check_radius_collision(circle_a.radius, circle_b.radius, circle_a.center, circle_b.center, PushVector)) {
                has_collided = true;
                OutPushVector += PushVector;
            }
        }
    }


    return has_collided;
}

//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::check_multicircle_rectangle_collision(const std::vector<Collisions::WorldCircle> &circles,
    const Sprite &sprite_b_rect, Vector2D &OutPushVector) {
    bool has_collided = false;
    for (auto& circle : circles) {
        Vector2D PushVector;
        if (check_radius_rectangle_collision(circle.radius, circle.center, sprite_b_rect, PushVector)) {
            has_collided = true;
            OutPushVector += PushVector;
        }
    }

    return has_collided;
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::check_rectangle_collision(const Sprite &sprite_a_rect, const Sprite &sprite_b_rect,
    Vector2D &OutPushVector) {

    float ax = sprite_a_rect.rect.x;
    float ay = sprite_a_rect.rect.y;
    float aw = sprite_a_rect.scaled_rect.w;
    float ah = sprite_a_rect.scaled_rect.h;

    float bx = sprite_b_rect.rect.x;
    float by = sprite_b_rect.rect.y;
    float bw = sprite_b_rect.scaled_rect.w;
    float bh = sprite_b_rect.scaled_rect.h;

    // Separating Axis Test (AABB)
    float overlap_x = std::min(ax + aw, bx + bw) - std::max(ax, bx);
    float overlap_y = std::min(ay + ah, by + bh) - std::max(ay, by);

    if (overlap_x <= 0.0f || overlap_y <= 0.0f) {
        return false;
    }

    // Push along the axis of minimum penetration
    if (overlap_x < overlap_y) {
        // Resolve on X
        float sign = (ax + aw * 0.5f < bx + bw * 0.5f) ? -1.0f : 1.0f;
        OutPushVector.x = sign * overlap_x;
        OutPushVector.y = 0.0f;
    } else {
        // Resolve on Y
        float sign = (ay + ah * 0.5f < by + bh * 0.5f) ? -1.0f : 1.0f;
        OutPushVector.x = 0.0f;
        OutPushVector.y = sign * overlap_y;
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::CheckPixelPerfectCollision(const Sprite &objA, const Sprite &objB, Uint8 alphaThreshold) {
    if (!SDL_HasRectIntersectionFloat(&objA.rect, &objB.rect)) {
        return false;
    }
    // PHASE 2: Calculate the screen-space intersection rectangle
    SDL_FRect intersectionF;
    SDL_GetRectIntersectionFloat(&objA.rect, &objB.rect, &intersectionF);

    // Convert the intersection to integer coordinates to iterate over the pixels
    int interX = (int)intersectionF.x;
    int interY = (int)intersectionF.y;
    int interW = (int)intersectionF.w;
    int interH = (int)intersectionF.h;

    // Make sure the pixel surfaces are valid
    if (!objA.surface || !objB.surface) return false;

    // PHASE 3: Scan the intersection area pixel by pixel
    for (int y = interY; y < interY + interH; ++y) {
        for (int x = interX; x < interX + interW; ++x) {

            // Convert screen coordinates to LOCAL coordinates of object A
            int localAX = x - (int)objA.rect.x;
            int localAY = y - (int)objA.rect.y;

            // Convert screen coordinates to LOCAL coordinates of object B
            int localBX = x - (int)objB.rect.x;
            int localBY = y - (int)objB.rect.y;

            // Extract the Alpha value of the pixel for object A
            Uint8 alphaA = 0;
            if (SDL_ReadSurfacePixel(objA.surface.get(), localAX, localAY, NULL, NULL, NULL, &alphaA)) {

                // If A's pixel is opaque, also check the corresponding pixel of B
                if (alphaA > alphaThreshold) {
                    Uint8 alphaB = 0;
                    if (SDL_ReadSurfacePixel(objB.surface.get(), localBX, localBY, NULL, NULL, NULL, &alphaB)) {

                        // If BOTH pixels at the same position are opaque, there is a collision!
                        if (alphaB > alphaThreshold) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------

float FunctionsLib::CalculateRectRadius(const SDL_FRect &rect) {
    return std::sqrt(rect.w * rect.w + rect.h * rect.h) / 2.0f ;
}
//-------------------------------------------------------------------------------------------------------------

void FunctionsLib::GenerateCircleCluster(Sprite& obj, int cellSize, Uint8 alphaThreshold) {
    if (!obj.surface) return;

    obj.localColliderCluster.clear();
    int w = obj.surface->w;
    int h = obj.surface->h;

    // Calculate the sprite center and use it as the origin (0,0) of the local coordinates
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;

    // Iterate through the image cell by cell (grid)
    for (int cellY = 0; cellY < h; cellY += cellSize) {
        for (int cellX = 0; cellX < w; cellX += cellSize) {

            float sumX = 0;
            float sumY = 0;
            int visiblePixelCount = 0;

            // Analyze the pixels inside the current cell
            for (int y = cellY; y < cellY + cellSize && y < h; ++y) {
                for (int x = cellX; x < cellX + cellSize && x < w; ++x) {
                    Uint8 alpha = 0;
                    if (SDL_ReadSurfacePixel(obj.surface.get(), x, y, NULL, NULL, NULL, &alpha)) {
                        if (alpha > alphaThreshold) {
                            sumX += x;
                            sumY += y;
                            visiblePixelCount++;
                        }
                    }
                }
            }

            // If the cell contains visible pixels (e.g. at least 15% of the cell size)
            if (visiblePixelCount > (cellSize * cellSize * 0.15f)) {
                // Calculate the center of mass (centroid) of the visible pixels in this cell
                float avgX = sumX / visiblePixelCount;
                float avgY = sumY / visiblePixelCount;

                // Find the minimum radius needed to cover all visible pixels in this cell
                float maxDistanzaSq = 0.0f;
                for (int y = cellY; y < cellY + cellSize && y < h; ++y) {
                    for (int x = cellX; x < cellX + cellSize && x < w; ++x) {
                        Uint8 alpha = 0;
                        SDL_ReadSurfacePixel(obj.surface.get(), x, y, NULL, NULL, NULL, &alpha);
                        if (alpha > alphaThreshold) {
                            float dx = x - avgX;
                            float dy = y - avgY;
                            float distSq = dx*dx + dy*dy;
                            if (distSq > maxDistanzaSq) maxDistanzaSq = distSq;
                        }
                    }
                }

                // Create the local circle using coordinates relative to the sprite center
                Collisions::LocalCircle c;
                c.local_center.x = avgX - centerX;
                c.local_center.y = avgY - centerY;
                c.radius = std::sqrt(maxDistanzaSq);

                obj.localColliderCluster.push_back(c);
            }
        }
    }
}
// -------------------------------------------------------------------------------------------------------------

std::vector<Collisions::WorldCircle> FunctionsLib::GetWorldColliders(const Sprite& obj) {
    std::vector<Collisions::WorldCircle> worldColliders;
    worldColliders.reserve(obj.localColliderCluster.size());

    // Calculate the sprite center in the game world (screen)
    Vector2D world_center = obj.center;

    // Convert the sprite angle to radians
    float rad = obj.angle * (std::numbers::pi / 180.0f);
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    for (const auto& local : obj.localColliderCluster) {
        Collisions::WorldCircle w;
        // 2D point rotation around origin formula
        w.center.x = world_center.x + (local.local_center.x * cosA - local.local_center.y * sinA);
        w.center.y = world_center.y + (local.local_center.x * sinA + local.local_center.y * cosA);

        w.radius = local.radius;

        worldColliders.push_back(w);
    }

    return worldColliders;
}
// -------------------------------------------------------------------------------------------------------------

float FunctionsLib::ComputeDepth(const Position &position, const Sprite &sprite) {
    float offset = sprite.scaled_rect.h * sprite.pivot_y;
    return position.pos.y + offset;
}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::InsertionSortByDepth(std::vector<ecs::RenderableEntry> &entries) {
    for (size_t i = 1; i < entries.size(); ++i) {
        auto key = entries[i];
        size_t j = i;

        while (j > 0 && entries[j - 1].depth > key.depth) {
            entries[j] = entries[j - 1];
            --j;
        }
        entries[j] = key;
    }
}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::DynamicZOrdering(ecs::World &world, std::vector<ecs::RenderableEntry> &entries, const bool &is_first_ordering) {
    bool any_changed = false;
    // Dynamic depth ordering
    for (auto& r : entries) {
        auto& visibility = world.get<Visibility>(r.entity);

        if (visibility.is_visible) {
            auto& pos = world.get<Position>(r.entity);
            auto& sprite = world.get<Sprite>(r.entity);
            auto& z_order = world.get<ZOrder>(r.entity);

            float new_depth = FunctionsLib::ComputeDepth(pos, sprite);
            if (new_depth != z_order.depth) {
                z_order.depth = new_depth;
                r.depth = z_order.depth;
                any_changed = true;
            }
        }
    }

    if (is_first_ordering) {
        // O(n log n)
        std::ranges::sort(entries, {}, &ecs::RenderableEntry::depth);
    }
    else {
        // O(n) -> O(n*n) (worst case)
        if (any_changed) {
            FunctionsLib::InsertionSortByDepth(entries);
        }
    }
}
// -------------------------------------------------------------------------------------------------------------

bool FunctionsLib::LoadSprite(SDL_Renderer* renderer, const Size& size, const Position &pos, Sprite& out_sprite) {
    const std::string filename = env::sprites_folder + out_sprite.filename;

    switch (out_sprite.collision_type) {

        case Collisions::NONE:
        case Collisions::RADIUS:
        case Collisions::RECTANGLE:

            if ( SDL_Texture* spriteTexture = IMG_LoadTexture(renderer, filename.c_str())) {
                out_sprite.texture.reset(spriteTexture, SDL_DestroyTexture);
                out_sprite.rect.w = spriteTexture->w;
                out_sprite.rect.h = spriteTexture->h;
                out_sprite.scaled_rect.w = spriteTexture->w * size.scale.x;
                out_sprite.scaled_rect.h = spriteTexture->h * size.scale.y;
            }
            else {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,"LoadSprite: Error loading Texture: %s (%s)", SDL_GetError(), filename.c_str());
                return false;
            }
            break;

        case Collisions::MULTI_CIRCLE:
            // Texture loading with collision data
            SDL_Surface *surface = IMG_Load(filename.c_str());
            if (surface) {
                out_sprite.surface.reset(surface);
                out_sprite.rect.w = surface->w;
                out_sprite.rect.h = surface->h;
                out_sprite.scaled_rect.w = surface->w * size.scale.x;
                out_sprite.scaled_rect.h = surface->h * size.scale.y;

                GenerateCircleCluster(out_sprite);

                if ( SDL_Texture* surfaceTexture = SDL_CreateTextureFromSurface(renderer, surface)) {
                    out_sprite.texture.reset(surfaceTexture, SDL_DestroyTexture);
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_RENDER,"LoadSprite: Error loading Texture: %s (%s)", SDL_GetError(), filename.c_str());
                    return false;
                }
            }
            else {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "LoadSprite: Error loading surface %s (%s)", SDL_GetError(), filename.c_str());
                return false;
            }
            break;
    }

    out_sprite.rect.x = out_sprite.scaled_rect.x= pos.pos.x;
    out_sprite.rect.y = out_sprite.scaled_rect.y = pos.pos.y;

    out_sprite.center.x = pos.pos.x + out_sprite.scaled_rect.w / 2;
    out_sprite.center.y = pos.pos.y + out_sprite.scaled_rect.h / 2;

    out_sprite.bounding_radius = CalculateRectRadius(out_sprite.scaled_rect);

    return true;
}
// -------------------------------------------------------------------------------------------------------------

bool FunctionsLib::LoadBackgroundSprite(ecs::World &world, const ecs::EntityID &id, SDL_Renderer *renderer) {

    if (!world.has<Size>(id)) {
        std::cerr << "Error: Entity " << id << " does not have a Size component." << std::endl;
        return false;
    }
    if (!world.has<Name>(id)) {
        std::cerr << "Error: Entity " << id << " does not have a Name component." << std::endl;
        return false;
    }
    if (!world.has<Position>(id)) {
        std::cerr << "Error: Entity " << id << " does not have a Position component." << std::endl;
        return false;
    }
    if (!world.has<Sprite>(id)) {
        std::cerr << "Error: Entity " << id << " does not have a Sprite component." << std::endl;
        return false;
    }
    if (!world.has<BackgroundTile>(id)) {
       std::cerr << "Error: Entity " << id << " does not have a BackgroundTile component." << std::endl;
       return false;
    }

    auto &size = world.get<Size>(id);
    auto &name = world.get<Name>(id);
    auto &pos = world.get<Position>(id);
    auto &sprite = world.get<Sprite>(id);
    auto &tile = world.get<BackgroundTile>(id);

    // copy key values to avoid sparse/set reallocation due to frequent changes
    const std::string name_str = name.name;
    const std::string sprite_filename = sprite.filename;
    const auto sprite_tiling_type = tile.tiling_type;
    const auto texture_scale = size.scale;

    const std::string filename = env::sprites_folder + sprite_filename;

    SDL_Texture* spriteTexture = IMG_LoadTexture(renderer, filename.c_str());

    if (!spriteTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER,"LoadBackgroundSprite: Error loading background Texture: %s (%s)", SDL_GetError(), filename.c_str());
        return false;
    }

    std::shared_ptr<SDL_Texture> shared_texture(spriteTexture, Sprite::SDLTextureDeleter{});

    // let's make this one a template (so it's not rendered)
    Template templ;
    world.add(id, std::move(templ));

    float x_limit = env::map_size.x;
    float y_limit = env::map_size.y;

    switch (sprite_tiling_type) {
        case UserInterface::TilingType::FILL:
            x_limit = 1;
            y_limit = 1;
            break;
    }

    for (float i=0; i<=x_limit; i+= spriteTexture->w*texture_scale.x) {
        for (float j=0; j<=y_limit; j+= spriteTexture->h*texture_scale.y) {
            ecs::EntityID bkg_id = world.create();

            Name bkg_name;
            bkg_name.name = name_str + "_" + std::to_string(i);

            Size bkg_size;
            bkg_size.scale = texture_scale;

            Sprite bkg_sprite;
            bkg_sprite.filename = sprite_filename;

            ZOrder z_order;
            z_order.layer = UserInterface::LayerType::BACKGROUND;

            Visibility bkg_vis;
            bkg_vis.is_visible = true;

            bkg_sprite.texture = shared_texture;
            if (sprite_tiling_type==UserInterface::TilingType::FILL) {
                bkg_sprite.rect.w = env::map_size.x;
                bkg_sprite.rect.h = env::map_size.y;
                bkg_sprite.scaled_rect.w = env::map_size.x;
                bkg_sprite.scaled_rect.h = env::map_size.y;
            }
            else {
                bkg_sprite.rect.w = bkg_sprite.texture->w;
                bkg_sprite.rect.h = bkg_sprite.texture->h;
                bkg_sprite.scaled_rect.w = bkg_sprite.texture->w * bkg_size.scale.x;
                bkg_sprite.scaled_rect.h = bkg_sprite.texture->h * bkg_size.scale.y;
            }


            Position bkg_pos;
            bkg_pos.pos.x = i;
            bkg_pos.pos.y = j;

            bkg_sprite.rect.x = bkg_sprite.scaled_rect.x = bkg_pos.pos.x;
            bkg_sprite.rect.y = bkg_sprite.scaled_rect.y = bkg_pos.pos.y;

            bkg_sprite.center.x = bkg_pos.pos.x + bkg_sprite.scaled_rect.w / 2;
            bkg_sprite.center.y = bkg_pos.pos.y + bkg_sprite.scaled_rect.h / 2;

            bkg_sprite.bounding_radius = CalculateRectRadius(bkg_sprite.scaled_rect);

            // Add components to world
            world.add(bkg_id, Actor{});
            world.add(bkg_id, Background{});
            world.add(bkg_id, std::move(bkg_name));
            world.add(bkg_id, std::move(bkg_pos));
            world.add(bkg_id, std::move(bkg_size));
            world.add(bkg_id, std::move(bkg_vis));
            world.add(bkg_id, std::move(bkg_sprite));
            world.add(bkg_id, std::move(z_order));
        }
    }



    return true;
}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::SpawnBullet(ecs::World &world, const ecs::EntityID owner_id, const env::BulletType bullet_type, const Vector2D &start_pos, const Vector2D &end_pos) {

    auto& ctx = world.get_resource<env::SDLContext>();

    ecs::EntityID e = world.create();
    world.add(e, Actor{});
    world.add(e, Name{ "Bullet_" });
    world.add(e, Position{ start_pos });

    Visibility visibility;
    visibility.is_visible = true;

    world.add(e, std::move(visibility));

    Bullet new_bullet;
    Sprite bullet_sprite;
    Size bullet_size;
    Velocity bullet_velocity;
    ZOrder bullet_z_order;

    bool is_template_found = false;

    world.each<Template, Bullet, Sprite, Size, Velocity, ZOrder>([&](ecs::EntityID id, Template &template_comp, Bullet& bullet, Sprite &sprite, Size &size, Velocity &velocity, ZOrder &z_order) {

        if (bullet_type == bullet.bullet_type) {
            bullet_sprite.CopyData(&sprite);
            bullet_size.CopyData(&size);
            bullet_velocity.CopyData(&velocity);
            bullet_z_order.CopyData(&z_order);
            new_bullet.CopyData(&bullet);
            is_template_found = true;
            return false; // exits each loop
        }

        return true;

    });

    if (!is_template_found) {
        std::cerr << "Error: no template found for bullet_type: " << static_cast<int>(bullet_type) << std::endl;
        world.destroy(e);
        return;
    }

    new_bullet.owner_id = owner_id;

    if (LoadSprite(ctx.renderer, bullet_size, start_pos, bullet_sprite)) {
        world.add(e, std::move(new_bullet));
        world.add(e, std::move(bullet_sprite));
        world.add(e, std::move(bullet_size));
        world.add(e, std::move(bullet_z_order));

        Vector2D bullet_dir = end_pos - start_pos;
        bullet_dir.normalize();

        bullet_velocity.vel = bullet_dir * new_bullet.speed;;

        world.add(e, std::move(bullet_velocity));

        if (world.has_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>()) {
            auto& renderables_by_layer = world.get_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>();

            auto [it, inserted] = renderables_by_layer.try_emplace(bullet_z_order.layer);

            renderables_by_layer[bullet_z_order.layer].push_back({e, bullet_z_order.layer, bullet_z_order.depth});
            std::ranges::sort(renderables_by_layer[bullet_z_order.layer], {}, &ecs::RenderableEntry::depth);

            // TODO: questo crea un crash
            //it->second.push_back({e, bullet_z_order.layer, bullet_z_order.depth});

            //std::ranges::sort(it->second, {}, &ecs::RenderableEntry::depth);
        }
        else {
            std::cerr << "Layer resource not found, cannot render Bullet" << std::endl;
        }


    }
    else {
        std::cerr << "Error loading bullet sprite" << std::endl;
    }



}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::DrawCircle(SDL_Renderer* renderer, const Vector2D &center, float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {

    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    float x = radius;
    float y = 0;
    float error = 1.0f - radius;

    while (x >= y) {
        // Use the circle's 8-way symmetry to draw the points
        SDL_RenderPoint(renderer, center.x + x, center.y + y);
        SDL_RenderPoint(renderer, center.x + y, center.y + x);
        SDL_RenderPoint(renderer, center.x - y, center.y + x);
        SDL_RenderPoint(renderer, center.x - x, center.y + y);
        SDL_RenderPoint(renderer, center.x - x, center.y - y);
        SDL_RenderPoint(renderer, center.x - y, center.y - x);
        SDL_RenderPoint(renderer, center.x + y, center.y - x);
        SDL_RenderPoint(renderer, center.x + x, center.y - y);

        y++;

        if (error < 0) {
            error += 2.0f * y + 1.0f;
        } else {
            x--;
            error += 2.0f * (y - x) + 1.0f;
        }
    }
}
// -------------------------------------------------------------------------------------------------------------
void FunctionsLib::DrawRectangle(SDL_Renderer *renderer, const SDL_FRect &rect, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderRect(renderer, &rect);
}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::DrawCirclesCluster(SDL_Renderer *renderer, const Sprite &obj, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    auto debugCircles = GetWorldColliders(obj);
    for (auto& c : debugCircles) {
        DrawCircle(renderer, c.center, c.radius, r, g, b, a);
    }
}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::DrawSprite(SDL_Renderer *renderer, const Sprite &sprite, const Name &name, const Visibility &visibility) {

    if (!visibility.is_visible) {
        return;
    }

    if (SDL_Texture *sprite_texture = sprite.texture.get()) {

        //  a pointer to a point indicating the point around which dstrect will be rotated (if NULL, rotation will be done around dstrect.w/2, dstrect.h/2).
        SDL_FPoint* rotation_center = NULL;

        // Draws the rotated texture on the GPU
        if (SDL_RenderTextureRotated(renderer, sprite_texture, NULL, &sprite.scaled_rect, sprite.angle, rotation_center, SDL_FLIP_NONE)) {
        //if (SDL_RenderTexture(ctx.renderer, SpriteTexture, nullptr, &sprite.scaled_rect)) {
            if (env::is_text_debug) {
                std::cout << "  ->Sprite rendered: " << name.name <<  " - pos: " << sprite.scaled_rect.x << "; " << sprite.scaled_rect.y <<
                " - size: "<< sprite.scaled_rect.w << "; " << sprite.scaled_rect.h << "\n";
            }

            if (sprite.draw_debug_shapes) {
                switch (sprite.collision_type) {
                    case Collisions::RADIUS:
                        FunctionsLib::DrawCircle(renderer, sprite.center, sprite.bounding_radius);
                        break;
                    case Collisions::RECTANGLE:
                        FunctionsLib::DrawRectangle(renderer, sprite.scaled_rect);
                        break;
                    case Collisions::MULTI_CIRCLE:
                        FunctionsLib::DrawCirclesCluster(renderer, sprite);
                        break;
                }
            }
        }
        else {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER,"Error rendering Texture: %s", SDL_GetError());
        }
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER,"Error loading Texture: %s", sprite.filename.c_str());
    }
}
// -------------------------------------------------------------------------------------------------------------
