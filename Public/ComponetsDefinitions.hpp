//
// Created by ymod1 on 07/05/2026.
//

#ifndef YMODECS_COMPONETSDEFINITIONS_HPP
#define YMODECS_COMPONETSDEFINITIONS_HPP

#include <string>
#include <vector>

#include "ecs.hpp"
#include "Utilities.hpp"

using namespace Utils;

struct NoMultithreading {
};

struct Actor{};
struct StaticObject {};
struct Character {};
struct Player {};
struct Enemy {};
struct UI {};
struct Template {};

struct Position {

    Vector2D pos;
    Vector2D old_pos;

    Position() {};
    Position(Vector2D start_pos) : pos(start_pos), old_pos(start_pos){ }
};

struct Size {
    Vector2D scale;

    void CopyData(const Size *in_size) {
        if (in_size == nullptr) return;
        scale = in_size->scale;
    }
};

struct Velocity {
    Vector2D vel;
    Vector2D max_vel;
    Vector2D acceleration;

    void CopyData(const Velocity *in_vel) {
        if (in_vel == nullptr) return;
        vel = in_vel->vel;
        max_vel = in_vel->max_vel;
        acceleration = in_vel->acceleration;
    }
};

struct Health {
    int hp=1, max_hp=1;
    bool IsAlive() const { return hp > 0; }
};

struct Name {
    std::string name;
};

struct Visibility {
    bool is_visible = true;
};

struct Sprite {
    std::string filename;
    SDL_FRect rect;
    SDL_FRect scaled_rect;
    Vector2D center;
    float angle=0.0f;

    // Collisions
    float bounding_radius;
    Collisions::CollisionType collision_type = Collisions::NONE;
    bool overlaps_only = false;
    bool can_push = false;
    bool is_static_obstacle = false;
    std::vector<ecs::EntityID> possible_collision_entities;
    bool draw_debug_shapes = false;

    // Custom deleters per SDL
    struct SDLTextureDeleter {
        void operator()(SDL_Texture* t) const { if (t) SDL_DestroyTexture(t); }
    };
    struct SDLSurfaceDeleter {
        void operator()(SDL_Surface* s) const { if (s) SDL_DestroySurface(s); }
    };

    std::unique_ptr<SDL_Texture, SDLTextureDeleter> texture;
    std::unique_ptr<SDL_Surface, SDLSurfaceDeleter> surface;

    std::vector<Collisions::LocalCircle> localColliderCluster;

    Sprite() {}

    Sprite( std::string inFilename) : filename(inFilename) {

    }

    Sprite( std::string in_filename,  Collisions::CollisionType in_collision_type) : filename(in_filename), collision_type(in_collision_type) {

    }

    void CopyData(const Sprite *in_sprite) {
        if (in_sprite == nullptr) return;

        filename = in_sprite->filename;
        collision_type = in_sprite->collision_type;
        overlaps_only = in_sprite->overlaps_only;
        can_push = in_sprite->can_push;
        is_static_obstacle = in_sprite->is_static_obstacle;
        draw_debug_shapes = in_sprite->draw_debug_shapes;
    }
};


struct Bullet {
    env::BulletType bullet_type = env::BulletType::NONE;
    float speed = 0;
    int damage = 1;
    float area_radius = 0;
    int area_damage = 0;
    float lifespan = 1.0f;
    float lifetimer = 0;
    std::uint32_t owner_id = 0;

    void CopyData(const Bullet *in_bullet) {
        if (in_bullet == nullptr) return;
        bullet_type = in_bullet->bullet_type;
        speed = in_bullet->speed;
        damage = in_bullet->damage;
        area_radius = in_bullet->area_radius;
        area_damage = in_bullet->area_damage;
        lifespan = in_bullet->lifespan;
        lifetimer = in_bullet->lifetimer;
        owner_id = in_bullet->owner_id;
    }

};

struct GameCursor
{
    Vector2D hotspot;
    bool is_enabled;
};

#endif //YMODECS_COMPONETSDEFINITIONS_HPP
