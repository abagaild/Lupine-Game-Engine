#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainChunk;

/**
 * @brief Terrain file format version
 */
constexpr uint32_t TERRAIN_FILE_VERSION = 1;

/**
 * @brief Terrain file magic number for format identification
 */
constexpr uint32_t TERRAIN_FILE_MAGIC = 0x54455252; // "TERR"

/**
 * @brief Terrain file header structure
 */
struct TerrainFileHeader {
    uint32_t magic;                 // Magic number for format identification
    uint32_t version;               // File format version
    uint32_t flags;                 // Feature flags (compression, etc.)
    uint32_t chunk_count;           // Number of terrain chunks
    uint32_t texture_layer_count;   // Number of texture layers
    uint32_t asset_count;           // Total number of assets
    float terrain_width;            // Terrain width in world units
    float terrain_height;           // Terrain height in world units
    float resolution;               // Height samples per world unit
    float chunk_size;               // Chunk size in world units
    uint32_t reserved[6];           // Reserved for future use
    
    TerrainFileHeader()
        : magic(TERRAIN_FILE_MAGIC)
        , version(TERRAIN_FILE_VERSION)
        , flags(0)
        , chunk_count(0)
        , texture_layer_count(0)
        , asset_count(0)
        , terrain_width(0.0f)
        , terrain_height(0.0f)
        , resolution(1.0f)
        , chunk_size(64.0f)
        , reserved{0}
    {}
};

/**
 * @brief Terrain file flags
 */
enum class TerrainFileFlags : uint32_t {
    None = 0,
    Compressed = 1 << 0,        // Data is compressed
    HasTextures = 1 << 1,       // File contains texture layers
    HasAssets = 1 << 2,         // File contains asset instances
    HasNormals = 1 << 3,        // Chunks contain pre-calculated normals
    HasLOD = 1 << 4,            // File contains LOD data
    Streaming = 1 << 5          // File supports streaming
};

/**
 * @brief Texture layer information in file
 */
struct TerrainFileTextureLayer {
    char texture_path[256];     // Path to texture file
    float scale;                // Texture scale
    float opacity;              // Default opacity
    uint32_t flags;             // Layer flags
    uint32_t reserved[4];       // Reserved for future use
    
    TerrainFileTextureLayer()
        : scale(1.0f)
        , opacity(1.0f)
        , flags(0)
        , reserved{0}
    {
        texture_path[0] = '\0';
    }
};

/**
 * @brief Chunk header in file
 */
struct TerrainFileChunkHeader {
    int32_t chunk_x;            // Chunk X coordinate
    int32_t chunk_z;            // Chunk Z coordinate
    uint32_t height_data_size;  // Size of height data in bytes
    uint32_t blend_data_size;   // Size of texture blend data in bytes
    uint32_t asset_count;       // Number of assets in this chunk
    uint32_t flags;             // Chunk flags
    uint32_t reserved[2];       // Reserved for future use
    
    TerrainFileChunkHeader()
        : chunk_x(0)
        , chunk_z(0)
        , height_data_size(0)
        , blend_data_size(0)
        , asset_count(0)
        , flags(0)
        , reserved{0}
    {}
};

/**
 * @brief Asset instance in file
 */
struct TerrainFileAsset {
    char asset_path[256];       // Path to asset file
    float position[3];          // World position
    float rotation[3];          // Rotation in degrees
    float scale[3];             // Scale factors
    float height_offset;        // Height offset from terrain
    uint32_t flags;             // Asset flags
    uint32_t reserved[3];       // Reserved for future use
    
    TerrainFileAsset()
        : position{0.0f, 0.0f, 0.0f}
        , rotation{0.0f, 0.0f, 0.0f}
        , scale{1.0f, 1.0f, 1.0f}
        , height_offset(0.0f)
        , flags(0)
        , reserved{0}
    {
        asset_path[0] = '\0';
    }
};

/**
 * @brief Terrain file serializer/deserializer
 */
class TerrainFileFormat {
public:
    /**
     * @brief Save terrain data to file
     * @param terrain_data Terrain data to save
     * @param file_path Output file path
     * @param compress Enable compression
     * @return True if successful
     */
    static bool SaveToFile(const TerrainData& terrain_data, const std::string& file_path, bool compress = true);
    
    /**
     * @brief Load terrain data from file
     * @param file_path Input file path
     * @param terrain_data Output terrain data
     * @return True if successful
     */
    static bool LoadFromFile(const std::string& file_path, std::unique_ptr<TerrainData>& terrain_data);
    
    /**
     * @brief Validate terrain file format
     * @param file_path File path to validate
     * @return True if file is valid terrain format
     */
    static bool ValidateFile(const std::string& file_path);
    
