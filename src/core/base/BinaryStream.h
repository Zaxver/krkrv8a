#pragma once
#ifndef TVP_BINARY_STREAM_H
#define TVP_BINARY_STREAM_H

#include "tvp_types.h"
#include <vector>
#include <string>
#include <cstdint>

// Binary stream for reading/writing binary data
class BinaryStream {
public:
    BinaryStream();
    BinaryStream(const uint8_t* data, size_t size);
    explicit BinaryStream(size_t initialCapacity);
    ~BinaryStream();
    
    // Read operations
    uint8_t  ReadByte();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    int16_t  ReadInt16();
    int32_t  ReadInt32();
    int64_t  ReadInt64();
    float    ReadFloat();
    double   ReadDouble();
    std::string ReadString(size_t len);
    std::wstring ReadWString(size_t len);  // UTF-16LE
    std::string ReadCString();  // null-terminated
    std::vector<uint8_t> ReadBytes(size_t count);
    
    // Write operations
    void WriteByte(uint8_t v);
    void WriteUInt16(uint16_t v);
    void WriteUInt32(uint32_t v);
    void WriteUInt64(uint64_t v);
    void WriteInt16(int16_t v);
    void WriteInt32(int32_t v);
    void WriteInt64(int64_t v);
    void WriteFloat(float v);
    void WriteDouble(double v);
    void WriteString(const std::string& s);
    void WriteWString(const std::wstring& s);  // UTF-16LE
    void WriteBytes(const uint8_t* data, size_t count);
    
    // Position
    size_t GetPosition() const { return m_pos; }
    size_t GetSize() const { return m_data.size(); }
    void Seek(size_t pos);
    void Skip(size_t count);
    bool IsEOF() const { return m_pos >= m_data.size(); }
    
    // Data access
    const uint8_t* GetData() const { return m_data.data(); }
    uint8_t* GetData() { return m_data.data(); }
    std::vector<uint8_t>& GetBuffer() { return m_data; }
    
    // Load/Save
    static std::unique_ptr<BinaryStream> LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;
    
private:
    std::vector<uint8_t> m_data;
    size_t m_pos = 0;
    
    void EnsureCapacity(size_t size);
};

#endif // TVP_BINARY_STREAM_H