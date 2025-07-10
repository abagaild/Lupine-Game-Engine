#include "lupine/scriptableobjects/ScriptableObjectInstance.h"
#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include <stdexcept>

namespace Lupine {

ScriptableObjectInstance::ScriptableObjectInstance()
    : m_uuid(UUID::Generate())
    , m_name("NewInstance")
    , m_template(nullptr) {
}

ScriptableObjectInstance::ScriptableObjectInstance(std::shared_ptr<ScriptableObjectTemplate> template_obj, const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_template(template_obj) {

    if (m_template) {
        InitializeFromTemplate();
    }
}

void ScriptableObjectInstance::SetTemplate(std::shared_ptr<ScriptableObjectTemplate> template_obj) {
    m_template = template_obj;
    m_field_values.clear();

    if (m_template) {
        InitializeFromTemplate();
    }
}

ScriptableObjectFieldValue ScriptableObjectInstance::GetFieldValue(const std::string& field_name) const {
    auto it = m_field_values.find(field_name);
    if (it != m_field_values.end()) {
        return it->second;
    }
    
    // Return default value from template if field not found in instance
    if (m_template) {
        const auto* field = m_template->GetField(field_name);
        if (field) {
            return field->default_value;
        }
    }
    
    // Return empty string as fallback
    return std::string("");
}

void ScriptableObjectInstance::SetFieldValue(const std::string& field_name, const ScriptableObjectFieldValue& value) {
    if (!m_template || !m_template->HasField(field_name)) {
        return; // Field doesn't exist in template
    }
    
    m_field_values[field_name] = value;
}

bool ScriptableObjectInstance::HasField(const std::string& field_name) const {
    return m_template && m_template->HasField(field_name);
}

std::string ScriptableObjectInstance::GetFieldValueAsString(const std::string& field_name) const {
    if (!HasField(field_name)) return "";
    
    const auto* field = m_template->GetField(field_name);
    if (!field) return "";
    
    auto value = GetFieldValue(field_name);
    return ScriptableObjectTemplate::FieldValueToString(value, field->type);
}

void ScriptableObjectInstance::SetFieldValueFromString(const std::string& field_name, const std::string& value_str) {
    if (!HasField(field_name)) return;
    
    const auto* field = m_template->GetField(field_name);
    if (!field) return;
    
    auto value = ScriptableObjectTemplate::StringToFieldValue(value_str, field->type);
    SetFieldValue(field_name, value);
}

int ScriptableObjectInstance::GetFieldValueAsInt(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<int>(value);
    } catch (const std::bad_variant_access&) {
        return 0;
    }
}

float ScriptableObjectInstance::GetFieldValueAsFloat(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<float>(value);
    } catch (const std::bad_variant_access&) {
        return 0.0f;
    }
}

bool ScriptableObjectInstance::GetFieldValueAsBool(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<bool>(value);
    } catch (const std::bad_variant_access&) {
        return false;
    }
}

glm::vec2 ScriptableObjectInstance::GetFieldValueAsVector2(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<glm::vec2>(value);
    } catch (const std::bad_variant_access&) {
        return glm::vec2(0.0f);
    }
}

glm::vec3 ScriptableObjectInstance::GetFieldValueAsVector3(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<glm::vec3>(value);
    } catch (const std::bad_variant_access&) {
        return glm::vec3(0.0f);
    }
}

glm::vec4 ScriptableObjectInstance::GetFieldValueAsVector4(const std::string& field_name) const {
    auto value = GetFieldValue(field_name);
    try {
        return std::get<glm::vec4>(value);
    } catch (const std::bad_variant_access&) {
        return glm::vec4(0.0f);
    }
}

void ScriptableObjectInstance::SetFieldValueAsInt(const std::string& field_name, int value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::SetFieldValueAsFloat(const std::string& field_name, float value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::SetFieldValueAsBool(const std::string& field_name, bool value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::SetFieldValueAsVector2(const std::string& field_name, const glm::vec2& value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::SetFieldValueAsVector3(const std::string& field_name, const glm::vec3& value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::SetFieldValueAsVector4(const std::string& field_name, const glm::vec4& value) {
    SetFieldValue(field_name, value);
}

void ScriptableObjectInstance::InitializeFromTemplate() {
    if (!m_template) return;
    
    m_field_values.clear();
    
    for (const auto& field : m_template->GetFields()) {
        m_field_values[field.name] = field.default_value;
    }
}

JsonNode ScriptableObjectInstance::ToJson() const {
    JsonNode json;
    json["uuid"] = m_uuid.ToString();
    json["name"] = m_name;
    
    if (m_template) {
        json["template_uuid"] = m_template->GetUUID().ToString();
        
        JsonNode field_values_json;
        for (const auto& pair : m_field_values) {
            const auto* field = m_template->GetField(pair.first);
            if (field) {
                field_values_json[pair.first] = ScriptableObjectTemplate::FieldValueToString(pair.second, field->type);
            }
        }
        json["field_values"] = field_values_json;
    }
    
    return json;
}

void ScriptableObjectInstance::FromJson(const JsonNode& json, const std::unordered_map<UUID, std::shared_ptr<ScriptableObjectTemplate>>& templates) {
    if (json.HasKey("uuid")) {
        m_uuid = UUID::FromString(json["uuid"].AsString());
    }
    if (json.HasKey("name")) {
        m_name = json["name"].AsString();
    }
    
    if (json.HasKey("template_uuid")) {
        UUID template_uuid = UUID::FromString(json["template_uuid"].AsString());
        auto it = templates.find(template_uuid);
        if (it != templates.end()) {
            m_template = it->second;
        }
    }
    
    if (m_template && json.HasKey("field_values") && json["field_values"].IsObject()) {
        m_field_values.clear();
        const auto& field_values_obj = json["field_values"].AsObject();
        
        for (const auto& pair : field_values_obj) {
            const std::string& field_name = pair.first;
            const auto* field = m_template->GetField(field_name);
            if (field) {
                auto value = ScriptableObjectTemplate::StringToFieldValue(pair.second.AsString(), field->type);
                m_field_values[field_name] = value;
            }
        }
    }
}

} // namespace Lupine
