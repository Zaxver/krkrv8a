#include "tvp_types.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>

static const char* level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static TVPLogLevel min_log_level = TVPLogLevel::Info;

// In-memory log buffer for on-screen display
static std::vector<std::string> g_logBuffer;
static std::mutex g_logMutex;
static const size_t MAX_LOG_LINES = 200;

static std::string g_logFilePath;

void TVPSetLogLevel(TVPLogLevel level) {
    min_log_level = level;
}

void TVPSetLogFile(const std::string& path) {
    g_logFilePath = path;
}

std::vector<std::string> TVPGetLogBuffer() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return g_logBuffer;
}

void TVPLog(TVPLogLevel level, const char* fmt, ...) {
    if (level < min_log_level) return;
    
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    
    char full[1200];
    snprintf(full, sizeof(full), "[%s] [%s] %s", time_buf, level_names[static_cast<int>(level)], msg);
    
    // Output to stderr
    fprintf(stderr, "%s\n", full);
    fflush(stderr);
    
    // Write to log file
    if (!g_logFilePath.empty()) {
        std::ofstream f(g_logFilePath, std::ios::app);
        if (f.is_open()) {
            f << full << "\n";
        }
    }
    
    // Add to in-memory buffer for on-screen display
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logBuffer.push_back(full);
        if (g_logBuffer.size() > MAX_LOG_LINES) {
            g_logBuffer.erase(g_logBuffer.begin());
        }
    }
}

std::vector<std::string> TVPGetScreenLogLines() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return g_logBuffer;
}