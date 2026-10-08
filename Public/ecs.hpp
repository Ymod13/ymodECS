//
// Created by ymod1 on 07/05/2026.
//

#ifndef YMODECS_ECS_HPP
#define YMODECS_ECS_HPP

#pragma once
// ============================================================
//  ECS — Entity Component System  (C++20, header-only)
// ============================================================
#include <algorithm>
#include <any>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Environments.hpp"

namespace ecs {

// ─── Configuration ──────────────────────────────────────────
    static constexpr std::size_t MAX_COMPONENTS = 64;
    static constexpr std::uint32_t INVALID_ENTITY = std::numeric_limits<std::uint32_t>::max();


// ─── Types ───────────────────────────────────────────────────
using EntityID    = std::uint32_t;
using ComponentID = std::uint8_t;
    /*
     *  Signature is a N-bit mask: which components an entity owns,
     *  or which components a query requires/excludes.
     */
using Signature   = std::bitset<MAX_COMPONENTS>;

static constexpr EntityID NULL_ENTITY = 0;

struct RenderableEntry {
    EntityID entity;
    int layer;
    float depth;
};

// ─── Component registry ──────────────────────────────────────
// Assigns a unique small integer ID to each component type at runtime.
// The ID is also the index of the type's pool inside World::pools_.
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

// ─── Type-erased pool interface ──────────────────────────────
// Lets World::destroy() clean up every pool without knowing T
// (replaces the old erasers_ map of std::function).
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID e) = 0;
};

// ─── Sparse-set component store ──────────────────────────────
// sparse_ : EntityID -> index in the dense arrays (plain vector, O(1), no hashing)
// dense_* : tightly packed entities / components, cache-friendly iteration
//
// Note: sparse_ grows up to (highest EntityID that ever got this component + 1) * 4 bytes.
template<typename T>
class ComponentPool final : public IComponentPool {
public:
    static constexpr std::uint32_t NPOS = std::numeric_limits<std::uint32_t>::max();

    void insert(EntityID e, T component) {
        if (e >= sparse_.size())
            sparse_.resize(static_cast<std::size_t>(e) + 1, NPOS);
        assert(sparse_[e] == NPOS && "Entity already has this component");
        sparse_[e] = static_cast<std::uint32_t>(dense_components_.size());
        dense_entities_.push_back(e);
        dense_components_.push_back(std::move(component));
    }

    void remove(EntityID e) override {
        if (!has(e)) return;
        const std::uint32_t idx  = sparse_[e];
        const std::uint32_t last = static_cast<std::uint32_t>(dense_components_.size()) - 1;
        if (idx != last) {
            dense_components_[idx] = std::move(dense_components_[last]);
            dense_entities_[idx]   = dense_entities_[last];
            sparse_[dense_entities_[idx]] = idx;
        }
        dense_components_.pop_back();
        dense_entities_.pop_back();
        sparse_[e] = NPOS;
    }

    // Caller must be sure the entity has the component (asserted in Debug).
    T& get(EntityID e) {
        assert(has(e) && "Entity does not have this component");
        return dense_components_[sparse_[e]];
    }

    // Safe variant: nullptr when the entity does not have the component.
    T* try_get(EntityID e) {
        return has(e) ? &dense_components_[sparse_[e]] : nullptr;
    }

    bool has(EntityID e) const {
        return e < sparse_.size() && sparse_[e] != NPOS;
    }

    const std::vector<EntityID>& entities() const { return dense_entities_; }
    std::vector<T>& components() { return dense_components_; }
    std::size_t size() const { return dense_entities_.size(); }

private:
    std::vector<std::uint32_t> sparse_;            // EntityID -> dense index (or NPOS)
    std::vector<EntityID>      dense_entities_;    // dense index -> EntityID
    std::vector<T>             dense_components_;  // dense index -> component
};



