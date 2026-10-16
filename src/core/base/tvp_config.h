#pragma once
#ifndef TVP_CONFIG_H
#define TVP_CONFIG_H

// Platform detection
#if defined(__linux__)
    #define TVP_LINUX 1
    #define TVP_PLATFORM_NAME "Linux"
#elif defined(__ANDROID__)
    #define TVP_ANDROID 1
    #define TVP_PLATFORM_NAME "Android"
#elif defined(_WIN32)
    #define TVP_WIN32 1
    #define TVP_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define TVP_APPLE 1
    #define TVP_PLATFORM_NAME "Apple"
#endif

// Architecture detection
#if defined(__aarch64__)
    #define TVP_ARCH_ARM64 1
    #define TVP_ARCH_NAME "arm64-v8a"
#elif defined(__arm__)
    #define TVP_ARCH_ARM32 1
    #define TVP_ARCH_NAME "armeabi-v7a"
#elif defined(__x86_64__)
    #define TVP_ARCH_X86_64 1
    #define TVP_ARCH_NAME "x86_64"
#elif defined(__i386__)
    #define TVP_ARCH_X86 1
    #define TVP_ARCH_NAME "x86"
#endif

// Build configuration
#define TVP_USE_SDL2 1
#define TVP_USE_SDL2_MIXER 1
#define TVP_USE_SDL2_IMAGE 1
#define TVP_USE_SDL2_TTF 1

// Feature flags
#define TVP_SUPPORT_XP3_ARCHIVE 1
#define TVP_SUPPORT_ZIP_ARCHIVE 1
#define TVP_SUPPORT_TAR_ARCHIVE 1
#define TVP_SUPPORT_OGG_VORBIS 1
#define TVP_SUPPORT_FREE_TYPE 1
#define TVP_SUPPORT_JPEG 1
#define TVP_SUPPORT_PNG 1
#define TVP_SUPPORT_OPENGL 1

// Compiler feature detection
#if defined(__GNUC__)
    #define TVP_GCC 1
    #define TVP_DEPRECATED __attribute__((deprecated))
    #define TVP_NORETURN __attribute__((noreturn))
    #define TVP_FORCEINLINE inline __attribute__((always_inline))
    #define TVP_FORMAT_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#elif defined(_MSC_VER)
    #define TVP_MSVC 1
    #define TVP_DEPRECATED __declspec(deprecated)
    #define TVP_NORETURN __declspec(noreturn)
    #define TVP_FORCEINLINE __forceinline
    #define TVP_FORMAT_PRINTF(fmt, args)
#else
    #define TVP_DEPRECATED
    #define TVP_NORETURN
    #define TVP_FORCEINLINE inline
    #define TVP_FORMAT_PRINTF(fmt, args)
#endif

// Endianness detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define TVP_BIG_ENDIAN 1
#else
    #define TVP_LITTLE_ENDIAN 1
#endif

// Maximum path length
#define TVP_MAX_PATH 4096

// Engine version
#define TVP_ENGINE_NAME "krkrv8a"
#define TVP_ENGINE_VERSION "1.0.0"

#endif // TVP_CONFIG_H