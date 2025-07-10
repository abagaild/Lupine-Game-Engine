#include "lupine/visualscripting/VScriptNode.h"
#include <algorithm>
#include <sstream>

namespace Lupine {

// VScriptPin implementation
VScriptPin::VScriptPin(const std::string& name, VScriptDataType data_type,
                       VScriptPinDirection direction, const std::string& default_value)
    : m_name(name)
    , m_data_type(data_type)
    , m_direction(direction)
    , m_default_value(default_value)
    , m_is_array(false)
    , m_element_type(data_type)
    , m_is_optional(false)
    , m_allow_multiple_connections(false)
{
    // Set default label to name if not provided
    if (m_label.empty()) {
        m_label = m_name;
    }
}

VScriptPin::VScriptPin(const std::string& name, VScriptDataType element_type,
                       VScriptPinDirection direction, bool is_array, const std::string& default_value)
    : m_name(name)
    , m_data_type(is_array ? VScriptDataType::Array : element_type)
    , m_direction(direction)
    , m_default_value(default_value)
    , m_is_array(is_array)
    , m_element_type(element_type)
    , m_is_optional(false)
    , m_allow_multiple_connections(false)
{
    // Set default label to name if not provided
    if (m_label.empty()) {
        m_label = m_name;
    }
}

bool VScriptPin::IsCompatibleWith(const VScriptPin& other) const {
    // Can't connect two pins of the same direction
    if (m_direction == other.m_direction) {
        return false;
    }

    // Execution pins can only connect to execution pins
    if (IsExecutionPin() || other.IsExecutionPin()) {
        return IsExecutionPin() && other.IsExecutionPin();
    }

    // Wildcard pins can connect to anything (except execution)
    if (IsWildcardPin() || other.IsWildcardPin()) {
        return !IsExecutionPin() && !other.IsExecutionPin();
    }

    // Any type can connect to any other type (with implicit conversion)
    if (m_data_type == VScriptDataType::Any || other.m_data_type == VScriptDataType::Any) {
        return true;
    }

    // Array compatibility
    if (IsArrayPin() && other.IsArrayPin()) {
        // Both are arrays, check element type compatibility
        VScriptPin temp_this(m_name, m_element_type, m_direction);
        VScriptPin temp_other(other.m_name, other.m_element_type, other.m_direction);
        return temp_this.IsCompatibleWith(temp_other);
    } else if (IsArrayPin() || other.IsArrayPin()) {
        // One is array, one is not - check if single element can connect to array
        const VScriptPin* array_pin = IsArrayPin() ? this : &other;
        const VScriptPin* single_pin = IsArrayPin() ? &other : this;

        VScriptPin temp_element(array_pin->m_name, array_pin->m_element_type, array_pin->m_direction);
        return temp_element.IsCompatibleWith(*single_pin);
    }

    // Same types are always compatible
    if (m_data_type == other.m_data_type) {
        return true;
    }

    // Numeric types can be converted between each other
    bool this_numeric = (m_data_type == VScriptDataType::Integer || m_data_type == VScriptDataType::Float);
    bool other_numeric = (other.m_data_type == VScriptDataType::Integer || other.m_data_type == VScriptDataType::Float);

    if (this_numeric && other_numeric) {
        return true;
    }

    // Vector types can be converted between each other
    bool this_vector = (m_data_type == VScriptDataType::Vector2 ||
                       m_data_type == VScriptDataType::Vector3 ||
                       m_data_type == VScriptDataType::Vector4);
    bool other_vector = (other.m_data_type == VScriptDataType::Vector2 ||
                        other.m_data_type == VScriptDataType::Vector3 ||
                        other.m_data_type == VScriptDataType::Vector4);

    if (this_vector && other_vector) {
        return true;
    }

    // Object types can connect to more specific object types
    if (m_data_type == VScriptDataType::Object || other.m_data_type == VScriptDataType::Object) {
        return true;
    }

    return false;
}

nlohmann::json VScriptPin::ToJson() const {
    nlohmann::json json;
    json["name"] = m_name;
    json["label"] = m_label;
    json["tooltip"] = m_tooltip;
    json["data_type"] = static_cast<int>(m_data_type);
    json["direction"] = static_cast<int>(m_direction);
    json["default_value"] = m_default_value;
    json["is_array"] = m_is_array;
    json["element_type"] = static_cast<int>(m_element_type);
    json["is_optional"] = m_is_optional;
    json["allow_multiple_connections"] = m_allow_multiple_connections;
    json["sub_category"] = m_sub_category;
    return json;
}

void VScriptPin::FromJson(const nlohmann::json& json) {
    if (json.contains("name")) {
        m_name = json["name"].get<std::string>();
    }
    if (json.contains("label")) {
        m_label = json["label"].get<std::string>();
    }
    if (json.contains("tooltip")) {
        m_tooltip = json["tooltip"].get<std::string>();
    }
    if (json.contains("data_type")) {
        m_data_type = static_cast<VScriptDataType>(json["data_type"].get<int>());
    }
    if (json.contains("direction")) {
        m_direction = static_cast<VScriptPinDirection>(json["direction"].get<int>());
    }
    if (json.contains("default_value")) {
        m_default_value = json["default_value"].get<std::string>();
    }
    if (json.contains("is_array")) {
        m_is_array = json["is_array"].get<bool>();
    }
    if (json.contains("element_type")) {
        m_element_type = static_cast<VScriptDataType>(json["element_type"].get<int>());
    }
    if (json.contains("is_optional")) {
        m_is_optional = json["is_optional"].get<bool>();
    }
    if (json.contains("allow_multiple_connections")) {
        m_allow_multiple_connections = json["allow_multiple_connections"].get<bool>();
    }
    if (json.contains("sub_category")) {
        m_sub_category = json["sub_category"].get<std::string>();
    }
}

// VScriptNode implementation
VScriptNode::VScriptNode(const std::string& id, const std::string& type, VScriptNodeCategory category)
    : m_id(id)
    , m_type(type)
    , m_category(category)
    , m_x(0.0f)
    , m_y(0.0f)
{
    InitializePins();
}

std::string VScriptNode::GetProperty(const std::string& key, const std::string& default_value) const {
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it->second : default_value;
}

void VScriptNode::AddPin(std::unique_ptr<VScriptPin> pin) {
    if (!pin) {
        return;
    }

    VScriptPin* pin_ptr = pin.get();
    m_pin_lookup[pin->GetName()] = pin_ptr;
    m_pins.push_back(std::move(pin));
}

VScriptPin* VScriptNode::GetPin(const std::string& name) const {
    auto it = m_pin_lookup.find(name);
    return (it != m_pin_lookup.end()) ? it->second : nullptr;
}

std::vector<VScriptPin*> VScriptNode::GetInputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Input) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<VScriptPin*> VScriptNode::GetOutputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Output) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<VScriptPin*> VScriptNode::GetExecutionInputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Input && pin->IsExecutionPin()) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<VScriptPin*> VScriptNode::GetExecutionOutputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Output && pin->IsExecutionPin()) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<VScriptPin*> VScriptNode::GetDataInputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Input && !pin->IsExecutionPin()) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<VScriptPin*> VScriptNode::GetDataOutputPins() const {
    std::vector<VScriptPin*> pins;
    for (const auto& pin : m_pins) {
        if (pin->GetDirection() == VScriptPinDirection::Output && !pin->IsExecutionPin()) {
            pins.push_back(pin.get());
        }
    }
    return pins;
}

