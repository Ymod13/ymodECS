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

// Wrapper polimorfico base
struct IEventCallback {
    virtual ~IEventCallback() = default;
};

// Specializzazione tipizzata
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

    // Helper per assegnare il callback in modo tipizzato
    template<typename... Args>
    void SetCallback(std::function<void(Args...)> fn) {
        callback = std::make_shared<EventCallback<Args...>>(std::move(fn));
    }

    // Helper per invocare
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

    // I parametri sono catturati, Run() diventa void()
    event->SetCallback<>([damage, entityId]() {
        // usa damage e entityId
    });

    void OnPlayerDeath() {...}

    event->SetCallback(OnPlayerDeath); // ✅ diretto
    Se la funzione esistente ha parametri, li bindi prima:
    cppvoid OnDamage(float dt, int damage) { ... }

    float dt = 0.16f;
    int damage = 42;

    // Con lambda che cattura i valori
    event->SetCallback([dt, damage]() {
        OnDamage(dt, damage); // ✅
    });

    // Con std::bind
    event->SetCallback(std::bind(OnDamage, dt, damage));



    // ProcessEvents rimane semplice:
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

