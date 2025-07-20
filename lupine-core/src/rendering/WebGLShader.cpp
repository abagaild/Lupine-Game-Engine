#include "lupine/rendering/WebGLShader.h"
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

namespace Lupine {

WebGLShader::WebGLShader(
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source)
    : m_program_id(0)
    , m_vertex_shader(0)
    , m_fragment_shader(0)
    , m_is_linked(false)
    , m_vertex_source(vertex_source)
    , m_fragment_source(fragment_source)
    , m_geometry_source(geometry_source)
    , m_reflection_cached(false) {
    
    if (!geometry_source.empty()) {
        std::cout << "Warning: Geometry shaders are not supported in WebGL, ignoring geometry shader source" << std::endl;
    }
    
    if (!Initialize()) {
        std::cerr << "Failed to initialize WebGL shader" << std::endl;
    }
}

WebGLShader::~WebGLShader() {
#ifdef __EMSCRIPTEN__
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
#endif
}

void WebGLShader::Use() {
#ifdef __EMSCRIPTEN__
    if (IsValid()) {
        glUseProgram(m_program_id);
    }
#endif
}

void WebGLShader::Bind() {
    Use(); // Bind is an alias for Use
}

void WebGLShader::Unbind() {
#ifdef __EMSCRIPTEN__
    glUseProgram(0);
#endif
}

std::string WebGLShader::GetCompileLog() const {
#ifdef __EMSCRIPTEN__
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
#else
    return "";
#endif
}

void WebGLShader::SetFloat(const std::string& name, float value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
#endif
}

void WebGLShader::SetVec2(const std::string& name, const glm::vec2& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetVec3(const std::string& name, const glm::vec3& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetVec4(const std::string& name, const glm::vec4& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetInt(const std::string& name, int value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
#endif
}

void WebGLShader::SetIVec2(const std::string& name, const glm::ivec2& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform2iv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetIVec3(const std::string& name, const glm::ivec3& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform3iv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetIVec4(const std::string& name, const glm::ivec4& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform4iv(location, 1, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetBool(const std::string& name, bool value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniform1i(location, value ? 1 : 0);
    }
#endif
}

void WebGLShader::SetMat2(const std::string& name, const glm::mat2& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetMat3(const std::string& name, const glm::mat3& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetMat4(const std::string& name, const glm::mat4& value) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
#endif
}

void WebGLShader::SetFloatArray(const std::string& name, const float* values, int count) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1 && values && count > 0) {
        glUniform1fv(location, count, values);
    }
#endif
}

void WebGLShader::SetVec3Array(const std::string& name, const glm::vec3* values, int count) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1 && values && count > 0) {
        glUniform3fv(location, count, glm::value_ptr(values[0]));
    }
#endif
}

void WebGLShader::SetMat4Array(const std::string& name, const glm::mat4* values, int count) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1 && values && count > 0) {
        glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values[0]));
    }
#endif
}

void WebGLShader::SetTexture(const std::string& name, uint32_t texture_id, int unit) {
#ifdef __EMSCRIPTEN__
    GLint location = GetUniformLocationCached(name);
    if (location != -1) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(location, unit);
    }
#endif
}

GLint WebGLShader::GetUniformLocationCached(const std::string& name) {
#ifdef __EMSCRIPTEN__
    auto it = m_uniform_cache.find(name);
    if (it != m_uniform_cache.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    m_uniform_cache[name] = location;
    return location;
#else
    return -1;
#endif
}

std::vector<UniformInfo> WebGLShader::GetUniforms() const {
    if (!m_reflection_cached) {
        CacheReflectionData();
    }
    return m_uniforms_cache;
}

std::vector<AttributeInfo> WebGLShader::GetAttributes() const {
    if (!m_reflection_cached) {
        CacheReflectionData();
    }
    return m_attributes_cache;
}

int WebGLShader::GetUniformLocation(const std::string& name) const {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        return -1;
    }
    return glGetUniformLocation(m_program_id, name.c_str());
#else
    return -1;
#endif
}

int WebGLShader::GetAttributeLocation(const std::string& name) const {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        return -1;
    }
    return glGetAttribLocation(m_program_id, name.c_str());
#else
    return -1;
#endif
}

bool WebGLShader::HasUniform(const std::string& name) const {
    return GetUniformLocation(name) != -1;
}

bool WebGLShader::HasAttribute(const std::string& name) const {
    return GetAttributeLocation(name) != -1;
}

std::string WebGLShader::GetSource(ShaderType type) const {
    switch (type) {
        case ShaderType::Vertex:   return m_vertex_source;
        case ShaderType::Fragment: return m_fragment_source;
        case ShaderType::Geometry: return m_geometry_source;
        default: return "";
    }
}

bool WebGLShader::Initialize() {
#ifdef __EMSCRIPTEN__
    // Create shader program
    m_program_id = glCreateProgram();
    if (m_program_id == 0) {
        std::cerr << "Failed to create WebGL shader program" << std::endl;
        return false;
    }

    // Convert and compile vertex shader
    std::string vertex_glsl_es = ConvertToGLSLES(m_vertex_source, GL_VERTEX_SHADER);
    m_vertex_shader = CompileShader(vertex_glsl_es, GL_VERTEX_SHADER);
    if (m_vertex_shader == 0) {
        return false;
    }

    // Convert and compile fragment shader
    std::string fragment_glsl_es = ConvertToGLSLES(m_fragment_source, GL_FRAGMENT_SHADER);
    m_fragment_shader = CompileShader(fragment_glsl_es, GL_FRAGMENT_SHADER);
    if (m_fragment_shader == 0) {
        return false;
    }

    // Attach shaders to program
    glAttachShader(m_program_id, m_vertex_shader);
    glAttachShader(m_program_id, m_fragment_shader);

    // Link program
    if (!LinkProgram()) {
        return false;
    }

    m_is_linked = true;
    return true;
#else
    // For non-Emscripten builds, just mark as valid
    m_program_id = 1; // Fake ID for non-WebGL builds
    m_is_linked = true;
    return true;
#endif
}

