#include "tvp_types.h"
#include "tvp_config.h"
#include "Application.h"
#include "Platform.h"
#include "Launcher.h"
#include "version.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

namespace fs = std::filesystem;

static void PrintUsage() {
    fprintf(stderr, 
        "krkrv8a %s - Kirikiri2 Engine for ARMv8a Linux\n"
        "\n"
        "Usage: krkrv8a [OPTIONS] [GAME_PATH]\n"
        "\n"
        "Options:\n"
        "  -h, --help        Show this help\n"
        "  -v, --version     Show version\n"
        "  -w, --width N     Set window width (default: 1280)\n"
        "  -H, --height N    Set window height (default: 720)\n"
        "  -f, --fullscreen  Start in fullscreen\n"
        "  -l, --launcher    Start in launcher mode (default if no game path)\n"
        "\n"
        "GAME_PATH can be:\n"
        "  - A folder containing Kirikiri2 game files\n"
        "  - An .xp3 archive file\n"
        "  - A .tjs script file\n"
        "\n"
        "If no GAME_PATH is given, the GUI launcher will start.\n"
        "\n"
        "https://github.com/Zaxver/krkrv8a\n",
        KRKR_VERSION_STRING);
}

static void PrintVersion() {
    printf("krkrv8a %s\n", KRKR_VERSION_STRING);
    printf("Platform: %s\n", Platform::GetPlatformName());
    printf("Architecture: %s\n", Platform::GetArchName());
    printf("CPU: %s (%d cores)\n", Platform::GetCPUModel().c_str(), Platform::GetCPUCount());
}

int main(int argc, char* argv[]) {
    std::string gamePath;
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool launcherMode = true;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintUsage();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            PrintVersion();
            return 0;
        } else if (arg == "-w" || arg == "--width") {
            if (++i < argc) width = std::atoi(argv[i]);
        } else if (arg == "-H" || arg == "--height") {
            if (++i < argc) height = std::atoi(argv[i]);
        } else if (arg == "-f" || arg == "--fullscreen") {
            fullscreen = true;
        } else if (arg == "-l" || arg == "--launcher") {
            launcherMode = true;
        } else if (arg[0] != '-') {
            gamePath = arg;
            launcherMode = false;
        }
    }
    
    TVP_LOG_INFO("krkrv8a %s starting...", KRKR_VERSION_STRING);
    TVP_LOG_INFO("Platform: %s (%s)", Platform::GetPlatformName(), Platform::GetArchName());
    
    // Set up log file
    const char* home = getenv("HOME");
    if (home) {
        std::string logPath = std::string(home) + "/.local/share/krkrv8a/krkrv8a.log";
        // Ensure directory exists
        fs::create_directories(std::string(home) + "/.local/share/krkrv8a");
        TVPSetLogFile(logPath);
        TVP_LOG_INFO("Log file: %s", logPath.c_str());
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        TVP_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    
    // Initialize SDL subsystems
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        TVP_LOG_WARN("IMG_Init: %s", IMG_GetError());
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        TVP_LOG_WARN("Mix_OpenAudio: %s", Mix_GetError());
    }
    
    if (TTF_Init() != 0) {
        TVP_LOG_WARN("TTF_Init: %s", TTF_GetError());
    }
    
    if (launcherMode) {
        // === Launcher Mode ===
        TVP_LOG_INFO("Starting in launcher mode");
        
        SDL_Window* window = SDL_CreateWindow(
            "krkrv8a Launcher",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            960, 640,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        
        if (!window) {
            TVP_LOG_ERROR("Failed to create window: %s", SDL_GetError());
            return 1;
        }
        
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
            SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            renderer = SDL_CreateRenderer(window, -1, 0);
        }
        
        if (!renderer) {
            TVP_LOG_ERROR("Failed to create renderer: %s", SDL_GetError());
            SDL_DestroyWindow(window);
            return 1;
        }
        
        Launcher launcher;
        if (!launcher.Initialize(window, renderer)) {
            TVP_LOG_ERROR("Failed to initialize launcher");
            return 1;
        }
        
        launcher.Run();
        
        // Check if a game was selected
        if (launcher.WasGameSelected()) {
            gamePath = launcher.GetSelectedGamePath();
            TVP_LOG_INFO("Selected game: %s", gamePath.c_str());
            
            // Fork + exec to start game as completely new process
            // This avoids SDL state conflicts between launcher and game
            pid_t pid = fork();
            if (pid == 0) {
                // Child process - exec self with game path
                // Close launcher's SDL resources first
                launcher.Shutdown();
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                
                // Set environment for the new process
                setenv("SDL_AUDIODRIVER", "dummy", 1);
                
                // exec self with game path
                execl(argv[0], argv[0], gamePath.c_str(), nullptr);
                // If exec fails
                TVP_LOG_ERROR("execl failed: %s", strerror(errno));
                _exit(127);
            } else if (pid > 0) {
                // Parent - wait for child to finish
                TVP_LOG_INFO("Game process started (PID=%d), waiting...", pid);
                int status;
                waitpid(pid, &status, 0);
                TVP_LOG_INFO("Game process exited (status=%d)", status);
                launcher.ClearSelection();
                // Re-show launcher window
                SDL_ShowWindow(window);
                SDL_RaiseWindow(window);
            } else {
                TVP_LOG_ERROR("fork() failed");
                launcher.ClearSelection();
            }
        }
    } else {
        // === Direct Game Mode ===
        TVP_LOG_INFO("Starting game: %s", gamePath.c_str());
        
        Application app;
        if (!app.Initialize(width, height, "krkrv8a - Game")) {
            TVP_LOG_ERROR("Failed to initialize application");
            return 1;
        }
        
        app.Run(gamePath);
    }
    
    // Cleanup
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    
    TVP_LOG_INFO("krkrv8a exited");
    return 0;
}