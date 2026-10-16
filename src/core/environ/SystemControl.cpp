#include "SystemControl.h"
#include "Application.h"
#include "Platform.h"
#include <SDL2/SDL.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <unistd.h>

SystemControl& SystemControl::GetInstance() {
    static SystemControl instance;
    return instance;
}

void SystemControl::Initialize(Application* app) {
    m_app = app;
    TVP_LOG_INFO("SystemControl: Initialized (platform: %s, arch: %s)",
                 Platform::GetPlatformName(), Platform::GetArchName());
}

void SystemControl::Shutdown() {
    m_app = nullptr;
}

std::string SystemControl::GetEngineVersion() const {
    return TVP_ENGINE_VERSION;
}

std::string SystemControl::GetPlatformName() const {
    return Platform::GetPlatformName();
}

int SystemControl::GetSystemFreeMemory() const {
    struct sysinfo info;
    if (sysinfo(&info) == -1) return -1;
    return (info.freeram * info.mem_unit) / (1024 * 1024);
}

int SystemControl::GetSelfUsedMemory() const {
    std::ifstream statm("/proc/self/statm");
    int pages = 0;
    statm >> pages;
    return (pages * sysconf(_SC_PAGESIZE)) / (1024 * 1024);
}

uint32_t SystemControl::GetTickCount() const {
    return SDL_GetTicks();
}

void SystemControl::Sleep(uint32_t ms) const {
    SDL_Delay(ms);
}

std::wstring SystemControl::GetClipboardText() const {
    if (!SDL_HasClipboardText()) return L"";
    char* text = SDL_GetClipboardText();
    if (!text) return L"";
    std::wstring result;
    // Simple UTF-8 to wide conversion
    // (Use CharacterSet for full conversion)
    result.reserve(strlen(text));
    for (size_t i = 0; text[i]; i++) {
        if ((unsigned char)text[i] < 0x80) {
            result += (wchar_t)text[i];
        }
    }
    SDL_free(text);
    return result;
}

void SystemControl::SetClipboardText(const std::wstring& text) const {
    // Simple wide to UTF-8 conversion
    std::string utf8;
    for (wchar_t wc : text) {
        if (wc < 0x80) {
            utf8 += (char)wc;
        } else if (wc < 0x800) {
            utf8 += (char)(0xC0 | (wc >> 6));
            utf8 += (char)(0x80 | (wc & 0x3F));
        } else {
            utf8 += (char)(0xE0 | (wc >> 12));
            utf8 += (char)(0x80 | ((wc >> 6) & 0x3F));
            utf8 += (char)(0x80 | (wc & 0x3F));
        }
    }
    SDL_SetClipboardText(utf8.c_str());
}