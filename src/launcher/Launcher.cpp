#include "Launcher.h"
#include "version.h"
#include "Platform.h"
#include <SDL2/SDL_image.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdio>

namespace fs = std::filesystem;

// ====================================================================
// Helpers
// ====================================================================

int Launcher::S(int val) const {
    return (int)(val * m_dpiScale);
}

TTF_Font* Launcher::GetFont(int size) const {
    int scaled = (int)(size * m_dpiScale);
    if (scaled >= 20 && m_fontLarge) {
        // For large text, use m_fontLarge but with different point sizes
        // TTF_OpenFont has already been set; we use what we have
        return m_fontLarge;
    } else if (scaled <= 12 && m_fontSmall) {
        return m_fontSmall;
    }
    return m_font;
}

void Launcher::DetectDPI() {
    m_androidDPI = 0;
    
    // Read Android DPI for display purposes
    FILE* fp = popen("getprop ro.sf.lcd_density 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            m_androidDPI = atoi(buf);
        }
        pclose(fp);
    }
    
    if (m_androidDPI == 0) {
        float ddpi = 0;
        int idx = SDL_GetWindowDisplayIndex(m_window);
        if (SDL_GetDisplayDPI(idx, nullptr, &ddpi, nullptr) == 0 && ddpi > 0) {
            m_androidDPI = (int)ddpi;
        }
    }
    if (m_androidDPI == 0) m_androidDPI = 160;
    
    // Scale: if auto, use detected. Otherwise use manual DPI.
    if (m_settings.autoDetectDPI) {
        m_dpiScale = m_androidDPI / 160.0f;
    } else {
        m_dpiScale = m_settings.manualDPI / 160.0f;
    }
    
    if (m_dpiScale < 0.5f) m_dpiScale = 0.5f;
    if (m_dpiScale > 4.0f) m_dpiScale = 4.0f;
    
    TVP_LOG_INFO("Launcher: DPI=%d, manual=%d, scale=%.2f, auto=%d",
                 m_androidDPI, m_settings.manualDPI, m_dpiScale, m_settings.autoDetectDPI);
}

std::string Launcher::GetGPUInfo() const {
    if (m_gpuInfoCached) return m_gpuInfo;
    
    FILE* fp = popen("glxinfo 2>/dev/null | grep 'OpenGL renderer'", "r");
    if (fp) {
        char buf[256];
        if (fgets(buf, sizeof(buf), fp)) {
            const char* p = strchr(buf, ':');
            if (p) {
                std::string s(p + 2);
                s.erase(s.find_last_not_of(" \n\r\t") + 1);
                const_cast<std::string&>(m_gpuInfo) = s;
            }
        }
        pclose(fp);
    }
    
    if (m_gpuInfo.empty()) const_cast<std::string&>(m_gpuInfo) = "Unknown";
    
    const_cast<bool&>(m_hwAccel) = (m_gpuInfo.find("llvmpipe") == std::string::npos &&
                 m_gpuInfo.find("softpipe") == std::string::npos &&
                 m_gpuInfo.find("swrast") == std::string::npos &&
                 m_gpuInfo.find("Unknown") == std::string::npos);
    
    const_cast<bool&>(m_gpuInfoCached) = true;
    return m_gpuInfo;
}

std::string Launcher::GetSDLRendererInfo() const {
    SDL_RendererInfo info;
    if (SDL_GetRenderDriverInfo(0, &info) == 0) {
        return std::string(info.name);
    }
    return "unknown";
}

bool Launcher::IsHardwareAccelerated() const {
    GetGPUInfo(); // ensure cached
    return m_hwAccel;
}

// ====================================================================
// File Browser
// ====================================================================

void Launcher::OpenFileBrowser(const std::string& startDir, std::function<void(const std::string&)> onSelected) {
    m_fileBrowser.open = true;
    m_fileBrowser.currentDir = startDir;
    m_fileBrowser.onSelected = onSelected;
    m_fileBrowser.scrollOffset = 0;
    RefreshFileBrowser();
}

