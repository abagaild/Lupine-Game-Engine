#include "lupine/rendering/OpenGLShader.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Lupine {

OpenGLShader::OpenGLShader(
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source)
    : m_program_id(0)
    , m_vertex_shader(0)
    , m_fragment_shader(0)
    , m_geometry_shader(0)
    , m_is_linked(false) {
    
    // Compile vertex shader
    m_vertex_shader = CompileShader(vertex_source, GL_VERTEX_SHADER);
    if (m_vertex_shader == 0) {
        std::cerr << "Failed to compile vertex shader" << std::endl;
        return;
    }
    
    // Compile fragment shader
    m_fragment_shader = CompileShader(fragment_source, GL_FRAGMENT_SHADER);
    if (m_fragment_shader == 0) {
        std::cerr << "Failed to compile fragment shader" << std::endl;
        return;
    }
    
    // Compile geometry shader if provided
    if (!geometry_source.empty()) {
        m_geometry_shader = CompileShader(geometry_source, GL_GEOMETRY_SHADER);
        if (m_geometry_shader == 0) {
            std::cerr << "Failed to compile geometry shader" << std::endl;
            return;
        }
    }
    
    // Create and link program
    m_program_id = glCreateProgram();
    if (m_program_id == 0) {
        std::cerr << "Failed to create shader program" << std::endl;
        return;
    }
    
    glAttachShader(m_program_id, m_vertex_shader);
    glAttachShader(m_program_id, m_fragment_shader);
    if (m_geometry_shader != 0) {
        glAttachShader(m_program_id, m_geometry_shader);
    }
    
    m_is_linked = LinkProgram();
}

OpenGLShader::~OpenGLShader() {
    if (m_program_id != 0) {
        glDeleteProgram(m_program_id);
        m_program_id = 0;
    }
    
    if (m_vertex_shader != 0) {
        glDeleteShader(m_vertex_shader);
        m_vertex_shader = 0;
    }
    
    if (m_fragment_shader != 0) {
        glDeleteShader(m_fragment_shader);
        m_fragment_shader = 0;
    }
    
    if (m_geometry_shader != 0) {
        glDeleteShader(m_geometry_shader);
        m_geometry_shader = 0;
    }
}

void OpenGLShader::Use() {
    if (IsValid()) {
        glUseProgram(m_program_id);
    }
}

void OpenGLShader::Bind() {
    Use(); // Bind is an alias for Use
}

void OpenGLShader::Unbind() {
    glUseProgram(0);
}

void OpenGLShader::SetBool(const std::string& name, bool value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1i(location, value ? 1 : 0);
    }
}

void OpenGLShader::SetInt(const std::string& name, int value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void OpenGLShader::SetFloat(const std::string& name, float value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void OpenGLShader::SetVec2(const std::string& name, const glm::vec2& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetVec3(const std::string& name, const glm::vec3& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetVec4(const std::string& name, const glm::vec4& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void OpenGLShader::SetIntArray(const std::string& name, const int* values, int count) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1iv(location, count, values);
    }
}

void OpenGLShader::SetFloatArray(const std::string& name, const float* values, int count) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1fv(location, count, values);
    }
}

void OpenGLShader::SetVec2Array(const std::string& name, const glm::vec2* values, int count) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform2fv(location, count, glm::value_ptr(values[0]));
    }
}

void OpenGLShader::SetVec3Array(const std::string& name, const glm::vec3* values, int count) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform3fv(location, count, glm::value_ptr(values[0]));
    }
}

void OpenGLShader::SetMat4Array(const std::string& name, const glm::mat4* values, int count) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values[0]));
    }
}

void OpenGLShader::SetTexture(const std::string& name, uint32_t texture_id, int unit) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(location, unit);
    }
}

