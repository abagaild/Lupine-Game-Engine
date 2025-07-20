#include "lupine/export/AssetBundler.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace Lupine {

// Supported asset extensions
const std::vector<std::string> AssetBundler::s_supported_extensions = {
    ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds", ".ico",  // Images
    ".obj", ".fbx", ".gltf", ".glb", ".dae", ".3ds",          // 3D Models
    ".wav", ".mp3", ".ogg", ".flac", ".aiff", ".m4a",         // Audio
    ".ttf", ".otf", ".woff", ".woff2",                        // Fonts
    ".scene", ".lupine", ".tileset", ".tilemap",              // Lupine files
    ".lua", ".py", ".js", ".cs", ".cpp", ".h",                // Scripts
    ".json", ".xml", ".txt", ".yaml", ".yml", ".cfg",         // Data files
    ".dll", ".so", ".dylib",                                  // Runtime libraries
    ".exe", ".app",                                           // Executables (for tools)
    ".shader", ".glsl", ".hlsl", ".vert", ".frag",            // Shaders
    ".mat", ".anim", ".prefab"                                // Additional game assets
};

AssetBundler::AssetBundler()
    : m_compression_level(6)
    , m_optimize_assets(true) {
}

AssetBundler::~AssetBundler() = default;

bool AssetBundler::AddAsset(const std::filesystem::path& asset_path, 
                           const std::string& bundle_path) {
    if (!std::filesystem::exists(asset_path)) {
        std::cerr << "Asset file does not exist: " << asset_path << std::endl;
        return false;
    }
    
    if (!IsSupportedAsset(asset_path)) {
        std::cerr << "Unsupported asset type: " << asset_path << std::endl;
        return false;
    }
    
    AssetEntry entry;
    entry.path = asset_path.string();
    entry.bundle_path = bundle_path.empty() ? asset_path.string() : bundle_path;
    entry.size = std::filesystem::file_size(asset_path);
    entry.checksum = CalculateChecksum(asset_path);
    entry.offset = 0; // Will be set when creating the bundle
    
    // Normalize bundle path (use forward slashes)
    std::replace(entry.bundle_path.begin(), entry.bundle_path.end(), '\\', '/');
    
    m_assets.push_back(entry);
    return true;
}