void Launcher::RefreshFileBrowser() {
    m_fileBrowser.entries.clear();
    
    std::string dir = m_fileBrowser.currentDir;
    if (dir.empty()) dir = "/";
    
    // Add parent dir
    fs::path p(dir);
    if (p.has_parent_path()) {
        m_fileBrowser.entries.push_back(".. (" + p.parent_path().string() + ")");
    }
    
    // List directory
    try {
        std::vector<std::string> dirs, files;
        for (const auto& entry : fs::directory_iterator(dir)) {
            std::string name = entry.path().filename().string();
            if (name.empty() || name[0] == '.') continue; // skip hidden
            
            if (entry.is_directory()) {
                dirs.push_back("[" + name + "]");
            } else {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".xp3" || ext == ".zip") {
                    files.push_back(name);
                }
            }
        }
        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());
        
        for (auto& d : dirs) m_fileBrowser.entries.push_back(d);
        for (auto& f : files) m_fileBrowser.entries.push_back(f);
    } catch (...) {
        m_fileBrowser.entries.push_back("(cannot read directory)");
    }
}

// ====================================================================
// Init / Shutdown
// ====================================================================

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
    const char* fontPaths[] = {
        "/data/data/com.termux/files/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/data/data/com.termux/files/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/MiSansVF.ttf",
        "/system/fonts/DroidSans.ttf",
        "/system/fonts/Roboto-Regular.ttf",
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
    
    std::string fontPath;
    for (int i = 0; fontPaths[i]; i++) {
        if (fs::exists(fontPaths[i])) {
            fontPath = fontPaths[i];
            break;
        }
    }
    
    // Fallback: recursive search
    if (fontPath.empty()) {
        const char* searchDirs[] = {
            "/data/data/com.termux/files/usr/share/fonts",
            "/system/fonts",
            "/usr/share/fonts",
            nullptr
        };
        for (int d = 0; searchDirs[d] && fontPath.empty(); d++) {
            if (!fs::exists(searchDirs[d])) continue;
            for (const auto& entry : fs::recursive_directory_iterator(searchDirs[d])) {
                if (!entry.is_regular_file()) continue;
                auto path = entry.path().string();
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".ttf" || ext == ".ttc" || ext == ".otf") {
                    fontPath = path;
                    break;
                }
            }
        }
    }
    
    // User-specified font from settings
    if (!m_settings.fontPath.empty() && fs::exists(m_settings.fontPath)) {
        fontPath = m_settings.fontPath;
    }
    
    if (!fontPath.empty()) {
        int base = (int)(16 * m_dpiScale);
        int large = (int)(24 * m_dpiScale);
        int small = (int)(12 * m_dpiScale);
        m_font = TTF_OpenFont(fontPath.c_str(), base);
        m_fontLarge = TTF_OpenFont(fontPath.c_str(), large);
        m_fontSmall = TTF_OpenFont(fontPath.c_str(), small);
        if (m_font) {
            TVP_LOG_INFO("Launcher: Font loaded: %s (base=%d)", fontPath.c_str(), base);
        }
    }
    
    if (!m_font) {
        TVP_LOG_ERROR("Launcher: No font found!");
    }
}

void Launcher::LoadSettings() {
    std::string file = m_configPath + "/settings.conf";
    std::ifstream f(file);
    if (!f.is_open()) return;
    
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        
        if (key == "window_width") m_settings.windowWidth = atoi(val.c_str());
        else if (key == "window_height") m_settings.windowHeight = atoi(val.c_str());
        else if (key == "fullscreen") m_settings.fullscreen = (val == "1");
        else if (key == "vsync") m_settings.vsync = (val == "1");
        else if (key == "audio_volume") m_settings.audioVolume = atoi(val.c_str());
        else if (key == "font_path") m_settings.fontPath = val;
        else if (key == "ui_scale") m_settings.uiScale = atoi(val.c_str());
        else if (key == "auto_dpi") m_settings.autoDetectDPI = (val == "1");
        else if (key == "manual_dpi") m_settings.manualDPI = atoi(val.c_str());
    }
}

void Launcher::SaveSettings() {
    std::string file = m_configPath + "/settings.conf";
    std::ofstream f(file);
    if (!f.is_open()) return;
    
    f << "window_width=" << m_settings.windowWidth << "\n";
    f << "window_height=" << m_settings.windowHeight << "\n";
    f << "fullscreen=" << (m_settings.fullscreen ? "1" : "0") << "\n";
    f << "vsync=" << (m_settings.vsync ? "1" : "0") << "\n";
    f << "audio_volume=" << m_settings.audioVolume << "\n";
    f << "font_path=" << m_settings.fontPath << "\n";
    f << "ui_scale=" << m_settings.uiScale << "\n";
    f << "auto_dpi=" << (m_settings.autoDetectDPI ? "1" : "0") << "\n";
    f << "manual_dpi=" << m_settings.manualDPI << "\n";
}

