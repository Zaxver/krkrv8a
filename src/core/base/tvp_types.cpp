#include "tvp_types.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

static const char* level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static TVPLogLevel min_log_level = TVPLogLevel::Info;

void TVPSetLogLevel(TVPLogLevel level) {
    min_log_level = level;
}

void TVPLog(TVPLogLevel level, const char* fmt, ...) {
    if (level < min_log_level) return;
    
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    
    fprintf(stderr, "[%s] [%s] ", time_buf, level_names[static_cast<int>(level)]);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    fflush(stderr);
}