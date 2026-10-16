#pragma once
#ifndef TVP_SYSTEM_CONTROL_H
#define TVP_SYSTEM_CONTROL_H

#include "tvp_types.h"
#include <string>

class Application;

// SystemControl: singleton managing global engine state
class SystemControl {
public:
    static SystemControl& GetInstance();
    
    void Initialize(Application* app);
    void Shutdown();
    
    Application* GetApp() { return m_app; }
    
    // System info
    std::string GetEngineVersion() const;
    std::string GetPlatformName() const;
    int GetSystemFreeMemory() const;
    int GetSelfUsedMemory() const;
    
    // Timer
    uint32_t GetTickCount() const;
    void Sleep(uint32_t ms) const;
    
    // Clipboard
    std::wstring GetClipboardText() const;
    void SetClipboardText(const std::wstring& text) const;
    
private:
    SystemControl() = default;
    Application* m_app = nullptr;
};

#endif // TVP_SYSTEM_CONTROL_H