bool Launcher::Initialize(SDL_Window* window, SDL_Renderer* renderer) {
    m_window = window;
    m_renderer = renderer;
    
    SDL_GetWindowSize(window, &m_windowWidth, &m_windowHeight);
    
    LoadSettings();
    DetectDPI();
    LoadFont();
    LoadLibrary();
    
    // Cache GPU info early
    GetGPUInfo();
    
    TVP_LOG_INFO("Launcher: Initialized (DPI=%d, scale=%.2f, %d games)", 
                 m_androidDPI, m_dpiScale, (int)m_games.size());
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

// ====================================================================
// Library
// ====================================================================

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
    if (fs::is_regular_file(gamePath)) gameDir = gameDir.parent_path();
    
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
        if (g.path == path) return;
    }
    GameEntry entry;
    entry.name = name;
    entry.path = path;
    entry.isXP3 = isXP3;
    entry.iconTexture = LoadGameIcon(path);
    m_games.push_back(entry);
    SaveLibrary();
    TVP_LOG_INFO("Launcher: Added game: %s", name.c_str());
}

void Launcher::RemoveGame(int index) {
    if (index < 0 || index >= (int)m_games.size()) return;
    if (m_games[index].iconTexture) SDL_DestroyTexture(m_games[index].iconTexture);
    m_games.erase(m_games.begin() + index);
    SaveLibrary();
}

// ====================================================================
// Rendering helpers
// ====================================================================

void Launcher::DrawRect(SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
}

void Launcher::DrawRoundedRect(SDL_Rect rect, SDL_Color color, int radius) {
    DrawRect(rect, color);
}

void Launcher::RenderText(const std::string& text, int x, int y, SDL_Color color, int size) {
    if (!m_font) return;
    TTF_Font* font = GetFont(size);
    if (!font) return;
    
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
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
        TTF_Font* font = GetFont(16);
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), col_text);
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

// ====================================================================
// Top bar
// ====================================================================

void Launcher::RenderTopBar() {
    SDL_Rect bar = {0, 0, m_windowWidth, S(60)};
    DrawRect(bar, {40, 40, 55, 255});
    
    RenderText("krkrv8a", S(20), S(18), col_accent, 24);
    
    int tabW = S(90), tabH = S(32), tabY = S(15), tabX = S(150);
    SDL_Rect tabLib = {tabX, tabY, tabW, tabH};
    SDL_Rect tabSet = {tabX + tabW + S(5), tabY, tabW, tabH};
    SDL_Rect tabAbout = {tabX + (tabW + S(5)) * 2, tabY, tabW, tabH};
    
    if (Button("Library", tabLib, m_mode == LauncherMode::Library ? col_accent : col_card, col_cardHover))
        m_mode = LauncherMode::Library;
    if (Button("Settings", tabSet, m_mode == LauncherMode::Settings ? col_accent : col_card, col_cardHover))
        m_mode = LauncherMode::Settings;
    if (Button("About", tabAbout, m_mode == LauncherMode::About ? col_accent : col_card, col_cardHover))
        m_mode = LauncherMode::About;
    
    // GPU status indicator (top right)
    bool hw = IsHardwareAccelerated();
    std::string gpuStatus = hw ? "HW Accel" : "SW Render";
    SDL_Color gpuCol = hw ? (SDL_Color){60, 160, 60, 255} : (SDL_Color){160, 100, 40, 255};
    int gpuW = S(90), gpuH = S(20);
    SDL_Rect gpuRect = {m_windowWidth - S(230), S(20), gpuW, gpuH};
    DrawRect(gpuRect, gpuCol);
    RenderText(gpuStatus, gpuRect.x + S(8), gpuRect.y + S(2), col_text, 12);
    
    // Add Game button
    SDL_Rect addBtn = {m_windowWidth - S(130), S(15), S(110), S(32)};
    if (Button("+ Add Game", addBtn, col_button, col_buttonHover)) {
        m_showAddDialog = true;
        m_inputPath.clear();
        m_inputActive = true;
    }
}

// ====================================================================
// Game grid
// ====================================================================

