#pragma once
#ifndef TVP_PFS_ARCHIVE_H
#define TVP_PFS_ARCHIVE_H

#include "tvp_types.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// PFS archive format reader (tyranor/KiriKiriZ variant)
// Format: "pf6" magic + header + file entries with name/offset/size

struct PFSEntry {
    std::string name;       // UTF-8 file path (backslashes)
    uint64_t offset;        // offset in file
    uint64_t size;          // uncompressed size
    bool compressed = false;
};

class PFSArchive {
public:
    PFSArchive();
    ~PFSArchive();
    
    bool Open(const std::string& filepath);
    void Close();
    
    bool HasFile(const std::string& name) const;
    std::vector<std::string> GetFileList() const;
    std::vector<uint8_t> ExtractFile(const std::string& name) const;
    uint64_t GetFileSize(const std::string& name) const;
    
    bool IsOpen() const { return m_isOpen; }
    int GetEntryCount() const { return (int)m_entries.size(); }
    
private:
    std::string m_filepath;
    std::vector<PFSEntry> m_entries;
    std::map<std::string, size_t> m_nameMap;
    bool m_isOpen = false;
    
    std::string NormalizeName(const std::string& name) const;
};

#endif // TVP_PFS_ARCHIVE_H