#include "Platform.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

std::string Platform::GetDataPath() {
    const char* home = getenv("HOME");
    if (!home) return ".";
    return std::string(home) + "/.local/share/krkrv8a";
}

std::string Platform::GetConfigPath() {
    const char* home = getenv("HOME");
    if (!home) return ".";
    return std::string(home) + "/.config/krkrv8a";
}

std::string Platform::GetSavePath() {
    return GetDataPath() + "/saves";
}

std::string Platform::GetTempPath() {
    return "/tmp/krkrv8a";
}

int Platform::GetCPUCount() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

std::string Platform::GetCPUModel() {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("model name") == 0 || line.find("Hardware") == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                return line.substr(colon + 2);
            }
        }
    }
    return "Unknown";
}

bool Platform::HasNEON() {
#ifdef TVP_ARCH_ARM64
    // ARM64 always has NEON
    return true;
#else
    // Check /proc/cpuinfo for NEON on ARM32
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("Features") != std::string::npos) {
            if (line.find("neon") != std::string::npos) return true;
        }
    }
    return false;
#endif
}