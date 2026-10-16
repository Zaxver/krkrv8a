#pragma once
#ifndef TVP_PLATFORM_H
#define TVP_PLATFORM_H

#include "tvp_config.h"
#include <string>

// Platform abstraction layer
class Platform {
public:
    static const char* GetPlatformName() {
        return TVP_PLATFORM_NAME;
    }
    
    static const char* GetArchName() {
        return TVP_ARCH_NAME;
    }
    
    static bool IsARM64() {
#ifdef TVP_ARCH_ARM64
        return true;
#else
        return false;
#endif
    }
    
    static bool IsLinux() {
#ifdef TVP_LINUX
        return true;
#else
        return false;
#endif
    }
    
    static std::string GetDataPath();
    static std::string GetConfigPath();
    static std::string GetSavePath();
    static std::string GetTempPath();
    
    // CPU info
    static int GetCPUCount();
    static std::string GetCPUModel();
    static bool HasNEON();
};

#endif // TVP_PLATFORM_H