std::vector<std::string> VScriptNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    // Default implementation - just add a comment
    lines.push_back(indent + "# " + GetDisplayName() + " node");
    
    return lines;
}

std::string VScriptNode::GetCodeTemplate() const {
    return "# {node_name} node";
}

nlohmann::json VScriptNode::ToJson() const {
    nlohmann::json json;
    json["id"] = m_id;
    json["type"] = m_type;
    json["display_name"] = m_display_name;
    json["description"] = m_description;
    json["category"] = static_cast<int>(m_category);
    json["position"] = {m_x, m_y};
    json["properties"] = m_properties;
    
    json["pins"] = nlohmann::json::array();
    for (const auto& pin : m_pins) {
        json["pins"].push_back(pin->ToJson());
    }
    
    return json;
}

void VScriptNode::FromJson(const nlohmann::json& json) {
    if (json.contains("display_name")) {
        m_display_name = json["display_name"].get<std::string>();
    }
    if (json.contains("description")) {
        m_description = json["description"].get<std::string>();
    }
    if (json.contains("position") && json["position"].is_array() && json["position"].size() >= 2) {
        m_x = json["position"][0].get<float>();
        m_y = json["position"][1].get<float>();
    }
    if (json.contains("properties")) {
        m_properties = json["properties"].get<std::unordered_map<std::string, std::string>>();
    }
    
    // Note: Pins are typically initialized by the specific node type, not loaded from JSON
}

} // namespace Lupine
