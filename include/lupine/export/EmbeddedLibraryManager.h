#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace Lupine {

/**
 * @brief Embedded library information
 */
struct EmbeddedLibrary {
    std::string name;
    std::string platform;
    std::string architecture;
    std::vector<uint8_t> data;
    size_t size;
    std::string checksum;
    std::string version;
};

/**
 * @brief Manager for embedded static libraries for cross-platform export
 * 
 * This class handles embedding Linux static libraries into the Windows editor
 * executable so they can be extracted and used for Linux export without requiring
 * the user to separately download Linux libraries.
 */
class EmbeddedLibraryManager {
public:
    EmbeddedLibraryManager();
    ~EmbeddedLibraryManager();

    /**
     * @brief Initialize the embedded library system
     * @return True if initialization successful
     */
    bool Initialize();

    /**
     * @brief Check if libraries for a platform are available
     * @param platform Platform name (e.g., "linux", "macos")
     * @param architecture Architecture (e.g., "x64", "arm64")
     * @return True if libraries are available
     */
    bool HasLibrariesForPlatform(const std::string& platform, const std::string& architecture) const;

    /**
     * @brief Extract embedded libraries to a directory
     * @param platform Platform name
     * @param architecture Architecture
     * @param output_dir Directory to extract libraries to
     * @return True if extraction successful
     */
    bool ExtractLibraries(const std::string& platform, const std::string& architecture, 
                         const std::filesystem::path& output_dir);

    /**
     * @brief Get list of available embedded libraries
     * @param platform Platform filter (empty for all)
     * @param architecture Architecture filter (empty for all)
     * @return Vector of library names
     */
    std::vector<std::string> GetAvailableLibraries(const std::string& platform = "", 
                                                   const std::string& architecture = "") const;

    /**
     * @brief Get embedded library information
     * @param name Library name
     * @param platform Platform name
     * @param architecture Architecture
     * @return Library info or nullptr if not found
     */
    const EmbeddedLibrary* GetLibraryInfo(const std::string& name, 
                                         const std::string& platform,
                                         const std::string& architecture) const;

    /**
     * @brief Extract a specific library to a file
     * @param name Library name
     * @param platform Platform name
     * @param architecture Architecture
     * @param output_path Output file path
     * @return True if extraction successful
     */
    bool ExtractLibrary(const std::string& name, const std::string& platform,
                       const std::string& architecture, const std::filesystem::path& output_path);

    /**
     * @brief Verify integrity of embedded libraries
     * @return True if all libraries pass integrity checks
     */
    bool VerifyIntegrity() const;

    /**
     * @brief Get total size of embedded libraries for a platform
     * @param platform Platform name
     * @param architecture Architecture
     * @return Total size in bytes
     */
    size_t GetTotalSize(const std::string& platform, const std::string& architecture) const;

    /**
     * @brief Check if embedded libraries need updating
     * @param platform Platform name
     * @param architecture Architecture
     * @return True if libraries are outdated
     */
    bool NeedsUpdate(const std::string& platform, const std::string& architecture) const;

private:
    /**
     * @brief Load embedded library data from resources
     */
    void LoadEmbeddedLibraries();

    /**
     * @brief Calculate checksum for data
     * @param data Data to checksum
     * @return Checksum string
     */
    std::string CalculateChecksum(const std::vector<uint8_t>& data) const;

    /**
     * @brief Create library key for lookup
     * @param name Library name
     * @param platform Platform name
     * @param architecture Architecture
     * @return Unique key string
     */
    std::string CreateLibraryKey(const std::string& name, const std::string& platform,
                                const std::string& architecture) const;

    // Map of library key to embedded library data
    std::unordered_map<std::string, std::unique_ptr<EmbeddedLibrary>> m_libraries;
    
    // Platform-specific library lists
    std::unordered_map<std::string, std::vector<std::string>> m_platform_libraries;
    
    bool m_initialized = false;
};

/**
 * @brief Get the global embedded library manager instance
 * @return Reference to the singleton instance
 */
EmbeddedLibraryManager& GetEmbeddedLibraryManager();

} // namespace Lupine
