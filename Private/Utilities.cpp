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

void FunctionsLib::UpdatePosition(const Vector2D &new_pos, Position &pos, Sprite &sprite, const Vector2D &scale, bool clamp_to_screen) {
    pos.old_pos = pos.pos;
    pos.pos = new_pos;

    // Clamp position to screen limits
    clamp_screen_position(pos.pos, scale, sprite.texture.get());

    sprite.rect.x = sprite.scaled_rect.x = pos.pos.x;
    sprite.rect.y = sprite.scaled_rect.y = pos.pos.y;

    sprite.scaled_rect.w = sprite.rect.w * scale.x;
    sprite.scaled_rect.h = sprite.rect.h * scale.y;

    sprite.center.x = pos.pos.x + sprite.scaled_rect.w / 2;
    sprite.center.y = pos.pos.y + sprite.scaled_rect.h / 2;

    sprite.bounding_radius = Utils::FunctionsLib::CalculateRectRadius(sprite.scaled_rect);
}

void FunctionsLib::RestoreOldPosition(Position &pos, Sprite &sprite, const Vector2D &scale) {
    UpdatePosition(pos.old_pos, pos, sprite, scale, true);
}

//-------------------------------------------------------------------------------------------------------------

void FunctionsLib::Keyboard_vel_axis_movement(const SDL_Scancode dir_1_key, const SDL_Scancode dir_2_key, const bool* keys, float &vel, const float &acceleration, const float &max_vel, const float &dt)
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
            vel = std::clamp(vel += acceleration*dt, -max_vel, 0.0f);
        } else if (vel > 0) {
            vel = std::clamp(vel -= acceleration*dt, 0.0f, max_vel);
        }
        else {
            vel = 0;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------

bool FunctionsLib::clamp_screen_position(Vector2D &pos, const Vector2D &scale, const SDL_Texture *SpriteTexture) {
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
        if (pos.x + SpriteTexture->w * scale.x > env::screen_width) {
            pos.x = env::screen_width - SpriteTexture->w * scale.x;
            clamped = true;
        }

        if (pos.y + SpriteTexture->h * scale.y > env::screen_height) {
            pos.y = env::screen_height - SpriteTexture->h * scale.y;
            clamped = true;
        }
    }

    return clamped;
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

bool FunctionsLib::LoadSprite(SDL_Renderer* renderer, const Size& size, const Position &pos, Sprite& out_sprite) {
    const std::string filename = env::sprites_folder + out_sprite.filename;

    switch (out_sprite.collision_type) {

        case Collisions::NONE:
        case Collisions::RADIUS:
        case Collisions::RECTANGLE:

            if ( SDL_Texture* spriteTexture = IMG_LoadTexture(renderer, filename.c_str())) {
                out_sprite.texture.reset(spriteTexture);
                out_sprite.rect.w = spriteTexture->w;
                out_sprite.rect.h = spriteTexture->h;
                out_sprite.scaled_rect.w = spriteTexture->w * size.scale.x;
                out_sprite.scaled_rect.h = spriteTexture->h * size.scale.y;
            }
            else {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER,"Error loading Texture: %s (%s)", SDL_GetError(), filename.c_str());
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
                    out_sprite.texture.reset(surfaceTexture);
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_RENDER,"Error loading Texture: %s (%s)", SDL_GetError(), filename.c_str());
                    return false;
                }
            }
            else {
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error loading surface %s (%s)", SDL_GetError(), filename.c_str());
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

    world.each<Template, Bullet, Sprite, Size, Velocity>([&](ecs::EntityID id, Template &template_comp, Bullet& bullet, Sprite &sprite, Size &size, Velocity &velocity) {

        if (bullet_type == bullet.bullet_type) {

            bullet_sprite.CopyData(&sprite);
            bullet_size.CopyData(&size);
            bullet_velocity.CopyData(&velocity);
            new_bullet.CopyData(&bullet);
            return false; // exits each loop
        }

        return true;

    });

    new_bullet.owner_id = owner_id;

    if (LoadSprite(ctx.renderer, bullet_size, start_pos, bullet_sprite)) {
        world.add(e, std::move(new_bullet));
        world.add(e, std::move(bullet_sprite));
        world.add(e, std::move(bullet_size));

        Vector2D bullet_dir = end_pos - start_pos;
        bullet_dir.normalize();

        bullet_velocity.vel = bullet_dir * new_bullet.speed;;

        world.add(e, std::move(bullet_velocity));
    }
    else {
        std::cout << "Error loading bullet sprite" << std::endl;
    }



}
// -------------------------------------------------------------------------------------------------------------

void FunctionsLib::DrawCircle(SDL_Renderer* renderer, Vector2D &center, float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {

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