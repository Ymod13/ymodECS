//
// Created by ymod1 on 18/09/2026.
//

#ifndef YMODECS_RMLINPUTBRIDGE_HPP
#define YMODECS_RMLINPUTBRIDGE_HPP

#pragma once
#include <RmlUi/Core/Input.h>
#include <SDL3/SDL.h>

namespace RmlInputBridge {

    inline int ConvertMouseButton(Uint8 sdlButton)
    {
        switch (sdlButton) {
            case SDL_BUTTON_LEFT:   return 0;
            case SDL_BUTTON_RIGHT:  return 1;
            case SDL_BUTTON_MIDDLE: return 2;
            default: return 0;
        }
    }

    inline int GetKeyModifierState()
    {
        SDL_Keymod mod = SDL_GetModState();
        int rmlMod = 0;
        if (mod & SDL_KMOD_CTRL)  rmlMod |= Rml::Input::KM_CTRL;
        if (mod & SDL_KMOD_SHIFT) rmlMod |= Rml::Input::KM_SHIFT;
        if (mod & SDL_KMOD_ALT)   rmlMod |= Rml::Input::KM_ALT;
        return rmlMod;
    }

    // partial tabel: more common keys. Extend upon necessity
    // (complete ref in Dependencies/RmlUi/Backends/RmlUi_Platform_SDL.cpp)
    inline Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode key)
    {
        using namespace Rml::Input;
        switch (key) {
            case SDLK_A: return KI_A;
            case SDLK_B: return KI_B;
            case SDLK_C: return KI_C;
            case SDLK_D: return KI_D;
            case SDLK_E: return KI_E;
            case SDLK_F: return KI_F;
            case SDLK_S: return KI_S;
            case SDLK_W: return KI_W;
                // ... add here remaining letters if needed for text fields

            case SDLK_UP:     return KI_UP;
            case SDLK_DOWN:   return KI_DOWN;
            case SDLK_LEFT:   return KI_LEFT;
            case SDLK_RIGHT:  return KI_RIGHT;
            case SDLK_SPACE:  return KI_SPACE;
            case SDLK_RETURN: return KI_RETURN;
            case SDLK_ESCAPE: return KI_ESCAPE;
            case SDLK_TAB:    return KI_TAB;
            case SDLK_BACKSPACE: return KI_BACK;
            case SDLK_DELETE: return KI_DELETE;
            case SDLK_HOME:   return KI_HOME;
            case SDLK_END:    return KI_END;

            default: return KI_UNKNOWN;
        }
    }

    inline void ProcessEvent(env::RmlUIContext &rmlCtx, SDL_Event *event) {
        // RmlUi input forwarding
        //-------------------------
        switch (event->type) {
            case SDL_EVENT_MOUSE_MOTION:
                rmlCtx.context->ProcessMouseMove(
                    static_cast<int>(event->motion.x),
                    static_cast<int>(event->motion.y),
                    RmlInputBridge::GetKeyModifierState());
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    // TODO: evaluate to change this according to gameplay events
                    //       if (event->button.button == SDL_BUTTON_LEFT) { etc..

                    // if mouse is hovering an interactive RmlUi element, then the click is owned by UI
                    Rml::Element* hovered = rmlCtx.context->GetHoverElement();
                    bool rmlCapturedClick = hovered != nullptr && hovered != rmlCtx.context->GetRootElement();

                    if (rmlCapturedClick) {
                        rmlCtx.context->ProcessMouseButtonDown(
                        RmlInputBridge::ConvertMouseButton(event->button.button),
                        RmlInputBridge::GetKeyModifierState());
                    }
                }

                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                rmlCtx.context->ProcessMouseButtonUp(
                    RmlInputBridge::ConvertMouseButton(event->button.button),
                    RmlInputBridge::GetKeyModifierState());
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                rmlCtx.context->ProcessMouseWheel(
                    -event->wheel.y,
                    RmlInputBridge::GetKeyModifierState());
                break;

            case SDL_EVENT_KEY_DOWN:
                rmlCtx.context->ProcessKeyDown(
                    RmlInputBridge::ConvertKey(event->key.key),
                    RmlInputBridge::GetKeyModifierState());
                break;

            case SDL_EVENT_KEY_UP:
                rmlCtx.context->ProcessKeyUp(
                    RmlInputBridge::ConvertKey(event->key.key),
                    RmlInputBridge::GetKeyModifierState());
                break;

            case SDL_EVENT_TEXT_INPUT:
                rmlCtx.context->ProcessTextInput(event->text.text);
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                rmlCtx.context->SetDimensions(
                    Rml::Vector2i(event->window.data1, event->window.data2));
                break;
        }
    };

} // namespace RmlInputBridge

#endif //YMODECS_RMLINPUTBRIDGE_HPP
