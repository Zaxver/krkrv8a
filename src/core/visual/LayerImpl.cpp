#include "LayerImpl.h"
#include "CharacterSet.h"
#include <SDL2/SDL_image.h>
#include <cstring>
#include <algorithm>

tTVPLayer::tTVPLayer() {}

tTVPLayer::tTVPLayer(int width, int height) {
    Create(width, height);
}

tTVPLayer::~tTVPLayer() {
    Destroy();
}

bool tTVPLayer::Create(int width, int height) {
    Destroy();
    if (width <= 0 || height <= 0) return false;
    
    m_width = width;
    m_height = height;
    
    // Create 32-bit ARGB surface
    m_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
        SDL_PIXELFORMAT_ARGB8888);
    
    if (!m_surface) {
        TVP_LOG_ERROR("Layer: SDL_CreateRGBSurface failed: %s", SDL_GetError());
        return false;
    }
    
    return true;
}

void tTVPLayer::Destroy() {
    if (m_surface) {
        SDL_FreeSurface(m_surface);
        m_surface = nullptr;
    }
    m_width = m_height = 0;
}

void tTVPLayer::Fill(uint32_t color) {
    if (!m_surface) return;
    SDL_FillRect(m_surface, nullptr, color);
}

void tTVPLayer::FillRect(int x, int y, int w, int h, uint32_t color) {
    if (!m_surface) return;
    SDL_Rect rect = {x, y, w, h};
    SDL_FillRect(m_surface, &rect, color);
}

void tTVPLayer::Clear() {
    if (!m_surface) return;
    SDL_FillRect(m_surface, nullptr, 0);
}

void tTVPLayer::CopyFrom(const tTVPLayer* src, int sx, int sy, int sw, int sh, int dx, int dy) {
    if (!m_surface || !src || !src->m_surface) return;
    
    SDL_Rect srcRect = {sx, sy, sw, sh};
    SDL_Rect dstRect = {dx, dy, sw, sh};
    
    SDL_BlitSurface(src->m_surface, &srcRect, m_surface, &dstRect);
}

void tTVPLayer::CopyFrom(const tTVPLayer* src) {
    if (!m_surface || !src || !src->m_surface) return;
    SDL_BlitSurface(src->m_surface, nullptr, m_surface, nullptr);
}

bool tTVPLayer::LoadImage(const uint8_t* data, size_t size) {
    Destroy();
    
    // Load from memory using SDL_image
    SDL_RWops* rw = SDL_RWFromMem((void*)data, (int)size);
    if (!rw) return false;
    
    m_surface = IMG_Load_RW(rw, 1); // 1 = free rw automatically
    if (!m_surface) {
        TVP_LOG_ERROR("Layer: IMG_Load_RW failed: %s", IMG_GetError());
        return false;
    }
    
    // Convert to ARGB8888 if needed
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(m_surface, SDL_PIXELFORMAT_ARGB8888, 0);
    if (converted) {
        SDL_FreeSurface(m_surface);
        m_surface = converted;
    }
    
    m_width = m_surface->w;
    m_height = m_surface->h;
    return true;
}

bool tTVPLayer::LoadImage(const std::string& path) {
    Destroy();
    
    m_surface = IMG_Load(path.c_str());
    if (!m_surface) {
        TVP_LOG_ERROR("Layer: IMG_Load failed for %s: %s", path.c_str(), IMG_GetError());
        return false;
    }
    
    // Convert to ARGB8888
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(m_surface, SDL_PIXELFORMAT_ARGB8888, 0);
    if (converted) {
        SDL_FreeSurface(m_surface);
        m_surface = converted;
    }
    
    m_width = m_surface->w;
    m_height = m_surface->h;
    return true;
}

uint32_t* tTVPLayer::GetPixels() const {
    if (!m_surface) return nullptr;
    return (uint32_t*)m_surface->pixels;
}

SDL_Texture* tTVPLayer::ToTexture(SDL_Renderer* renderer) const {
    if (!m_surface || !renderer) return nullptr;
    return SDL_CreateTextureFromSurface(renderer, m_surface);
}