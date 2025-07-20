#pragma once

#include "GraphicsShader.h"
#include <unordered_map>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
// Forward declare WebGL types for non-Emscripten builds
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;
#endif

namespace Lupine {

/**
 * @brief WebGL implementation of GraphicsShader
 * 
 * Provides WebGL-specific shader functionality while implementing
 * the abstract GraphicsShader interface for cross-platform compatibility.
 * Handles GLSL ES conversion and WebGL-specific shader limitations.
 */
class WebGLShader : public GraphicsShader {
public:
    /**
     * @brief Constructor
     * @param vertex_source Vertex shader source code
     * @param fragment_source Fragment shader source code
     * @param geometry_source Geometry shader source code (ignored in WebGL)
     */
    WebGLShader(
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    );

    /**
     * @brief Destructor - cleans up WebGL shader program
     */
    virtual ~WebGLShader();

    // === GraphicsShader Interface ===

    void Use() override;
    void Bind() override;

    bool IsValid() const override { return m_program_id != 0 && m_is_linked; }
    std::string GetCompileLog() const override;

    // Uniform setters
    void SetFloat(const std::string& name, float value) override;
    void SetVec2(const std::string& name, const glm::vec2& value) override;
    void SetVec3(const std::string& name, const glm::vec3& value) override;
    void SetVec4(const std::string& name, const glm::vec4& value) override;
    void SetInt(const std::string& name, int value) override;
    void SetIVec2(const std::string& name, const glm::ivec2& value) override;
    void SetIVec3(const std::string& name, const glm::ivec3& value) override;
    void SetIVec4(const std::string& name, const glm::ivec4& value) override;
    void SetBool(const std::string& name, bool value) override;
    void SetMat2(const std::string& name, const glm::mat2& value) override;
    void SetMat3(const std::string& name, const glm::mat3& value) override;
    void SetMat4(const std::string& name, const glm::mat4& value) override;

    // Array uniform setters
    void SetFloatArray(const std::string& name, const float* values, int count) override;
    void SetVec3Array(const std::string& name, const glm::vec3* values, int count) override;
    void SetMat4Array(const std::string& name, const glm::mat4* values, int count) override;

    // Texture binding
    void SetTexture(const std::string& name, uint32_t texture_id, int unit) override;

    // Reflection methods
    std::vector<UniformInfo> GetUniforms() const override;
    std::vector<AttributeInfo> GetAttributes() const override;
    int GetUniformLocation(const std::string& name) const override;
    int GetAttributeLocation(const std::string& name) const override;
    bool HasUniform(const std::string& name) const override;
    bool HasAttribute(const std::string& name) const override;
    std::string GetSource(ShaderType type) const override;

    uint32_t GetNativeHandle() const override { return m_program_id; }

    // === WebGL-specific methods ===

    /**
     * @brief Get the WebGL program ID
     * @return WebGL program ID
     */
    GLuint GetProgramID() const { return m_program_id; }

    /**
     * @brief Unbind the shader program
     */
    void Unbind();

    /**
     * @brief Get cached uniform location
     * @param name Uniform name
     * @return Uniform location, -1 if not found
     */
    GLint GetUniformLocationCached(const std::string& name);

private:
    // Initialization
    bool Initialize();
    GLuint CompileShader(const std::string& source, GLenum type);
    bool LinkProgram();
    
    // Error checking
    bool CheckShaderErrors(GLuint shader, const std::string& type);
    bool CheckProgramErrors();

    // GLSL ES conversion
    std::string ConvertToGLSLES(const std::string& source, GLenum type) const;

    // WebGL state
    GLuint m_program_id;
    GLuint m_vertex_shader;
    GLuint m_fragment_shader;
    bool m_is_linked;

    // Source code storage
    std::string m_vertex_source;
    std::string m_fragment_source;
    std::string m_geometry_source; // Stored but not used in WebGL

    // Uniform location cache
    mutable std::unordered_map<std::string, GLint> m_uniform_cache;

    // Reflection data cache
    mutable std::vector<UniformInfo> m_uniforms_cache;
    mutable std::vector<AttributeInfo> m_attributes_cache;
    mutable bool m_reflection_cached;

    // Helper methods
    void CacheReflectionData() const;
    UniformType GLTypeToUniformType(GLenum gl_type) const;
};

} // namespace Lupine
