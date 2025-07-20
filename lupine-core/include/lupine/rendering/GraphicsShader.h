#pragma once

#include "GraphicsDevice.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Lupine {

/**
 * @brief Shader uniform types
 */
enum class UniformType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Int,
    IVec2,
    IVec3,
    IVec4,
    Bool,
    Mat2,
    Mat3,
    Mat4,
    Sampler2D,
    SamplerCube,
    Sampler3D
};

/**
 * @brief Shader uniform information
 */
struct UniformInfo {
    std::string name;
    UniformType type;
    int location = -1;
    int size = 1;           // Array size (1 for non-arrays)
    int binding = -1;       // Texture binding point for samplers
    
    UniformInfo() = default;
    UniformInfo(const std::string& n, UniformType t, int loc, int s = 1, int b = -1)
        : name(n), type(t), location(loc), size(s), binding(b) {}
};

/**
 * @brief Shader attribute information
 */
struct AttributeInfo {
    std::string name;
    int location = -1;
    UniformType type;
    
    AttributeInfo() = default;
    AttributeInfo(const std::string& n, int loc, UniformType t)
        : name(n), location(loc), type(t) {}
};

/**
 * @brief Abstract graphics shader interface
 * 
 * Represents a shader program that can be executed on the GPU.
 * This abstraction allows for different graphics backends while
 * maintaining a consistent interface for shader management.
 */
class GraphicsShader {
public:
    virtual ~GraphicsShader() = default;

    /**
     * @brief Use/bind this shader program
     */
    virtual void Use() = 0;

    /**
     * @brief Bind this shader program (alias for Use)
     */
    virtual void Bind() { Use(); }

    // === Generic Uniform Setters ===

    /**
     * @brief Generic uniform setter for different types
     */
    virtual void SetUniform(const std::string& name, int value) { SetInt(name, value); }
    virtual void SetUniform(const std::string& name, float value) { SetFloat(name, value); }
    virtual void SetUniform(const std::string& name, bool value) { SetBool(name, value); }
    virtual void SetUniform(const std::string& name, const glm::vec2& value) { SetVec2(name, value); }
    virtual void SetUniform(const std::string& name, const glm::vec3& value) { SetVec3(name, value); }
    virtual void SetUniform(const std::string& name, const glm::vec4& value) { SetVec4(name, value); }
    virtual void SetUniform(const std::string& name, const glm::mat4& value) { SetMat4(name, value); }

    /**
     * @brief Check if the shader is valid and compiled successfully
     * @return True if shader is valid
     */
    virtual bool IsValid() const = 0;

    /**
     * @brief Get the shader compilation log
     * @return Compilation log string
     */
    virtual std::string GetCompileLog() const = 0;

    // === Uniform Setters ===

    /**
     * @brief Set a float uniform
     * @param name Uniform name
     * @param value Float value
     */
    virtual void SetFloat(const std::string& name, float value) = 0;

    /**
     * @brief Set a vec2 uniform
     * @param name Uniform name
     * @param value Vec2 value
     */
    virtual void SetVec2(const std::string& name, const glm::vec2& value) = 0;

    /**
     * @brief Set a vec3 uniform
     * @param name Uniform name
     * @param value Vec3 value
     */
    virtual void SetVec3(const std::string& name, const glm::vec3& value) = 0;

    /**
     * @brief Set a vec4 uniform
     * @param name Uniform name
     * @param value Vec4 value
     */
    virtual void SetVec4(const std::string& name, const glm::vec4& value) = 0;

    /**
     * @brief Set an integer uniform
     * @param name Uniform name
     * @param value Integer value
     */
    virtual void SetInt(const std::string& name, int value) = 0;

    /**
     * @brief Set an ivec2 uniform
     * @param name Uniform name
     * @param value IVec2 value
     */
    virtual void SetIVec2(const std::string& name, const glm::ivec2& value) = 0;

    /**
     * @brief Set an ivec3 uniform
     * @param name Uniform name
     * @param value IVec3 value
     */
    virtual void SetIVec3(const std::string& name, const glm::ivec3& value) = 0;

    /**
     * @brief Set an ivec4 uniform
     * @param name Uniform name
     * @param value IVec4 value
     */
    virtual void SetIVec4(const std::string& name, const glm::ivec4& value) = 0;

    /**
     * @brief Set a boolean uniform
     * @param name Uniform name
     * @param value Boolean value
     */
    virtual void SetBool(const std::string& name, bool value) = 0;

    /**
     * @brief Set a mat2 uniform
     * @param name Uniform name
     * @param value Mat2 value
     */
    virtual void SetMat2(const std::string& name, const glm::mat2& value) = 0;

    /**
     * @brief Set a mat3 uniform
     * @param name Uniform name
     * @param value Mat3 value
     */
    virtual void SetMat3(const std::string& name, const glm::mat3& value) = 0;

    /**
     * @brief Set a mat4 uniform
     * @param name Uniform name
     * @param value Mat4 value
     */
    virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

    // === Array Uniform Setters ===

    /**
     * @brief Set a float array uniform
     * @param name Uniform name
     * @param values Array of float values
     * @param count Number of elements
     */
    virtual void SetFloatArray(const std::string& name, const float* values, int count) = 0;

    /**
     * @brief Set a vec3 array uniform
     * @param name Uniform name
     * @param values Array of vec3 values
     * @param count Number of elements
     */
    virtual void SetVec3Array(const std::string& name, const glm::vec3* values, int count) = 0;

    /**
     * @brief Set a mat4 array uniform
     * @param name Uniform name
     * @param values Array of mat4 values
     * @param count Number of elements
     */
    virtual void SetMat4Array(const std::string& name, const glm::mat4* values, int count) = 0;

    // === Texture Binding ===

    /**
     * @brief Bind a texture to a sampler uniform
     * @param name Sampler uniform name
     * @param texture_id Native texture handle
     * @param unit Texture unit to bind to
     */
    virtual void SetTexture(const std::string& name, uint32_t texture_id, int unit) = 0;

    // === Reflection ===

    /**
     * @brief Get all uniform information
     * @return Vector of uniform info structures
     */
    virtual std::vector<UniformInfo> GetUniforms() const = 0;

    /**
     * @brief Get all attribute information
     * @return Vector of attribute info structures
     */
    virtual std::vector<AttributeInfo> GetAttributes() const = 0;

    /**
     * @brief Get uniform location by name
     * @param name Uniform name
     * @return Uniform location, -1 if not found
     */
    virtual int GetUniformLocation(const std::string& name) const = 0;

    /**
     * @brief Get attribute location by name
     * @param name Attribute name
     * @return Attribute location, -1 if not found
     */
    virtual int GetAttributeLocation(const std::string& name) const = 0;

    /**
     * @brief Check if uniform exists
     * @param name Uniform name
     * @return True if uniform exists
     */
    virtual bool HasUniform(const std::string& name) const = 0;

    /**
     * @brief Check if attribute exists
     * @param name Attribute name
     * @return True if attribute exists
     */
    virtual bool HasAttribute(const std::string& name) const = 0;

    // === Utility ===

    /**
     * @brief Get the native shader program handle (backend-specific)
     * @return Native shader handle (e.g., OpenGL program ID)
     */
    virtual uint32_t GetNativeHandle() const = 0;

    /**
     * @brief Get shader source code (if available)
     * @param type Shader type
     * @return Shader source code
     */
    virtual std::string GetSource(ShaderType type) const = 0;

protected:
    // Protected constructor to prevent direct instantiation
    GraphicsShader() = default;
};

} // namespace Lupine
