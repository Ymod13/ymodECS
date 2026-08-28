//
// Created by ymod1 on 07/05/2026.
//

#ifndef YMODECS_ECS_HPP
#define YMODECS_ECS_HPP

#pragma once
// ============================================================
//  ECS — Entity Component System  (C++17, header-only)
// ============================================================
#include <algorithm>
#include <any>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ecs {

// ─── Configuration ──────────────────────────────────────────
static constexpr std::size_t MAX_COMPONENTS = 64;

// ─── Types ───────────────────────────────────────────────────
using EntityID    = std::uint32_t;
using ComponentID = std::uint8_t;
    /*
    *  Signature it's a N bits array
    *  The Signature is a bit mask representing which components are owned by an entity, or which components a system requires
       Components:   Health Transform Velocity Sprite  ...
       ID:              0       1        2       3

       Entity A:     [  1       1        0       0  ]  → has Health and Transform
       Entity B:     [  0       1        1       1  ]  → has Transform, Velocity, Sprite

       rendering system requires: [  0   1   0   1  ]  → Transform + Sprite

       Entity A & System: [ 0  1  0  0 ] ≠ System  → NOT processed
       Entity B & System: [ 0  1  0  1 ] == System → processed

       */
using Signature   = std::bitset<MAX_COMPONENTS>;


static constexpr EntityID NULL_ENTITY = 0;

// ─── Component registry ──────────────────────────────────────
// Assigns a unique integer ID to each component type at runtime.
class ComponentRegistry {
public:
    template<typename T>
    static ComponentID GetId() {
        static ComponentID cid = next_id++;
        assert(cid < MAX_COMPONENTS && "Too many component types!");
        return cid;
    }
private:
    inline static ComponentID next_id = 0;
};

// ─── Sparse-set component store ──────────────────────────────
// Maps EntityID → component value in a cache-friendly dense array.
// Iteration over all components is O(n) with good locality.
template<typename T>
class ComponentPool {
public:
    void insert(EntityID e, T component) {
        assert(SparseAllComponents.find(e) == SparseAllComponents.end() && "Entity already has this component");
        SparseAllComponents[e] = static_cast<uint32_t>(DenseAllComponents.size());
        DenseAllEntities.push_back(e);
        DenseAllComponents.push_back(std::move(component));
    }

    void remove(EntityID e) {
        auto it = SparseAllComponents.find(e);
        if (it == SparseAllComponents.end()) return;
        uint32_t idx = it->second;
        uint32_t last = static_cast<uint32_t>(DenseAllComponents.size()) - 1;
        if (idx != last) {
            DenseAllComponents[idx] = std::move(DenseAllComponents[last]);
            DenseAllEntities[idx] = DenseAllEntities[last];
            SparseAllComponents[DenseAllEntities[idx]] = idx;
        }
        DenseAllComponents.pop_back();
        DenseAllEntities.pop_back();
        SparseAllComponents.erase(it);
    }

    T& get(EntityID e) {
        return DenseAllComponents[SparseAllComponents.at(e)];
    }

    bool has(EntityID e) const {
        return SparseAllComponents.contains(e);
    }

    // Iterate over all components (great for systems)
    const std::vector<EntityID>& entities() const { return DenseAllEntities; }
    std::vector<T>& components() { return DenseAllComponents; }

private:
    // EntityID -> index of the component in DenseAllComponents
    std::unordered_map<EntityID, uint32_t> SparseAllComponents;
    // All entities
    std::vector<EntityID>                  DenseAllEntities;
    // All components
    std::vector<T>                         DenseAllComponents;
};



// ─── World ───────────────────────────────────────────────────
// The central registry: creates entities, stores components, runs systems.
class World {
public:
    // Resources

    template<typename T>
    void add_resource(T resource) {
        if (resource_entity_ == NULL_ENTITY)
            resource_entity_ = create();
        add(resource_entity_, std::move(resource));
    }

    template<typename T>
    T& get_resource() {
        return get<T>(resource_entity_);
    }

