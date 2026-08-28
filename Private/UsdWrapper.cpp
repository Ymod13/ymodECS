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
        if (!HandleEntityType(world, e, prim, entity_type)) {
            std::cerr << "[Error] Impossible to determine entity type for'" << entity_type << "'! check USD file definitions!" << std::endl;
        }
        else {
            // we can on with attributes reading

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

            UsdWrapper::LoadComponentPrim<Bullet>(world, e, prim, "Bullet",
            FieldPack<Bullet, std::string, env::BulletType>{"bullet_type", &Bullet::bullet_type},
            FieldPack<Bullet, float, float>{"speed", &Bullet::speed},
            FieldPack<Bullet, int, int>{"damage", &Bullet::damage},
            FieldPack<Bullet, float, float>{"area_radius", &Bullet::area_radius},
            FieldPack<Bullet, int, int>{"area_damage", &Bullet::area_damage},
            FieldPack<Bullet, float, float>{"lifespan", &Bullet::lifespan});

            UsdWrapper::LoadComponentPrim<GameCursor>(world, e, prim, "GameCursor",
            FieldPack<GameCursor, GfVec2f, Vector2D>{"hotspot", &GameCursor::hotspot},
            FieldPack<GameCursor, bool, bool>{"is_enabled", &GameCursor::is_enabled});
        }
    }

    std::cout << "=== USD Loader: completato ===\n";

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------

bool UsdWrapper::HandleEntityType(ecs::World &world, const ecs::EntityID &e, const UsdPrim &prim, std::string &out_entity_type) {
    bool res = true;

    if (GetAttr<std::string>(prim, "entity_type",  out_entity_type)) {
        if (out_entity_type == "actor") {
            world.add(e, Actor{});
        } else  if (out_entity_type == "static_object") {
            world.add(e, Actor{});
            world.add(e, StaticObject{});
        } else  if (out_entity_type == "character") {
            world.add(e, Actor{});
            world.add(e, Character{});
        } else if (out_entity_type == "player") {
            world.add(e, Actor{});
            world.add(e, Character{});
            world.add(e, Player{});
            // save player id for future usage
            env::player_id = e;
        } else if (out_entity_type == "enemy") {
            world.add(e, Actor{});
            world.add(e, Character{});
            world.add(e, Enemy{});
        } else if (out_entity_type == "game_cursor") {
            world.add(e, Actor{});
            world.add(e, UI{});
        } else if (out_entity_type == "bullet") {
            // TODO:
        }
        else {
            res = false;
        }
    }
    else {
        res = false;
    }

    return res;
}
