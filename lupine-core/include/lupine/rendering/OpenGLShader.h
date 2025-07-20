#pragma once

#include "GraphicsShader.h"
#include <glad/glad.h>
#include <unordered_map>

namespace Lupine {

/**
 * @brief OpenGL implementation of GraphicsShader
 * 
 * Provides OpenGL-specific shader functionality while implementing
 * the abstract GraphicsShader interface for cross-platform compatibility.
 */
class OpenGLShader : public GraphicsShader {
public:
    /**
     * @brief Constructor
     * @param vertex_source Vertex shader source code
     * @param fragment_source Fragment shader source code
     * @param geometry_source Geometry shader source code (optional)
     */
    OpenGLShader(
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    );

    /**
     * @brief Destructor - cleans up OpenGL shader program
     */
    virtual ~OpenGLShader();

    // === GraphicsShader Interface ===

    void Use() override;
    void Bind() override;
    void Unbind(); // OpenGL-specific method

    bool IsValid() const override { return m_program_id != 0 && m_is_linked; }
    std::string GetCompileLog() const override;

    // Uniform setters
    void SetBool(const std::string& name, bool value) override;
    void SetInt(const std::string& name, int value) override;
    void SetFloat(const std::string& name, float value) override;
    void SetVec2(const std::string& name, const glm::vec2& value) override;
    void SetVec3(const std::string& name, const glm::vec3& value) override;
    void SetVec4(const std::string& name, const glm::vec4& value) override;
    void SetIVec2(const std::string& name, const glm::ivec2& value) override;
    void SetIVec3(const std::string& name, const glm::ivec3& value) override;
    void SetIVec4(const std::string& name, const glm::ivec4& value) override;
    void SetMat2(const std::string& name, const glm::mat2& value) override;
    void SetMat3(const std::string& name, const glm::mat3& value) override;
    void SetMat4(const std::string& name, const glm::mat4& value) override;

    // Array uniform setters (OpenGL-specific)
    void SetIntArray(const std::string& name, const int* values, int count);
    void SetFloatArray(const std::string& name, const float* values, int count) override;
    void SetVec2Array(const std::string& name, const glm::vec2* values, int count);
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

    // === OpenGL-specific methods ===

    /**
     * @brief Get the OpenGL program ID
     * @return OpenGL program ID
     */
    GLuint GetProgramID() const { return m_program_id; }

    /**
     * @brief Get uniform location (cached, non-const for internal use)
     * @param name Uniform name
     * @return Uniform location or -1 if not found
     */
    GLint GetUniformLocationCached(const std::string& name);

    /**
     * @brief Check if shader compilation was successful
     * @return True if all shaders compiled and linked successfully
     */
    bool IsLinked() const { return m_is_linked; }

private:
    GLuint m_program_id;
    GLuint m_vertex_shader;
    GLuint m_fragment_shader;
    GLuint m_geometry_shader;
    bool m_is_linked;
    
    // Cached uniform locations for performance
    std::unordered_map<std::string, GLint> m_uniform_cache;

    /**
     * @brief Compile a shader from source
     * @param source Shader source code
     * @param type Shader type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
     * @return Shader ID or 0 if compilation failed
     */
    GLuint CompileShader(const std::string& source, GLenum type);

    /**
     * @brief Link the shader program
     * @return True if linking was successful
     */
    bool LinkProgram();

    /**
     * @brief Check for shader compilation errors
     * @param shader Shader ID
     * @param type Shader type name for error reporting
     * @return True if no errors
     */
    bool CheckShaderErrors(GLuint shader, const std::string& type);

    /**
     * @brief Check for program linking errors
     * @return True if no errors
     */
    bool CheckProgramErrors();
};

} // namespace Lupine
