#include "Application.h"
#include "SystemControl.h"
#include "Platform.h"
#include "CharacterSet.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

Application::Application() {}

Application::~Application() { Shutdown(); }

bool Application::InitSDL() {
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        // Already initialized (by launcher or previous call)
        return true;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        TVP_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        TVP_LOG_WARN("IMG_Init: %s", IMG_GetError());
    }
    
    if (TTF_Init() != 0) {
        TVP_LOG_WARN("TTF_Init: %s", TTF_GetError());
    }
    
    TVP_LOG_INFO("SDL2 initialized successfully");
    return true;
}

bool Application::InitWindow(int width, int height, const char* title) {
    m_width = width;
    m_height = height;
    
    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) {
        TVP_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    
    // Use software renderer - compatible with llvmpipe
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer(m_window, -1, 0);
    }
    if (!m_renderer) {
        TVP_LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }
    
    TVP_LOG_INFO("Window created: %dx%d (software renderer)", width, height);
    return true;
}

void Application::LoadFont() {
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    
    const char* fontPaths[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/data/data/com.termux/files/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        nullptr
    };
    
    for (int i = 0; fontPaths[i]; i++) {
        m_font = TTF_OpenFont(fontPaths[i], 14);
        if (m_font) {
            TVP_LOG_INFO("Font loaded: %s", fontPaths[i]);
            return;
        }
    }
    TVP_LOG_WARN("No font found - text will not be rendered");
}

bool Application::Initialize(int width, int height, const char* title) {
    if (m_initialized) return true;
    
    if (!InitSDL()) return false;
    if (!InitWindow(width, height, title)) return false;
    LoadFont();
    
    m_event.Initialize();
    
    m_initialized = true;
    TVP_LOG_INFO("Application initialized");
    return true;
}

void Application::Shutdown() {
    if (!m_initialized) return;
    
    m_storage.Shutdown();
    m_event.Shutdown();
    
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    m_initialized = false;
    TVP_LOG_INFO("Application shutdown");
}

void Application::RenderText(const std::string& text, int x, int y, SDL_Color color, int fontSize) {
    if (!m_font || text.empty()) return;
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(m_font, text.c_str(), color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    if (texture) {
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(m_renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void Application::RenderLogScreen() {
    // Dark background
    SDL_SetRenderDrawColor(m_renderer, 20, 20, 30, 255);
    SDL_RenderClear(m_renderer);
    
    // Title bar
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 60, 255);
    SDL_Rect titleBar = {0, 0, m_width, 40};
    SDL_RenderFillRect(m_renderer, &titleBar);
    
    SDL_Color white = {220, 220, 220, 255};
    SDL_Color gray = {140, 140, 160, 255};
    SDL_Color green = {100, 200, 100, 255};
    SDL_Color red = {220, 80, 80, 255};
    SDL_Color yellow = {220, 180, 40, 255};
    SDL_Color blue = {100, 160, 220, 255};
    
    RenderText("krkrv8a - Game Mode (Loading...)", 10, 10, white, 14);
    
    // Instruction
    RenderText("Press ESC or close window to quit", m_width - 300, 12, gray, 12);
    
    // Log area
    auto lines = TVPGetScreenLogLines();
    
    int lineHeight = 18;
    int startY = 50 - m_logScroll;
    int maxLines = (m_height - 60) / lineHeight;
    
    // Clip area
    SDL_Rect clipRect = {0, 45, m_width, m_height - 50};
    SDL_RenderSetClipRect(m_renderer, &clipRect);
    
    int start = 0;
    if ((int)lines.size() > maxLines) {
        start = (int)lines.size() - maxLines;
    }
    
    for (int i = start; i < (int)lines.size(); i++) {
        const std::string& line = lines[i];
        SDL_Color color = gray;
        
        if (line.find("[ERROR]") != std::string::npos) color = red;
        else if (line.find("[WARN]") != std::string::npos) color = yellow;
        else if (line.find("[INFO]") != std::string::npos) color = blue;
        else if (line.find("[DEBUG]") != std::string::npos) color = gray;
        else if (line.find("SUCCESS") != std::string::npos) color = green;
        
        RenderText(line, 10, startY + (i - start) * lineHeight, color, 12);
    }
    
    SDL_RenderSetClipRect(m_renderer, nullptr);
}

void Application::MainLoop() {
    TVP_LOG_INFO("Entering main loop");
    
    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                TVP_LOG_INFO("Quit event received");
                m_running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    TVP_LOG_INFO("ESC pressed - quitting");
                    m_running = false;
                }
            }
            if (event.type == SDL_MOUSEWHEEL) {
                m_logScroll -= event.wheel.y * 30;
                if (m_logScroll < 0) m_logScroll = 0;
            }
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                }
            }
        }
        
        // Render: show log screen (game rendering will be added later)
        RenderLogScreen();
        
        SDL_RenderPresent(m_renderer);
        
        // Small delay to avoid 100% CPU
        SDL_Delay(16); // ~60fps
    }
    
    TVP_LOG_INFO("Main loop ended");
}

int Application::Run(const std::string& gamePath) {
    if (!m_initialized) {
        if (!Initialize()) return 1;
    }
    
    TVP_LOG_INFO("Loading game: %s", gamePath.c_str());
    
    // Initialize storage with game path
    m_storage.Initialize(gamePath);
    TVP_LOG_INFO("Storage initialized");
    
    // Initialize system control
    SystemControl::GetInstance().Initialize(this);
    TVP_LOG_INFO("System control initialized");
    
    // Try to load archives
    TVP_LOG_INFO("Storage initialized with: %s", gamePath.c_str());
    
    m_running = true;
    MainLoop();
    
    return 0;
}

int Application::RunLauncher() {
    if (!m_initialized) {
        if (!Initialize(960, 640, "krkrv8a Launcher")) return 1;
    }
    
    m_running = true;
    MainLoop();
    
    return 0;
}