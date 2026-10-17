#pragma once
#ifndef TVP_STORAGE_IMPL_H
#define TVP_STORAGE_IMPL_H

#include "tvp_types.h"
#include "XP3Archive.h"
#include "PFSArchive.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Storage system: handles file I/O for Kirikiri2
// Manages both loose files and XP3 archives
class StorageImpl {
public:
    StorageImpl();
    ~StorageImpl();
    
    // Initialize with a game root path
    bool Initialize(const std::string& gamePath);
    void Shutdown();
    
    // Check if a file exists (in archives or on disk)
    bool FileExists(const std::wstring& name) const;
    
    // Read a file fully
    std::vector<uint8_t> ReadFile(const std::wstring& name) const;
    
    // Write a file to disk (for saves)
    bool WriteFile(const std::wstring& name, const uint8_t* data, size_t size) const;
    
    // List files in a directory
    std::vector<std::wstring> ListFiles(const std::wstring& dirPath) const;
    
    // Get the game root path
    const std::string& GetGamePath() const { return m_gamePath; }
    
    // Add an XP3 archive
    bool AddArchive(const std::string& archivePath);
    
    // Get the save data path
    std::string GetSavePath() const { return m_savePath; }
    void SetSavePath(const std::string& path) { m_savePath = path; }
    
private:
    std::string m_gamePath;
    std::string m_savePath;
    std::vector<std::unique_ptr<XP3Archive>> m_archives;
    std::vector<std::unique_ptr<PFSArchive>> m_pfsArchives;
    
    std::string WideToNative(const std::wstring& wpath) const;
    std::wstring NormalizeStoragePath(const std::wstring& path) const;
};

#endif // TVP_STORAGE_IMPL_H