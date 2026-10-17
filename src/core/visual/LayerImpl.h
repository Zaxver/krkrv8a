#pragma once
#ifndef TVP_LAYER_IMPL_H
#define TVP_LAYER_IMPL_H

#include "tvp_types.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <memory>

// Layer operations
enum class TVPLayerOp {
    Copy,           // Direct copy
    Add,            // Additive blending
    Sub,            // Subtractive
    Mul,            // Multiplicative
    Div,            // Divisive
    AlphaBlend,     // Alpha blending
    TransAlpha,     // Transparent alpha
    AlphaFade       // Alpha fade
};

// A visual layer - bitmap with position and blending
class tTVPLayer {
public:
    tTVPLayer();
    tTVPLayer(int width, int height);
    ~tTVPLayer();
    
    // Create/resize
    bool Create(int width, int height);
    void Destroy();
    
    // Properties
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsValid() const { return m_surface != nullptr; }
    
    // Position
    void SetPosition(int x, int y) { m_posX = x; m_posY = y; }
    int GetX() const { return m_posX; }
    int GetY() const { return m_posY; }
    
    // Opacity (0-255)
    void SetOpacity(int opacity) { m_opacity = opacity; }
    int GetOpacity() const { return m_opacity; }
    
    // Visible flag
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    // Blend operation
    void SetBlendOp(TVPLayerOp op) { m_blendOp = op; }
    TVPLayerOp GetBlendOp() const { return m_blendOp; }
    
    // Drawing operations
    void Fill(uint32_t color);
    void FillRect(int x, int y, int w, int h, uint32_t color);
    void Clear();
    
    // Copy from another layer
    void CopyFrom(const tTVPLayer* src, int sx, int sy, int sw, int sh, int dx, int dy);
    void CopyFrom(const tTVPLayer* src);
    
    // Load image from data
    bool LoadImage(const uint8_t* data, size_t size);
    bool LoadImage(const std::string& path);
    
    // Get pixel data
    uint32_t* GetPixels() const;
    SDL_Surface* GetSurface() const { return m_surface; }
    
    // Convert to SDL texture
    SDL_Texture* ToTexture(SDL_Renderer* renderer) const;
    
private:
    int m_width = 0;
    int m_height = 0;
    int m_posX = 0;
    int m_posY = 0;
    int m_opacity = 255;
    bool m_visible = true;
    TVPLayerOp m_blendOp = TVPLayerOp::AlphaBlend;
    
    SDL_Surface* m_surface = nullptr;
};

#endif // TVP_LAYER_IMPL_H