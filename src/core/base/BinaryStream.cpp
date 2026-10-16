#include "BinaryStream.h"
#include <cstring>
#include <fstream>

BinaryStream::BinaryStream() {}

BinaryStream::BinaryStream(const uint8_t* data, size_t size) : m_data(data, data + size), m_pos(0) {}

BinaryStream::BinaryStream(size_t initialCapacity) { m_data.reserve(initialCapacity); }

BinaryStream::~BinaryStream() {}

void BinaryStream::EnsureCapacity(size_t size) {
    if (m_data.size() < size) m_data.resize(size);
}

// Read operations
uint8_t BinaryStream::ReadByte() {
    if (m_pos + 1 > m_data.size()) return 0;
    return m_data[m_pos++];
}
uint16_t BinaryStream::ReadUInt16() {
    if (m_pos + 2 > m_data.size()) { m_pos = m_data.size(); return 0; }
    uint16_t v;
    memcpy(&v, &m_data[m_pos], 2);
    m_pos += 2;
    return v;
}
uint32_t BinaryStream::ReadUInt32() {
    if (m_pos + 4 > m_data.size()) { m_pos = m_data.size(); return 0; }
    uint32_t v;
    memcpy(&v, &m_data[m_pos], 4);
    m_pos += 4;
    return v;
}
uint64_t BinaryStream::ReadUInt64() {
    if (m_pos + 8 > m_data.size()) { m_pos = m_data.size(); return 0; }
    uint64_t v;
    memcpy(&v, &m_data[m_pos], 8);
    m_pos += 8;
    return v;
}
int16_t BinaryStream::ReadInt16() { return (int16_t)ReadUInt16(); }
int32_t BinaryStream::ReadInt32() { return (int32_t)ReadUInt32(); }
int64_t BinaryStream::ReadInt64() { return (int64_t)ReadUInt64(); }

float BinaryStream::ReadFloat() {
    uint32_t v = ReadUInt32();
    float f;
    memcpy(&f, &v, 4);
    return f;
}
double BinaryStream::ReadDouble() {
    uint64_t v = ReadUInt64();
    double d;
    memcpy(&d, &v, 8);
    return d;
}

std::string BinaryStream::ReadString(size_t len) {
    if (m_pos + len > m_data.size()) { m_pos = m_data.size(); return ""; }
    std::string s((char*)&m_data[m_pos], len);
    m_pos += len;
    return s;
}

std::wstring BinaryStream::ReadWString(size_t len) {
    if (m_pos + len * 2 > m_data.size()) { m_pos = m_data.size(); return L""; }
    std::wstring ws;
    ws.reserve(len);
    for (size_t i = 0; i < len; i++) {
        uint16_t ch = m_data[m_pos + i * 2] | (m_data[m_pos + i * 2 + 1] << 8);
        ws += (wchar_t)ch;
    }
    m_pos += len * 2;
    return ws;
}

std::string BinaryStream::ReadCString() {
    std::string s;
    while (m_pos < m_data.size() && m_data[m_pos] != 0) {
        s += (char)m_data[m_pos++];
    }
    if (m_pos < m_data.size()) m_pos++; // skip null
    return s;
}

std::vector<uint8_t> BinaryStream::ReadBytes(size_t count) {
    if (m_pos + count > m_data.size()) count = m_data.size() - m_pos;
    std::vector<uint8_t> bytes(m_data.begin() + m_pos, m_data.begin() + m_pos + count);
    m_pos += count;
    return bytes;
}

// Write operations
void BinaryStream::WriteByte(uint8_t v) { m_data.push_back(v); }
void BinaryStream::WriteUInt16(uint16_t v) {
    size_t pos = m_data.size();
    m_data.resize(pos + 2);
    memcpy(&m_data[pos], &v, 2);
}
void BinaryStream::WriteUInt32(uint32_t v) {
    size_t pos = m_data.size();
    m_data.resize(pos + 4);
    memcpy(&m_data[pos], &v, 4);
}
void BinaryStream::WriteUInt64(uint64_t v) {
    size_t pos = m_data.size();
    m_data.resize(pos + 8);
    memcpy(&m_data[pos], &v, 8);
}
void BinaryStream::WriteInt16(int16_t v) { WriteUInt16((uint16_t)v); }
void BinaryStream::WriteInt32(int32_t v) { WriteUInt32((uint32_t)v); }
void BinaryStream::WriteInt64(int64_t v) { WriteUInt64((uint64_t)v); }
void BinaryStream::WriteFloat(float v) { WriteUInt32(*(uint32_t*)&v); }
void BinaryStream::WriteDouble(double v) { WriteUInt64(*(uint64_t*)&v); }

void BinaryStream::WriteString(const std::string& s) {
    m_data.insert(m_data.end(), s.begin(), s.end());
}
void BinaryStream::WriteWString(const std::wstring& s) {
    for (wchar_t wc : s) {
        uint16_t ch = (uint16_t)wc;
        m_data.push_back(ch & 0xFF);
        m_data.push_back((ch >> 8) & 0xFF);
    }
}
void BinaryStream::WriteBytes(const uint8_t* data, size_t count) {
    m_data.insert(m_data.end(), data, data + count);
}

void BinaryStream::Seek(size_t pos) { m_pos = pos; }
void BinaryStream::Skip(size_t count) { m_pos += count; }

std::unique_ptr<BinaryStream> BinaryStream::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return nullptr;
    auto size = file.tellg();
    file.seekg(0);
    auto stream = std::make_unique<BinaryStream>(size);
    stream->m_data.resize(size);
    file.read((char*)stream->m_data.data(), size);
    return stream;
}

bool BinaryStream::SaveToFile(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write((const char*)m_data.data(), m_data.size());
    return true;
}