//
// Created by ymod1 on 14/09/2026.
//

#ifndef YMODECS_ECSINSPECTOR_HPP
#define YMODECS_ECSINSPECTOR_HPP

#pragma once
#include "imgui.h"
#include "Environments.hpp"
#include "ecs.hpp"
#include "Utilities.hpp"

class EcsInspector {
public:
    void Draw(ecs::World& world) {
        if (!ImGui::Begin("ECS Inspector")) {
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("InspectorTabs")) {

            if (ImGui::BeginTabItem("Entities")) {
                DrawEntitiesTab(world);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Stats")) {
                DrawStatsTab(world);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

private:
    ecs::EntityID m_selected = ecs::INVALID_ENTITY; // adatta al tuo tipo/valore invalido

    void DrawEntitiesTab(ecs::World& world) {
        auto& renderables = world.get_resource<std::vector<ecs::RenderableEntry>>();

        ImGui::Text("Renderable entities: %zu", renderables.size());
        ImGui::Separator();

        ImGui::BeginChild("EntityList", ImVec2(160, 0), true);
        for (auto& r : renderables) {
            bool selected = (r.entity == m_selected);
            std::string label = "Entity " + std::to_string(r.entity);

            // Se hai un componente Name, mostralo invece del solo ID
            if (world.has<Name>(r.entity)) {
                label += " (" + world.get<Name>(r.entity).name + ")";
            }

            if (ImGui::Selectable(label.c_str(), selected))
                m_selected = r.entity;
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ComponentDetail", ImVec2(0, 0), true);
        if (m_selected != ecs::INVALID_ENTITY && world.has<Position>(m_selected)) {
            DrawComponents(world, m_selected);
        } else {
            ImGui::TextDisabled("Select an entity");
        }
        ImGui::EndChild();
    }

    void DrawComponents(ecs::World& world, ecs::EntityID e) {
        if (world.has<Position>(e)) {
            auto& pos = world.get<Position>(e);
            if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen)) {
                float v[2] = {pos.pos.x, pos.pos.y};
                ImGui::DragFloat2("xy", v);
            }
        }
        if (world.has<Sprite>(e)) {
            auto& sprite = world.get<Sprite>(e);
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Sprite: %s", sprite.filename.c_str());
                std::string coll_type = Utils::FunctionsLib::EnumToString<Collisions::CollisionType>(sprite.collision_type);
                ImGui::Text("Collision Type: %s", coll_type.c_str());
            }
        }
        if (world.has<Visibility>(e)) {
            auto& vis = world.get<Visibility>(e);
            ImGui::Checkbox("Visible", &vis.is_visible);
        }
        if (world.has<Player>(e)) ImGui::TextColored({0,1,0,1}, "[Player]");
        if (world.has<Enemy>(e))  ImGui::TextColored({1,0,0,1}, "[Enemy]");
    }

    void DrawStatsTab(ecs::World& world) {
        auto& stats = world.get_resource<env::Stats>();

        ImGui::Text("Mouse screen pos: (%.1f, %.1f)", stats.mouse_screen_pos.x, stats.mouse_screen_pos.y);

        ImGui::Text("Frame time: %.3f ms", ImGui::GetIO().DeltaTime * 1000.0f);
        ImGui::Text("FPS (ImGui): %.1f", ImGui::GetIO().Framerate);
    }
};

#endif //YMODECS_ECSINSPECTOR_HPP