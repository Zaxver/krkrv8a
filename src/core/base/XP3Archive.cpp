#include "XP3Archive.h"
#include "CharacterSet.h"
#include <zlib.h>
#include <cstring>
#include <fstream>
#include <cstdint>

XP3Archive::XP3Archive() {}

XP3Archive::~XP3Archive() { Close(); }

bool XP3Archive::Open(const std::string& filepath) {
    Close();
    m_filepath = filepath;
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        TVP_LOG_ERROR("XP3Archive: Cannot open file: %s", filepath.c_str());
        return false;
    }
    
    // Read header
    char magic[4];
    file.read(magic, 4);
    if (memcmp(magic, "XP3", 3) != 0) {
        TVP_LOG_ERROR("XP3Archive: Invalid magic: %s", filepath.c_str());
        return false;
    }
    
    // Read header size (little-endian 64-bit)
    tjs_uint64 header_size = 0;
    file.read((char*)&header_size, sizeof(header_size));
    
    // Read header data (index table)
    std::vector<uint8_t> header_data(header_size);
    file.read((char*)header_data.data(), header_size);
    
    // Parse index
    size_t pos = 0;
    while (pos < header_data.size()) {
        // Read chunk type
        if (pos + 4 > header_data.size()) break;
        char chunk_type[4];
        memcpy(chunk_type, &header_data[pos], 4);
        pos += 4;
        
        // Read chunk size
        if (pos + 8 > header_data.size()) break;
        tjs_uint64 chunk_size;
        memcpy(&chunk_size, &header_data[pos], 8);
        pos += 8;
        
        if (memcmp(chunk_type, "File", 4) == 0) {
            size_t chunk_end = pos + chunk_size;
            size_t file_start = pos;
            
            XP3IndexEntry entry = {};
            
            // Read file info sub-chunks
            while (pos < chunk_end) {
                if (pos + 4 > header_data.size()) break;
                char sub_type[4];
                memcpy(sub_type, &header_data[pos], 4);
                pos += 4;
                
                if (pos + 8 > header_data.size()) break;
                tjs_uint64 sub_size;
                memcpy(&sub_size, &header_data[pos], 8);
                pos += 8;
                
                if (memcmp(sub_type, "info", 4) == 0) {
                    // info block: flags, offset, comp_size, uncomp_size
                    if (pos + 32 > header_data.size()) break;
                    memcpy(&entry.flags, &header_data[pos], 8);
                    memcpy(&entry.offset, &header_data[pos+8], 8);
                    memcpy(&entry.comp_size, &header_data[pos+16], 8);
                    memcpy(&entry.uncomp_size, &header_data[pos+24], 8);
                    pos += sub_size;
                } else if (memcmp(sub_type, "name", 4) == 0) {
                    // name block: UTF-16LE string with length prefix
                    if (pos + 4 > header_data.size()) break;
                    uint32_t name_len;
                    memcpy(&name_len, &header_data[pos], 4);
                    pos += 4;
                    
                    if (pos + name_len * 2 > header_data.size()) break;
                    entry.name.resize(name_len);
                    for (uint32_t i = 0; i < name_len; i++) {
                        uint16_t ch;
                        memcpy(&ch, &header_data[pos + i * 2], 2);
                        entry.name[i] = (wchar_t)ch;
                    }
                    pos += name_len * 2;
                    
                    // Skip remaining sub_size data
                    size_t consumed = 4 + name_len * 2;
                    if (sub_size > consumed) pos += sub_size - consumed;
                } else {
                    // Skip unknown sub-chunk
                    pos += sub_size;
                }
            }
            
            // Normalize file name
            entry.name = CharacterSet::NormalizePath(entry.name);
            
            m_entries.push_back(entry);
            size_t idx = m_entries.size() - 1;
            m_nameMap[entry.name] = idx;
            
            pos = chunk_end;
        } else {
            // Skip unknown chunk
            pos += chunk_size;
        }
    }
    
    m_isOpen = true;
    TVP_LOG_INFO("XP3Archive: Opened %s (%zu files)", filepath.c_str(), m_entries.size());
    return true;
}

void XP3Archive::Close() {
    m_entries.clear();
    m_nameMap.clear();
    m_filepath.clear();
    m_isOpen = false;
}

bool XP3Archive::HasFile(const std::wstring& name) const {
    auto n = CharacterSet::NormalizePath(name);
    return m_nameMap.find(n) != m_nameMap.end();
}

std::vector<std::wstring> XP3Archive::GetFileList() const {
    std::vector<std::wstring> list;
    list.reserve(m_entries.size());
    for (const auto& e : m_entries) list.push_back(e.name);
    return list;
}

std::vector<uint8_t> XP3Archive::ExtractFile(const std::wstring& name) const {
    std::vector<uint8_t> result;
    auto n = CharacterSet::NormalizePath(name);
    auto it = m_nameMap.find(n);
    if (it == m_nameMap.end()) {
        TVP_LOG_WARN("XP3Archive: File not found: %s", CharacterSet::WideToUTF8(n).c_str());
        return result;
    }
    
    const XP3IndexEntry& entry = m_entries[it->second];
    result.resize(entry.uncomp_size);
    
    std::ifstream file(m_filepath, std::ios::binary);
    if (!file.is_open()) return result;
    
    file.seekg(entry.offset);
    std::vector<uint8_t> comp_data(entry.comp_size);
    file.read((char*)comp_data.data(), entry.comp_size);
    
    if (entry.flags & XP3_FLAG_COMPRESSED) {
        // Decompress with zlib
        if (!DecompressZLIB(comp_data.data(), comp_data.size(), result.data(), result.size())) {
            TVP_LOG_ERROR("XP3Archive: Decompression failed for: %s", CharacterSet::WideToUTF8(n).c_str());
            result.clear();
        }
    } else {
        // No compression, direct copy
        size_t copy_size = std::min((size_t)entry.comp_size, (size_t)entry.uncomp_size);
        memcpy(result.data(), comp_data.data(), copy_size);
    }
    
    return result;
}

bool XP3Archive::ExtractFileToDisk(const std::wstring& name, const std::string& destpath) const {
    auto data = ExtractFile(name);
    if (data.empty()) return false;
    
    std::ofstream out(destpath, std::ios::binary);
    if (!out.is_open()) return false;
    out.write((char*)data.data(), data.size());
    return true;
}

tjs_uint64 XP3Archive::GetFileSize(const std::wstring& name) const {
    auto n = CharacterSet::NormalizePath(name);
    auto it = m_nameMap.find(n);
    if (it == m_nameMap.end()) return 0;
    return m_entries[it->second].uncomp_size;
}

bool XP3Archive::DecompressZLIB(const uint8_t* src, size_t srcSize, uint8_t* dst, size_t dstSize) const {
    z_stream strm = {};
    strm.next_in = (Bytef*)src;
    strm.avail_in = srcSize;
    strm.next_out = (Bytef*)dst;
    strm.avail_out = dstSize;
    
    if (inflateInit(&strm) != Z_OK) {
        TVP_LOG_ERROR("XP3Archive: inflateInit failed");
        return false;
    }
    
    int ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    
    if (ret != Z_STREAM_END && ret != Z_OK) {
        TVP_LOG_ERROR("XP3Archive: inflate error: %d", ret);
        return false;
    }
    
    return true;
}