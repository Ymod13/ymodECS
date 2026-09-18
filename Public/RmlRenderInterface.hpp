//
// Created by ymod1 on 18/09/2026.
//

#ifndef YMODECS_RMLRENDERINTERFACE_HPP
#define YMODECS_RMLRENDERINTERFACE_HPP

#pragma once
#include <RmlUi/Core/RenderInterface.h>
#include <SDL3/SDL.h>
#include <unordered_map>
#include <SDL3_image/SDL_image.h>

class RmlRenderInterface : public Rml::RenderInterface
{
public:
    explicit RmlRenderInterface(SDL_Renderer* renderer) : renderer(renderer) {}

    // --- Geometria ---
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                 Rml::Span<const int> indices) override
    {
        auto* geometry = new CompiledGeometry();
        geometry->vertices.assign(vertices.begin(), vertices.end());
        geometry->indices.assign(indices.begin(), indices.end());
        return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
    }

    void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation,
                         Rml::TextureHandle texture) override
    {
        auto* geometry = reinterpret_cast<CompiledGeometry*>(handle);

        // Converte i vertici RmlUi (posizione + colore + UV) in SDL_Vertex
        std::vector<SDL_Vertex> sdlVertices;
        sdlVertices.reserve(geometry->vertices.size());
        for (const auto& v : geometry->vertices)
        {
            SDL_Vertex sv{};
            sv.position = { v.position.x + translation.x, v.position.y + translation.y };
            sv.color = { v.colour.red / 255.f, v.colour.green / 255.f,
                         v.colour.blue / 255.f, v.colour.alpha / 255.f };
            sv.tex_coord = { v.tex_coord.x, v.tex_coord.y };
            sdlVertices.push_back(sv);
        }

        SDL_Texture* sdlTexture = texture ? reinterpret_cast<SDL_Texture*>(texture) : nullptr;

        SDL_RenderGeometry(renderer, sdlTexture,
                            sdlVertices.data(), static_cast<int>(sdlVertices.size()),
                            geometry->indices.data(), static_cast<int>(geometry->indices.size()));
    }

    void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override
    {
        delete reinterpret_cast<CompiledGeometry*>(handle);
    }

    // --- Texture ---
    Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) override
    {
        SDL_Surface* surface = IMG_Load(source.c_str()); // usi già SDL3_image
        if (!surface) return {};

        textureDimensions = { surface->w, surface->h };
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return reinterpret_cast<Rml::TextureHandle>(texture);
    }

    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions) override
    {
        // source è RGBA8 premultiplicato riga per riga — usato per il testo (glyph atlas) e immagini generate a runtime
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            sourceDimensions.x, sourceDimensions.y,
            SDL_PIXELFORMAT_RGBA32,
            const_cast<Rml::byte*>(source.data()),
            sourceDimensions.x * 4);

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return reinterpret_cast<Rml::TextureHandle>(texture);
    }

    void ReleaseTexture(Rml::TextureHandle texture) override
    {
        SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(texture));
    }

    // --- Scissor region (clipping) ---
    void EnableScissorRegion(bool enable) override
    {
        scissorEnabled = enable;
        if (!enable) SDL_SetRenderClipRect(renderer, nullptr);
    }

    void SetScissorRegion(Rml::Rectanglei region) override
    {
        SDL_Rect rect{ region.Left(), region.Top(), region.Width(), region.Height() };
        SDL_SetRenderClipRect(renderer, &rect);
    }

private:
    struct CompiledGeometry
    {
        std::vector<Rml::Vertex> vertices;
        std::vector<int> indices;
    };

    SDL_Renderer* renderer;
    bool scissorEnabled = false;
};

#endif //YMODECS_RMLRENDERINTERFACE_HPP