GLuint WebGLShader::CompileShader(const std::string& source, GLenum type) {
#ifdef __EMSCRIPTEN__
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
        default: type_name = "unknown"; break;
    }

    if (!CheckShaderErrors(shader, type_name)) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
#else
    return 1; // Fake shader ID for non-WebGL builds
#endif
}

bool WebGLShader::LinkProgram() {
#ifdef __EMSCRIPTEN__
    glLinkProgram(m_program_id);
    return CheckProgramErrors();
#else
    return true;
#endif
}

bool WebGLShader::CheckShaderErrors(GLuint shader, const std::string& type) {
#ifdef __EMSCRIPTEN__
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 1) {
            std::string log(log_length, '\0');
            glGetShaderInfoLog(shader, log_length, nullptr, &log[0]);

            // Remove null terminator if present
            if (!log.empty() && log.back() == '\0') {
                log.pop_back();
            }

            std::cerr << "WebGL " << type << " shader compilation failed:\n" << log << std::endl;
        } else {
            std::cerr << "WebGL " << type << " shader compilation failed (no log available)" << std::endl;
        }

        return false;
    }

    return true;
#else
    return true;
#endif
}

bool WebGLShader::CheckProgramErrors() {
#ifdef __EMSCRIPTEN__
    GLint success;
    glGetProgramiv(m_program_id, GL_LINK_STATUS, &success);

    if (!success) {
        GLint log_length = 0;
        glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 1) {
            std::string log(log_length, '\0');
            glGetProgramInfoLog(m_program_id, log_length, nullptr, &log[0]);

            // Remove null terminator if present
            if (!log.empty() && log.back() == '\0') {
                log.pop_back();
            }

            std::cerr << "WebGL shader program linking failed:\n" << log << std::endl;
        } else {
            std::cerr << "WebGL shader program linking failed (no log available)" << std::endl;
        }

        return false;
    }

    return true;
#else
    return true;
#endif
}

std::string WebGLShader::ConvertToGLSLES(const std::string& source, GLenum type) const {
    // Convert GLSL to GLSL ES
    std::string converted = source;

    // Replace version directive
    size_t version_pos = converted.find("#version");
    if (version_pos != std::string::npos) {
        size_t line_end = converted.find('\n', version_pos);
        // Use WebGL 2.0 GLSL ES 3.00
        converted.replace(version_pos, line_end - version_pos, "#version 300 es");
    } else {
        // Add version directive if missing
        converted = "#version 300 es\n" + converted;
    }

#ifdef __EMSCRIPTEN__
    // Add precision qualifiers for fragment shaders
    if (type == GL_FRAGMENT_SHADER) {
        size_t insert_pos = converted.find('\n') + 1;
        converted.insert(insert_pos, "precision mediump float;\n");
    }
#endif

    return converted;
}

void WebGLShader::CacheReflectionData() const {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        m_reflection_cached = true;
        return;
    }

    m_uniforms_cache.clear();
    m_attributes_cache.clear();

    // Query uniforms
    GLint uniform_count = 0;
    glGetProgramiv(m_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);

    for (int i = 0; i < uniform_count; ++i) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveUniform(m_program_id, i, sizeof(name), &length, &size, &type, name);

        UniformInfo info;
        info.name = std::string(name);
        info.location = glGetUniformLocation(m_program_id, name);
        info.type = GLTypeToUniformType(type);

        m_uniforms_cache.push_back(info);
    }

    // Query attributes
    GLint attribute_count = 0;
    glGetProgramiv(m_program_id, GL_ACTIVE_ATTRIBUTES, &attribute_count);

    for (int i = 0; i < attribute_count; ++i) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveAttrib(m_program_id, i, sizeof(name), &length, &size, &type, name);

        AttributeInfo info;
        info.name = std::string(name);
        info.location = glGetAttribLocation(m_program_id, name);
        info.type = GLTypeToUniformType(type);

        m_attributes_cache.push_back(info);
    }

    m_reflection_cached = true;
#endif
}

UniformType WebGLShader::GLTypeToUniformType(GLenum gl_type) const {
#ifdef __EMSCRIPTEN__
    switch (gl_type) {
        case GL_FLOAT:        return UniformType::Float;
        case GL_FLOAT_VEC2:   return UniformType::Vec2;
        case GL_FLOAT_VEC3:   return UniformType::Vec3;
        case GL_FLOAT_VEC4:   return UniformType::Vec4;
        case GL_INT:          return UniformType::Int;
        case GL_INT_VEC2:     return UniformType::IVec2;
        case GL_INT_VEC3:     return UniformType::IVec3;
        case GL_INT_VEC4:     return UniformType::IVec4;
        case GL_BOOL:         return UniformType::Bool;
        case GL_FLOAT_MAT2:   return UniformType::Mat2;
        case GL_FLOAT_MAT3:   return UniformType::Mat3;
        case GL_FLOAT_MAT4:   return UniformType::Mat4;
        case GL_SAMPLER_2D:   return UniformType::Sampler2D;
        case GL_SAMPLER_CUBE: return UniformType::SamplerCube;
        default:              return UniformType::Float;
    }
#else
    return UniformType::Float;
#endif
}

} // namespace Lupine