void Launcher::RenderGameGrid() {
    int gridY = S(80);
    int cardW = S(220), cardH = S(110), gap = S(15);
    int cols = std::max(1, (m_windowWidth - S(40)) / (cardW + gap));
    int startX = (m_windowWidth - cols * (cardW + gap) + gap) / 2;
    
    m_hoveredIndex = -1;
    
    for (size_t i = 0; i < m_games.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = startX + col * (cardW + gap);
        int y = gridY + row * (cardH + gap) - m_scrollOffset;
        
        if (y + cardH < S(60) || y > m_windowHeight) continue;
        
        bool hovered = (m_mouseX >= x && m_mouseX < x + cardW &&
                        m_mouseY >= y && m_mouseY < y + cardH);
        if (hovered) m_hoveredIndex = (int)i;
        
        SDL_Rect card = {x, y, cardW, cardH};
        DrawRoundedRect(card, hovered ? col_cardHover : col_card, 8);
        
        int iconSize = S(90);
        if (m_games[i].iconTexture) {
            SDL_Rect iconRect = {x + S(8), y + S(8), iconSize, iconSize};
            SDL_RenderCopy(m_renderer, m_games[i].iconTexture, nullptr, &iconRect);
        } else {
            SDL_Rect iconRect = {x + S(8), y + S(8), iconSize, iconSize};
            DrawRect(iconRect, {60, 60, 75, 255});
            RenderText("[GAME]", x + S(25), y + S(40), col_textDim, 12);
        }
        
        std::string displayName = m_games[i].name;
        if (displayName.length() > 14) displayName = displayName.substr(0, 13) + "...";
        RenderText(displayName, x + iconSize + S(16), y + S(15), col_text, 14);
        RenderText(m_games[i].isXP3 ? "XP3" : "Folder", x + iconSize + S(16), y + S(40), col_textDim, 12);
        
        // Remove button
        SDL_Rect removeBtn = {x + cardW - S(28), y + S(5), S(22), S(22)};
        bool removeHovered = (m_mouseX >= removeBtn.x && m_mouseX < removeBtn.x + removeBtn.w &&
                              m_mouseY >= removeBtn.y && m_mouseY < removeBtn.y + removeBtn.h);
        DrawRect(removeBtn, removeHovered ? (SDL_Color){180, 60, 60, 255} : (SDL_Color){80, 50, 50, 255});
        RenderText("x", removeBtn.x + S(6), removeBtn.y + S(2), col_text, 14);
        
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
        RenderText("No games in library", cx - S(100), cy - S(30), col_textDim, 18);
        RenderText("Click '+ Add Game' to add a game folder or XP3 file", cx - S(180), cy + S(5), col_textDim, 14);
    }
}

// ====================================================================
// Add Game dialog
// ====================================================================

