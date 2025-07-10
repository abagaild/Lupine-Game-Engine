#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>

namespace Lupine {

/**
 * @brief Asset entry in the bundle
 */
struct AssetEntry {
    std::string path;           // Original asset path
    std::string bundle_path;    // Path within the bundle
    size_t offset;              // Offset in the bundle file
    size_t size;                // Size of the asset
    uint32_t checksum;          // CRC32 checksum for integrity
};

/**
 * @brief Asset bundle header
 */
struct AssetBundleHeader {
    char magic[8] = {'L', 'U', 'P', 'I', 'N', 'E', 'A', 'B'}; // "LUPINEAB"
    uint32_t version = 1;
    uint32_t asset_count;
    uint64_t index_offset;      // Offset to the asset index
    uint64_t index_size;        // Size of the asset index
};

/**
 * @brief Asset bundler for packaging game assets
 */
class AssetBundler {
public:
    AssetBundler();
    ~AssetBundler();
    
    /**
     * @brief Add an asset to the bundle
     * @param asset_path Path to the asset file
     * @param bundle_path Path within the bundle (optional, uses asset_path if empty)
     * @return True if asset was added successfully
     */
    bool AddAsset(const std::filesystem::path& asset_path, 
                  const std::string& bundle_path = "");
    
    /**
     * @brief Add all assets from a directory recursively
     * @param directory_path Directory to scan for assets
     * @param base_bundle_path Base path within the bundle
     * @return Number of assets added
     */
    size_t AddAssetsFromDirectory(const std::filesystem::path& directory_path,
                                 const std::string& base_bundle_path = "");

    /**
     * @brief Add a complete project with all its assets and dependencies
     * @param project_path Path to the project file (.lupine)
     * @param include_runtime_dlls Whether to include runtime DLL dependencies
     * @return True if project was added successfully
     */
    bool AddProject(const std::filesystem::path& project_path,
                   bool include_runtime_dlls = true);
    
    /**
     * @brief Create the asset bundle file
     * @param bundle_path Path to the output bundle file
     * @return True if bundle was created successfully
     */
    bool CreateBundle(const std::filesystem::path& bundle_path);
    
    /**
     * @brief Embed the asset bundle into an executable
     * @param executable_path Path to the executable
     * @param bundle_path Path to the bundle file
     * @return True if embedding was successful
     */
    bool EmbedBundleInExecutable(const std::filesystem::path& executable_path,
                                const std::filesystem::path& bundle_path);
    
    /**
     * @brief Get list of assets in the bundle
     * @return Vector of asset entries
     */
    const std::vector<AssetEntry>& GetAssets() const { return m_assets; }
    
    /**
     * @brief Clear all assets from the bundle
     */
    void Clear();
    
    /**
     * @brief Get total size of all assets
     * @return Total size in bytes
     */
    size_t GetTotalSize() const;
    
    /**
     * @brief Set compression level (0 = no compression, 9 = maximum compression)
     * @param level Compression level
     */
    void SetCompressionLevel(int level) { m_compression_level = level; }
    
    /**
     * @brief Enable/disable asset optimization
     * @param optimize Whether to optimize assets
     */
    void SetOptimizeAssets(bool optimize) { m_optimize_assets = optimize; }

private:
    /**
     * @brief Calculate CRC32 checksum for a file
     * @param file_path Path to the file
     * @return CRC32 checksum
     */
    uint32_t CalculateChecksum(const std::filesystem::path& file_path);
    
    /**
     * @brief Optimize an asset (compress, convert format, etc.)
     * @param asset_path Path to the asset
     * @param output_path Path for the optimized asset
     * @return True if optimization was successful
     */
    bool OptimizeAsset(const std::filesystem::path& asset_path,
                      const std::filesystem::path& output_path);
    
    /**
     * @brief Check if a file is a supported asset type
     * @param file_path Path to the file
     * @return True if the file is a supported asset
     */
    bool IsSupportedAsset(const std::filesystem::path& file_path);

    /**
     * @brief Add runtime dependencies (DLLs) to the bundle
     * @return True if dependencies were added successfully
     */
    bool AddRuntimeDependencies();
    
    /**
     * @brief Write the asset index to the bundle
     * @param bundle_file Output file stream
     * @return True if index was written successfully
     */
    bool WriteAssetIndex(std::ofstream& bundle_file);

    std::vector<AssetEntry> m_assets;
    int m_compression_level;
    bool m_optimize_assets;
    
    // Supported asset extensions
    static const std::vector<std::string> s_supported_extensions;
};

/**
 * @brief Asset bundle reader for runtime asset loading
 */
class AssetBundleReader {
public:
    AssetBundleReader();
    ~AssetBundleReader();
    
    /**
     * @brief Open an asset bundle file
     * @param bundle_path Path to the bundle file
     * @return True if bundle was opened successfully
     */
    bool OpenBundle(const std::filesystem::path& bundle_path);
    
    /**
     * @brief Open an embedded asset bundle from an executable
     * @param executable_path Path to the executable
     * @return True if embedded bundle was found and opened
     */
    bool OpenEmbeddedBundle(const std::filesystem::path& executable_path);
    
    /**
     * @brief Check if an asset exists in the bundle
     * @param asset_path Path to the asset within the bundle
     * @return True if asset exists
     */
    bool HasAsset(const std::string& asset_path) const;
    
    /**
     * @brief Load an asset from the bundle
     * @param asset_path Path to the asset within the bundle
     * @param data Output buffer for the asset data
     * @return True if asset was loaded successfully
     */
    bool LoadAsset(const std::string& asset_path, std::vector<uint8_t>& data);
    
    /**
     * @brief Get asset information
     * @param asset_path Path to the asset within the bundle
     * @return Pointer to asset entry or nullptr if not found
     */
    const AssetEntry* GetAssetInfo(const std::string& asset_path) const;
    
    /**
     * @brief Get list of all assets in the bundle
     * @return Vector of asset entries
     */
    const std::vector<AssetEntry>& GetAllAssets() const { return m_assets; }
    
    /**
     * @brief Close the bundle
     */
    void Close();

private:
    /**
     * @brief Read the asset index from the bundle
     * @return True if index was read successfully
     */
    bool ReadAssetIndex();
    
    /**
     * @brief Find embedded bundle in executable
     * @param executable_path Path to the executable
     * @param bundle_offset Output offset of the bundle
     * @param bundle_size Output size of the bundle
     * @return True if embedded bundle was found
     */
    bool FindEmbeddedBundle(const std::filesystem::path& executable_path,
                           size_t& bundle_offset, size_t& bundle_size);

    std::unique_ptr<std::ifstream> m_bundle_file;
    std::vector<AssetEntry> m_assets;
    std::unordered_map<std::string, size_t> m_asset_index;
    AssetBundleHeader m_header;
    size_t m_bundle_offset; // For embedded bundles
};

} // namespace Lupine
