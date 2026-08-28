//
// Created by ymod1 on 15/06/2026.
//

#include "EventsListener.hpp"

#include <iostream>
#include <ostream>

void EventListener::RegisterEvent(std::unique_ptr<GameEvent> new_event) {

    if (!new_event)
        return;

    switch (new_event->type) {
        case GameEventType::IMMEDIATE:
            new_event->Run();
            break;
        case GameEventType::STANDARD:
            game_events.push(std::move(new_event));
            break;
        default:
            std::cout << "UNKNOWN" << std::endl;
            break;
    }
}
//---------------------------------------------------------------------------------------------------

void EventListener::ProcessEvents() {
    if (game_events.empty()) return;

    while (game_events.size() > 0) {

        GameEvent* current_event = game_events.front().get();

        current_event->Run();

        game_events.pop();

    }
}
