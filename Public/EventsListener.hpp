//
// Created by ymod1 on 15/06/2026.
//

#ifndef YMODECS_EVENTSLISTENER_HPP
#define YMODECS_EVENTSLISTENER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <queue>

enum GameEventType {
    NONE,
    STANDARD,
    IMMEDIATE
};

using GameEventID = std::uint8_t;

// Basic polymorphic wrapper
struct IEventCallback {
    virtual ~IEventCallback() = default;
};

// Typed specialization
template<typename... Args>
struct EventCallback : IEventCallback {
    std::function<void(Args...)> fn;

    explicit EventCallback(std::function<void(Args...)> f) : fn(std::move(f)) {}

    void invoke(Args... args) { fn(std::forward<Args>(args)...); }
};

class GameEvent {
public:

    GameEvent() = default;

    GameEvent(GameEventType new_type) : type(new_type) {
        //
    }

    GameEventType type = NONE;
    void *data = nullptr;
    std::shared_ptr<IEventCallback> callback;

    // Helper to assign the callback in a typed manner
    template<typename... Args>
    void SetCallback(std::function<void(Args...)> fn) {
        callback = std::make_shared<EventCallback<Args...>>(std::move(fn));
    }

    // Helper to invoke
    void Run() {
        auto* cb = dynamic_cast<EventCallback<>*>(callback.get());
        if (cb) cb->invoke();
    }

    static GameEventID GetId() {
        static std::uint8_t cid = next_id++;
        return cid;
    }

private:
    inline static GameEventID next_id = 0;
};

// Usage:
/*
    auto event = std::make_unique<GameEvent>();

    float damage = 42.0f;
    int entityId = 7;

    // The parameters are captured, Run() becomes void()
    event->SetCallback<>([damage, entityId]() {
        // use damage and entityId
    });

    void OnPlayerDeath() {...}

    event->SetCallback(OnPlayerDeath); // ✅ direct
    If the existing function has parameters, bind them first:
    cppvoid OnDamage(float dt, int damage) { ... }

    float dt = 0.16f;
    int damage = 42;

    // With a lambda that captures the values
    event->SetCallback([dt, damage]() {
        OnDamage(dt, damage); // ✅
    });

    // With std::bind
    event->SetCallback(std::bind(OnDamage, dt, damage));



    // ProcessEvents stays simple:
    current_event->Run(); // ✅

    auto event = std::make_unique<GameEvent>();
    listener.RegisterEvent(std::move(event));
*/

class EventListener {
public:
    void RegisterEvent(std::unique_ptr<GameEvent> new_event);
    void ProcessEvents();

private:
    std::queue<std::unique_ptr<GameEvent>> game_events;
};

#endif //YMODECS_EVENTSLISTENER_HPP