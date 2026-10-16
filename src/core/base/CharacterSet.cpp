#include "CharacterSet.h"
#include <iconv.h>
#include <algorithm>
#include <cstring>
#include <codecvt>
#include <locale>
#include <filesystem>

// UTF-8 to Wide (UTF-16/UTF-32 depending on platform)
std::wstring CharacterSet::UTF8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.from_bytes(utf8);
    } catch (...) {
        // Fallback: manual conversion
        std::wstring result;
        size_t i = 0;
        while (i < utf8.size()) {
            unsigned char c = (unsigned char)utf8[i];
            if (c < 0x80) {
                result += (wchar_t)c;
                i++;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) {
                result += (wchar_t)(((c & 0x1F) << 6) | ((unsigned char)utf8[i+1] & 0x3F));
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8.size()) {
                result += (wchar_t)(((c & 0x0F) << 12) | (((unsigned char)utf8[i+1] & 0x3F) << 6) | ((unsigned char)utf8[i+2] & 0x3F));
                i += 3;
            } else {
                result += L'?';
                i++;
            }
        }
        return result;
    }
}

std::string CharacterSet::WideToUTF8(const std::wstring& wide) {
    if (wide.empty()) return "";
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(wide);
    } catch (...) {
        std::string result;
        for (wchar_t wc : wide) {
            if (wc < 0x80) {
                result += (char)wc;
            } else if (wc < 0x800) {
                result += (char)(0xC0 | (wc >> 6));
                result += (char)(0x80 | (wc & 0x3F));
            } else {
                result += (char)(0xE0 | (wc >> 12));
                result += (char)(0x80 | ((wc >> 6) & 0x3F));
                result += (char)(0x80 | (wc & 0x3F));
            }
        }
        return result;
    }
}

// Shift-JIS to Wide using iconv
std::wstring CharacterSet::SJISToWide(const std::string& sjis) {
    if (sjis.empty()) return L"";
    
    iconv_t cd = iconv_open("WCHAR_T", "SHIFT_JIS");
    if (cd == (iconv_t)-1) {
        // Try CP932 as fallback
        cd = iconv_open("WCHAR_T", "CP932");
        if (cd == (iconv_t)-1) {
            TVP_LOG_ERROR("iconv_open failed for SHIFT_JIS");
            return L"";
        }
    }
    
    size_t inbytes = sjis.size();
    char* inbuf = (char*)sjis.data();
    size_t outbytes = inbytes * sizeof(wchar_t) * 2;
    std::vector<char> outbuf(outbytes);
    char* out = outbuf.data();
    
    size_t ret = iconv(cd, &inbuf, &inbytes, &out, &outbytes);
    iconv_close(cd);
    
    if (ret == (size_t)-1) {
        TVP_LOG_WARN("SJISToWide: iconv conversion error");
        // Return partial result
    }
    
    size_t wlen = (outbuf.size() - outbytes) / sizeof(wchar_t);
    return std::wstring((wchar_t*)outbuf.data(), wlen);
}

std::string CharacterSet::WideToSJIS(const std::wstring& wide) {
    if (wide.empty()) return "";
    
    iconv_t cd = iconv_open("SHIFT_JIS", "WCHAR_T");
    if (cd == (iconv_t)-1) cd = iconv_open("CP932", "WCHAR_T");
    if (cd == (iconv_t)-1) return "";
    
    size_t inbytes = wide.size() * sizeof(wchar_t);
    char* inbuf = (char*)wide.data();
    size_t outbytes = inbytes;
    std::vector<char> outbuf(outbytes);
    char* out = outbuf.data();
    
    iconv(cd, &inbuf, &inbytes, &out, &outbytes);
    iconv_close(cd);
    
    return std::string(outbuf.data(), outbuf.size() - outbytes);
}

std::string CharacterSet::SJISToUTF8(const std::string& sjis) {
    return WideToUTF8(SJISToWide(sjis));
}

std::string CharacterSet::UTF8ToSJIS(const std::string& utf8) {
    return WideToSJIS(UTF8ToWide(utf8));
}

std::wstring CharacterSet::Latin1ToWide(const std::string& str) {
    std::wstring result;
    result.reserve(str.size());
    for (char c : str) result += (wchar_t)(unsigned char)c;
    return result;
}

std::wstring CharacterSet::ToUpper(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), towupper);
    return r;
}

std::wstring CharacterSet::ToLower(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), towlower);
    return r;
}

std::wstring CharacterSet::NormalizePath(const std::wstring& path) {
    std::wstring p = path;
    // Replace backslashes with forward slashes
    for (auto& c : p) if (c == L'\\') c = L'/';
    // Remove duplicate slashes
    std::wstring result;
    bool lastSlash = false;
    for (auto c : p) {
        if (c == L'/') {
            if (!lastSlash) result += c;
            lastSlash = true;
        } else {
            result += c;
            lastSlash = false;
        }
    }
    // Remove trailing slash (except for root)
    if (result.size() > 1 && result.back() == L'/') result.pop_back();
    return result;
}

bool CharacterSet::IsAbsolutePath(const std::wstring& path) {
    if (path.empty()) return false;
    if (path[0] == L'/') return true;
#ifdef TVP_WIN32
    if (path.size() >= 3 && iswalpha(path[0]) && path[1] == L':' && path[2] == L'\\') return true;
#endif
    return false;
}

std::wstring CharacterSet::GetDirectoryName(const std::wstring& path) {
    auto p = NormalizePath(path);
    size_t pos = p.rfind(L'/');
    if (pos == std::wstring::npos) return L".";
    return p.substr(0, pos);
}

std::wstring CharacterSet::GetFileName(const std::wstring& path) {
    auto p = NormalizePath(path);
    size_t pos = p.rfind(L'/');
    if (pos == std::wstring::npos) return p;
    return p.substr(pos + 1);
}

std::wstring CharacterSet::GetExtension(const std::wstring& path) {
    auto name = GetFileName(path);
    size_t pos = name.rfind(L'.');
    if (pos == std::wstring::npos || pos == 0) return L"";
    return ToLower(name.substr(pos));
}

std::wstring CharacterSet::ChangeExtension(const std::wstring& path, const std::wstring& ext) {
    auto p = NormalizePath(path);
    size_t pos = p.rfind(L'.');
    size_t slash = p.rfind(L'/');
    if (pos != std::wstring::npos && (slash == std::wstring::npos || pos > slash)) {
        p = p.substr(0, pos);
    }
    if (!ext.empty()) {
        if (ext[0] != L'.') p += L'.';
        p += ext;
    }
    return p;
}