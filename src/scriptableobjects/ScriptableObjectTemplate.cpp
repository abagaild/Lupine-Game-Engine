#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include <algorithm>
#include <sstream>
#include <glm/glm.hpp>

namespace Lupine {

ScriptableObjectTemplate::ScriptableObjectTemplate()
    : m_uuid(UUID::Generate())
    , m_name("NewTemplate")
    , m_description("")
    , m_script_language("python") {
}

ScriptableObjectTemplate::ScriptableObjectTemplate(const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_description("")
    , m_script_language("python") {
}

void ScriptableObjectTemplate::AddField(const ScriptableObjectField& field) {
    // Check if field already exists
    auto it = std::find_if(m_fields.begin(), m_fields.end(),
        [&field](const ScriptableObjectField& f) { return f.name == field.name; });
    
    if (it != m_fields.end()) {
        // Replace existing field
        *it = field;
    } else {
        // Add new field
        m_fields.push_back(field);
    }
}

void ScriptableObjectTemplate::RemoveField(const std::string& field_name) {
    m_fields.erase(
        std::remove_if(m_fields.begin(), m_fields.end(),
            [&field_name](const ScriptableObjectField& f) { return f.name == field_name; }),
        m_fields.end());
}

const ScriptableObjectField* ScriptableObjectTemplate::GetField(const std::string& name) const {
    auto it = std::find_if(m_fields.begin(), m_fields.end(),
        [&name](const ScriptableObjectField& f) { return f.name == name; });
    
    return (it != m_fields.end()) ? &(*it) : nullptr;
}

bool ScriptableObjectTemplate::HasField(const std::string& name) const {
    return GetField(name) != nullptr;
}

JsonNode ScriptableObjectTemplate::ToJson() const {
    JsonNode json;
    json["uuid"] = m_uuid.ToString();
    json["name"] = m_name;
    json["description"] = m_description;
    json["script_language"] = m_script_language;
    json["custom_methods"] = m_custom_methods;
    
    std::vector<JsonNode> fields_json;
    for (const auto& field : m_fields) {
        JsonNode field_json;
        field_json["name"] = field.name;
        field_json["type"] = FieldTypeToString(field.type);
        field_json["default_value"] = FieldValueToString(field.default_value, field.type);
        field_json["description"] = field.description;
        fields_json.push_back(field_json);
    }
    json["fields"] = fields_json;
    
    return json;
}

void ScriptableObjectTemplate::FromJson(const JsonNode& json) {
    if (json.HasKey("uuid")) {
        m_uuid = UUID::FromString(json["uuid"].AsString());
    }
    if (json.HasKey("name")) {
        m_name = json["name"].AsString();
    }
    if (json.HasKey("description")) {
        m_description = json["description"].AsString();
    }
    if (json.HasKey("script_language")) {
        m_script_language = json["script_language"].AsString();
    }
    if (json.HasKey("custom_methods")) {
        m_custom_methods = json["custom_methods"].AsString();
    }
    
    if (json.HasKey("fields") && json["fields"].IsArray()) {
        m_fields.clear();
        const auto& fields_array = json["fields"].AsArray();
        for (const auto& field_json : fields_array) {
            ScriptableObjectField field;
            if (field_json.HasKey("name")) {
                field.name = field_json["name"].AsString();
            }
            if (field_json.HasKey("type")) {
                field.type = StringToFieldType(field_json["type"].AsString());
            }
            if (field_json.HasKey("default_value")) {
                field.default_value = StringToFieldValue(field_json["default_value"].AsString(), field.type);
            }
            if (field_json.HasKey("description")) {
                field.description = field_json["description"].AsString();
            }
            m_fields.push_back(field);
        }
    }
}

std::string ScriptableObjectTemplate::FieldTypeToString(ScriptableObjectFieldType type) {
    switch (type) {
        case ScriptableObjectFieldType::String: return "String";
        case ScriptableObjectFieldType::Integer: return "Integer";
        case ScriptableObjectFieldType::Float: return "Float";
        case ScriptableObjectFieldType::Boolean: return "Boolean";
        case ScriptableObjectFieldType::Vector2: return "Vector2";
        case ScriptableObjectFieldType::Vector3: return "Vector3";
        case ScriptableObjectFieldType::Vector4: return "Vector4";
        case ScriptableObjectFieldType::Color: return "Color";
        case ScriptableObjectFieldType::FilePath: return "FilePath";
        case ScriptableObjectFieldType::NodeReference: return "NodeReference";
        default: return "String";
    }
}

ScriptableObjectFieldType ScriptableObjectTemplate::StringToFieldType(const std::string& type_str) {
    if (type_str == "String") return ScriptableObjectFieldType::String;
    if (type_str == "Integer") return ScriptableObjectFieldType::Integer;
    if (type_str == "Float") return ScriptableObjectFieldType::Float;
    if (type_str == "Boolean") return ScriptableObjectFieldType::Boolean;
    if (type_str == "Vector2") return ScriptableObjectFieldType::Vector2;
    if (type_str == "Vector3") return ScriptableObjectFieldType::Vector3;
    if (type_str == "Vector4") return ScriptableObjectFieldType::Vector4;
    if (type_str == "Color") return ScriptableObjectFieldType::Color;
    if (type_str == "FilePath") return ScriptableObjectFieldType::FilePath;
    if (type_str == "NodeReference") return ScriptableObjectFieldType::NodeReference;
    return ScriptableObjectFieldType::String;
}

std::string ScriptableObjectTemplate::FieldValueToString(const ScriptableObjectFieldValue& value, ScriptableObjectFieldType type) {
    std::ostringstream oss;
    
    switch (type) {
        case ScriptableObjectFieldType::String:
        case ScriptableObjectFieldType::FilePath:
        case ScriptableObjectFieldType::NodeReference:
            oss << std::get<std::string>(value);
            break;
        case ScriptableObjectFieldType::Integer:
            oss << std::get<int>(value);
            break;
        case ScriptableObjectFieldType::Float:
            oss << std::get<float>(value);
            break;
        case ScriptableObjectFieldType::Boolean:
            oss << (std::get<bool>(value) ? "true" : "false");
            break;
        case ScriptableObjectFieldType::Vector2: {
            const auto& vec = std::get<glm::vec2>(value);
            oss << vec.x << "," << vec.y;
            break;
        }
        case ScriptableObjectFieldType::Vector3: {
            const auto& vec = std::get<glm::vec3>(value);
            oss << vec.x << "," << vec.y << "," << vec.z;
            break;
        }
        case ScriptableObjectFieldType::Vector4:
        case ScriptableObjectFieldType::Color: {
            const auto& vec = std::get<glm::vec4>(value);
            oss << vec.x << "," << vec.y << "," << vec.z << "," << vec.w;
            break;
        }
    }
    
    return oss.str();
}

ScriptableObjectFieldValue ScriptableObjectTemplate::StringToFieldValue(const std::string& value_str, ScriptableObjectFieldType type) {
    switch (type) {
        case ScriptableObjectFieldType::String:
        case ScriptableObjectFieldType::FilePath:
        case ScriptableObjectFieldType::NodeReference:
            return value_str;
        case ScriptableObjectFieldType::Integer:
            return std::stoi(value_str);
        case ScriptableObjectFieldType::Float:
            return std::stof(value_str);
        case ScriptableObjectFieldType::Boolean:
            return value_str == "true" || value_str == "1";
        case ScriptableObjectFieldType::Vector2: {
            std::istringstream iss(value_str);
            std::string token;
            glm::vec2 vec(0.0f);
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            return vec;
        }
        case ScriptableObjectFieldType::Vector3: {
            std::istringstream iss(value_str);
            std::string token;
            glm::vec3 vec(0.0f);
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            if (std::getline(iss, token, ',')) vec.z = std::stof(token);
            return vec;
        }
        case ScriptableObjectFieldType::Vector4:
        case ScriptableObjectFieldType::Color: {
            std::istringstream iss(value_str);
            std::string token;
            glm::vec4 vec(0.0f);
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            if (std::getline(iss, token, ',')) vec.z = std::stof(token);
            if (std::getline(iss, token, ',')) vec.w = std::stof(token);
            return vec;
        }
    }
    
    return std::string("");
}

} // namespace Lupine