    template<typename T>
    bool has_resource() const {
        return resource_entity_ != NULL_ENTITY
            && has<T>(resource_entity_);
    }


    // ── Entity management ────────────────────────────────────
    EntityID create() {
        EntityID id = ++next_entity_;
        signatures_[id] = {};
        alive_.push_back(id);
        return id;
    }

    void destroy(EntityID e) {
        for (auto& [tid, eraser] : erasers_) {
            eraser(e);  // remove all components
        }
        signatures_.erase(e);
        alive_.erase(std::remove(alive_.begin(), alive_.end(), e), alive_.end());
    }

    bool alive(EntityID e) const {
        return signatures_.count(e) > 0;
    }

    // ── Component management ─────────────────────────────────
    template<typename T>
    void add(EntityID e, T component) {
        GetPool<T>().insert(e, std::move(component));
        signatures_[e].set(ComponentRegistry::GetId<T>());
    }

    template<typename T>
    void remove(EntityID e) {
        GetPool<T>().remove(e);
        signatures_[e].reset(ComponentRegistry::GetId<T>());
    }

    template<typename T>
    T& get(EntityID e) {
        return GetPool<T>().get(e);
    }

    template<typename T>
    bool has(EntityID e) const {
        auto it = signatures_.find(e);
        if (it == signatures_.end()) return false;
        return it->second.test(ComponentRegistry::GetId<T>());
    }

    // Returns the signature (component bitmask) of an entity
    const Signature& signature(EntityID e) const {
        return signatures_.at(e);
    }

    // ── Query helpers ─────────────────────────────────────────
    // Returns all entities that have ALL of the listed component types.
    template<typename... Ts>
    std::vector<EntityID> query() {
        Signature required;
        (required.set(ComponentRegistry::GetId<Ts>()), ...);

        std::vector<EntityID> result;
        for (EntityID e : alive_) {
            if ((signatures_[e] & required) == required) {
                result.push_back(e);
            }
        }
        return result;
    }

    template<typename... Ts>
    struct Exclude {};

    // Iterate over entities with a given component set, calling a function. With optional exclusion list
    template<typename... Includes, typename... Excludes, typename Fn>
    void each(Fn&& fn, Exclude<Excludes...> = {}) {
        // Included components signatures
        Signature required;
        (required.set(ComponentRegistry::GetId<Includes>()), ...);

        // Excluded components signatures
        Signature excluded;
        if constexpr (sizeof...(Excludes) > 0)
            (excluded.set(ComponentRegistry::GetId<Excludes>()), ...);

        for (EntityID e : alive_) {
            const Signature& sig = signatures_[e];

            // MUST have all required components
            if ((sig & required) != required) continue;

            // MUST NOT have excluded components
            if constexpr (sizeof...(Excludes) > 0)
                if ((sig & excluded).any()) continue;

            if constexpr (std::is_same_v<std::invoke_result_t<Fn, EntityID, Includes&...>, bool>) {
                if (!fn(e, get<Includes>(e)...)) return;
            } else {
                fn(e, get<Includes>(e)...);
            }
        }
    }

    std::size_t entity_count() const { return alive_.size(); }

private:
    template<typename T>
    ComponentPool<T>& GetPool() {
        auto tid = std::type_index(typeid(T));
        if (!pools_.contains(tid)) {
            pools_[tid] = std::make_shared<ComponentPool<T>>();

            // Register an eraser so destroy() can clean up without knowing T
            erasers_[tid] = [this](EntityID e) {
                GetPool<T>().remove(e);
            };
        }
        return *std::static_pointer_cast<ComponentPool<T>>(pools_.at(tid));
    }

    EntityID resource_entity_ = NULL_ENTITY;
    EntityID next_entity_ = NULL_ENTITY;
    std::vector<EntityID> alive_;
    std::unordered_map<EntityID, Signature> signatures_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> pools_;
    std::unordered_map<std::type_index, std::function<void(EntityID)>> erasers_;
};

} // namespace ecs

#endif //YMODECS_ECS_HPP
