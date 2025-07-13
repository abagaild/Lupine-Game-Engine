#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "lupine/core/UUID.h"
#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include "lupine/serialization/JsonUtils.h"

namespace Lupine {

// Forward declarations
class ScriptableObjectTemplate;

/**
 * @brief Instance of a scriptable object based on a template
 * 
 * Contains actual field values and provides getter/setter methods.
 * Can execute custom methods defined in the template.
 */
class ScriptableObjectInstance {
public:
    /**
     * @brief Constructor
     */
    ScriptableObjectInstance();
    
    /**
     * @brief Constructor with template and name
     */
    ScriptableObjectInstance(std::shared_ptr<ScriptableObjectTemplate> template_obj, const std::string& name);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ScriptableObjectInstance() = default;
    
    /**
     * @brief Get instance UUID
     */
    const UUID& GetUUID() const { return m_uuid; }
    
    /**
     * @brief Get instance name
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set instance name
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get template
     */
    std::shared_ptr<ScriptableObjectTemplate> GetTemplate() const { return m_template; }
    
    /**
     * @brief Set template (will reset field values to defaults)
     */
    void SetTemplate(std::shared_ptr<ScriptableObjectTemplate> template_obj);
    
    /**
     * @brief Get field value
     */
    ScriptableObjectFieldValue GetFieldValue(const std::string& field_name) const;
    
    /**
     * @brief Set field value
     */
    void SetFieldValue(const std::string& field_name, const ScriptableObjectFieldValue& value);
    
    /**
     * @brief Check if field exists
     */
    bool HasField(const std::string& field_name) const;
    
    /**
     * @brief Get all field values
     */
    const std::unordered_map<std::string, ScriptableObjectFieldValue>& GetFieldValues() const { return m_field_values; }
    
    /**
     * @brief Get field value as string
     */
    std::string GetFieldValueAsString(const std::string& field_name) const;
    
    /**
     * @brief Set field value from string
     */
    void SetFieldValueFromString(const std::string& field_name, const std::string& value_str);
    
    /**
     * @brief Get field value as integer
     */
    int GetFieldValueAsInt(const std::string& field_name) const;
    
    /**
     * @brief Get field value as float
     */
    float GetFieldValueAsFloat(const std::string& field_name) const;
    
    /**
     * @brief Get field value as boolean
     */
    bool GetFieldValueAsBool(const std::string& field_name) const;
    
    /**
     * @brief Get field value as Vector2
     */
    glm::vec2 GetFieldValueAsVector2(const std::string& field_name) const;
    
    /**
     * @brief Get field value as Vector3
     */
    glm::vec3 GetFieldValueAsVector3(const std::string& field_name) const;
    
    /**
     * @brief Get field value as Vector4
     */
    glm::vec4 GetFieldValueAsVector4(const std::string& field_name) const;
    
    /**
     * @brief Set field value as integer
     */
    void SetFieldValueAsInt(const std::string& field_name, int value);
    
    /**
     * @brief Set field value as float
     */
    void SetFieldValueAsFloat(const std::string& field_name, float value);
    
    /**
     * @brief Set field value as boolean
     */
    void SetFieldValueAsBool(const std::string& field_name, bool value);
    
    /**
     * @brief Set field value as Vector2
     */
    void SetFieldValueAsVector2(const std::string& field_name, const glm::vec2& value);
    
    /**
     * @brief Set field value as Vector3
     */
    void SetFieldValueAsVector3(const std::string& field_name, const glm::vec3& value);
    
    /**
     * @brief Set field value as Vector4
     */
    void SetFieldValueAsVector4(const std::string& field_name, const glm::vec4& value);
    
    /**
     * @brief Initialize field values from template defaults
     */
    void InitializeFromTemplate();
    
    /**
     * @brief Serialize instance to JSON
     */
    JsonNode ToJson() const;
    
    /**
     * @brief Deserialize instance from JSON
     */
    void FromJson(const JsonNode& json, const std::unordered_map<UUID, std::shared_ptr<ScriptableObjectTemplate>>& templates);

private:
    UUID m_uuid;
    std::string m_name;
    std::shared_ptr<ScriptableObjectTemplate> m_template;
    std::unordered_map<std::string, ScriptableObjectFieldValue> m_field_values;
};

} // namespace Lupine
