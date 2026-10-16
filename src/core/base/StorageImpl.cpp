#include "StorageImpl.h"
#include "CharacterSet.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

StorageImpl::StorageImpl() {}

StorageImpl::~StorageImpl() { Shutdown(); }

bool StorageImpl::Initialize(const std::string& gamePath) {
    m_gamePath = gamePath;
    
    // Set default save path: ~/.local/share/krkrv8a/saves/
    const char* home = getenv("HOME");
    if (home) {
        m_savePath = std::string(home) + "/.local/share/krkrv8a/saves";
        fs::create_directories(m_savePath);
    }
    
    TVP_LOG_INFO("StorageImpl: Game path = %s", gamePath.c_str());
    TVP_LOG_INFO("StorageImpl: Save path = %s", m_savePath.c_str());
    
    // Auto-detect and load XP3 archives in the game directory
    if (!gamePath.empty() && fs::is_directory(gamePath)) {
        for (const auto& entry : fs::directory_iterator(gamePath)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                // Convert extension to lowercase
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".xp3") {
                    TVP_LOG_INFO("StorageImpl: Found XP3 archive: %s", entry.path().string().c_str());
                    AddArchive(entry.path().string());
                }
            }
        }
    }
    
    return true;
}

void StorageImpl::Shutdown() {
    m_archives.clear();
    m_gamePath.clear();
}

std::wstring StorageImpl::NormalizeStoragePath(const std::wstring& path) const {
    return CharacterSet::NormalizePath(path);
}

std::string StorageImpl::WideToNative(const std::wstring& wpath) const {
    return CharacterSet::WideToUTF8(wpath);
}

bool StorageImpl::FileExists(const std::wstring& name) const {
    auto n = NormalizeStoragePath(name);
    
    // Check archives first (latest added takes priority)
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        if ((*it)->HasFile(n)) return true;
    }
    
    // Check on disk
    std::string native = WideToNative(n);
    std::string fullpath;
    
    // Try relative to game path
    if (!m_gamePath.empty()) {
        fullpath = m_gamePath + "/" + native;
        if (fs::exists(fullpath)) return true;
    }
    
    // Try absolute
    if (fs::exists(native)) return true;
    
    return false;
}

std::vector<uint8_t> StorageImpl::ReadFile(const std::wstring& name) const {
    auto n = NormalizeStoragePath(name);
    std::vector<uint8_t> result;
    
    // Check archives first (latest added takes priority)
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        if ((*it)->HasFile(n)) {
            result = (*it)->ExtractFile(n);
            if (!result.empty()) return result;
        }
    }
    
    // Read from disk
    std::string native = WideToNative(n);
    std::string fullpath;
    
    if (!m_gamePath.empty()) {
        fullpath = m_gamePath + "/" + native;
        if (fs::exists(fullpath)) {
            std::ifstream file(fullpath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                auto size = file.tellg();
                file.seekg(0);
                result.resize(size);
                file.read((char*)result.data(), size);
                return result;
            }
        }
    }
    
    // Try save path
    if (!m_savePath.empty()) {
        fullpath = m_savePath + "/" + native;
        if (fs::exists(fullpath)) {
            std::ifstream file(fullpath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                auto size = file.tellg();
                file.seekg(0);
                result.resize(size);
                file.read((char*)result.data(), size);
                return result;
            }
        }
    }
    
    // Try absolute path
    if (fs::exists(native)) {
        std::ifstream file(native, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            auto size = file.tellg();
            file.seekg(0);
            result.resize(size);
            file.read((char*)result.data(), size);
            return result;
        }
    }
    
    TVP_LOG_WARN("StorageImpl: File not found: %s", native.c_str());
    return result;
}

bool StorageImpl::WriteFile(const std::wstring& name, const uint8_t* data, size_t size) const {
    auto n = NormalizeStoragePath(name);
    std::string native = WideToNative(n);
    
    // Write to save path
    std::string fullpath = m_savePath + "/" + native;
    
    // Create directories if needed
    fs::path p(fullpath);
    fs::create_directories(p.parent_path());
    
    std::ofstream file(fullpath, std::ios::binary);
    if (!file.is_open()) {
        TVP_LOG_ERROR("StorageImpl: Cannot write file: %s", fullpath.c_str());
        return false;
    }
    file.write((const char*)data, size);
    return true;
}

std::vector<std::wstring> StorageImpl::ListFiles(const std::wstring& dirPath) const {
    std::vector<std::wstring> result;
    auto n = NormalizeStoragePath(dirPath);
    std::string native = WideToNative(n);
    
    // List from archives
    for (const auto& archive : m_archives) {
        auto fileList = archive->GetFileList();
        for (const auto& f : fileList) {
            // Check if file is under dirPath
            if (f.find(n) == 0) {
                result.push_back(f);
            }
        }
    }
    
    // List from disk
    std::string fullpath;
    if (!m_gamePath.empty()) {
        fullpath = m_gamePath + "/" + native;
    } else {
        fullpath = native;
    }
    
    if (fs::is_directory(fullpath)) {
        for (const auto& entry : fs::directory_iterator(fullpath)) {
            result.push_back(CharacterSet::UTF8ToWide(entry.path().filename().string()));
        }
    }
    
    return result;
}

bool StorageImpl::AddArchive(const std::string& archivePath) {
    auto archive = std::make_unique<XP3Archive>();
    if (archive->Open(archivePath)) {
        m_archives.push_back(std::move(archive));
        return true;
    }
    return false;
}