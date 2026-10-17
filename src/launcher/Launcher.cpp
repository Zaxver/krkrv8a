#include "Launcher.h"
#include "version.h"
#include "Platform.h"
#include <SDL2/SDL_image.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ctime>

namespace fs = std::filesystem;

Launcher::Launcher() {
    const char* home = getenv("HOME");
    if (home) {
        m_configPath = std::string(home) + "/.config/krkrv8a";
        fs::create_directories(m_configPath);
    }
    m_lastBrowsePath = home ? home : ".";
}

Launcher::~Launcher() { Shutdown(); }

void Launcher::LoadFont() {
    // Font search paths - covers Termux, standard Linux, and Android system fonts
    const char* fontPaths[] = {
        // Termux paths
        "/data/data/com.termux/files/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/data/data/com.termux/files/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        // Android system fonts (CJK capable)
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/MiSansVF.ttf",
        "/system/fonts/DroidSans.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        // Standard Linux paths
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/wqy-zenhei/wqy-zenhei.ttc",
        "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc",
        "/usr/share/fonts/wqy-microhei/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        nullptr
    };
    
    bool loaded = false;
    for (int i = 0; fontPaths[i]; i++) {
        if (fs::exists(fontPaths[i])) {
            m_font = TTF_OpenFont(fontPaths[i], 16);
            if (m_font) {
                m_fontLarge = TTF_OpenFont(fontPaths[i], 24);
                m_fontSmall = TTF_OpenFont(fontPaths[i], 12);
                TVP_LOG_INFO("Launcher: Loaded font: %s", fontPaths[i]);
                loaded = true;
                break;
            }
        }
    }
    
    // If still no font, try fontconfig via SDL2_ttf
    if (!loaded) {
        TVP_LOG_WARN("Launcher: No font file found, trying fallback...");
        // Last resort: try opening a font by family name (SDL2_ttf may use fontconfig)
        // This won't work with file paths, so we search harder
        const char* searchDirs[] = {
            "/data/data/com.termux/files/usr/share/fonts",
            "/system/fonts",
            "/usr/share/fonts",
            nullptr
        };
        for (int d = 0; searchDirs[d] && !loaded; d++) {
            if (!fs::exists(searchDirs[d])) continue;
            for (const auto& entry : fs::recursive_directory_iterator(searchDirs[d])) {
                if (!entry.is_regular_file()) continue;
                auto path = entry.path().string();
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".ttf" || ext == ".ttc" || ext == ".otf") {
                    m_font = TTF_OpenFont(path.c_str(), 16);
                    if (m_font) {
                        m_fontLarge = TTF_OpenFont(path.c_str(), 24);
                        m_fontSmall = TTF_OpenFont(path.c_str(), 12);
                        TVP_LOG_INFO("Launcher: Fallback font: %s", path.c_str());
                        loaded = true;
                        break;
                    }
                }
            }
        }
    }
    
    if (!loaded) {
        TVP_LOG_ERROR("Launcher: No font found at all! Text will not render.");
    }
}

bool Launcher::Initialize(SDL_Window* window, SDL_Renderer* renderer) {
    m_window = window;
    m_renderer = renderer;
    
    SDL_GetWindowSize(window, &m_windowWidth, &m_windowHeight);
    
    LoadFont();
    LoadLibrary();
    
    TVP_LOG_INFO("Launcher: Initialized (%d games in library)", (int)m_games.size());
    return true;
}

void Launcher::Shutdown() {
    for (auto& game : m_games) {
        if (game.iconTexture) SDL_DestroyTexture(game.iconTexture);
    }
    if (m_font) TTF_CloseFont(m_font);
    if (m_fontLarge) TTF_CloseFont(m_fontLarge);
    if (m_fontSmall) TTF_CloseFont(m_fontSmall);
    m_font = m_fontLarge = m_fontSmall = nullptr;
}

