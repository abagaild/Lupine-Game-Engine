#include "lupine/animation/EnhancedAnimationSystem.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/core/Scene.h"
#include "lupine/serialization/SerializationUtils.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace Lupine {

// EnhancedAnimationValue implementation
std::string EnhancedAnimationValue::ToString() const {
    switch (type) {
        case ExportVariableType::Bool:
            return std::get<bool>(value) ? "true" : "false";
        case ExportVariableType::Int:
            return std::to_string(std::get<int>(value));
        case ExportVariableType::Float:
            return std::to_string(std::get<float>(value));
        case ExportVariableType::String:
            return std::get<std::string>(value);
        case ExportVariableType::Vec2:
            return SerializationUtils::SerializeVec2(std::get<glm::vec2>(value));
        case ExportVariableType::Vec3:
            return SerializationUtils::SerializeVec3(std::get<glm::vec3>(value));
        case ExportVariableType::Vec4:
            return SerializationUtils::SerializeVec4(std::get<glm::vec4>(value));
        case ExportVariableType::FontPath:
            return std::get<FontPath>(value).path;
        case ExportVariableType::NodeReference:
            return std::get<UUID>(value).ToString();
        default:
            return "";
    }
}

EnhancedAnimationValue EnhancedAnimationValue::FromString(const std::string& str, ExportVariableType type) {
    EnhancedAnimationValue result;
    result.type = type;
    
    try {
        switch (type) {
            case ExportVariableType::Bool:
                result.value = (str == "true" || str == "1");
                break;
            case ExportVariableType::Int:
                result.value = std::stoi(str);
                break;
            case ExportVariableType::Float:
                result.value = std::stof(str);
                break;
            case ExportVariableType::String:
                result.value = str;
                break;
            case ExportVariableType::Vec2:
                result.value = SerializationUtils::ParseVec2(str);
                break;
            case ExportVariableType::Vec3:
                result.value = SerializationUtils::ParseVec3(str);
                break;
            case ExportVariableType::Vec4:
                result.value = SerializationUtils::ParseVec4(str);
                break;
            case ExportVariableType::FontPath:
                result.value = FontPath{str};
                break;
            case ExportVariableType::NodeReference:
                result.value = UUID::FromString(str);
                break;
            default:
                result.value = 0.0f;
                result.type = ExportVariableType::Float;
                break;
        }
    } catch (const std::exception&) {
        // If parsing fails, return a default value
        result.value = 0.0f;
        result.type = ExportVariableType::Float;
    }
    
    return result;
}

// PropertyFilter implementation
bool PropertyFilter::ShouldIncludeProperty(const PropertyDescriptor& desc) const {
    // Check excluded properties first
    if (std::find(excludedProperties.begin(), excludedProperties.end(), desc.name) != excludedProperties.end()) {
        return false;
    }
    
    // Check excluded categories
    if (std::find(excludedCategories.begin(), excludedCategories.end(), desc.category) != excludedCategories.end()) {
        return false;
    }
    
    // Check included properties (if specified, only these are included)
    if (!includedProperties.empty()) {
        return std::find(includedProperties.begin(), includedProperties.end(), desc.name) != includedProperties.end();
    }
    
    // Check included categories (if specified, only these are included)
    if (!includedCategories.empty()) {
        return std::find(includedCategories.begin(), includedCategories.end(), desc.category) != includedCategories.end();
    }
    
    // Check type-based filters
    if (desc.category == "Transform" && !includeTransforms) return false;
    if (desc.category == "Visibility" && !includeVisibility) return false;
    if (desc.category == "Color" && !includeColors) return false;
    if (desc.category == "Custom" && !includeCustomProperties) return false;
    
    return true;
}

