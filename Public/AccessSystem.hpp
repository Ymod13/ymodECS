//
// Created by ymod1 on 11/05/2026.
//

#ifndef YMODECS_ACCESSSYSTEM_HPP
#define YMODECS_ACCESSSYSTEM_HPP

#include "ecs.hpp"

// ============================================================
//  Access<> — declaration of a system's dependencies
// ============================================================

namespace ecs {
    // Tags that describe the type of access
    struct ReadTag              { static constexpr bool required = true;  static constexpr bool writable = false; };
    struct WriteTag             { static constexpr bool required = true;  static constexpr bool writable = true;  };
    struct ReadIfExistsTag      { static constexpr bool required = false; static constexpr bool writable = false; };
    struct WriteIfExistsTag     { static constexpr bool required = false; static constexpr bool writable = true;  };
    struct FilterComponentTag   { static constexpr bool included = true; };

    // Single dependency: pairs a component type with an access tag
    template<typename TComponent, typename TAccessTag>
    struct Dep {
        using component  = TComponent;
        using access_tag = TAccessTag;
    };

    // ── Access<Deps...> ──────────────────────────────────────────
    // Accumulates dependencies via nested type aliases.
    // Each ::Read<T> produces a new Access with Dep<T,ReadTag> appended.
    template<typename... Deps>
    struct AccessMode {
        using dependencies = std::tuple<Deps...>;

        template<typename T>
        using Read           = AccessMode<Deps..., Dep<T, ReadTag>>;

        template<typename T>
        using Write          = AccessMode<Deps..., Dep<T, WriteTag>>;

        template<typename T>
        using ReadIfExists   = AccessMode<Deps..., Dep<T, ReadIfExistsTag>>;

        template<typename T>
        using WriteIfExists  = AccessMode<Deps..., Dep<T, WriteIfExistsTag>>;

        template<typename T>
        using With  = AccessMode<Deps..., Dep<T, FilterComponentTag>>;

        // checks if T has 'writable' field
        template<typename T, typename = void>
        struct has_writable : std::false_type {};

        template<typename T>
        struct has_writable<T, std::void_t<decltype(T::writable)>> : std::true_type {};

        // checks if T has 'required' field
        template<typename T, typename = void>
        struct has_required : std::false_type {};

        template<typename T>
        struct has_required<T, std::void_t<decltype(T::required)>> : std::true_type {};

        // checks if T has 'included' field
        template<typename T, typename = void>
        struct has_included : std::false_type {};

        template<typename T>
        struct has_included<T, std::void_t<decltype(T::included)>> : std::true_type {};

        // ── Helpers for the scheduler ─────────────────────────────

        // Fills a Signature with the *required* components (Read + Write)
        static Signature required_signature() {
            Signature s;
            fill_required<Deps...>(s);
            return s;
        }

        // Fills a Signature with the *writable* components (Write + WriteIfExists)
        static Signature write_signature() {
            Signature s;
            fill_writes<Deps...>(s);
            return s;
        }

        // Fills a Signature with *all* touched components (including IfExists)
        static Signature full_signature() {
            Signature s;
            fill_all<Deps...>(s);
            return s;
        }

        //
        static Signature filter_signature() {
            Signature s;
            fill_filters<Deps...>(s);
            return s;
        }

    private:
        // ── Recursive fold over Dep<T,Tag> ────────────────────────
        template<typename... Ds>
        static void fill_required(Signature& s) {
            (([&](){
                using Tag = typename Ds::access_tag;
                if constexpr (has_writable<Tag>::value) {
                    if constexpr (Tag::required)          {
                        s.set(ComponentRegistry::GetId<typename Ds::component>());
                    }
                }
            }()), ...);
        }

        template<typename... Ds>
        static void fill_writes(Signature& s) {
            (([&](){
                using Tag = typename Ds::access_tag;
                if constexpr (has_writable<Tag>::value) {
                    if constexpr (Tag::writable)          {
                        s.set(ComponentRegistry::GetId<typename Ds::component>());
                    }
                }
            }()), ...);
        }

        template<typename... Ds>
        static void fill_filters(Signature& s) {
            (([&](){
                using Tag = typename Ds::access_tag;
                if constexpr (has_included<Tag>::value) {
                    if constexpr (Tag::included)          {
                        s.set(ComponentRegistry::GetId<typename Ds::component>());
                    }
                }
            }()), ...);
        }

        template<typename... Ds>
        static void fill_all(Signature& s) {
            (s.set(ComponentRegistry::GetId<typename Ds::component>()), ...);
        }
    };
} // namespace ecs

#endif //YMODECS_ACCESSSYSTEM_HPP