void Launcher::LoadLibrary() {
    std::string libFile = m_configPath + "/library.json";
    std::ifstream file(libFile);
    if (!file.is_open()) return;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    size_t pos = 0;
    while ((pos = content.find("\"name\"", pos)) != std::string::npos) {
        GameEntry entry;
        size_t valStart = content.find('"', pos + 6) + 1;
        size_t valEnd = content.find('"', valStart);
        entry.name = content.substr(valStart, valEnd - valStart);
        
        size_t pathPos = content.find("\"path\"", valEnd);
        if (pathPos == std::string::npos) break;
        valStart = content.find('"', pathPos + 6) + 1;
        valEnd = content.find('"', valStart);
        entry.path = content.substr(valStart, valEnd - valStart);
        
        size_t xp3Pos = content.find("\"isXP3\"", valEnd);
        if (xp3Pos != std::string::npos) {
            valStart = content.find(':', xp3Pos) + 1;
            entry.isXP3 = content[valStart] == 't';
        }
        
        entry.iconTexture = LoadGameIcon(entry.path);
        m_games.push_back(entry);
        pos = valEnd;
    }
    
    TVP_LOG_INFO("Launcher: Loaded %d games from library", (int)m_games.size());
}

void Launcher::SaveLibrary() {
    std::string libFile = m_configPath + "/library.json";
    std::ofstream file(libFile);
    if (!file.is_open()) return;
    
    file << "[\n";
    for (size_t i = 0; i < m_games.size(); i++) {
        file << "  {\n";
        file << "    \"name\": \"" << m_games[i].name << "\",\n";
        file << "    \"path\": \"" << m_games[i].path << "\",\n";
        file << "    \"isXP3\": " << (m_games[i].isXP3 ? "true" : "false") << "\n";
        file << "  }";
        if (i < m_games.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
}

bool Launcher::DetectGame(const std::string& path, std::string& name, bool& isXP3) {
    fs::path p(path);
    if (fs::is_regular_file(p)) {
        auto ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".xp3") {
            isXP3 = true;
            name = p.stem().string();
            return true;
        }
    } else if (fs::is_directory(p)) {
        isXP3 = false;
        name = p.filename().string();
        if (name.empty()) name = p.parent_path().filename().string();
        return true;
    }
    return false;
}

bool Launcher::FindStartupFile(const std::string& gamePath, std::string& startupFile) {
    if (fs::is_directory(gamePath)) {
        for (const auto& entry : fs::directory_iterator(gamePath)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".tjs" || ext == ".ks") {
                    startupFile = entry.path().string();
                    return true;
                }
            }
        }
    }
    return false;
}

SDL_Texture* Launcher::LoadGameIcon(const std::string& gamePath) {
    const char* iconNames[] = {
        "icon.png", "icon.jpg", "icon.bmp",
        "title.png", "title.jpg",
        "logo.png", "logo.jpg",
        nullptr
    };
    
    fs::path gameDir(gamePath);
    if (fs::is_regular_file(gamePath)) {
        gameDir = gameDir.parent_path();
    }
    
    for (int i = 0; iconNames[i]; i++) {
        fs::path iconPath = gameDir / iconNames[i];
        if (fs::exists(iconPath)) {
            SDL_Surface* surface = IMG_Load(iconPath.string().c_str());
            if (surface) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
                SDL_FreeSurface(surface);
                return tex;
            }
        }
    }
    return nullptr;
}

void Launcher::AddGame(const std::string& path) {
    std::string name;
    bool isXP3 = false;
    if (!DetectGame(path, name, isXP3)) {
        TVP_LOG_WARN("Launcher: Cannot detect game at: %s", path.c_str());
        return;
    }
    for (const auto& g : m_games) {
        if (g.path == path) {
            TVP_LOG_INFO("Launcher: Game already in library: %s", path.c_str());
            return;
        }
    }
    GameEntry entry;
    entry.name = name;
    entry.path = path;
    entry.isXP3 = isXP3;
    entry.iconTexture = LoadGameIcon(path);
    m_games.push_back(entry);
    SaveLibrary();
    TVP_LOG_INFO("Launcher: Added game: %s (%s)", name.c_str(), path.c_str());
}

void Launcher::RemoveGame(int index) {
    if (index < 0 || index >= (int)m_games.size()) return;
    if (m_games[index].iconTexture) SDL_DestroyTexture(m_games[index].iconTexture);
    m_games.erase(m_games.begin() + index);
    SaveLibrary();
}

// === Rendering ===

void Launcher::DrawRect(SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
}

void Launcher::DrawRoundedRect(SDL_Rect rect, SDL_Color color, int radius) {
    DrawRect(rect, color);
}

