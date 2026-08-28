//
// Created by ymod1 on 29/05/2026.
//

#include "UsdWrapper.hpp"

#include <pxr/usd/usd/stage.h>
#include <iostream>

#include "ComponetsDefinitions.hpp"
#include "Environments.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

bool UsdWrapper::LoadUsdFile(const std::string &filepath, ecs::World &world) {
    std::cout << "\n=== USD Loader: " << filepath << " ===\n";

    UsdStageRefPtr stage = UsdStage::Open(filepath);
    if (!stage) {
        std::cerr << "FAIL: impossibile aprire " << filepath << "\n";
        return false;
    }
    else {
        std::cout << "file aperto correttamente! -> " << filepath << "\n";
    }

    UsdPrim scene = stage->GetPrimAtPath(SdfPath("/Scene"));
    if (!scene) {
        std::cerr << "FAIL: prim /Scene non trovato\n";
        return false;
    }

    for (const UsdPrim& prim : scene.GetChildren()) {

        std::string primName = prim.GetName().GetString();
        std::cout << "\nLoading prim: " << primName << "\n";

        // Create the entity in game world
        ecs::EntityID e = world.create();

        // entity_type reading
        std::string entity_type;
        if (GetAttr<std::string>(prim, "entity_type",  entity_type)) {

            if (entity_type == "Actor") {
                world.add(e, Actor{});
            } else  if (entity_type == "static_object") {
                world.add(e, Actor{});
                world.add(e, StaticObject{});
            } else  if (entity_type == "character") {
                world.add(e, Actor{});
                world.add(e, Character{});
            } else if (entity_type == "player") {
                world.add(e, Actor{});
                world.add(e, Character{});
                world.add(e, Player{});
                // save player id for future usage
                env::player_id = e;
            } else if (entity_type == "enemy") {
                world.add(e, Actor{});
                world.add(e, Character{});
                world.add(e, Enemy{});
            } else if (entity_type == "game_cursor") {
                world.add(e, Actor{});
                world.add(e, UI{});
            } else if (entity_type == "bullet") {
                // TODO:
            }
        }

        // attributes reading
        //--------------------

        UsdWrapper::LoadComponentPrim<Template>(world, e, prim, "Template");

        UsdWrapper::LoadComponentPrim<Name>(world, e, prim, "Name",
          FieldPack<Name, std::string, std::string>{"name", &Name::name});

        UsdWrapper::LoadComponentPrim<Position>(world, e, prim, "Position",
           FieldPack<Position, GfVec2f, Vector2D>{"pos", &Position::pos});

        UsdWrapper::LoadComponentPrim<Size>(world, e, prim, "Size",
           FieldPack<Size, GfVec2f, Vector2D>{"scale", &Size::scale});

        UsdWrapper::LoadComponentPrim<Visibility>(world, e, prim, "Visibility",
          FieldPack<Visibility, bool, bool>{"is_visible", &Visibility::is_visible});

        UsdWrapper::LoadComponentPrim<Velocity>(world, e, prim, "Velocity",
           FieldPack<Velocity, GfVec2f, Vector2D>{"vel", &Velocity::vel},
           FieldPack<Velocity, GfVec2f, Vector2D>{"max_vel", &Velocity::max_vel},
           FieldPack<Velocity, GfVec2f, Vector2D>{"acceleration", &Velocity::acceleration});

        UsdWrapper::LoadComponentPrim<Health>(world, e, prim, "Health",
         FieldPack<Health, int, int>{"hp", &Health::hp},
         FieldPack<Health, int, int>{"max_hp", &Health::max_hp});

        UsdWrapper::LoadComponentPrim<Sprite>(world, e, prim, "Sprite",
        FieldPack<Sprite, std::string, std::string>{"filename", &Sprite::filename},
        FieldPack<Sprite, std::string, Collisions::CollisionType>{"collision_type", &Sprite::collision_type},
        FieldPack<Sprite, bool, bool>{"can_push", &Sprite::can_push},
        FieldPack<Sprite, bool, bool>{"is_static_obstacle", &Sprite::is_static_obstacle},
        FieldPack<Sprite, bool, bool>{"overlaps_only", &Sprite::overlaps_only},
        FieldPack<Sprite, bool, bool>{"draw_debug_shapes", &Sprite::draw_debug_shapes});

        // Bullet
        UsdPrim bullet_prim = prim.GetChild(TfToken("Bullet"));
        if (bullet_prim) {
            Bullet bullet;

            std::string bullet_type;
            if (GetAttr<std::string>(bullet_prim, "bullet_type",  bullet_type)) {

                if      (bullet_type == "PISTOL")   bullet.bullet_type = env::PISTOL;
                else if (bullet_type == "SHOTGUN")  bullet.bullet_type = env::SHOTGUN;
                else if (bullet_type == "ROCKET")   bullet.bullet_type = env::ROCKET;
                else if (bullet_type == "GRENADE")  bullet.bullet_type = env::GRENADE;

            }

            float speed;
            if (GetAttr<float>(bullet_prim, "speed",  speed)) {
                bullet.speed = speed;
            }
            
            int damage;
            if (GetAttr<int>(bullet_prim, "damage",  damage)) {
                bullet.damage = damage;
            }

            float area_radius;
            if (GetAttr<float>(bullet_prim, "area_radius",  area_radius)) {
                bullet.area_radius = area_radius;
            }

            int area_damage;
            if (GetAttr<int>(bullet_prim, "area_damage",  area_damage)) {
                bullet.area_damage = area_damage;
            }

            float lifespan;
            if (GetAttr<float>(bullet_prim, "lifespan",  lifespan)) {
                bullet.lifespan = lifespan;
            }

            world.add(e, std::move(bullet));
        }

        // Game Cursor
        UsdPrim game_cursor_prim = prim.GetChild(TfToken("GameCursor"));
        if (game_cursor_prim) {
            GameCursor game_cursor;

            GfVec2f hotspot;
            if (GetAttr<GfVec2f>(game_cursor_prim, "hotspot",  hotspot)) {
                game_cursor.hotspot = hotspot;
            }

            bool is_enabled;
            if (GetAttr<bool>(game_cursor_prim, "is_enabled",  is_enabled)) {
                game_cursor.is_enabled = is_enabled;
            }

            world.add(e, std::move(game_cursor));

        }
    }

    std::cout << "=== USD Loader: completato ===\n";

    return true;
}
