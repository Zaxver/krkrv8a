#pragma once
#ifndef KRKR_LAUNCHER_H
#define KRKR_LAUNCHER_H

#include "tvp_types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct GameEntry {
    std::string name;
    std::string path;
    std::string iconPath;
    std::string lastPlayed;
    bool isXP3 = false;
    SDL_Texture* iconTexture = nullptr;
    int iconW = 0, iconH = 0;
};

enum class LauncherMode {
    Library,
    Settings,
    About
};

    // File browser - removed, using zenity now

// Settings state
struct Settings {
    int windowWidth = 960;
    int windowHeight = 640;
    bool fullscreen = false;
    bool vsync = true;
    int audioVolume = 64;
    std::string fontPath;
    int uiScale = 100; // percentage, 100 = 1x
    bool autoDetectDPI = false; // default OFF - user controls scale
    int manualDPI = 160; // user-specified DPI (160 = 1x)
};

class Launcher {
public:
    Launcher();
    ~Launcher();
    
    bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
    void Shutdown();
    
    void Run();
    void Stop() { m_running = false; }
    
    void AddGame(const std::string& path);
    void RemoveGame(int index);
    void LoadLibrary();
    void SaveLibrary();
    void LoadSettings();
    void SaveSettings();
    
    std::string GetSelectedGamePath() const { return m_selectedGamePath; }
    bool WasGameSelected() const { return m_launchGame; }
    void ClearSelection() { m_launchGame = false; }
    
private:
    void Render();
    void HandleEvent(const SDL_Event& event);
    void RenderLibrary();
    void RenderSettings();
    void RenderAbout();
    void RenderTopBar();
    void RenderGameGrid();
    void RenderAddGameDialog();
    
    void LoadFont();
    void DetectDPI();
    int S(int val) const; // scale by DPI
    TTF_Font* GetFont(int size) const; // get font at scaled size
    
    void RenderText(const std::string& text, int x, int y, SDL_Color color, int size = 16);
    void RenderTextWrapped(const std::string& text, int x, int y, int w, SDL_Color color, int size = 16);
    
    bool Button(const std::string& text, SDL_Rect rect, SDL_Color bg, SDL_Color hover);
    void DrawRect(SDL_Rect rect, SDL_Color color);
    void DrawRoundedRect(SDL_Rect rect, SDL_Color color, int radius);
    
    bool DetectGame(const std::string& path, std::string& name, bool& isXP3);
    bool FindStartupFile(const std::string& gamePath, std::string& startupFile);
    SDL_Texture* LoadGameIcon(const std::string& gamePath);
    
    // File browser - using zenity now
    // void OpenFileBrowser(...);
    
    // Hardware detection
    std::string GetGPUInfo() const;
    std::string GetRendererType() const;
    bool IsHardwareAccelerated() const;
    std::string GetSDLRendererInfo() const;
    
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    TTF_Font* m_fontLarge = nullptr;
    TTF_Font* m_fontSmall = nullptr;
    
    std::vector<GameEntry> m_games;
    LauncherMode m_mode = LauncherMode::Library;
    
    int m_windowWidth = 960;
    int m_windowHeight = 640;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    
    // DPI scaling
    float m_dpiScale = 1.0f;
    int m_androidDPI = 0;
    
    bool m_running = false;
    bool m_launchGame = false;
    bool m_showAddDialog = false;
    std::string m_selectedGamePath;
    
    // Colors (dark theme)
    SDL_Color col_bg       = {30, 30, 40, 255};
    SDL_Color col_card     = {50, 50, 65, 255};
    SDL_Color col_cardHover= {65, 65, 85, 255};
    SDL_Color col_text     = {220, 220, 230, 255};
    SDL_Color col_textDim  = {140, 140, 155, 255};
    SDL_Color col_accent   = {80, 150, 220, 255};
    SDL_Color col_button   = {60, 120, 180, 255};
    SDL_Color col_buttonHover = {80, 150, 220, 255};
    SDL_Color col_input    = {40, 40, 55, 255};
    SDL_Color col_inputFocus= {55, 55, 75, 255};
    
    std::string m_inputPath;
    bool m_inputActive = false;
    
    std::string m_configPath;
    std::string m_lastBrowsePath;
    Settings m_settings;
    
    int m_scrollOffset = 0;
    
    int m_mouseX = 0, m_mouseY = 0;
    bool m_mousePressed = false;
    bool m_mouseReleased = false;
    int m_lastClickIndex = -1;
    uint32_t m_lastClickTime = 0;
    
    // No more FileBrowser member - using zenity
    
    // Cached GPU info
    std::string m_gpuInfo;
    std::string m_rendererInfo;
    bool m_hwAccel = false;
    bool m_gpuInfoCached = false;
    
    // Settings page scroll
    int m_settingsScroll = 0;
    
    // Settings editing state
    int m_editingItem = -1; // index of setting being edited (slider drag)
};

#endif // KRKR_LAUNCHER_H