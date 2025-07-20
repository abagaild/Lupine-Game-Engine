#pragma once

#include "GraphicsBuffer.h"
#include <cstdint>

namespace Lupine {

/**
 * @brief Vertex attribute data types
 */
enum class VertexAttributeType : uint32_t {
    Float = 0x1406,     // GL_FLOAT
    Int = 0x1404,       // GL_INT
    UInt = 0x1405,      // GL_UNSIGNED_INT
    Short = 0x1402,     // GL_SHORT
    UShort = 0x1403,    // GL_UNSIGNED_SHORT
    Byte = 0x1400,      // GL_BYTE
    UByte = 0x1401      // GL_UNSIGNED_BYTE
};

/**
 * @brief Get the size in bytes of a vertex attribute type
 * @param type The vertex attribute type
 * @return Size in bytes
 */
inline uint32_t GetVertexAttributeTypeSize(VertexAttributeType type) {
    switch (type) {
        case VertexAttributeType::Float:
            return 4;
        case VertexAttributeType::Int:
        case VertexAttributeType::UInt:
            return 4;
        case VertexAttributeType::Short:
        case VertexAttributeType::UShort:
            return 2;
        case VertexAttributeType::Byte:
        case VertexAttributeType::UByte:
            return 1;
        default:
            return 4; // Default to float size
    }
}

/**
 * @brief Extended vertex layout class with convenience methods
 */
class ExtendedVertexLayout : public VertexLayout {
public:
    /**
     * @brief Add a vertex attribute with typed enum
     * @param type Vertex attribute type
     * @param components Number of components (1-4)
     * @param normalized Whether to normalize integer data
     */
    void AddAttribute(VertexAttributeType type, uint32_t components, bool normalized = false) {
        uint32_t location = static_cast<uint32_t>(GetAttributes().size());
        VertexLayout::AddAttribute(location, components, static_cast<uint32_t>(type), normalized);
    }

    /**
     * @brief Add a position attribute (3 floats)
     */
    void AddPositionAttribute() {
        AddAttribute(VertexAttributeType::Float, 3, false);
    }

    /**
     * @brief Add a normal attribute (3 floats)
     */
    void AddNormalAttribute() {
        AddAttribute(VertexAttributeType::Float, 3, false);
    }

    /**
     * @brief Add a texture coordinate attribute (2 floats)
     */
    void AddTexCoordAttribute() {
        AddAttribute(VertexAttributeType::Float, 2, false);
    }

    /**
     * @brief Add a color attribute (4 floats)
     */
    void AddColorAttribute() {
        AddAttribute(VertexAttributeType::Float, 4, false);
    }

    /**
     * @brief Add a tangent attribute (3 floats)
     */
    void AddTangentAttribute() {
        AddAttribute(VertexAttributeType::Float, 3, false);
    }

    /**
     * @brief Add bone IDs attribute (4 ints)
     */
    void AddBoneIDsAttribute() {
        AddAttribute(VertexAttributeType::Int, 4, false);
    }

    /**
     * @brief Add bone weights attribute (4 floats)
     */
    void AddBoneWeightsAttribute() {
        AddAttribute(VertexAttributeType::Float, 4, false);
    }
};

} // namespace Lupine
