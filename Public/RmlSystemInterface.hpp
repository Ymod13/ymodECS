//
// Created by ymod1 on 18/09/2026.
//

#ifndef YMODECS_RMLSYSTEMINTERFACE_HPP
#define YMODECS_RMLSYSTEMINTERFACE_HPP

#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <SDL3/SDL.h>

class RmlSystemInterface : public Rml::SystemInterface
{
public:
    double GetElapsedTime() override
    {
        return static_cast<double>(SDL_GetTicksNS()) / 1e9;
    }

    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override
    {
        SDL_Log("[RmlUi] %s", message.c_str());
        return true; // true = continua l'esecuzione anche dopo errori/assert
    }
};
#endif //YMODECS_RMLSYSTEMINTERFACE_HPP
