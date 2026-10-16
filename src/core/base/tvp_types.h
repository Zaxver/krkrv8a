#pragma once
#ifndef TVP_TYPES_H
#define TVP_TYPES_H

#include "tvp_config.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>

// TJS2 basic types
typedef int32_t     tjs_int;
typedef uint32_t    tjs_uint;
typedef int64_t     tjs_int64;
typedef uint64_t    tjs_uint64;
typedef tjs_int     tjs_error;
typedef wchar_t     tjs_char;  // TJS2 uses wide chars internally
typedef std::wstring tjs_string;
typedef std::string  tjs_string_n; // narrow string (UTF-8)

// TVP integer types (compatibility with original Kirikiri)
typedef tjs_int     TVPInteger;
typedef tjs_int64   TVPLargeInt;
typedef double      TVPReal;

// Pixel types
typedef uint32_t    TVPPixel32;  // ARGB 8888
typedef uint16_t    TVPPixel16;  // RGB 565

// Status codes
#define TVP_S_OK             ((tjs_error)0)
#define TVP_S_FALSE         ((tjs_error)1)
#define TVP_E_FAIL          ((tjs_error)-1)
#define TVP_E_INVALIDARG    ((tjs_error)-2)
#define TVP_E_OUTOFMEMORY   ((tjs_error)-3)
#define TVP_E_NOTIMPL       ((tjs_error)-4)
#define TVP_E_ABORT         ((tjs_error)-5)

// File seek origins
#define TVP_SEEK_SET    0
#define TVP_SEEK_CUR    1
#define TVP_SEEK_END    2

// Forward declarations
class tTJS;
class tTJSVariant;
class tTJSString;
class iTVPBaseFileSystem;
class tTVPLayer;
class tTVPBitmap;
class tTVPWindow;

// Smart pointer helpers
template <typename T>
class tTVPComPtr {
public:
    tTVPComPtr() : ptr(nullptr) {}
    tTVPComPtr(T* p) : ptr(p) { if (ptr) ptr->AddRef(); }
    tTVPComPtr(const tTVPComPtr& o) : ptr(o.ptr) { if (ptr) ptr->AddRef(); }
    ~tTVPComPtr() { if (ptr) ptr->Release(); }
    
    tTVPComPtr& operator=(T* p) {
        if (ptr) ptr->Release();
        ptr = p;
        if (ptr) ptr->AddRef();
        return *this;
    }
    tTVPComPtr& operator=(const tTVPComPtr& o) {
        if (this != &o) {
            if (ptr) ptr->Release();
            ptr = o.ptr;
            if (ptr) ptr->AddRef();
        }
        return *this;
    }
    
    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }
    T* get() const { return ptr; }
    T** put() { if (ptr) { ptr->Release(); } ptr = nullptr; return &ptr; }
    operator bool() const { return ptr != nullptr; }
    
private:
    T* ptr;
};

// Reference counting base
class tTVPBaseObject {
public:
    tTVPBaseObject() : ref_count(0) {}
    virtual ~tTVPBaseObject() = default;
    
    virtual int AddRef() { return ++ref_count; }
    virtual int Release() {
        int c = --ref_count;
        if (c == 0) delete this;
        return c;
    }
    
private:
    std::atomic<int> ref_count{0};
};

#include <atomic>

// Logging
enum class TVPLogLevel {
    Trace, Debug, Info, Warn, Error, Fatal
};

void TVPLog(TVPLogLevel level, const char* fmt, ...);
#define TVP_LOG_TRACE(fmt, ...) TVPLog(TVPLogLevel::Trace, fmt, ##__VA_ARGS__)
#define TVP_LOG_DEBUG(fmt, ...) TVPLog(TVPLogLevel::Debug, fmt, ##__VA_ARGS__)
#define TVP_LOG_INFO(fmt, ...)  TVPLog(TVPLogLevel::Info, fmt, ##__VA_ARGS__)
#define TVP_LOG_WARN(fmt, ...) TVPLog(TVPLogLevel::Warn, fmt, ##__VA_ARGS__)
#define TVP_LOG_ERROR(fmt, ...) TVPLog(TVPLogLevel::Error, fmt, ##__VA_ARGS__)
#define TVP_LOG_FATAL(fmt, ...) TVPLog(TVPLogLevel::Fatal, fmt, ##__VA_ARGS__)

#endif // TVP_TYPES_H