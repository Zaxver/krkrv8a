#pragma once
#ifndef TVP_APPLICATION_H
#define TVP_APPLICATION_H

#include "tvp_types.h"
#include "tvp_config.h"
#include "StorageImpl.h"
#include "EventImpl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>
#include <string>
#include <functional>
#include <vector>

// Main application class
// Manages SDL initialization, window, game lifecycle
class Application {
public:
    Application();
    ~Application();
    
    // Initialize SDL and subsystems
    bool Initialize(int width = 1280, int height = 720, const char* title = "krkrv8a");
    void Shutdown();
    
    // Run the game engine
    int Run(const std::string& gamePath);
    
    // Run without a game (for launcher mode)
    int RunLauncher();
    
    // Get the window
    SDL_Window* GetWindow() { return m_window; }
    SDL_Renderer* GetRenderer() { return m_renderer; }
    
    // Get subsystems
    StorageImpl& GetStorage() { return m_storage; }
    EventImpl& GetEvent() { return m_event; }
    
    // Window dimensions
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    // Quit flag
    void Quit() { m_running = false; }
    bool IsRunning() const { return m_running; }
    
private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    int m_width = 1280;
    int m_height = 720;
    bool m_initialized = false;
    bool m_running = false;
    int m_logScroll = 0;
    
    StorageImpl m_storage;
    EventImpl m_event;
    
    bool InitSDL();
    bool InitWindow(int width, int height, const char* title);
    void LoadFont();
    void MainLoop();
    void RenderLogScreen();
    void RenderText(const std::string& text, int x, int y, SDL_Color color, int fontSize = 14);
};

#endif // TVP_APPLICATION_H