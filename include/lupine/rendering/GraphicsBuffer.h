#pragma once

#include "GraphicsDevice.h"
#include <cstdint>
#include <memory>

namespace Lupine {

/**
 * @brief Abstract graphics buffer interface
 * 
 * Represents a buffer object that can store vertex data, index data,
 * or uniform data on the GPU. This abstraction allows for different
 * graphics backends while maintaining a consistent interface.
 */
class GraphicsBuffer {
public:
    virtual ~GraphicsBuffer() = default;

    /**
     * @brief Get the buffer type
     * @return Buffer type enumeration
     */
    virtual BufferType GetType() const = 0;

    /**
     * @brief Get the buffer usage pattern
     * @return Buffer usage enumeration
     */
    virtual BufferUsage GetUsage() const = 0;

    /**
     * @brief Get the buffer size in bytes
     * @return Buffer size
     */
    virtual size_t GetSize() const = 0;

    /**
     * @brief Bind the buffer for use
     */
    virtual void Bind() = 0;

    /**
     * @brief Unbind the buffer
     */
    virtual void Unbind() = 0;

    /**
     * @brief Update buffer data
     * @param offset Offset in bytes from start of buffer
     * @param size Size of data to update in bytes
     * @param data Pointer to new data
     */
    virtual void UpdateData(size_t offset, size_t size, const void* data) = 0;

    /**
     * @brief Update entire buffer data
     * @param data Pointer to new data
     * @param size Size of data in bytes (must match buffer size)
     */
    virtual void UpdateData(const void* data, size_t size) = 0;

    /**
     * @brief Map buffer memory for direct access
     * @param access Access mode (read, write, or read-write)
     * @return Pointer to mapped memory, nullptr if mapping failed
     */
    enum class MapAccess {
        ReadOnly,
        WriteOnly,
        ReadWrite
    };
    virtual void* Map(MapAccess access) = 0;

    /**
     * @brief Unmap buffer memory
     */
    virtual void Unmap() = 0;

    /**
     * @brief Get the native buffer handle (backend-specific)
     * @return Native buffer handle (e.g., OpenGL buffer ID)
     */
    virtual uint32_t GetNativeHandle() const = 0;

    /**
     * @brief Check if the buffer is valid
     * @return True if buffer is valid and can be used
     */
    virtual bool IsValid() const = 0;

protected:
    // Protected constructor to prevent direct instantiation
    GraphicsBuffer() = default;
};

/**
 * @brief Vertex attribute description
 */
struct VertexAttribute {
    uint32_t location = 0;          // Attribute location/index
    uint32_t components = 0;        // Number of components (1-4)
    uint32_t type = 0;              // Data type (backend-specific)
    bool normalized = false;        // Whether to normalize integer data
    uint32_t stride = 0;            // Stride between vertices
    uint32_t offset = 0;            // Offset within vertex structure
    
    VertexAttribute() = default;
    
    VertexAttribute(uint32_t loc, uint32_t comp, uint32_t data_type, 
                   bool norm = false, uint32_t str = 0, uint32_t off = 0)
        : location(loc), components(comp), type(data_type), 
          normalized(norm), stride(str), offset(off) {}
};

/**
 * @brief Vertex buffer layout description
 */
class VertexLayout {
public:
    VertexLayout() = default;

    /**
     * @brief Add a vertex attribute to the layout
     * @param attribute Vertex attribute description
     */
    void AddAttribute(const VertexAttribute& attribute) {
        m_attributes.push_back(attribute);
        m_stride = CalculateStride();
    }

    /**
     * @brief Add a vertex attribute with automatic stride calculation
     * @param location Attribute location
     * @param components Number of components
     * @param type Data type
     * @param normalized Whether to normalize
     */
    void AddAttribute(uint32_t location, uint32_t components, uint32_t type, bool normalized = false) {
        uint32_t offset = m_stride;
        m_attributes.emplace_back(location, components, type, normalized, 0, offset);
        m_stride = CalculateStride();
        
        // Update stride for all attributes
        for (auto& attr : m_attributes) {
            attr.stride = m_stride;
        }
    }

    /**
     * @brief Get all vertex attributes
     * @return Vector of vertex attributes
     */
    const std::vector<VertexAttribute>& GetAttributes() const {
        return m_attributes;
    }

    /**
     * @brief Get the vertex stride
     * @return Stride in bytes
     */
    uint32_t GetStride() const {
        return m_stride;
    }

    /**
     * @brief Clear all attributes
     */
    void Clear() {
        m_attributes.clear();
        m_stride = 0;
    }

    /**
     * @brief Check if layout is empty
     * @return True if no attributes defined
     */
    bool IsEmpty() const {
        return m_attributes.empty();
    }

private:
    std::vector<VertexAttribute> m_attributes;
    uint32_t m_stride = 0;

    uint32_t CalculateStride() const {
        uint32_t stride = 0;
        for (const auto& attr : m_attributes) {
            stride += GetTypeSize(attr.type) * attr.components;
        }
        return stride;
    }

    uint32_t GetTypeSize(uint32_t type) const {
        // This will be implemented by backend-specific classes
        // For now, assume float (4 bytes) as default
        (void)type; // Suppress unused parameter warning
        return 4;
    }
};

} // namespace Lupine
