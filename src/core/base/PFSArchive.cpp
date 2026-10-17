#include "PFSArchive.h"
#include "CharacterSet.h"
#include <zlib.h>
#include <cstring>
#include <fstream>
#include <algorithm>

PFSArchive::PFSArchive() {}
PFSArchive::~PFSArchive() { Close(); }

std::string PFSArchive::NormalizeName(const std::string& name) const {
    std::string n = name;
    // Convert backslashes to forward slashes
    for (auto& c : n) if (c == '\\') c = '/';
    // Lowercase for case-insensitive matching
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    return n;
}

bool PFSArchive::Open(const std::string& filepath) {
    Close();
    m_filepath = filepath;
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        TVP_LOG_ERROR("PFSArchive: Cannot open: %s", filepath.c_str());
        return false;
    }
    
    // Read magic: "pf6" (3 bytes)
    char magic[3];
    file.read(magic, 3);
    if (memcmp(magic, "pf6", 3) != 0) {
        // Check for "pf6" with different version byte
        if (memcmp(magic, "pf", 2) != 0) {
            TVP_LOG_ERROR("PFSArchive: Invalid magic: %s", filepath.c_str());
            return false;
        }
        // pf + version byte, re-read as 2 byte magic
    }
    
    // After magic(3): two uint32 fields
    // field1 = index_size (total size of the index/header area)
    // field2 = entry_count (number of file entries)
    uint32_t index_size = 0, entry_count = 0;
    file.read((char*)&index_size, 4);
    file.read((char*)&entry_count, 4);
    
    TVP_LOG_INFO("PFSArchive: index_size=%u entry_count=%u", index_size, entry_count);
    
    if (entry_count > 100000) {
        TVP_LOG_ERROR("PFSArchive: entry count too large: %u", entry_count);
        return false;
    }
    
    // Read all entries from the index
    // Each entry: name_len(4) + name(name_len bytes, null-terminated, padded to 4) 
    //             + offset(4) + size(4)
    // The padding aligns the total entry size to 4 bytes from the start of name_len
    
    for (uint32_t i = 0; i < entry_count; i++) {
        uint32_t name_len = 0;
        file.read((char*)&name_len, 4);
        if (!file || name_len == 0 || name_len > 4096) {
            TVP_LOG_ERROR("PFSArchive: bad name_len=%u at entry %u (pos=%zu)", 
                          name_len, i, (size_t)file.tellg());
            break;
        }
        
        // Read name (name_len bytes)
        long name_field_start = (long)file.tellg();
        std::vector<char> name_buf(name_len);
        file.read(name_buf.data(), name_len);
        if (!file) { TVP_LOG_ERROR("PFSArchive: read name failed at entry %u", i); break; }
        std::string name(name_buf.begin(), name_buf.end());
        while (!name.empty() && name.back() == '\0') name.pop_back();
        
        // PFS padding: 4 bytes after name (null terminator + 3 bytes padding)
        // This seems to work for most entries
        uint32_t pad_bytes = 4;
        file.seekg(pad_bytes, std::ios::cur);
        
        // Read offset(4) and size(4)
        uint32_t offset = 0, size = 0;
        file.read((char*)&offset, 4);
        file.read((char*)&size, 4);
        if (!file) { TVP_LOG_ERROR("PFSArchive: read fields failed at entry %u", i); break; }
        
        if (i < 3) {
            TVP_LOG_INFO("PFSArchive: entry[%u] name='%s' name_len=%u offset=%u size=%u",
                         i, name.c_str(), name_len, offset, size);
        }
        
        PFSEntry entry;
        entry.name = name;
        entry.offset = offset;
        entry.size = size;
        
        std::string normName = NormalizeName(name);
        m_nameMap[normName] = m_entries.size();
        m_entries.push_back(entry);
    }
    
    m_isOpen = true;
    TVP_LOG_INFO("PFSArchive: Opened %s (%zu files)", filepath.c_str(), m_entries.size());
    return true;
}

void PFSArchive::Close() {
    m_entries.clear();
    m_nameMap.clear();
    m_filepath.clear();
    m_isOpen = false;
}

bool PFSArchive::HasFile(const std::string& name) const {
    return m_nameMap.find(NormalizeName(name)) != m_nameMap.end();
}

std::vector<std::string> PFSArchive::GetFileList() const {
    std::vector<std::string> list;
    list.reserve(m_entries.size());
    for (const auto& e : m_entries) list.push_back(e.name);
    return list;
}

std::vector<uint8_t> PFSArchive::ExtractFile(const std::string& name) const {
    std::vector<uint8_t> result;
    auto it = m_nameMap.find(NormalizeName(name));
    if (it == m_nameMap.end()) {
        TVP_LOG_WARN("PFSArchive: File not found: %s", name.c_str());
        return result;
    }
    
    const PFSEntry& entry = m_entries[it->second];
    result.resize(entry.size);
    
    std::ifstream file(m_filepath, std::ios::binary);
    if (!file.is_open()) return result;
    
    file.seekg(entry.offset);
    file.read((char*)result.data(), entry.size);
    
    if (!file) {
        TVP_LOG_WARN("PFSArchive: Read error for %s", name.c_str());
        result.clear();
    }
    
    return result;
}

uint64_t PFSArchive::GetFileSize(const std::string& name) const {
    auto it = m_nameMap.find(NormalizeName(name));
    if (it == m_nameMap.end()) return 0;
    return m_entries[it->second].size;
}