#pragma once

#include "GraphicsDevice.h"
#include "GraphicsBuffer.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace Lupine {

/**
 * @brief Abstract graphics vertex array interface
 * 
 * Represents a vertex array object (VAO) that encapsulates vertex
 * buffer bindings and attribute configurations. This abstraction
 * allows for different graphics backends while maintaining a
 * consistent interface for vertex data management.
 */
class GraphicsVertexArray {
public:
    virtual ~GraphicsVertexArray() = default;

    /**
     * @brief Bind the vertex array for use
     */
    virtual void Bind() = 0;

    /**
     * @brief Unbind the vertex array
     */
    virtual void Unbind() = 0;

    /**
     * @brief Set the vertex buffer
     * @param buffer Vertex buffer to bind
     * @param layout Vertex layout description
     */
    virtual void SetVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout
    ) = 0;

    /**
     * @brief Add a vertex buffer (for multiple vertex streams)
     * @param buffer Vertex buffer to add
     * @param layout Vertex layout description
     * @param binding_index Buffer binding index
     */
    virtual void AddVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout,
        int binding_index = 0
    ) = 0;

    /**
     * @brief Set the index buffer
     * @param buffer Index buffer to bind
     */
    virtual void SetIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) = 0;

    /**
     * @brief Get the vertex buffer at the specified binding
     * @param binding_index Buffer binding index
     * @return Shared pointer to vertex buffer, nullptr if not set
     */
    virtual std::shared_ptr<GraphicsBuffer> GetVertexBuffer(int binding_index = 0) const = 0;

    /**
     * @brief Get the index buffer
     * @return Shared pointer to index buffer, nullptr if not set
     */
    virtual std::shared_ptr<GraphicsBuffer> GetIndexBuffer() const = 0;

    /**
     * @brief Get the number of vertex buffers bound
     * @return Number of vertex buffers
     */
    virtual int GetVertexBufferCount() const = 0;

    /**
     * @brief Check if an index buffer is bound
     * @return True if index buffer is bound
     */
    virtual bool HasIndexBuffer() const = 0;

    /**
     * @brief Get the vertex layout for a specific binding
     * @param binding_index Buffer binding index
     * @return Vertex layout, empty if binding not found
     */
    virtual VertexLayout GetVertexLayout(int binding_index = 0) const = 0;

    /**
     * @brief Enable a vertex attribute
     * @param location Attribute location
     */
    virtual void EnableAttribute(int location) = 0;

    /**
     * @brief Disable a vertex attribute
     * @param location Attribute location
     */
    virtual void DisableAttribute(int location) = 0;

    /**
     * @brief Set vertex attribute pointer manually
     * @param location Attribute location
     * @param components Number of components (1-4)
     * @param type Data type (backend-specific)
     * @param normalized Whether to normalize integer data
     * @param stride Stride between vertices
     * @param offset Offset within vertex structure
     */
    virtual void SetAttributePointer(
        int location,
        int components,
        uint32_t type,
        bool normalized,
        int stride,
        int offset
    ) = 0;

    /**
     * @brief Set vertex attribute divisor for instanced rendering
     * @param location Attribute location
     * @param divisor Divisor value (0 = per vertex, 1 = per instance, etc.)
     */
    virtual void SetAttributeDivisor(int location, int divisor) = 0;

    /**
     * @brief Get the native vertex array handle (backend-specific)
     * @return Native VAO handle (e.g., OpenGL VAO ID)
     */
    virtual uint32_t GetNativeHandle() const = 0;

    /**
     * @brief Check if the vertex array is valid
     * @return True if vertex array is valid and can be used
     */
    virtual bool IsValid() const = 0;

    // === Utility Methods ===

    /**
     * @brief Calculate total vertex count from all vertex buffers
     * @return Total number of vertices
     */
    virtual int GetVertexCount() const = 0;

    /**
     * @brief Calculate index count from index buffer
     * @return Number of indices, 0 if no index buffer
     */
    virtual int GetIndexCount() const = 0;

    /**
     * @brief Get memory usage of all bound buffers
     * @return Total memory usage in bytes
     */
    virtual size_t GetMemoryUsage() const = 0;

    /**
     * @brief Validate the vertex array configuration
     * @return True if configuration is valid for rendering
     */
    virtual bool Validate() const = 0;

    /**
     * @brief Get debug information about the vertex array
     * @return Debug info string
     */
    virtual std::string GetDebugInfo() const = 0;

protected:
    // Protected constructor to prevent direct instantiation
    GraphicsVertexArray() = default;
};

/**
 * @brief Vertex array builder helper class
 * 
 * Provides a fluent interface for building vertex arrays with
 * proper error checking and validation.
 */
class VertexArrayBuilder {
public:
    VertexArrayBuilder(std::shared_ptr<GraphicsDevice> device)
        : m_device(device) {
        m_vertex_array = device->CreateVertexArray();
    }

    /**
     * @brief Add a vertex buffer with layout
     * @param buffer Vertex buffer
     * @param layout Vertex layout
     * @return Reference to this builder for chaining
     */
    VertexArrayBuilder& WithVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout
    ) {
        if (m_vertex_array && buffer) {
            m_vertex_array->AddVertexBuffer(buffer, layout, m_next_binding++);
        }
        return *this;
    }

    /**
     * @brief Set the index buffer
     * @param buffer Index buffer
     * @return Reference to this builder for chaining
     */
    VertexArrayBuilder& WithIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) {
        if (m_vertex_array && buffer) {
            m_vertex_array->SetIndexBuffer(buffer);
        }
        return *this;
    }

    /**
     * @brief Build and return the vertex array
     * @return Shared pointer to completed vertex array
     */
    std::shared_ptr<GraphicsVertexArray> Build() {
        if (m_vertex_array && m_vertex_array->Validate()) {
            return m_vertex_array;
        }
        return nullptr;
    }

private:
    std::shared_ptr<GraphicsDevice> m_device;
    std::shared_ptr<GraphicsVertexArray> m_vertex_array;
    int m_next_binding = 0;
};

} // namespace Lupine
