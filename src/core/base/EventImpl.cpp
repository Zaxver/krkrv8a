#include "EventImpl.h"
#include <cstring>

EventImpl::EventImpl() {}
EventImpl::~EventImpl() { Shutdown(); }

bool EventImpl::Initialize() {
    m_initialized = true;
    TVP_LOG_INFO("EventImpl: Initialized");
    return true;
}

void EventImpl::Shutdown() {
    m_initialized = false;
    std::queue<TVPEvent> empty;
    std::swap(m_eventQueue, empty);
}

void EventImpl::ProcessEvents() {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        TVPEvent tvpEvent;
        TranslateSDLEvent(sdlEvent, tvpEvent);
        if (tvpEvent.type != TVPEventType::Unknown) {
            if (tvpEvent.type == TVPEventType::Quit && onQuit) {
                onQuit();
            }
            if (onEvent) onEvent(tvpEvent);
            PushEvent(tvpEvent);
        }
    }
}

bool EventImpl::PollEvent(TVPEvent& ev) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_eventQueue.empty()) return false;
    ev = m_eventQueue.front();
    m_eventQueue.pop();
    return true;
}

bool EventImpl::WaitEvent(TVPEvent& ev, int timeoutMs) {
    // Wait for SDL event
    if (!SDL_WaitEventTimeout(nullptr, timeoutMs)) return false;
    ProcessEvents();
    return PollEvent(ev);
}

void EventImpl::PushEvent(const TVPEvent& ev) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventQueue.push(ev);
}

void EventImpl::TranslateSDLEvent(const SDL_Event& sdlEvent, TVPEvent& tvpEvent) {
    tvpEvent.timestamp = sdlEvent.common.timestamp;
    
    switch (sdlEvent.type) {
        case SDL_QUIT:
            tvpEvent.type = TVPEventType::Quit;
            break;
        case SDL_MOUSEBUTTONDOWN:
            tvpEvent.type = TVPEventType::MouseDown;
            tvpEvent.x = sdlEvent.button.x;
            tvpEvent.y = sdlEvent.button.y;
            tvpEvent.button = sdlEvent.button.button;
            tvpEvent.timestamp = sdlEvent.button.timestamp;
            if (sdlEvent.button.clicks >= 2) {
                tvpEvent.type = TVPEventType::MouseDoubleClick;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            tvpEvent.type = TVPEventType::MouseUp;
            tvpEvent.x = sdlEvent.button.x;
            tvpEvent.y = sdlEvent.button.y;
            tvpEvent.button = sdlEvent.button.button;
            break;
        case SDL_MOUSEMOTION:
            tvpEvent.type = TVPEventType::MouseMove;
            tvpEvent.x = sdlEvent.motion.x;
            tvpEvent.y = sdlEvent.motion.y;
            break;
        case SDL_KEYDOWN:
            tvpEvent.type = TVPEventType::KeyDown;
            tvpEvent.keycode = sdlEvent.key.keysym.sym;
            tvpEvent.modifiers = sdlEvent.key.keysym.mod;
            break;
        case SDL_KEYUP:
            tvpEvent.type = TVPEventType::KeyUp;
            tvpEvent.keycode = sdlEvent.key.keysym.sym;
            tvpEvent.modifiers = sdlEvent.key.keysym.mod;
            break;
        case SDL_TEXTINPUT:
            tvpEvent.type = TVPEventType::KeyChar;
            // Convert UTF-8 input to wide char
            if (sdlEvent.text.text[0] != 0) {
                std::string utf8(sdlEvent.text.text);
                std::wstring wide;
                // Simple UTF-8 decode for first char
                unsigned char c = (unsigned char)utf8[0];
                if (c < 0x80) {
                    tvpEvent.ch = (wchar_t)c;
                } else if ((c & 0xE0) == 0xC0 && utf8.size() >= 2) {
                    tvpEvent.ch = ((wchar_t)(c & 0x1F) << 6) | ((unsigned char)utf8[1] & 0x3F);
                } else if ((c & 0xF0) == 0xE0 && utf8.size() >= 3) {
                    tvpEvent.ch = ((wchar_t)(c & 0x0F) << 12) | (((unsigned char)utf8[1] & 0x3F) << 6) | ((unsigned char)utf8[2] & 0x3F);
                }
            }
            break;
        case SDL_WINDOWEVENT:
            if (sdlEvent.window.event == SDL_WINDOWEVENT_RESIZED ||
                sdlEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                tvpEvent.type = TVPEventType::Resize;
                tvpEvent.width = sdlEvent.window.data1;
                tvpEvent.height = sdlEvent.window.data2;
            }
            break;
        case SDL_FINGERDOWN:
            tvpEvent.type = TVPEventType::TouchDown;
            tvpEvent.x = (int)(sdlEvent.tfinger.x * 65535);
            tvpEvent.y = (int)(sdlEvent.tfinger.y * 65535);
            break;
        case SDL_FINGERUP:
            tvpEvent.type = TVPEventType::TouchUp;
            tvpEvent.x = (int)(sdlEvent.tfinger.x * 65535);
            tvpEvent.y = (int)(sdlEvent.tfinger.y * 65535);
            break;
        case SDL_FINGERMOTION:
            tvpEvent.type = TVPEventType::TouchMove;
            tvpEvent.x = (int)(sdlEvent.tfinger.x * 65535);
            tvpEvent.y = (int)(sdlEvent.tfinger.y * 65535);
            break;
        default:
            tvpEvent.type = TVPEventType::Unknown;
            break;
    }
}