void Launcher::RenderAddGameDialog() {
    SDL_Rect overlay = {0, 0, m_windowWidth, m_windowHeight};
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(m_renderer, &overlay);
    
    int dlgW = S(520), dlgH = S(220);
    int dlgX = (m_windowWidth - dlgW) / 2;
    int dlgY = (m_windowHeight - dlgH) / 2;
    SDL_Rect dlg = {dlgX, dlgY, dlgW, dlgH};
    DrawRoundedRect(dlg, {55, 55, 70, 255}, 12);
    
    RenderText("Add Game", dlgX + S(20), dlgY + S(15), col_text, 20);
    
    // Path input
    SDL_Rect inputRect = {dlgX + S(20), dlgY + S(60), dlgW - S(40), S(36)};
    DrawRect(inputRect, m_inputActive ? col_inputFocus : col_input);
    if (m_inputActive) {
        SDL_Rect border = {inputRect.x - 1, inputRect.y - 1, inputRect.w + 2, inputRect.h + 2};
        SDL_SetRenderDrawColor(m_renderer, col_accent.r, col_accent.g, col_accent.b, 255);
        SDL_RenderDrawRect(m_renderer, &border);
    }
    RenderText(m_inputPath.empty() ? "Enter game path..." : m_inputPath,
               inputRect.x + S(8), inputRect.y + S(10),
               m_inputPath.empty() ? col_textDim : col_text, 14);
    
    // Browse button - NOW WORKS
    SDL_Rect browseBtn = {dlgX + dlgW - S(130), dlgY + S(105), S(110), S(30)};
    if (Button("Browse...", browseBtn, col_button, col_buttonHover)) {
        std::string startDir = m_inputPath.empty() ? m_lastBrowsePath : m_inputPath;
        if (startDir.empty()) startDir = "/";
        // If it's a file, start from its parent
        if (fs::exists(startDir) && fs::is_regular_file(startDir)) {
            startDir = fs::path(startDir).parent_path().string();
        }
        OpenFileBrowser(startDir, [this](const std::string& selected) {
            m_inputPath = selected;
            m_inputActive = true;
        });
    }
    
    // OK / Cancel
    SDL_Rect okBtn = {dlgX + dlgW - S(230), dlgY + dlgH - S(45), S(90), S(32)};
    SDL_Rect cancelBtn = {dlgX + dlgW - S(130), dlgY + dlgH - S(45), S(110), S(32)};
    
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

// ====================================================================
// File browser dialog
// ====================================================================

void Launcher::RenderFileBrowser() {
    // Full screen overlay
    SDL_Rect overlay = {0, 0, m_windowWidth, m_windowHeight};
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(m_renderer, &overlay);
    
    int bw = S(600), bh = S(480);
    int bx = (m_windowWidth - bw) / 2;
    int by = (m_windowHeight - bh) / 2;
    
    SDL_Rect dlg = {bx, by, bw, bh};
    DrawRoundedRect(dlg, {50, 50, 65, 255}, 12);
    
    // Title bar
    RenderText("Browse - " + m_fileBrowser.currentDir, bx + S(20), by + S(15), col_text, 18);
    
    // Up button
    SDL_Rect upBtn = {bx + bw - S(160), by + S(12), S(60), S(28)};
    if (Button("Up", upBtn, col_button, col_buttonHover)) {
        fs::path p(m_fileBrowser.currentDir);
        if (p.has_parent_path()) {
            m_fileBrowser.currentDir = p.parent_path().string();
            RefreshFileBrowser();
            m_fileBrowser.scrollOffset = 0;
        }
    }
    
    // Home button
    SDL_Rect homeBtn = {bx + bw - S(90), by + S(12), S(70), S(28)};
    if (Button("Home", homeBtn, col_button, col_buttonHover)) {
        const char* home = getenv("HOME");
        m_fileBrowser.currentDir = home ? home : "/";
        RefreshFileBrowser();
        m_fileBrowser.scrollOffset = 0;
    }
    
    // List area
    int listY = by + S(55);
    int listH = bh - S(55) - S(50);
    int itemH = S(28);
    int maxVisible = listH / itemH;
    
    // Clip area
    SDL_Rect clipRect = {bx + S(10), listY, bw - S(20), listH};
    SDL_RenderSetClipRect(m_renderer, &clipRect);
    
    for (int i = 0; i < (int)m_fileBrowser.entries.size(); i++) {
        int y = listY + i * itemH - m_fileBrowser.scrollOffset;
        if (y + itemH < listY || y > listY + listH) continue;
        
        bool isDir = (!m_fileBrowser.entries[i].empty() && m_fileBrowser.entries[i][0] == '[');
        bool isParent = (!m_fileBrowser.entries[i].empty() && m_fileBrowser.entries[i][0] == '.');
        
        bool hovered = (m_mouseX >= bx + S(10) && m_mouseX < bx + bw - S(10) &&
                        m_mouseY >= y && m_mouseY < y + itemH);
        
        SDL_Rect itemRect = {bx + S(10), y, bw - S(20), itemH};
        DrawRect(itemRect, hovered ? col_cardHover : (SDL_Color){45, 45, 60, 255});
        
        SDL_Color textCol = isDir ? col_accent : col_text;
        if (isParent) textCol = col_textDim;
        RenderText(m_fileBrowser.entries[i], bx + S(20), y + S(5), textCol, 14);
        
        if (hovered && m_mouseReleased) {
            if (isParent) {
                fs::path p(m_fileBrowser.currentDir);
                if (p.has_parent_path()) {
                    m_fileBrowser.currentDir = p.parent_path().string();
                    RefreshFileBrowser();
                    m_fileBrowser.scrollOffset = 0;
                }
            } else if (isDir) {
                // Extract dir name from [name]
                std::string dirName = m_fileBrowser.entries[i].substr(1, m_fileBrowser.entries[i].size() - 2);
                m_fileBrowser.currentDir += "/" + dirName;
                RefreshFileBrowser();
                m_fileBrowser.scrollOffset = 0;
            } else {
                // File selected
                std::string selected = m_fileBrowser.currentDir + "/" + m_fileBrowser.entries[i];
                m_fileBrowser.open = false;
                if (m_fileBrowser.onSelected) {
                    m_fileBrowser.onSelected(selected);
                }
                SDL_RenderSetClipRect(m_renderer, nullptr);
                return;
            }
        }
    }
    
    SDL_RenderSetClipRect(m_renderer, nullptr);
    
    // Cancel button
    SDL_Rect cancelBtn = {bx + bw - S(100), by + bh - S(38), S(80), S(30)};
    if (Button("Cancel", cancelBtn, col_card, col_cardHover)) {
        m_fileBrowser.open = false;
    }
    
    // Select current dir button
    SDL_Rect selectDirBtn = {bx + S(20), by + bh - S(38), S(140), S(30)};
    if (Button("Select This Folder", selectDirBtn, col_button, col_buttonHover)) {
        std::string selected = m_fileBrowser.currentDir;
        m_fileBrowser.open = false;
        if (m_fileBrowser.onSelected) {
            m_fileBrowser.onSelected(selected);
        }
    }
}

// ====================================================================
// Settings page
// ====================================================================

void Launcher::RenderSettings() {
    int x = S(20);
    int y = S(80) - m_settingsScroll;
    int lineH = S(35);
    int valW = S(200);
    int labelW = S(280);
    int totalW = m_windowWidth - S(40);
    
    RenderText("Settings", x, y, col_text, 24);
    y += S(55);
    
    // --- Display section ---
    RenderText("--- Display ---", x, y, col_accent, 16);
    y += lineH;
    
    // Window resolution
    RenderText("Window Size:", x, y + S(5), col_text, 16);
    std::string resStr = std::to_string(m_settings.windowWidth) + " x " + std::to_string(m_settings.windowHeight);
    RenderText(resStr, x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    // Fullscreen toggle
    RenderText("Fullscreen:", x, y + S(5), col_text, 16);
    SDL_Rect fsBtn = {x + labelW, y, S(50), S(28)};
    if (Button(m_settings.fullscreen ? "ON" : "OFF", fsBtn, 
        m_settings.fullscreen ? (SDL_Color){60, 140, 60, 255} : col_card, col_cardHover)) {
        m_settings.fullscreen = !m_settings.fullscreen;
        SaveSettings();
    }
    y += lineH;
    
    // VSync toggle
    RenderText("VSync:", x, y + S(5), col_text, 16);
    SDL_Rect vsBtn = {x + labelW, y, S(50), S(28)};
    if (Button(m_settings.vsync ? "ON" : "OFF", vsBtn,
        m_settings.vsync ? (SDL_Color){60, 140, 60, 255} : col_card, col_cardHover)) {
        m_settings.vsync = !m_settings.vsync;
        SaveSettings();
    }
    y += lineH;
    
    // DPI display
    RenderText("Screen DPI (detected):", x, y + S(5), col_text, 16);
    RenderText(std::to_string(m_androidDPI), x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    // Auto DPI toggle
    RenderText("Auto DPI (use detected):", x, y + S(5), col_text, 16);
    SDL_Rect adBtn = {x + labelW, y, S(50), S(28)};
    if (Button(m_settings.autoDetectDPI ? "ON" : "OFF", adBtn,
        m_settings.autoDetectDPI ? (SDL_Color){60, 140, 60, 255} : col_card, col_cardHover)) {
        m_settings.autoDetectDPI = !m_settings.autoDetectDPI;
        DetectDPI();
        LoadFont();
        SaveSettings();
    }
    y += lineH;
    
    // Manual DPI slider (only meaningful when auto is OFF)
    RenderText("Manual DPI:", x, y + S(5), col_text, 16);
    RenderText(std::to_string(m_settings.manualDPI) + "  (scale=" + 
               std::to_string((int)(m_settings.manualDPI / 160.0f * 100)) + "%)",
               x + labelW, y + S(5), col_textDim, 16);
    // DPI slider: 80 to 800
    SDL_Rect dpiTrack = {x + labelW + S(180), y + S(8), S(200), S(12)};
    DrawRect(dpiTrack, {50, 50, 60, 255});
    int dpiRange = 800 - 80;
    int dpiPos = (m_settings.manualDPI - 80) * S(200) / dpiRange;
    if (dpiPos < 0) dpiPos = 0;
    if (dpiPos > S(200)) dpiPos = S(200);
    SDL_Rect dpiFill = {dpiTrack.x, dpiTrack.y, dpiPos, S(12)};
    DrawRect(dpiFill, col_accent);
    SDL_Rect dpiHandle = {dpiTrack.x + dpiPos - S(6), dpiTrack.y - S(4), S(12), S(20)};
    DrawRect(dpiHandle, col_text);
    // Preset buttons
    int presetY = y + S(8);
    int presetX = dpiTrack.x + dpiTrack.w + S(10);
    const char* presets[] = {"80", "120", "160", "240", "320", nullptr};
    int presetVals[] = {80, 120, 160, 240, 320};
    for (int i = 0; presets[i]; i++) {
        SDL_Rect pBtn = {presetX + i * S(35), presetY, S(30), S(20)};
        if (Button(presets[i], pBtn,
            m_settings.manualDPI == presetVals[i] ? col_accent : col_card, col_cardHover)) {
            m_settings.manualDPI = presetVals[i];
            m_settings.autoDetectDPI = false;
            DetectDPI();
            LoadFont();
            SaveSettings();
        }
    }
    // Slider interaction
    bool dpiHover = (m_mouseX >= dpiTrack.x - S(5) && m_mouseX < dpiTrack.x + dpiTrack.w + S(5) &&
                    m_mouseY >= dpiTrack.y - S(5) && m_mouseY < dpiTrack.y + dpiTrack.h + S(5));
    if (dpiHover && m_mousePressed) {
        m_editingItem = 3; // editing DPI slider
    }
    if (m_editingItem == 3 && m_mousePressed) {
        int rel = m_mouseX - dpiTrack.x;
        if (rel < 0) rel = 0;
        if (rel > S(200)) rel = S(200);
        m_settings.manualDPI = 80 + rel * dpiRange / S(200);
        m_settings.autoDetectDPI = false;
        DetectDPI();
        LoadFont();
        SaveSettings();
    }
    y += lineH;
    y += S(10);
    
    // --- GPU/Rendering section ---
    RenderText("--- Rendering ---", x, y, col_accent, 16);
    y += lineH;
    
    RenderText("GPU:", x, y + S(5), col_text, 16);
    RenderText(GetGPUInfo(), x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    RenderText("Hardware Acceleration:", x, y + S(5), col_text, 16);
    bool hw = IsHardwareAccelerated();
    RenderText(hw ? "YES" : "NO (Software)", x + labelW, y + S(5), 
               hw ? (SDL_Color){80, 200, 80, 255} : (SDL_Color){220, 120, 40, 255}, 16);
    y += lineH;
    
    if (!hw) {
        RenderText("Tip: llvmpipe = software rendering.", x, y + S(5), col_textDim, 12);
        y += S(20);
        RenderText("Install mesa-freedreno/turnip for HW accel.", x, y + S(5), col_textDim, 12);
        y += S(20);
    }
    y += S(5);
    
    // --- Audio section ---
    RenderText("--- Audio ---", x, y, col_accent, 16);
    y += lineH;
    
    RenderText("Master Volume:", x, y + S(5), col_text, 16);
    RenderText(std::to_string(m_settings.audioVolume), x + labelW, y + S(5), col_textDim, 16);
    // Volume slider
    SDL_Rect volTrack = {x + labelW + S(50), y + S(8), S(200), S(12)};
    DrawRect(volTrack, {50, 50, 60, 255});
    int volPos = m_settings.audioVolume * S(200) / 128;
    SDL_Rect volFill = {volTrack.x, volTrack.y, volPos, S(12)};
    DrawRect(volFill, col_accent);
    SDL_Rect volHandle = {volTrack.x + volPos - S(6), volTrack.y - S(4), S(12), S(20)};
    DrawRect(volHandle, col_text);
    
    bool volHover = (m_mouseX >= volTrack.x && m_mouseX < volTrack.x + volTrack.w + S(20) &&
                    m_mouseY >= volTrack.y - S(5) && m_mouseY < volTrack.y + volTrack.h + S(5));
    if (volHover && m_mousePressed) {
        m_editingItem = 2; // editing volume slider
    }
    if (m_editingItem == 2 && m_mousePressed) {
        int rel = m_mouseX - volTrack.x;
        if (rel < 0) rel = 0;
        if (rel > S(200)) rel = S(200);
        m_settings.audioVolume = rel * 128 / S(200);
        SaveSettings();
    }
    y += lineH;
    y += S(10);
    
    // --- System info ---
    RenderText("--- System ---", x, y, col_accent, 16);
    y += lineH;
    
    RenderText("Engine Version:", x, y + S(5), col_text, 16);
    RenderText(KRKR_VERSION_STRING, x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    RenderText("Platform:", x, y + S(5), col_text, 16);
    RenderText(std::string(Platform::GetPlatformName()) + " (" + Platform::GetArchName() + ")", x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    RenderText("CPU:", x, y + S(5), col_text, 16);
    RenderText(Platform::GetCPUModel() + " (" + std::to_string(Platform::GetCPUCount()) + " cores)", x + labelW, y + S(5), col_textDim, 16);
    y += lineH;
    
    RenderText("Data Path:", x, y + S(5), col_text, 16);
    RenderText(Platform::GetDataPath(), x + labelW, y + S(5), col_textDim, 14);
    y += lineH;
    
    RenderText("Save Path:", x, y + S(5), col_text, 16);
    RenderText(Platform::GetSavePath(), x + labelW, y + S(5), col_textDim, 14);
    y += lineH;
    
    // Apply / Restart button
    y += S(10);
    SDL_Rect restartBtn = {x, y, S(180), S(36)};
    if (Button("Apply & Restart Launcher", restartBtn, col_button, col_buttonHover)) {
        SaveSettings();
        // Restart: reload font, detect DPI
        DetectDPI();
        LoadFont();
    }
}

void Launcher::RenderAbout() {
    int x = S(20);
    int y = S(80);
    
    RenderText("About krkrv8a", x, y, col_text, 24);
    y += S(55);
    RenderText("krkrv8a - Kirikiri2 Engine for ARMv8a Linux", x, y, col_text, 18);
    y += S(35);
    RenderText("Version " KRKR_VERSION_STRING, x, y, col_textDim, 16);
    y += S(30);
    RenderText("A lightweight visual novel engine for ARM64 Linux.", x, y, col_textDim, 16);
    y += S(30);
    RenderText("https://github.com/Zaxver/krkrv8a", x, y, col_accent, 16);
    y += S(45);
    RenderText("References:", x, y, col_text, 16);
    y += S(25);
    RenderText("- krkrsdl2 (SDL2 Kirikiri2 backend)", x, y, col_textDim, 14);
    y += S(25);
    RenderText("- krkr2-linux (Cocos2d-x Kirikiri2)", x, y, col_textDim, 14);
    y += S(25);
    RenderText("- Kirikiroid2 (Android Kirikiri2)", x, y, col_textDim, 14);
    y += S(25);
    RenderText("- tyranor (Kirikiri2 fork)", x, y, col_textDim, 14);
}

void Launcher::RenderLibrary() {
    RenderGameGrid();
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
    if (m_fileBrowser.open) RenderFileBrowser();
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
                // Stop slider editing
                m_editingItem = -1;
            }
            break;
        case SDL_MOUSEWHEEL:
            if (m_fileBrowser.open) {
                m_fileBrowser.scrollOffset -= event.wheel.y * S(30);
                if (m_fileBrowser.scrollOffset < 0) m_fileBrowser.scrollOffset = 0;
            } else if (m_mode == LauncherMode::Settings) {
                m_settingsScroll -= event.wheel.y * S(30);
                if (m_settingsScroll < 0) m_settingsScroll = 0;
            } else {
                m_scrollOffset -= event.wheel.y * S(30);
                if (m_scrollOffset < 0) m_scrollOffset = 0;
            }
            break;
        case SDL_KEYDOWN:
            if (m_fileBrowser.open) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_fileBrowser.open = false;
                } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    fs::path p(m_fileBrowser.currentDir);
                    if (p.has_parent_path()) {
                        m_fileBrowser.currentDir = p.parent_path().string();
                        RefreshFileBrowser();
                    }
                }
            } else if (m_showAddDialog && m_inputActive) {
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
            if (m_showAddDialog && m_inputActive && !m_fileBrowser.open) {
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