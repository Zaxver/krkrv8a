#pragma once
#ifndef TVP_EVENT_IMPL_H
#define TVP_EVENT_IMPL_H

#include "tvp_types.h"
#include <SDL2/SDL.h>
#include <functional>
#include <queue>
#include <mutex>

// Event types
enum class TVPEventType {
    Unknown,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseDoubleClick,
    KeyDown,
    KeyUp,
    KeyChar,
    TouchDown,
    TouchUp,
    TouchMove,
    Resize,
    Quit,
    User
};

struct TVPEvent {
    TVPEventType type = TVPEventType::Unknown;
    int x = 0, y = 0;
    int button = 0;
    int keycode = 0;
    int modifiers = 0;
    wchar_t ch = 0;
    int width = 0, height = 0;
    uint32_t timestamp = 0;
};

// Event system: handles input events
class EventImpl {
public:
    EventImpl();
    ~EventImpl();
    
    bool Initialize();
    void Shutdown();
    
    // Process SDL events and convert to TVP events
    void ProcessEvents();
    
    // Poll a TVP event
    bool PollEvent(TVPEvent& ev);
    
    // Wait for an event (with timeout in ms, -1 = infinite)
    bool WaitEvent(TVPEvent& ev, int timeoutMs = -1);
    
    // Push a user event
    void PushEvent(const TVPEvent& ev);
    
    // Event callbacks
    std::function<void(const TVPEvent&)> onEvent;
    std::function<void()> onQuit;
    
private:
    void TranslateSDLEvent(const SDL_Event& sdlEvent, TVPEvent& tvpEvent);
    
    std::queue<TVPEvent> m_eventQueue;
    std::mutex m_eventMutex;
    bool m_initialized = false;
};

#endif // TVP_EVENT_IMPL_H