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
    
    // XP3 header is 11 bytes: "XP3\r\n \n\x1A\x8B\x67\x01"
    // (0x58 0x50 0x33 0x0D 0x0A 0x20 0x0A 0x1A 0x8B 0x67 0x01)
    static const uint8_t XP3Mark[11] = {
        0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01
    };
    
    uint8_t mark[11];
    file.read((char*)mark, 11);
    if (memcmp(mark, XP3Mark, 11) != 0) {
        // Check if it's an EXE with embedded XP3 (starts with "MZ")
        if (mark[0] == 0x4D && mark[1] == 0x5A) {
            TVP_LOG_WARN("XP3Archive: EXE-embedded XP3 not supported yet");
            // TODO: search for XP3 mark in EXE
        }
        TVP_LOG_ERROR("XP3Archive: Invalid header: %s", filepath.c_str());
        return false;
    }
    
    // After 11-byte header: 8-byte index_offset (little-endian)
    uint64_t index_offset = 0;
    file.read((char*)&index_offset, 8);
    TVP_LOG_INFO("XP3Archive: index_offset=%zu", (size_t)index_offset);
    
    if (index_offset > 0xFFFFFFFF) {
        TVP_LOG_ERROR("XP3Archive: index_offset too large: %zu", (size_t)index_offset);
        return false;
    }
    
    // Seek to index
    file.seekg(index_offset);
    
    // Read index flag (1 byte)
    // bit 0-3: encoding method (0=raw, 1=zlib compressed)
    uint8_t index_flag;
    file.read((char*)&index_flag, 1);
    
    std::vector<uint8_t> index_data;
    
    if (index_flag & 0x07) {
        // Compressed index (zlib)
        // Format: flag(1) + compressed_size(8) + uncompressed_size(8) + zlib_data
        uint64_t comp_size, uncomp_size;
        file.read((char*)&comp_size, 8);
        file.read((char*)&uncomp_size, 8);
        TVP_LOG_INFO("XP3Archive: compressed index, comp=%zu uncomp=%zu", 
                     (size_t)comp_size, (size_t)uncomp_size);
        
        if (uncomp_size > 256 * 1024 * 1024) {
            TVP_LOG_ERROR("XP3Archive: uncompressed index too large: %zu MB", (size_t)(uncomp_size / 1024 / 1024));
            return false;
        }
        
        // Read exactly compressed_size bytes
        std::vector<uint8_t> comp_data(comp_size);
        file.read((char*)comp_data.data(), comp_size);
        
        // Decompress with zlib
        index_data.resize(uncomp_size);
        uLongf dest_len = uncomp_size;
        int zret = uncompress(
            (Bytef*)index_data.data(), &dest_len,
            (const Bytef*)comp_data.data(), (uLong)comp_size
        );
        
        if (zret != Z_OK) {
            TVP_LOG_ERROR("XP3Archive: zlib uncompress failed: %d", zret);
            return false;
        }
        
        index_data.resize(dest_len);
        TVP_LOG_INFO("XP3Archive: decompressed %zu -> %zu bytes", (size_t)comp_size, index_data.size());
    } else {
        // Uncompressed index: read remaining data from current position
        std::streampos cur_pos = file.tellg();
        file.seekg(0, std::ios::end);
        size_t remaining = (size_t)((std::streamoff)file.tellg() - (std::streamoff)cur_pos);
        file.seekg(cur_pos);
        
        if (remaining > 256 * 1024 * 1024) {
            TVP_LOG_ERROR("XP3Archive: index data too large: %zu MB", remaining / 1024 / 1024);
            return false;
        }
        
        index_data.resize(remaining);
        file.read((char*)index_data.data(), remaining);
    }
    
    // Parse File chunks from index_data
    {
        size_t pos = 0;
        while (pos + 12 <= index_data.size()) {
            char type[4];
            memcpy(type, &index_data[pos], 4);
            pos += 4;
            uint64_t csize;
            memcpy(&csize, &index_data[pos], 8);
            pos += 8;
            
            if (memcmp(type, "File", 4) != 0) {
                pos += csize;
                continue;
            }
            
            size_t chunk_end = pos + csize;
            XP3IndexEntry entry = {};
            
            while (pos + 12 <= chunk_end) {
                char stype[4];
                memcpy(stype, &index_data[pos], 4);
                pos += 4;
                uint64_t ssize;
                memcpy(&ssize, &index_data[pos], 8);
                pos += 8;
                
                if (memcmp(stype, "info", 4) == 0) {
                    if (pos + 32 > index_data.size()) break;
                    memcpy(&entry.flags, &index_data[pos], 8);
                    memcpy(&entry.offset, &index_data[pos+8], 8);
                    memcpy(&entry.comp_size, &index_data[pos+16], 8);
                    memcpy(&entry.uncomp_size, &index_data[pos+24], 8);
                    pos += ssize;
                } else if (memcmp(stype, "name", 4) == 0) {
                    if (pos + 4 > index_data.size()) break;
                    uint32_t name_len;
                    memcpy(&name_len, &index_data[pos], 4);
                    pos += 4;
                    if (pos + name_len * 2 > index_data.size()) break;
                    entry.name.resize(name_len);
                    for (uint32_t i = 0; i < name_len; i++) {
                        uint16_t ch;
                        memcpy(&ch, &index_data[pos + i * 2], 2);
                        entry.name[i] = (wchar_t)ch;
                    }
                    pos += name_len * 2;
                    size_t consumed = 4 + name_len * 2;
                    if (ssize > consumed) pos += ssize - consumed;
                } else if (memcmp(stype, "segm", 4) == 0) {
                    pos += ssize;
                } else if (memcmp(stype, "adlr", 4) == 0) {
                    pos += ssize;
                } else {
                    pos += ssize;
                }
            }
            
            entry.name = CharacterSet::NormalizePath(entry.name);
            m_entries.push_back(entry);
            m_nameMap[entry.name] = m_entries.size() - 1;
            pos = chunk_end;
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