size_t AssetBundler::AddAssetsFromDirectory(const std::filesystem::path& directory_path,
                                           const std::string& base_bundle_path) {
    size_t added_count = 0;
    
    if (!std::filesystem::exists(directory_path)) {
        return 0;
    }
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                auto relative_path = std::filesystem::relative(entry.path(), directory_path);
                std::string bundle_path = base_bundle_path;
                
                if (!bundle_path.empty() && bundle_path.back() != '/') {
                    bundle_path += "/";
                }
                bundle_path += relative_path.string();
                
                if (AddAsset(entry.path(), bundle_path)) {
                    added_count++;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    return added_count;
}

bool AssetBundler::AddProject(const std::filesystem::path& project_path,
                             bool include_runtime_dlls) {
    if (!std::filesystem::exists(project_path)) {
        std::cerr << "Project file does not exist: " << project_path << std::endl;
        return false;
    }

    // Add the project file itself
    if (!AddAsset(project_path, "project.lupine")) {
        std::cerr << "Failed to add project file: " << project_path << std::endl;
        return false;
    }

    std::filesystem::path project_dir = project_path.parent_path();

    // Add standard project directories
    std::vector<std::string> project_dirs = {"assets", "scenes", "scripts"};
    for (const auto& dir_name : project_dirs) {
        auto dir_path = project_dir / dir_name;
        if (std::filesystem::exists(dir_path)) {
            size_t added = AddAssetsFromDirectory(dir_path, dir_name);
            std::cout << "Added " << added << " files from " << dir_name << " directory" << std::endl;
        }
    }

    // Add runtime DLLs if requested
    if (include_runtime_dlls) {
        AddRuntimeDependencies();
    }

    return true;
}

bool AssetBundler::CreateBundle(const std::filesystem::path& bundle_path) {
    if (m_assets.empty()) {
        std::cerr << "No assets to bundle" << std::endl;
        return false;
    }
    
    std::ofstream bundle_file(bundle_path, std::ios::binary);
    if (!bundle_file) {
        std::cerr << "Failed to create bundle file: " << bundle_path << std::endl;
        return false;
    }
    
    // Write header (will be updated later)
    AssetBundleHeader header;
    header.asset_count = static_cast<uint32_t>(m_assets.size());
    bundle_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write asset data
    size_t current_offset = sizeof(AssetBundleHeader);
    
    for (auto& asset : m_assets) {
        asset.offset = current_offset;
        
        std::ifstream asset_file(asset.path, std::ios::binary);
        if (!asset_file) {
            std::cerr << "Failed to open asset file: " << asset.path << std::endl;
            continue;
        }
        
        // Copy asset data
        std::vector<char> buffer(8192);
        size_t bytes_written = 0;
        
        while (asset_file.read(buffer.data(), buffer.size()) || asset_file.gcount() > 0) {
            size_t bytes_read = asset_file.gcount();
            bundle_file.write(buffer.data(), bytes_read);
            bytes_written += bytes_read;
        }
        
        asset.size = bytes_written;
        current_offset += bytes_written;
        
        asset_file.close();
    }
    
    // Write asset index
    header.index_offset = current_offset;
    if (!WriteAssetIndex(bundle_file)) {
        std::cerr << "Failed to write asset index" << std::endl;
        return false;
    }

    header.index_size = static_cast<uint64_t>(bundle_file.tellp()) - header.index_offset;
    
    // Update header
    bundle_file.seekp(0);
    bundle_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    bundle_file.close();
    
    std::cout << "Created asset bundle: " << bundle_path << std::endl;
    std::cout << "Assets: " << m_assets.size() << ", Size: " << current_offset << " bytes" << std::endl;
    
    return true;
}

bool AssetBundler::EmbedBundleInExecutable(const std::filesystem::path& executable_path,
                                          const std::filesystem::path& bundle_path) {
    try {
        // Read the bundle file
        std::ifstream bundle_file(bundle_path, std::ios::binary);
        if (!bundle_file) {
            return false;
        }
        
        // Get bundle size
        bundle_file.seekg(0, std::ios::end);
        size_t bundle_size = bundle_file.tellg();
        bundle_file.seekg(0, std::ios::beg);
        
        // Read bundle data
        std::vector<char> bundle_data(bundle_size);
        bundle_file.read(bundle_data.data(), bundle_size);
        bundle_file.close();
        
        // Append to executable
        std::ofstream exe_file(executable_path, std::ios::binary | std::ios::app);
        if (!exe_file) {
            return false;
        }
        
        // Write bundle data
        exe_file.write(bundle_data.data(), bundle_size);
        
        // Write bundle footer (for finding the embedded bundle)
        struct BundleFooter {
            uint64_t bundle_size;
            char magic[8] = {'L', 'U', 'P', 'B', 'U', 'N', 'D', 'L'}; // "LUPBUNDL"
        } footer;
        
        footer.bundle_size = bundle_size;
        exe_file.write(reinterpret_cast<const char*>(&footer), sizeof(footer));
        
        exe_file.close();
        
        std::cout << "Embedded bundle in executable: " << executable_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to embed bundle: " << e.what() << std::endl;
        return false;
    }
}

void AssetBundler::Clear() {
    m_assets.clear();
}

size_t AssetBundler::GetTotalSize() const {
    size_t total = 0;
    for (const auto& asset : m_assets) {
        total += asset.size;
    }
    return total;
}

uint32_t AssetBundler::CalculateChecksum(const std::filesystem::path& file_path) {
    // Simple CRC32 implementation
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return 0;
    }
    
    uint32_t crc = 0xFFFFFFFF;
    char byte;
    
    while (file.read(&byte, 1)) {
        crc ^= static_cast<uint32_t>(byte);
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

bool AssetBundler::OptimizeAsset(const std::filesystem::path& asset_path,
                                const std::filesystem::path& output_path) {
    // For now, just copy the file
    // In a full implementation, you would optimize based on file type
    try {
        std::filesystem::copy_file(asset_path, output_path,
                                  std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to optimize asset: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBundler::IsSupportedAsset(const std::filesystem::path& file_path) {
    std::string extension = file_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return std::find(s_supported_extensions.begin(), s_supported_extensions.end(), extension) 
           != s_supported_extensions.end();
}

bool AssetBundler::WriteAssetIndex(std::ofstream& bundle_file) {
    try {
        // Write number of assets
        uint32_t asset_count = static_cast<uint32_t>(m_assets.size());
        bundle_file.write(reinterpret_cast<const char*>(&asset_count), sizeof(asset_count));
        
        // Write asset entries
        for (const auto& asset : m_assets) {
            // Write bundle path length and string
            uint32_t path_length = static_cast<uint32_t>(asset.bundle_path.length());
            bundle_file.write(reinterpret_cast<const char*>(&path_length), sizeof(path_length));
            bundle_file.write(asset.bundle_path.c_str(), path_length);
            
            // Write asset metadata
            bundle_file.write(reinterpret_cast<const char*>(&asset.offset), sizeof(asset.offset));
            bundle_file.write(reinterpret_cast<const char*>(&asset.size), sizeof(asset.size));
            bundle_file.write(reinterpret_cast<const char*>(&asset.checksum), sizeof(asset.checksum));
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write asset index: " << e.what() << std::endl;
        return false;
    }
}

bool AssetBundler::AddRuntimeDependencies() {
    // Skip DLL inclusion since we're using static linking
    std::cout << "Using static linking - no runtime DLLs needed" << std::endl;
    return true;
}

// AssetBundleReader implementation
AssetBundleReader::AssetBundleReader()
    : m_bundle_offset(0) {
}

AssetBundleReader::~AssetBundleReader() {
    Close();
}

bool AssetBundleReader::OpenBundle(const std::filesystem::path& bundle_path) {
    Close();

    m_bundle_file = std::make_unique<std::ifstream>(bundle_path, std::ios::binary);
    if (!m_bundle_file->is_open()) {
        std::cerr << "Failed to open bundle file: " << bundle_path << std::endl;
        return false;
    }

    m_bundle_offset = 0;

    // Read header
    m_bundle_file->read(reinterpret_cast<char*>(&m_header), sizeof(m_header));

    // Verify magic
    if (std::memcmp(m_header.magic, "LUPINEAB", 8) != 0) {
        std::cerr << "Invalid bundle file format" << std::endl;
        Close();
        return false;
    }

    return ReadAssetIndex();
}

bool AssetBundleReader::OpenEmbeddedBundle(const std::filesystem::path& executable_path) {
    Close();

    size_t bundle_offset, bundle_size;
    if (!FindEmbeddedBundle(executable_path, bundle_offset, bundle_size)) {
        return false;
    }

    m_bundle_file = std::make_unique<std::ifstream>(executable_path, std::ios::binary);
    if (!m_bundle_file->is_open()) {
        return false;
    }

    m_bundle_offset = bundle_offset;

    // Seek to bundle start and read header
    m_bundle_file->seekg(m_bundle_offset);
    m_bundle_file->read(reinterpret_cast<char*>(&m_header), sizeof(m_header));

    // Verify magic
    if (std::memcmp(m_header.magic, "LUPINEAB", 8) != 0) {
        std::cerr << "Invalid embedded bundle format" << std::endl;
        Close();
        return false;
    }

    return ReadAssetIndex();
}

bool AssetBundleReader::HasAsset(const std::string& asset_path) const {
    return m_asset_index.find(asset_path) != m_asset_index.end();
}

bool AssetBundleReader::LoadAsset(const std::string& asset_path, std::vector<uint8_t>& data) {
    auto it = m_asset_index.find(asset_path);
    if (it == m_asset_index.end()) {
        return false;
    }

    const AssetEntry& asset = m_assets[it->second];

    if (!m_bundle_file) {
        return false;
    }

    // Seek to asset data
    m_bundle_file->seekg(m_bundle_offset + asset.offset);

    // Read asset data
    data.resize(asset.size);
    m_bundle_file->read(reinterpret_cast<char*>(data.data()), asset.size);

    return m_bundle_file->gcount() == static_cast<std::streamsize>(asset.size);
}

const AssetEntry* AssetBundleReader::GetAssetInfo(const std::string& asset_path) const {
    auto it = m_asset_index.find(asset_path);
    if (it == m_asset_index.end()) {
        return nullptr;
    }

    return &m_assets[it->second];
}

void AssetBundleReader::Close() {
    if (m_bundle_file) {
        m_bundle_file->close();
        m_bundle_file.reset();
    }

    m_assets.clear();
    m_asset_index.clear();
    m_bundle_offset = 0;
}

bool AssetBundleReader::ReadAssetIndex() {
    if (!m_bundle_file) {
        return false;
    }

    // Seek to index
    m_bundle_file->seekg(m_bundle_offset + m_header.index_offset);

    // Read asset count
    uint32_t asset_count;
    m_bundle_file->read(reinterpret_cast<char*>(&asset_count), sizeof(asset_count));

    m_assets.clear();
    m_asset_index.clear();
    m_assets.reserve(asset_count);

    // Read asset entries
    for (uint32_t i = 0; i < asset_count; ++i) {
        AssetEntry entry;

        // Read bundle path
        uint32_t path_length;
        m_bundle_file->read(reinterpret_cast<char*>(&path_length), sizeof(path_length));

        entry.bundle_path.resize(path_length);
        m_bundle_file->read(entry.bundle_path.data(), path_length);

        // Read metadata
        m_bundle_file->read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
        m_bundle_file->read(reinterpret_cast<char*>(&entry.size), sizeof(entry.size));
        m_bundle_file->read(reinterpret_cast<char*>(&entry.checksum), sizeof(entry.checksum));

        m_asset_index[entry.bundle_path] = m_assets.size();
        m_assets.push_back(entry);
    }

    std::cout << "Loaded asset bundle with " << asset_count << " assets" << std::endl;
    return true;
}

bool AssetBundleReader::FindEmbeddedBundle(const std::filesystem::path& executable_path,
                                          size_t& bundle_offset, size_t& bundle_size) {
    std::ifstream exe_file(executable_path, std::ios::binary);
    if (!exe_file) {
        return false;
    }

    // Get file size
    exe_file.seekg(0, std::ios::end);
    size_t file_size = exe_file.tellg();

    // Look for bundle footer at the end of the file
    struct BundleFooter {
        uint64_t bundle_size;
        char magic[8];
    } footer;

    if (file_size < sizeof(footer)) {
        return false;
    }

    exe_file.seekg(file_size - sizeof(footer));
    exe_file.read(reinterpret_cast<char*>(&footer), sizeof(footer));

    // Check magic
    if (std::memcmp(footer.magic, "LUPBUNDL", 8) != 0) {
        return false;
    }

    bundle_size = footer.bundle_size;
    bundle_offset = file_size - sizeof(footer) - bundle_size;

    return true;
}

} // namespace Lupine
