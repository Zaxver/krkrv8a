#include "WaveImpl.h"
#include <SDL2/SDL.h>

WaveImpl::WaveImpl() {}

WaveImpl::~WaveImpl() { Shutdown(); }

bool WaveImpl::Initialize() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        TVP_LOG_WARN("WaveImpl: Mix_OpenAudio failed: %s", Mix_GetError());
        return false;
    }
    
    // Allocate mixing channels
    Mix_AllocateChannels(16);
    
    TVP_LOG_INFO("WaveImpl: Initialized (SDL2_mixer)");
    return true;
}

void WaveImpl::Shutdown() {
    for (auto& pair : m_sounds) {
        if (pair.second) Mix_FreeChunk(pair.second);
    }
    m_sounds.clear();
    
    for (auto& pair : m_music) {
        if (pair.second) Mix_FreeMusic(pair.second);
    }
    m_music.clear();
    
    Mix_CloseAudio();
}

int WaveImpl::LoadSound(const uint8_t* data, size_t size) {
    if (!data || size == 0) return -1;
    
    // Create RWops from memory
    SDL_RWops* rw = SDL_RWFromMem((void*)data, (int)size);
    if (!rw) return -1;
    
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);
    if (!chunk) {
        TVP_LOG_ERROR("WaveImpl: Mix_LoadWAV_RW failed: %s", Mix_GetError());
        return -1;
    }
    
    int id = m_nextId++;
    m_sounds[id] = chunk;
    return id;
}

int WaveImpl::LoadSound(const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        TVP_LOG_ERROR("WaveImpl: Mix_LoadWAV failed for %s: %s", path.c_str(), Mix_GetError());
        return -1;
    }
    
    int id = m_nextId++;
    m_sounds[id] = chunk;
    return id;
}

bool WaveImpl::Play(int channel, int soundId, int loops) {
    auto it = m_sounds.find(soundId);
    if (it == m_sounds.end() || !it->second) return false;
    
    int ch = Mix_PlayChannel(channel, it->second, loops);
    if (ch < 0) {
        TVP_LOG_WARN("WaveImpl: Mix_PlayChannel failed: %s", Mix_GetError());
        return false;
    }
    return true;
}

void WaveImpl::Stop(int channel) {
    Mix_HaltChannel(channel);
}

void WaveImpl::StopAll() {
    Mix_HaltChannel(-1);
}

void WaveImpl::SetVolume(int channel, int volume) {
    Mix_Volume(channel, volume);
}

int WaveImpl::GetVolume(int channel) const {
    return Mix_Volume(channel, -1);
}

bool WaveImpl::IsPlaying(int channel) const {
    return Mix_Playing(channel) != 0;
}

void WaveImpl::FreeSound(int soundId) {
    auto it = m_sounds.find(soundId);
    if (it != m_sounds.end()) {
        if (it->second) Mix_FreeChunk(it->second);
        m_sounds.erase(it);
    }
}