void Launcher::RenderText(const std::string& text, int x, int y, SDL_Color color, int size) {
    if (!m_font) return;
    TTF_Font* font = m_font;
    if (size >= 20 && m_fontLarge) font = m_fontLarge;
    else if (size <= 12 && m_fontSmall) font = m_fontSmall;
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        TVP_LOG_WARN("Launcher: TTF_RenderUTF8_Blended failed: %s", TTF_GetError());
        return;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    if (!tex) return;
    
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void Launcher::RenderTextWrapped(const std::string& text, int x, int y, int w, SDL_Color color, int size) {
    RenderText(text, x, y, color, size);
}

bool Launcher::Button(const std::string& text, SDL_Rect rect, SDL_Color bg, SDL_Color hover) {
    bool hovered = (m_mouseX >= rect.x && m_mouseX < rect.x + rect.w &&
                    m_mouseY >= rect.y && m_mouseY < rect.y + rect.h);
    bool clicked = hovered && m_mouseReleased;
    
    DrawRect(rect, hovered ? hover : bg);
    
    if (m_font) {
        SDL_Surface* surface = TTF_RenderUTF8_Blended(m_font, text.c_str(), col_text);
        if (surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_FreeSurface(surface);
            if (tex) {
                int tw, th;
                SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
                SDL_Rect dst = {rect.x + (rect.w - tw) / 2, rect.y + (rect.h - th) / 2, tw, th};
                SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
            }
        }
    }
    
    return clicked;
}

void Launcher::RenderTopBar() {
    SDL_Rect bar = {0, 0, m_windowWidth, 60};
    DrawRect(bar, {40, 40, 55, 255});
    
    RenderText("krkrv8a", 20, 18, col_accent, 24);
    
    SDL_Rect tabLib = {150, 15, 80, 32};
    SDL_Rect tabSet = {240, 15, 80, 32};
    SDL_Rect tabAbout = {330, 15, 80, 32};
    
    if (Button("Library", tabLib, 
        m_mode == LauncherMode::Library ? col_accent : col_card, col_cardHover)) {
        m_mode = LauncherMode::Library;
    }
    if (Button("Settings", tabSet,
        m_mode == LauncherMode::Settings ? col_accent : col_card, col_cardHover)) {
        m_mode = LauncherMode::Settings;
    }
    if (Button("About", tabAbout,
        m_mode == LauncherMode::About ? col_accent : col_card, col_cardHover)) {
        m_mode = LauncherMode::About;
    }
    
    SDL_Rect addBtn = {m_windowWidth - 120, 15, 100, 32};
    if (Button("+ Add Game", addBtn, col_button, col_buttonHover)) {
        m_showAddDialog = true;
        m_inputPath.clear();
        m_inputActive = true;
    }
}

void Launcher::RenderGameGrid() {
    int gridY = 80;
    int cardW = 200;
    int cardH = 100;
    int gap = 15;
    int cols = std::max(1, (m_windowWidth - 40) / (cardW + gap));
    int startX = (m_windowWidth - cols * (cardW + gap) + gap) / 2;
    
    m_hoveredIndex = -1;
    
    for (size_t i = 0; i < m_games.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = startX + col * (cardW + gap);
        int y = gridY + row * (cardH + gap) - m_scrollOffset;
        
        if (y + cardH < 60 || y > m_windowHeight) continue;
        
        bool hovered = (m_mouseX >= x && m_mouseX < x + cardW &&
                        m_mouseY >= y && m_mouseY < y + cardH);
        if (hovered) m_hoveredIndex = (int)i;
        
        SDL_Rect card = {x, y, cardW, cardH};
        DrawRoundedRect(card, hovered ? col_cardHover : col_card, 8);
        
        if (m_games[i].iconTexture) {
            SDL_Rect iconRect = {x + 8, y + 8, 84, 84};
            SDL_RenderCopy(m_renderer, m_games[i].iconTexture, nullptr, &iconRect);
        } else {
            SDL_Rect iconRect = {x + 8, y + 8, 84, 84};
            DrawRect(iconRect, {60, 60, 75, 255});
            RenderText("[GAME]", x + 25, y + 40, col_textDim, 12);
        }
        
        std::string displayName = m_games[i].name;
        if (displayName.length() > 14) displayName = displayName.substr(0, 13) + "...";
        RenderText(displayName, x + 100, y + 15, col_text, 14);
        
        RenderText(m_games[i].isXP3 ? "XP3" : "Folder", x + 100, y + 40, col_textDim, 12);
        
        SDL_Rect removeBtn = {x + cardW - 25, y + 5, 20, 20};
        bool removeHovered = (m_mouseX >= removeBtn.x && m_mouseX < removeBtn.x + removeBtn.w &&
                              m_mouseY >= removeBtn.y && m_mouseY < removeBtn.y + removeBtn.h);
        DrawRect(removeBtn, removeHovered ? (SDL_Color){180, 60, 60, 255} : (SDL_Color){80, 50, 50, 255});
        RenderText("x", removeBtn.x + 6, removeBtn.y + 2, col_text, 14);
        
        if (removeHovered && m_mouseReleased) {
            RemoveGame((int)i);
            return;
        }
        
        if (hovered && m_mouseReleased) {
            uint32_t now = SDL_GetTicks();
            if (m_lastClickIndex == (int)i && (now - m_lastClickTime) < 500) {
                m_selectedGamePath = m_games[i].path;
                m_launchGame = true;
                m_running = false;
                return;
            }
            m_lastClickIndex = (int)i;
            m_lastClickTime = now;
        }
    }
    
    if (m_games.empty()) {
        int cx = m_windowWidth / 2;
        int cy = m_windowHeight / 2;
        RenderText("No games in library", cx - 80, cy - 30, col_textDim, 18);
        RenderText("Click '+ Add Game' to add a game folder or XP3 file", cx - 160, cy + 5, col_textDim, 14);
    }
}

void Launcher::RenderAddGameDialog() {
    SDL_Rect overlay = {0, 0, m_windowWidth, m_windowHeight};
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(m_renderer, &overlay);
    
    int dlgW = 500;
    int dlgH = 200;
    int dlgX = (m_windowWidth - dlgW) / 2;
    int dlgY = (m_windowHeight - dlgH) / 2;
    SDL_Rect dlg = {dlgX, dlgY, dlgW, dlgH};
    DrawRoundedRect(dlg, {55, 55, 70, 255}, 12);
    
    RenderText("Add Game", dlgX + 20, dlgY + 15, col_text, 20);
    
    SDL_Rect inputRect = {dlgX + 20, dlgY + 60, dlgW - 40, 36};
    DrawRect(inputRect, col_input);
    if (m_inputActive) {
        SDL_Rect border = {inputRect.x - 1, inputRect.y - 1, inputRect.w + 2, inputRect.h + 2};
        SDL_SetRenderDrawColor(m_renderer, col_accent.r, col_accent.g, col_accent.b, 255);
        SDL_RenderDrawRect(m_renderer, &border);
    }
    RenderText(m_inputPath.empty() ? "Enter game path..." : m_inputPath, 
               inputRect.x + 8, inputRect.y + 10, 
               m_inputPath.empty() ? col_textDim : col_text, 14);
    
    SDL_Rect browseBtn = {dlgX + dlgW - 120, dlgY + 105, 100, 30};
    if (Button("Browse...", browseBtn, col_button, col_buttonHover)) {
    }
    
    SDL_Rect okBtn = {dlgX + dlgW - 220, dlgY + dlgH - 45, 90, 32};
    SDL_Rect cancelBtn = {dlgX + dlgW - 120, dlgY + dlgH - 45, 100, 32};
    
    if (Button("OK", okBtn, col_button, col_buttonHover)) {
        if (!m_inputPath.empty()) AddGame(m_inputPath);
        m_showAddDialog = false;
        m_inputActive = false;
    }
    
    if (Button("Cancel", cancelBtn, col_card, col_cardHover)) {
        m_showAddDialog = false;
        m_inputActive = false;
    }
}

void Launcher::RenderLibrary() {
    RenderGameGrid();
}

void Launcher::RenderSettings() {
    int y = 80;
    RenderText("Settings", 20, y, col_text, 24);
    y += 50;
    RenderText("Engine Version: " KRKR_VERSION_STRING, 20, y, col_textDim, 16);
    y += 30;
    RenderText("Platform: " + std::string(Platform::GetPlatformName()), 20, y, col_textDim, 16);
    y += 30;
    RenderText("Architecture: " + std::string(Platform::GetArchName()), 20, y, col_textDim, 16);
    y += 30;
    RenderText("CPU: " + Platform::GetCPUModel(), 20, y, col_textDim, 16);
    y += 30;
    RenderText("CPU Cores: " + std::to_string(Platform::GetCPUCount()), 20, y, col_textDim, 16);
    y += 30;
    RenderText("NEON Support: " + std::string(Platform::HasNEON() ? "Yes" : "No"), 20, y, col_textDim, 16);
    y += 30;
    RenderText("Data Path: " + Platform::GetDataPath(), 20, y, col_textDim, 16);
    y += 30;
    RenderText("Save Path: " + Platform::GetSavePath(), 20, y, col_textDim, 16);
}

void Launcher::RenderAbout() {
    int y = 80;
    RenderText("About krkrv8a", 20, y, col_text, 24);
    y += 50;
    RenderText("krkrv8a - Kirikiri2 Engine for ARMv8a Linux", 20, y, col_text, 18);
    y += 35;
    RenderText("Version " KRKR_VERSION_STRING, 20, y, col_textDim, 16);
    y += 30;
    RenderText("A lightweight visual novel engine for ARM64 Linux.", 20, y, col_textDim, 16);
    y += 30;
    RenderText("https://github.com/Zaxver/krkrv8a", 20, y, col_accent, 16);
    y += 40;
    RenderText("References:", 20, y, col_text, 16);
    y += 25;
    RenderText("- krkrsdl2 (SDL2 Kirikiri2 backend)", 20, y, col_textDim, 14);
    y += 25;
    RenderText("- krkr2-linux (Cocos2d-x Kirikiri2)", 20, y, col_textDim, 14);
    y += 25;
    RenderText("- Kirikiroid2 (Android Kirikiri2)", 20, y, col_textDim, 14);
    y += 25;
    RenderText("- tyranor (Kirikiri2 fork)", 20, y, col_textDim, 14);
}

void Launcher::Render() {
    SDL_SetRenderDrawColor(m_renderer, col_bg.r, col_bg.g, col_bg.b, 255);
    SDL_RenderClear(m_renderer);
    
    RenderTopBar();
    
    switch (m_mode) {
        case LauncherMode::Library:  RenderLibrary(); break;
        case LauncherMode::Settings:  RenderSettings(); break;
        case LauncherMode::About:    RenderAbout(); break;
    }
    
    if (m_showAddDialog) RenderAddGameDialog();
}

void Launcher::HandleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            m_running = false;
            break;
        case SDL_MOUSEMOTION:
            m_mouseX = event.motion.x;
            m_mouseY = event.motion.y;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mousePressed = true;
                m_mouseReleased = false;
                m_mouseX = event.button.x;
                m_mouseY = event.button.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mouseReleased = true;
                m_mousePressed = false;
                m_mouseX = event.button.x;
                m_mouseY = event.button.y;
            }
            break;
        case SDL_MOUSEWHEEL:
            m_scrollOffset += event.wheel.y * 30;
            if (m_scrollOffset < 0) m_scrollOffset = 0;
            break;
        case SDL_KEYDOWN:
            if (m_showAddDialog && m_inputActive) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && !m_inputPath.empty()) {
                    m_inputPath.pop_back();
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    if (!m_inputPath.empty()) AddGame(m_inputPath);
                    m_showAddDialog = false;
                    m_inputActive = false;
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_showAddDialog = false;
                    m_inputActive = false;
                }
            } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                if (m_showAddDialog) {
                    m_showAddDialog = false;
                    m_inputActive = false;
                } else {
                    m_running = false;
                }
            }
            break;
        case SDL_TEXTINPUT:
            if (m_showAddDialog && m_inputActive) {
                m_inputPath += event.text.text;
            }
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                m_windowWidth = event.window.data1;
                m_windowHeight = event.window.data2;
            }
            break;
    }
}

void Launcher::Run() {
    m_running = true;
    SDL_StartTextInput();
    
    while (m_running) {
        SDL_Event event;
        m_mouseReleased = false;
        
        while (SDL_PollEvent(&event)) {
            HandleEvent(event);
        }
        
        Render();
        SDL_RenderPresent(m_renderer);
        
        SDL_Delay(1);
    }
    
    SDL_StopTextInput();
}