GLint OpenGLShader::GetUniformLocationCached(const std::string& name) {
    auto it = m_uniform_cache.find(name);
    if (it != m_uniform_cache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    m_uniform_cache[name] = location;

    return location;
}

GLuint OpenGLShader::CompileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);
    
    std::string type_name;
    switch (type) {
        case GL_VERTEX_SHADER:   type_name = "vertex"; break;
        case GL_FRAGMENT_SHADER: type_name = "fragment"; break;
        case GL_GEOMETRY_SHADER: type_name = "geometry"; break;
        default: type_name = "unknown"; break;
    }
    
    if (!CheckShaderErrors(shader, type_name)) {
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool OpenGLShader::LinkProgram() {
    glLinkProgram(m_program_id);
    return CheckProgramErrors();
}

bool OpenGLShader::CheckShaderErrors(GLuint shader, const std::string& type) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLchar info_log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info_log);
        std::cerr << "Shader compilation error (" << type << "): " << info_log << std::endl;
        return false;
    }
    
    return true;
}

bool OpenGLShader::CheckProgramErrors() {
    GLint success;
    glGetProgramiv(m_program_id, GL_LINK_STATUS, &success);
    
    if (!success) {
        GLchar info_log[1024];
        glGetProgramInfoLog(m_program_id, 1024, nullptr, info_log);
        std::cerr << "Shader program linking error: " << info_log << std::endl;
        return false;
    }
    
    return true;
}

std::string OpenGLShader::GetCompileLog() const {
    if (!IsValid()) {
        return "Shader program is not valid";
    }

    GLint log_length = 0;
    glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &log_length);

    if (log_length <= 1) {
        return ""; // No log or just null terminator
    }

    std::string log(log_length, '\0');
    glGetProgramInfoLog(m_program_id, log_length, nullptr, &log[0]);

    // Remove null terminator if present
    if (!log.empty() && log.back() == '\0') {
        log.pop_back();
    }

    return log;
}

void OpenGLShader::SetIVec2(const std::string& name, const glm::ivec2& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform2iv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetIVec3(const std::string& name, const glm::ivec3& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform3iv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetIVec4(const std::string& name, const glm::ivec4& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform4iv(location, 1, glm::value_ptr(value));
    }
}

void OpenGLShader::SetMat2(const std::string& name, const glm::mat2& value) {
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

std::vector<UniformInfo> OpenGLShader::GetUniforms() const {
    std::vector<UniformInfo> uniforms;

    if (!IsValid()) {
        return uniforms;
    }

    GLint uniform_count = 0;
    glGetProgramiv(m_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);

    for (GLint i = 0; i < uniform_count; ++i) {
        GLchar name[256];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveUniform(m_program_id, i, sizeof(name), &length, &size, &type, name);

        UniformInfo info;
        info.name = std::string(name, length);
        info.location = glGetUniformLocation(m_program_id, name);
        info.type = UniformType::Float; // Default, would need proper mapping
        info.size = size;

        uniforms.push_back(info);
    }

    return uniforms;
}

std::vector<AttributeInfo> OpenGLShader::GetAttributes() const {
    std::vector<AttributeInfo> attributes;

    if (!IsValid()) {
        return attributes;
    }

    GLint attribute_count = 0;
    glGetProgramiv(m_program_id, GL_ACTIVE_ATTRIBUTES, &attribute_count);

    for (GLint i = 0; i < attribute_count; ++i) {
        GLchar name[256];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveAttrib(m_program_id, i, sizeof(name), &length, &size, &type, name);

        AttributeInfo info;
        info.name = std::string(name, length);
        info.location = glGetAttribLocation(m_program_id, name);
        info.type = UniformType::Float; // Default, would need proper mapping

        attributes.push_back(info);
    }

    return attributes;
}

int OpenGLShader::GetUniformLocation(const std::string& name) const {
    if (!IsValid()) {
        return -1;
    }
    return glGetUniformLocation(m_program_id, name.c_str());
}

int OpenGLShader::GetAttributeLocation(const std::string& name) const {
    if (!IsValid()) {
        return -1;
    }
    return glGetAttribLocation(m_program_id, name.c_str());
}

bool OpenGLShader::HasUniform(const std::string& name) const {
    return GetUniformLocation(name) != -1;
}

bool OpenGLShader::HasAttribute(const std::string& name) const {
    return GetAttributeLocation(name) != -1;
}

std::string OpenGLShader::GetSource(ShaderType type) const {
    // OpenGL doesn't provide a way to retrieve original source code
    // This would need to be stored during compilation if needed
    return "Source code not available (not stored during compilation)";
}

} // namespace Lupine
