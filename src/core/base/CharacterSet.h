#pragma once
#ifndef TVP_CHARACTER_SET_H
#define TVP_CHARACTER_SET_H

#include "tvp_types.h"
#include <string>
#include <vector>

// Character encoding conversion utilities
// Based on the Kirikiri2 CharacterSet module

class CharacterSet {
public:
    // UTF-8 <-> UTF-16 (wide) conversions
    static std::wstring UTF8ToWide(const std::string& utf8);
    static std::string  WideToUTF8(const std::wstring& wide);
    
    // Shift-JIS <-> UTF-16
    static std::wstring SJISToWide(const std::string& sjis);
    static std::string  WideToSJIS(const std::wstring& wide);
    
    // UTF-8 <-> Shift-JIS
    static std::string  SJISToUTF8(const std::string& sjis);
    static std::string  UTF8ToSJIS(const std::string& utf8);
    
    // ASCII / Latin1
    static std::wstring Latin1ToWide(const std::string& str);
    
    // Case conversion (wide)
    static std::wstring ToUpper(const std::wstring& s);
    static std::wstring ToLower(const std::wstring& s);
    
    // Path normalization
    static std::wstring NormalizePath(const std::wstring& path);
    static bool IsAbsolutePath(const std::wstring& path);
    static std::wstring GetDirectoryName(const std::wstring& path);
    static std::wstring GetFileName(const std::wstring& path);
    static std::wstring GetExtension(const std::wstring& path);
    static std::wstring ChangeExtension(const std::wstring& path, const std::wstring& ext);
};

#endif // TVP_CHARACTER_SET_H