    /**
     * @brief Get file format information
     * @param file_path File path to analyze
     * @param header Output header information
     * @return True if successful
     */
    static bool GetFileInfo(const std::string& file_path, TerrainFileHeader& header);

private:
    /**
     * @brief Write terrain header to stream
     * @param stream Output stream
     * @param terrain_data Terrain data
     * @param header File header
     * @return True if successful
     */
    static bool WriteHeader(std::ostream& stream, const TerrainData& terrain_data, const TerrainFileHeader& header);
    
    /**
     * @brief Read terrain header from stream
     * @param stream Input stream
     * @param header Output header
     * @return True if successful
     */
    static bool ReadHeader(std::istream& stream, TerrainFileHeader& header);
    
    /**
     * @brief Write texture layers to stream
     * @param stream Output stream
     * @param texture_layers Texture layer data
     * @return True if successful
     */
    static bool WriteTextureLayers(std::ostream& stream, const std::vector<TerrainFileTextureLayer>& texture_layers);
    
    /**
     * @brief Read texture layers from stream
     * @param stream Input stream
     * @param layer_count Number of layers to read
     * @param texture_layers Output texture layers
     * @return True if successful
     */
    static bool ReadTextureLayers(std::istream& stream, uint32_t layer_count, std::vector<TerrainFileTextureLayer>& texture_layers);
    
    /**
     * @brief Write chunk data to stream
     * @param stream Output stream
     * @param chunk Terrain chunk
     * @param compress Enable compression
     * @return True if successful
     */
    static bool WriteChunk(std::ostream& stream, const TerrainChunk& chunk, bool compress);
    
    /**
     * @brief Read chunk data from stream
     * @param stream Input stream
     * @param chunk Output terrain chunk
     * @param header Chunk header
     * @return True if successful
     */
    static bool ReadChunk(std::istream& stream, TerrainChunk& chunk, const TerrainFileChunkHeader& header);
    
    /**
     * @brief Write assets to stream
     * @param stream Output stream
     * @param assets Asset data
     * @return True if successful
     */
    static bool WriteAssets(std::ostream& stream, const std::vector<TerrainFileAsset>& assets);
    
    /**
     * @brief Read assets from stream
     * @param stream Input stream
     * @param asset_count Number of assets to read
     * @param assets Output assets
     * @return True if successful
     */
    static bool ReadAssets(std::istream& stream, uint32_t asset_count, std::vector<TerrainFileAsset>& assets);
    
    /**
     * @brief Compress data using zlib
     * @param input Input data
     * @param output Compressed output data
     * @return True if successful
     */
    static bool CompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    
    /**
     * @brief Decompress data using zlib
     * @param input Compressed input data
     * @param output Decompressed output data
     * @param expected_size Expected size of decompressed data
     * @return True if successful
     */
    static bool DecompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, size_t expected_size);
    
    /**
     * @brief Convert terrain texture blend to file format
     * @param blend_data Terrain texture blend data
     * @param file_data Output file data
     */
    static void ConvertBlendDataToFile(const std::vector<class TerrainTextureBlend>& blend_data, std::vector<uint8_t>& file_data);
    
    /**
     * @brief Convert file format to terrain texture blend
     * @param file_data Input file data
     * @param blend_data Output terrain texture blend data
     * @param layer_count Number of texture layers
     */
    static void ConvertBlendDataFromFile(const std::vector<uint8_t>& file_data, std::vector<class TerrainTextureBlend>& blend_data, uint32_t layer_count);
    
    /**
     * @brief Calculate CRC32 checksum
     * @param data Input data
     * @param size Data size
     * @return CRC32 checksum
     */
    static uint32_t CalculateCRC32(const uint8_t* data, size_t size);
};

/**
 * @brief OBJ export options
 */
struct OBJExportOptions {
    bool export_materials = true;      // Export material file (.mtl)
    bool export_textures = true;       // Export texture coordinates
    bool export_normals = true;        // Export vertex normals
    bool merge_chunks = true;           // Merge all chunks into single mesh
    float texture_scale = 1.0f;        // Texture coordinate scale
    std::string material_name = "terrain"; // Base material name
    
    OBJExportOptions() = default;
};

/**
 * @brief OBJ file exporter for terrain
 */
class TerrainOBJExporter {
public:
    /**
     * @brief Export terrain to OBJ format
     * @param terrain_data Terrain data to export
     * @param file_path Output file path (without extension)
     * @param options Export options
     * @return True if successful
     */
    static bool ExportToOBJ(const TerrainData& terrain_data, const std::string& file_path, const OBJExportOptions& options = OBJExportOptions());

private:
    /**
     * @brief Write OBJ file
     * @param terrain_data Terrain data
     * @param file_path Output file path
     * @param options Export options
     * @return True if successful
     */
    static bool WriteOBJFile(const TerrainData& terrain_data, const std::string& file_path, const OBJExportOptions& options);
    
    /**
     * @brief Write MTL file
     * @param file_path Output file path
     * @param options Export options
     * @return True if successful
     */
    static bool WriteMTLFile(const std::string& file_path, const OBJExportOptions& options);
};

} // namespace Lupine