// PropertyReflectionSystem implementation
std::vector<PropertyDescriptor> PropertyReflectionSystem::DiscoverProperties(Node* node) const {
    if (!node) return {};
    
    std::vector<PropertyDescriptor> properties;
    
    // Add node-specific properties
    auto nodeProps = DiscoverNodeProperties(node);
    properties.insert(properties.end(), nodeProps.begin(), nodeProps.end());
    
    // Add component properties
    auto components = node->GetComponents<Component>();
    for (const auto* component : components) {
        auto componentProps = DiscoverComponentProperties(const_cast<Component*>(component));
        properties.insert(properties.end(), componentProps.begin(), componentProps.end());
    }
    
    return properties;
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::DiscoverNodeProperties(Node* node) const {
    if (!node) return {};
    
    std::vector<PropertyDescriptor> properties;
    
    // Add common node properties
    properties.emplace_back("name", "Name", "General", ExportVariableType::String, "", true);
    properties.emplace_back("active", "Active", "Visibility", ExportVariableType::Bool, "", true);
    
    // Add type-specific properties
    if (auto* node2d = dynamic_cast<Node2D*>(node)) {
        auto node2dProps = GetNode2DProperties();
        properties.insert(properties.end(), node2dProps.begin(), node2dProps.end());
    } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
        auto node3dProps = GetNode3DProperties();
        properties.insert(properties.end(), node3dProps.begin(), node3dProps.end());
    } else if (auto* control = dynamic_cast<Control*>(node)) {
        auto controlProps = GetControlProperties();
        properties.insert(properties.end(), controlProps.begin(), controlProps.end());
    }
    
    return properties;
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::DiscoverComponentProperties(Component* component) const {
    if (!component) return {};
    
    std::vector<PropertyDescriptor> properties;
    
    // Use the component's export variables system
    const auto& exportVars = component->GetAllExportVariables();
    for (const auto& [varName, exportVar] : exportVars) {
        std::string fullName = component->GetTypeName() + "." + varName;
        std::string category = component->GetCategory();
        if (category.empty()) category = "Component";
        
        properties.emplace_back(fullName, exportVar.description.empty() ? varName : exportVar.description, 
                               category, exportVar.type, component->GetTypeName(), false);
    }
    
    return properties;
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::GetNode2DProperties() const {
    return {
        {"position", "Position", "Transform", ExportVariableType::Vec2, "", true},
        {"rotation", "Rotation", "Transform", ExportVariableType::Float, "", true},
        {"scale", "Scale", "Transform", ExportVariableType::Vec2, "", true},
        {"visible", "Visible", "Visibility", ExportVariableType::Bool, "", true}
    };
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::GetNode3DProperties() const {
    return {
        {"position", "Position", "Transform", ExportVariableType::Vec3, "", true},
        {"rotation", "Rotation", "Transform", ExportVariableType::Vec3, "", true},
        {"scale", "Scale", "Transform", ExportVariableType::Vec3, "", true},
        {"visible", "Visible", "Visibility", ExportVariableType::Bool, "", true}
    };
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::GetControlProperties() const {
    return {
        {"position", "Position", "Transform", ExportVariableType::Vec2, "", true},
        {"size", "Size", "Transform", ExportVariableType::Vec2, "", true},
        {"anchor_min", "Anchor Min", "Layout", ExportVariableType::Vec2, "", true},
        {"anchor_max", "Anchor Max", "Layout", ExportVariableType::Vec2, "", true},
        {"margin_left", "Margin Left", "Layout", ExportVariableType::Float, "", true},
        {"margin_top", "Margin Top", "Layout", ExportVariableType::Float, "", true},
        {"margin_right", "Margin Right", "Layout", ExportVariableType::Float, "", true},
        {"margin_bottom", "Margin Bottom", "Layout", ExportVariableType::Float, "", true},
        {"visible", "Visible", "Visibility", ExportVariableType::Bool, "", true}
    };
}

EnhancedAnimationValue PropertyReflectionSystem::GetPropertyValue(Node* node, const std::string& propertyName) const {
    if (!node) return EnhancedAnimationValue();
    
    // Check if it's a component property
    if (propertyName.find('.') != std::string::npos) {
        size_t dotPos = propertyName.find('.');
        std::string componentType = propertyName.substr(0, dotPos);
        std::string propName = propertyName.substr(dotPos + 1);
        
        auto components = node->GetComponents<Component>();
        for (const auto* component : components) {
            if (component->GetTypeName() == componentType) {
                return GetComponentPropertyValue(const_cast<Component*>(component), propName);
            }
        }
        return EnhancedAnimationValue();
    }
    
    // It's a node property
    return GetNodePropertyValue(node, propertyName);
}

EnhancedAnimationValue PropertyReflectionSystem::GetNodePropertyValue(Node* node, const std::string& propertyName) const {
    if (!node) return EnhancedAnimationValue();
    
    if (propertyName == "name") {
        return EnhancedAnimationValue(ExportValue(node->GetName()), ExportVariableType::String);
    } else if (propertyName == "active") {
        return EnhancedAnimationValue(ExportValue(node->IsActive()), ExportVariableType::Bool);
    }
    
    // Type-specific properties
    if (auto* node2d = dynamic_cast<Node2D*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(node2d->GetPosition()), ExportVariableType::Vec2);
        } else if (propertyName == "rotation") {
            return EnhancedAnimationValue(ExportValue(node2d->GetRotation()), ExportVariableType::Float);
        } else if (propertyName == "scale") {
            return EnhancedAnimationValue(ExportValue(node2d->GetScale()), ExportVariableType::Vec2);
        } else if (propertyName == "visible") {
            return EnhancedAnimationValue(ExportValue(node2d->IsActive()), ExportVariableType::Bool);
        }
    } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(node3d->GetPosition()), ExportVariableType::Vec3);
        } else if (propertyName == "rotation") {
            glm::vec3 euler = glm::eulerAngles(node3d->GetRotation());
            return EnhancedAnimationValue(ExportValue(euler), ExportVariableType::Vec3);
        } else if (propertyName == "scale") {
            return EnhancedAnimationValue(ExportValue(node3d->GetScale()), ExportVariableType::Vec3);
        } else if (propertyName == "visible") {
            return EnhancedAnimationValue(ExportValue(node3d->IsActive()), ExportVariableType::Bool);
        }
    } else if (auto* control = dynamic_cast<Control*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(control->GetPosition()), ExportVariableType::Vec2);
        } else if (propertyName == "size") {
            return EnhancedAnimationValue(ExportValue(control->GetSize()), ExportVariableType::Vec2);
        } else if (propertyName == "anchor_min") {
            return EnhancedAnimationValue(ExportValue(control->GetAnchorMin()), ExportVariableType::Vec2);
        } else if (propertyName == "anchor_max") {
            return EnhancedAnimationValue(ExportValue(control->GetAnchorMax()), ExportVariableType::Vec2);
        } else if (propertyName == "visible") {
            return EnhancedAnimationValue(ExportValue(control->IsActive()), ExportVariableType::Bool);
        }
    }
    
    return EnhancedAnimationValue();
}

