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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        TVP_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        TVP_LOG_WARN("IMG_Init: %s", IMG_GetError());
    }
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        TVP_LOG_WARN("Mix_OpenAudio: %s", Mix_GetError());
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() != 0) {
        TVP_LOG_WARN("TTF_Init: %s", TTF_GetError());
    }
    
    TVP_LOG_INFO("SDL2 initialized successfully");
    return true;
}

bool Application::InitWindow(int width, int height, const char* title) {
    m_width = width;
    m_height = height;
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    
    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) {
        TVP_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    
    TVP_LOG_INFO("Window created: %dx%d", width, height);
    return true;
}

bool Application::InitGL() {
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        TVP_LOG_ERROR("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return false;
    }
    
    // Enable vsync
    SDL_GL_SetSwapInterval(1);
    
    TVP_LOG_INFO("OpenGL context created");
    return true;
}

bool Application::Initialize(int width, int height, const char* title) {
    if (m_initialized) return true;
    
    if (!InitSDL()) return false;
    if (!InitWindow(width, height, title)) return false;
    if (!InitGL()) return false;
    
    m_event.Initialize();
    
    m_initialized = true;
    TVP_LOG_INFO("Application initialized");
    return true;
}

void Application::Shutdown() {
    if (!m_initialized) return;
    
    m_storage.Shutdown();
    m_event.Shutdown();
    
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    
    m_initialized = false;
    TVP_LOG_INFO("Application shutdown");
}

int Application::Run(const std::string& gamePath) {
    if (!m_initialized) {
        if (!Initialize()) return 1;
    }
    
    // Initialize storage with game path
    m_storage.Initialize(gamePath);
    
    // Initialize system control
    SystemControl::GetInstance().Initialize(this);
    
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

void Application::MainLoop() {
    while (m_running) {
        // Process events
        m_event.ProcessEvents();
        
        // Check for quit
        TVPEvent ev;
        while (m_event.PollEvent(ev)) {
            if (ev.type == TVPEventType::Quit) {
                m_running = false;
            }
            if (ev.type == TVPEventType::KeyDown && ev.keycode == SDLK_ESCAPE) {
                // ESC = quit for now (will be configurable)
                // Don't quit in game mode; just pass to engine
            }
            if (ev.type == TVPEventType::Resize) {
                m_width = ev.width;
                m_height = ev.height;
            }
        }
        
        // Clear screen via SDL
        SDL_SetRenderDrawColor(m_renderer ? m_renderer : nullptr, 0, 0, 0, 255);
        // Note: In launcher mode, SDL_Renderer is used.
        // In game mode with GL context, we use SDL_GL_SwapWindow.
        
        // Swap buffers
        SDL_GL_SwapWindow(m_window);
    }
}