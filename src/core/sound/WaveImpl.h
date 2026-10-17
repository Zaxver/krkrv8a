#pragma once
#ifndef TVP_WAVE_IMPL_H
#define TVP_WAVE_IMPL_H

#include "tvp_types.h"
#include <SDL2/SDL_mixer.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

// Audio playback implementation using SDL2_mixer
class WaveImpl {
public:
    WaveImpl();
    ~WaveImpl();
    
    bool Initialize();
    void Shutdown();
    
    // Load sound from data (WAV, OGG, MP3)
    int LoadSound(const uint8_t* data, size_t size);
    int LoadSound(const std::string& path);
    
    // Playback control
    bool Play(int channel, int soundId, int loops = 0);
    void Stop(int channel);
    void StopAll();
    
    // Volume (0-128)
    void SetVolume(int channel, int volume);
    int GetVolume(int channel) const;
    
    void SetMasterVolume(int volume) { m_masterVolume = volume; Mix_Volume(-1, volume); }
    int GetMasterVolume() const { return m_masterVolume; }
    
    // Check if playing
    bool IsPlaying(int channel) const;
    
    // Free a loaded sound
    void FreeSound(int soundId);
    
private:
    int m_masterVolume = 64;
    int m_nextId = 1;
    std::map<int, Mix_Chunk*> m_sounds;
    std::map<int, Mix_Music*> m_music;
};

#endif // TVP_WAVE_IMPL_H