EnhancedAnimationValue PropertyReflectionSystem::GetComponentPropertyValue(Component* component, const std::string& propertyName) const {
    if (!component) return EnhancedAnimationValue();
    
    const ExportValue* value = component->GetExportVariable(propertyName);
    if (!value) return EnhancedAnimationValue();
    
    const auto& exportVars = component->GetAllExportVariables();
    auto it = exportVars.find(propertyName);
    if (it != exportVars.end()) {
        return EnhancedAnimationValue(*value, it->second.type);
    }
    
    return EnhancedAnimationValue();
}

bool PropertyReflectionSystem::SetPropertyValue(Node* node, const std::string& propertyName, const EnhancedAnimationValue& value) const {
    if (!node || !value.IsValid()) return false;

    // Check if it's a component property
    if (propertyName.find('.') != std::string::npos) {
        size_t dotPos = propertyName.find('.');
        std::string componentType = propertyName.substr(0, dotPos);
        std::string propName = propertyName.substr(dotPos + 1);

        auto components = node->GetComponents<Component>();
        for (auto* component : components) {
            if (component->GetTypeName() == componentType) {
                return SetComponentPropertyValue(component, propName, value);
            }
        }
        return false;
    }

    // It's a node property
    return SetNodePropertyValue(node, propertyName, value);
}