// ─── World ───────────────────────────────────────────────────
// The central registry: creates entities, stores components, runs queries.
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
        const EntityID id = ++next_entity_;
        if (id >= signatures_.size()) {
            signatures_.resize(static_cast<std::size_t>(id) + 1);
            alive_flags_.resize(static_cast<std::size_t>(id) + 1, 0);
        }
        signatures_[id].reset();
        alive_flags_[id] = 1;
        ++alive_count_;
        return id;
    }

    void destroy(EntityID e) {
        if (!alive(e)) return;

        if (has_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>()) {
            auto& renderables_by_layer = get_resource<std::map<UserInterface::LayerType, std::vector<ecs::RenderableEntry>>>();

            for (auto& [layer, renderable_entries] : renderables_by_layer) {
                auto it = std::find_if(renderable_entries.begin(), renderable_entries.end(), [e](const RenderableEntry& entry) {
                   return entry.entity == e;
                });

                if (it != renderable_entries.end()) {
                    renderable_entries.erase(it);
                }
            }
        }

        for (auto& pool : pools_) {
            if (pool) pool->remove(e);   // remove all components
        }
        signatures_[e].reset();
        alive_flags_[e] = 0;
        --alive_count_;
    }

    bool alive(EntityID e) const {
        return e < alive_flags_.size() && alive_flags_[e] != 0;
    }

    // ── Component management ─────────────────────────────────
    template<typename T>
    void add(EntityID e, T component) {
        assert(alive(e) && "add() on an entity that is not alive");
        GetPool<T>().insert(e, std::move(component));
        signatures_[e].set(ComponentRegistry::GetId<T>());
    }

    template<typename T>
    void remove(EntityID e) {
        assert(alive(e) && "remove() on an entity that is not alive");
        GetPool<T>().remove(e);
        signatures_[e].reset(ComponentRegistry::GetId<T>());
    }

    // Fast path: the entity MUST have T (asserted in Debug).
    template<typename T>
    T& get(EntityID e) {
        return GetPool<T>().get(e);
    }

    // Safe path: nullptr when the entity does not have T.
    template<typename T>
    T* try_get(EntityID e) {
        return GetPool<T>().try_get(e);
    }

    template<typename T>
    bool has(EntityID e) const {
        if (e >= signatures_.size()) return false;
        return signatures_[e].test(ComponentRegistry::GetId<T>());
    }

    // Returns the signature (component bitmask) of an entity
    const Signature& signature(EntityID e) const {
        return signatures_.at(e);
    }

    template<typename... Ts>
    struct Exclude {};

    // Iterate over entities with a given component set, calling a function. With optional exclusion list.
    // Cost is proportional to the SMALLEST pool among the included components, not to the total
    // number of entities in the world.
    //
    // Rules while iterating: do not destroy entities or remove components of the iterated types
    // from inside the callback (collect them and do it afterwards).
    template<typename... Includes, typename... Excludes, typename Fn>
    void each(Fn&& fn, Exclude<Excludes...> = {}) {
        static_assert(sizeof...(Includes) > 0, "each<>() needs at least one component type");

        // Included components signatures
        Signature required;
        (required.set(ComponentRegistry::GetId<Includes>()), ...);

        // Excluded components signatures
        Signature excluded;
        if constexpr (sizeof...(Excludes) > 0)
            (excluded.set(ComponentRegistry::GetId<Excludes>()), ...);

        // Pick the smallest pool to drive the iteration
        const std::vector<EntityID>* driver = nullptr;
        ((driver = PickSmaller(driver, GetPool<Includes>().entities())), ...);

        // Index-based: stays valid even if the callback adds components (vector growth)
        for (std::size_t i = 0; i < driver->size(); ++i) {
            const EntityID e = (*driver)[i];
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

    // Returns all entities that have ALL of the listed component types.
    // (order follows the smallest pool, not creation order)
    template<typename... Ts>
    std::vector<EntityID> query() {
        std::vector<EntityID> result;
        each<Ts...>([&](EntityID e, Ts&...) { result.push_back(e); });
        return result;
    }

    // Counts all the alive entities
    std::size_t entity_count() const { return alive_count_; }

    // Counts entities with all listed components and no EXCLUDES
    template<typename... Includes, typename... Excludes>
    std::size_t count(Exclude<Excludes...> = {}) {
        std::size_t result = 0;
        each<Includes...>([&](EntityID, Includes&...) { ++result; }, Exclude<Excludes...>{});
        return result;
    }

private:
    // One slot per ComponentID: direct array access, no hashing, no shared_ptr refcount.
    template<typename T>
    ComponentPool<T>& GetPool() {
        auto& slot = pools_[ComponentRegistry::GetId<T>()];
        if (!slot) slot = std::make_unique<ComponentPool<T>>();
        return static_cast<ComponentPool<T>&>(*slot);
    }

    static const std::vector<EntityID>* PickSmaller(const std::vector<EntityID>* a,
                                                    const std::vector<EntityID>& b) {
        return (!a || b.size() < a->size()) ? &b : a;
    }

    EntityID resource_entity_ = NULL_ENTITY;
    EntityID next_entity_ = NULL_ENTITY;
    std::size_t alive_count_ = 0;

    // Indexed by EntityID (IDs are sequential and never reused)
    std::vector<Signature>     signatures_;
    std::vector<std::uint8_t>  alive_flags_;

    std::array<std::unique_ptr<IComponentPool>, MAX_COMPONENTS> pools_;
};

} // namespace ecs

#endif //YMODECS_ECS_HPP
