#pragma once
#ifndef TVP_XP3_ARCHIVE_H
#define TVP_XP3_ARCHIVE_H

#include "tvp_types.h"
#include "BinaryStream.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// XP3 archive format reader
// XP3 is Kirikiri's custom archive format for packaging game resources

#pragma pack(push, 1)

struct XP3Header {
    char magic[3];      // "XP3"
    char version;       // version byte
    char reserved[4];   // reserved
    tjs_uint64 header_size; // size of header data
};

struct XP3IndexEntry {
    tjs_uint64 flags;       // flags (compressed, etc.)
    tjs_uint64 offset;      // offset in file
    tjs_uint64 comp_size;   // compressed size
    tjs_uint64 uncomp_size; // uncompressed size
    std::wstring name;      // file name (UTF-16)
};

#pragma pack(pop)

class XP3Archive {
public:
    XP3Archive();
    ~XP3Archive();
    
    // Open an XP3 archive file
    bool Open(const std::string& filepath);
    void Close();
    
    // Check if a file exists in the archive
    bool HasFile(const std::wstring& name) const;
    
    // Get list of all files
    std::vector<std::wstring> GetFileList() const;
    
    // Extract a file from the archive
    // Returns the file data as a byte vector
    std::vector<uint8_t> ExtractFile(const std::wstring& name) const;
    
    // Extract a file to disk
    bool ExtractFileToDisk(const std::wstring& name, const std::string& destpath) const;
    
    // Get uncompressed size of a file
    tjs_uint64 GetFileSize(const std::wstring& name) const;
    
    bool IsOpen() const { return m_isOpen; }
    
private:
    bool ReadIndex();
    bool DecompressZLIB(const uint8_t* src, size_t srcSize, uint8_t* dst, size_t dstSize) const;
    
    std::string m_filepath;
    std::vector<XP3IndexEntry> m_entries;
    std::map<std::wstring, size_t> m_nameMap;
    bool m_isOpen = false;
    
    // XP3 flags
    static constexpr tjs_uint64 XP3_FLAG_COMPRESSED = 0x01;
    static constexpr tjs_uint64 XP3_FLAG_ENCRYPTED  = 0x02;
    static constexpr tjs_uint64 XP3_FLAG_ARCHIVED   = 0x04;
};

#endif // TVP_XP3_ARCHIVE_H