bool PropertyReflectionSystem::SetNodePropertyValue(Node* node, const std::string& propertyName, const EnhancedAnimationValue& value) const {
    if (!node || !value.IsValid()) return false;

    if (propertyName == "name" && value.type == ExportVariableType::String) {
        node->SetName(value.GetValue<std::string>());
        return true;
    } else if (propertyName == "active" && value.type == ExportVariableType::Bool) {
        node->SetActive(value.GetValue<bool>());
        return true;
    }

    // Type-specific properties
    if (auto* node2d = dynamic_cast<Node2D*>(node)) {
        if (propertyName == "position" && value.type == ExportVariableType::Vec2) {
            node2d->SetPosition(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "rotation" && value.type == ExportVariableType::Float) {
            node2d->SetRotation(value.GetValue<float>());
            return true;
        } else if (propertyName == "scale" && value.type == ExportVariableType::Vec2) {
            node2d->SetScale(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "visible" && value.type == ExportVariableType::Bool) {
            node2d->SetActive(value.GetValue<bool>());
            return true;
        }
    } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
        if (propertyName == "position" && value.type == ExportVariableType::Vec3) {
            node3d->SetPosition(value.GetValue<glm::vec3>());
            return true;
        } else if (propertyName == "rotation" && value.type == ExportVariableType::Vec3) {
            glm::vec3 euler = value.GetValue<glm::vec3>();
            glm::quat quat = glm::quat(euler);
            node3d->SetRotation(quat);
            return true;
        } else if (propertyName == "scale" && value.type == ExportVariableType::Vec3) {
            node3d->SetScale(value.GetValue<glm::vec3>());
            return true;
        } else if (propertyName == "visible" && value.type == ExportVariableType::Bool) {
            node3d->SetActive(value.GetValue<bool>());
            return true;
        }
    } else if (auto* control = dynamic_cast<Control*>(node)) {
        if (propertyName == "position" && value.type == ExportVariableType::Vec2) {
            control->SetPosition(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "size" && value.type == ExportVariableType::Vec2) {
            control->SetSize(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "anchor_min" && value.type == ExportVariableType::Vec2) {
            control->SetAnchorMin(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "anchor_max" && value.type == ExportVariableType::Vec2) {
            control->SetAnchorMax(value.GetValue<glm::vec2>());
            return true;
        } else if (propertyName == "visible" && value.type == ExportVariableType::Bool) {
            control->SetActive(value.GetValue<bool>());
            return true;
        }
    }

    return false;
}

bool PropertyReflectionSystem::SetComponentPropertyValue(Component* component, const std::string& propertyName, const EnhancedAnimationValue& value) const {
    if (!component || !value.IsValid()) return false;

    return component->SetExportVariable(propertyName, value.value);
}

bool PropertyReflectionSystem::IsPropertyAnimatable(const PropertyDescriptor& desc) const {
    // Most properties are animatable, but some might not be
    switch (desc.type) {
        case ExportVariableType::Bool:
        case ExportVariableType::Int:
        case ExportVariableType::Float:
        case ExportVariableType::Vec2:
        case ExportVariableType::Vec3:
        case ExportVariableType::Vec4:
            return true;
        case ExportVariableType::String:
        case ExportVariableType::FontPath:
        case ExportVariableType::NodeReference:
            return false; // These types are typically not smoothly animatable
        default:
            return false;
    }
}

bool PropertyReflectionSystem::IsPropertyValid(Node* node, const std::string& propertyName) const {
    if (!node) return false;

    auto properties = DiscoverProperties(node);
    return std::any_of(properties.begin(), properties.end(),
                      [&propertyName](const PropertyDescriptor& desc) {
                          return desc.name == propertyName;
                      });
}

std::string PropertyReflectionSystem::GetPropertyDisplayName(const std::string& propertyName) const {
    // Convert property names to display names
    if (propertyName == "position") return "Position";
    if (propertyName == "rotation") return "Rotation";
    if (propertyName == "scale") return "Scale";
    if (propertyName == "visible") return "Visible";
    if (propertyName == "active") return "Active";
    if (propertyName == "name") return "Name";
    if (propertyName == "z_index") return "Z Index";
    if (propertyName == "anchor_min") return "Anchor Min";
    if (propertyName == "anchor_max") return "Anchor Max";
    if (propertyName == "size") return "Size";

    // For component properties, extract the property part
    if (propertyName.find('.') != std::string::npos) {
        size_t dotPos = propertyName.find('.');
        std::string propName = propertyName.substr(dotPos + 1);
        // Convert snake_case to Title Case
        std::string result = propName;
        if (!result.empty()) {
            result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
            for (size_t i = 1; i < result.length(); ++i) {
                if (result[i] == '_') {
                    result[i] = ' ';
                    if (i + 1 < result.length()) {
                        result[i + 1] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i + 1])));
                    }
                }
            }
        }
        return result;
    }

    return propertyName;
}

std::string PropertyReflectionSystem::GetPropertyCategory(const std::string& propertyName) const {
    if (propertyName == "position" || propertyName == "rotation" || propertyName == "scale" || propertyName == "z_index") {
        return "Transform";
    }
    if (propertyName == "visible" || propertyName == "active") {
        return "Visibility";
    }
    if (propertyName == "anchor_min" || propertyName == "anchor_max" || propertyName == "size") {
        return "Layout";
    }
    if (propertyName == "name") {
        return "General";
    }

    // For component properties
    if (propertyName.find('.') != std::string::npos) {
        if (propertyName.find("color") != std::string::npos || propertyName.find("modulate") != std::string::npos) {
            return "Color";
        }
        return "Component";
    }

    return "General";
}

ExportVariableType PropertyReflectionSystem::GetPropertyType(Node* node, const std::string& propertyName) const {
    if (!node) return ExportVariableType::Float;

    auto properties = DiscoverProperties(node);
    auto it = std::find_if(properties.begin(), properties.end(),
                          [&propertyName](const PropertyDescriptor& desc) {
                              return desc.name == propertyName;
                          });

    if (it != properties.end()) {
        return it->type;
    }

    return ExportVariableType::Float;
}

} // namespace Lupine
