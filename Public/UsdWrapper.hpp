//
// Created by ymod1 on 29/05/2026.
//

#ifndef YMODECS_USDWRAPPER_HPP
#define YMODECS_USDWRAPPER_HPP
#include <iostream>
#include <string>

#include <pxr/usd/usd/attribute.h>
#include "ecs.hpp"
#include "Environments.hpp"

#endif //YMODECS_USDWRAPPER_HPP

PXR_NAMESPACE_USING_DIRECTIVE

namespace ecs {
    class World;
}

template <typename ComponentType, typename FieldType, typename ComponentFieldType>
struct FieldPack {
    using Owner = ComponentType;
    using Type  = ComponentFieldType;

    std::string name;
    ComponentFieldType ComponentType::* member;
    FieldType value{};
};

//template<typename ComponentType, typename FieldType>
//FieldPack(std::string, FieldType ComponentType::*) -> FieldPack<ComponentType, FieldType>;

template<typename T>
struct is_field_pack : std::false_type {};

template<typename C, typename F, typename CF>
struct is_field_pack<FieldPack<C, F, CF>> : std::true_type {};

template<typename T>
inline constexpr bool is_field_pack_v = is_field_pack<T>::value;

class UsdWrapper {
public:

    template<typename T>
    static bool GetAttr(const UsdPrim& prim, const std::string& name, T &out_value ) {
        if (UsdAttribute attr = prim.GetAttribute(TfToken(name))) {
            attr.Get(&out_value);
            std::cout << "     " << prim.GetName().GetString() << " -> " << name << " = " << out_value << "\n";
            return true;
        }

        return false;
    }

    template<typename ComponentType, typename... Fields>
    static void LoadComponentPrim(ecs::World &in_world, const ecs::EntityID &entity_id, const UsdPrim& root_prim, const std::string &prim_name, Fields&& ... fields) {
        static_assert((is_field_pack_v<Fields> && ...),  "All fields MUST be FieldPack<T>");
        static_assert((std::is_same_v<typename Fields::Owner, ComponentType> && ...), "FieldPack owner non corrisponde a ComponentType");

        UsdPrim child_prim = root_prim.GetChild(TfToken(prim_name));
        if (!child_prim) return;

        ComponentType component;

        ([&](auto& field) {
            using ValueType = decltype(field.value);
                if (GetAttr<ValueType>(child_prim, field.name, field.value)) {
                    if constexpr (std::is_same_v<typename Fields::Type, Collisions::CollisionType> &&  std::is_same_v<ValueType, std::string>) {
                        Collisions::CollisionType coll_type = Collisions::NONE;

                        if      (field.value == "RADIUS")       coll_type = Collisions::RADIUS;
                        else if (field.value == "RECTANGLE")    coll_type = Collisions::RECTANGLE;
                        else if (field.value == "MULTI_CIRCLE") coll_type = Collisions::MULTI_CIRCLE;

                        component.*(field.member) = coll_type;

                        // TODO: add bullet type string to enum check and unify the previous checks
                    } else {
                        component.*(field.member) = field.value;
                    }

                }
        }(fields), ...);

        in_world.add(entity_id, std::move(component));
    }


    // OpenUSD
    static bool LoadUsdFile(const std::string& filepath, ecs::World& world);
};
