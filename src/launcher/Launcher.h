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

// Game entry in the launcher
struct GameEntry {
    std::string name;           // Display name
    std::string path;           // Path to game folder or XP3 file
    std::string iconPath;       // Optional icon
    std::string lastPlayed;     // Last played timestamp
    bool isXP3 = false;         // Is it an XP3 file (vs folder)
    SDL_Texture* iconTexture = nullptr;
    int iconW = 0, iconH = 0;
};

// Launcher mode
enum class LauncherMode {
    Library,       // Main game library view
    Settings,      // Settings page
    About          // About page
};

// The GUI Launcher
// Renders an SDL2-based GUI (no Dear ImGui dependency, pure SDL2)
class Launcher {
public:
    Launcher();
    ~Launcher();
    
    bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
    void Shutdown();
    
    // Main loop
    void Run();
    
    // Stop the launcher
    void Stop() { m_running = false; }
    
    // Game library management
    void AddGame(const std::string& path);
    void RemoveGame(int index);
    void LoadLibrary();
    void SaveLibrary();
    
    // Get the selected game path for launching
    std::string GetSelectedGamePath() const { return m_selectedGamePath; }
    
    // Check if a game was selected to launch
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
    void RenderText(const std::string& text, int x, int y, SDL_Color color, int size = 16);
    void RenderTextWrapped(const std::string& text, int x, int y, int w, SDL_Color color, int size = 16);
    
    // UI helpers
    bool Button(const std::string& text, SDL_Rect rect, SDL_Color bg, SDL_Color hover);
    void DrawRect(SDL_Rect rect, SDL_Color color);
    void DrawRoundedRect(SDL_Rect rect, SDL_Color color, int radius);
    
    // Game detection
    bool DetectGame(const std::string& path, std::string& name, bool& isXP3);
    bool FindStartupFile(const std::string& gamePath, std::string& startupFile);
    SDL_Texture* LoadGameIcon(const std::string& gamePath);
    
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
    
    // Input dialog state
    std::string m_inputPath;
    bool m_inputActive = false;
    
    // Config
    std::string m_configPath;
    std::string m_lastBrowsePath;
    
    // Scroll offset for game grid
    int m_scrollOffset = 0;
    
    // Mouse state
    int m_mouseX = 0, m_mouseY = 0;
    bool m_mousePressed = false;
    bool m_mouseReleased = false;
    int m_lastClickIndex = -1;
    uint32_t m_lastClickTime = 0;
    
    // UI button tracking
    struct UIButton {
        SDL_Rect rect;
        std::string id;
        bool hovered = false;
        bool pressed = false;
    };
    std::vector<UIButton> m_buttons;
};

#endif // KRKR_LAUNCHER_H