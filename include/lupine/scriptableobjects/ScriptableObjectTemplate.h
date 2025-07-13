#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <glm/glm.hpp>
#include "lupine/core/UUID.h"
#include "lupine/serialization/JsonUtils.h"

namespace Lupine {

/**
 * @brief Supported field types for scriptable object templates
 */
enum class ScriptableObjectFieldType {
    String,
    Integer,
    Float,
    Boolean,
    Vector2,
    Vector3,
    Vector4,
    Color,
    FilePath,
    NodeReference
};

/**
 * @brief Field value variant type
 */
using ScriptableObjectFieldValue = std::variant<
    std::string,    // String, FilePath, NodeReference (UUID as string)
    int,            // Integer
    float,          // Float
    bool,           // Boolean
    glm::vec2,      // Vector2
    glm::vec3,      // Vector3
    glm::vec4       // Vector4, Color
>;

/**
 * @brief Field definition for scriptable object templates
 */
struct ScriptableObjectField {
    std::string name;
    ScriptableObjectFieldType type;
    ScriptableObjectFieldValue default_value;
    std::string description;
    
    ScriptableObjectField() = default;
    ScriptableObjectField(const std::string& name, ScriptableObjectFieldType type, 
                         const ScriptableObjectFieldValue& default_val, 
                         const std::string& desc = "")
        : name(name), type(type), default_value(default_val), description(desc) {}
};

/**
 * @brief Template definition for scriptable objects
 * 
 * Defines the structure and fields that instances can have.
 * Templates can have custom methods defined by users.
 */
class ScriptableObjectTemplate {
public:
    /**
     * @brief Constructor
     */
    ScriptableObjectTemplate();
    
    /**
     * @brief Constructor with name
     */
    ScriptableObjectTemplate(const std::string& name);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ScriptableObjectTemplate() = default;
    
    /**
     * @brief Get template UUID
     */
    const UUID& GetUUID() const { return m_uuid; }
    
    /**
     * @brief Get template name
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set template name
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get template description
     */
    const std::string& GetDescription() const { return m_description; }
    
    /**
     * @brief Set template description
     */
    void SetDescription(const std::string& description) { m_description = description; }
    
    /**
     * @brief Add a field to the template
     */
    void AddField(const ScriptableObjectField& field);
    
    /**
     * @brief Remove a field from the template
     */
    void RemoveField(const std::string& field_name);
    
    /**
     * @brief Get all fields
     */
    const std::vector<ScriptableObjectField>& GetFields() const { return m_fields; }
    
    /**
     * @brief Get field by name
     */
    const ScriptableObjectField* GetField(const std::string& name) const;
    
    /**
     * @brief Check if field exists
     */
    bool HasField(const std::string& name) const;
    
    /**
     * @brief Get custom methods code
     */
    const std::string& GetCustomMethods() const { return m_custom_methods; }
    
    /**
     * @brief Set custom methods code
     */
    void SetCustomMethods(const std::string& methods) { m_custom_methods = methods; }
    
    /**
     * @brief Get script language for custom methods
     */
    const std::string& GetScriptLanguage() const { return m_script_language; }
    
    /**
     * @brief Set script language for custom methods
     */
    void SetScriptLanguage(const std::string& language) { m_script_language = language; }
    
    /**
     * @brief Serialize template to JSON
     */
    JsonNode ToJson() const;
    
    /**
     * @brief Deserialize template from JSON
     */
    void FromJson(const JsonNode& json);
    
    /**
     * @brief Convert field type to string
     */
    static std::string FieldTypeToString(ScriptableObjectFieldType type);
    
    /**
     * @brief Convert string to field type
     */
    static ScriptableObjectFieldType StringToFieldType(const std::string& type_str);
    
    /**
     * @brief Convert field value to string
     */
    static std::string FieldValueToString(const ScriptableObjectFieldValue& value, ScriptableObjectFieldType type);
    
    /**
     * @brief Convert string to field value
     */
    static ScriptableObjectFieldValue StringToFieldValue(const std::string& value_str, ScriptableObjectFieldType type);

private:
    UUID m_uuid;
    std::string m_name;
    std::string m_description;
    std::vector<ScriptableObjectField> m_fields;
    std::string m_custom_methods;
    std::string m_script_language; // "python" or "lua"
};

} // namespace Lupine
