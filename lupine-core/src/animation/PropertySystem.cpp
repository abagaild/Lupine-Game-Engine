#include "lupine/animation/PropertySystem.h"
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
    std::ostringstream oss;
    
    switch (type) {
        case ExportVariableType::Float:
            oss << GetValue<float>();
            break;
        case ExportVariableType::Int:
            oss << GetValue<int>();
            break;
        case ExportVariableType::Bool:
            oss << (GetValue<bool>() ? "true" : "false");
            break;
        case ExportVariableType::String:
            oss << GetValue<std::string>();
            break;
        case ExportVariableType::Vec2: {
            auto vec = GetValue<glm::vec2>();
            oss << vec.x << "," << vec.y;
            break;
        }
        case ExportVariableType::Vec3: {
            auto vec = GetValue<glm::vec3>();
            oss << vec.x << "," << vec.y << "," << vec.z;
            break;
        }
        case ExportVariableType::Vec4: {
            auto vec = GetValue<glm::vec4>();
            oss << vec.x << "," << vec.y << "," << vec.z << "," << vec.w;
            break;
        }
        default:
            oss << "unknown";
            break;
    }
    
    return oss.str();
}

EnhancedAnimationValue EnhancedAnimationValue::FromString(const std::string& str, ExportVariableType type) {
    EnhancedAnimationValue result;
    result.type = type;
    
    switch (type) {
        case ExportVariableType::Float:
            result.value = std::stof(str);
            break;
        case ExportVariableType::Int:
            result.value = std::stoi(str);
            break;
        case ExportVariableType::Bool:
            result.value = (str == "true" || str == "1");
            break;
        case ExportVariableType::String:
            result.value = str;
            break;
        case ExportVariableType::Vec2: {
            std::istringstream iss(str);
            std::string token;
            glm::vec2 vec;
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            result.value = vec;
            break;
        }
        case ExportVariableType::Vec3: {
            std::istringstream iss(str);
            std::string token;
            glm::vec3 vec;
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            if (std::getline(iss, token, ',')) vec.z = std::stof(token);
            result.value = vec;
            break;
        }
        case ExportVariableType::Vec4: {
            std::istringstream iss(str);
            std::string token;
            glm::vec4 vec;
            if (std::getline(iss, token, ',')) vec.x = std::stof(token);
            if (std::getline(iss, token, ',')) vec.y = std::stof(token);
            if (std::getline(iss, token, ',')) vec.z = std::stof(token);
            if (std::getline(iss, token, ',')) vec.w = std::stof(token);
            result.value = vec;
            break;
        }
        default:
            result.value = 0.0f;
            break;
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
    
    // Check included properties (if specified)
    if (!includedProperties.empty()) {
        return std::find(includedProperties.begin(), includedProperties.end(), desc.name) != includedProperties.end();
    }
    
    // Check included categories (if specified)
    if (!includedCategories.empty()) {
        return std::find(includedCategories.begin(), includedCategories.end(), desc.category) != includedCategories.end();
    }
    
    // Check type-specific filters
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
    
    // Common node properties
    properties.emplace_back("name", "Name", "Node", ExportVariableType::String, "", true);
    // Note: Visibility property not available in current Node implementation
    
    // Type-specific properties
    if (auto node2D = dynamic_cast<Node2D*>(node)) {
        auto node2DProps = GetNode2DProperties();
        properties.insert(properties.end(), node2DProps.begin(), node2DProps.end());
    } else if (auto node3D = dynamic_cast<Node3D*>(node)) {
        auto node3DProps = GetNode3DProperties();
        properties.insert(properties.end(), node3DProps.begin(), node3DProps.end());
    } else if (auto control = dynamic_cast<Control*>(node)) {
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
        {"scale", "Scale", "Transform", ExportVariableType::Vec2, "", true}
        // Note: z_index and z_as_relative not available in current Node2D implementation
    };
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::GetNode3DProperties() const {
    return {
        {"position", "Position", "Transform", ExportVariableType::Vec3, "", true},
        {"rotation", "Rotation", "Transform", ExportVariableType::Vec3, "", true},
        {"scale", "Scale", "Transform", ExportVariableType::Vec3, "", true}
    };
}

std::vector<PropertyDescriptor> PropertyReflectionSystem::GetControlProperties() const {
    return {
        {"position", "Position", "Transform", ExportVariableType::Vec2, "", true},
        {"size", "Size", "Transform", ExportVariableType::Vec2, "", true},
        {"anchor_left", "Anchor Left", "Layout", ExportVariableType::Float, "", true},
        {"anchor_top", "Anchor Top", "Layout", ExportVariableType::Float, "", true},
        {"anchor_right", "Anchor Right", "Layout", ExportVariableType::Float, "", true},
        {"anchor_bottom", "Anchor Bottom", "Layout", ExportVariableType::Float, "", true},
        {"margin_left", "Margin Left", "Layout", ExportVariableType::Float, "", true},
        {"margin_top", "Margin Top", "Layout", ExportVariableType::Float, "", true},
        {"margin_right", "Margin Right", "Layout", ExportVariableType::Float, "", true},
        {"margin_bottom", "Margin Bottom", "Layout", ExportVariableType::Float, "", true}
    };
}

// Property access methods
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

// Helper methods for property access
EnhancedAnimationValue PropertyReflectionSystem::GetNodePropertyValue(Node* node, const std::string& propertyName) const {
    if (!node) return EnhancedAnimationValue();

    // Handle common node properties
    if (propertyName == "name") {
        return EnhancedAnimationValue(ExportValue(node->GetName()), ExportVariableType::String);
    }
    // Note: Node class doesn't have visibility methods in current implementation

    // Handle type-specific properties
    if (auto node2D = dynamic_cast<Node2D*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(node2D->GetPosition()), ExportVariableType::Vec2);
        } else if (propertyName == "rotation") {
            return EnhancedAnimationValue(ExportValue(node2D->GetRotation()), ExportVariableType::Float);
        } else if (propertyName == "scale") {
            return EnhancedAnimationValue(ExportValue(node2D->GetScale()), ExportVariableType::Vec2);
        }
        // Note: Node2D doesn't have z_index or z_as_relative methods in current implementation
    } else if (auto node3D = dynamic_cast<Node3D*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(node3D->GetPosition()), ExportVariableType::Vec3);
        } else if (propertyName == "rotation") {
            // Convert quaternion to Euler angles for animation
            glm::vec3 eulerAngles = glm::eulerAngles(node3D->GetRotation());
            return EnhancedAnimationValue(ExportValue(eulerAngles), ExportVariableType::Vec3);
        } else if (propertyName == "scale") {
            return EnhancedAnimationValue(ExportValue(node3D->GetScale()), ExportVariableType::Vec3);
        }
    } else if (auto control = dynamic_cast<Control*>(node)) {
        if (propertyName == "position") {
            return EnhancedAnimationValue(ExportValue(control->GetPosition()), ExportVariableType::Vec2);
        } else if (propertyName == "size") {
            return EnhancedAnimationValue(ExportValue(control->GetSize()), ExportVariableType::Vec2);
        }
        // Add more control properties as needed
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

bool PropertyReflectionSystem::SetNodePropertyValue(Node* node, const std::string& propertyName, const EnhancedAnimationValue& value) const {
    if (!node || !value.IsValid()) return false;

    // Handle common node properties
    if (propertyName == "name" && value.type == ExportVariableType::String) {
        if (std::holds_alternative<std::string>(value.value)) {
            node->SetName(std::get<std::string>(value.value));
            return true;
        }
    }
    // Note: Node class doesn't have visibility methods in current implementation

    // Handle type-specific properties
    if (auto node2D = dynamic_cast<Node2D*>(node)) {
        if (propertyName == "position" && value.type == ExportVariableType::Vec2) {
            if (std::holds_alternative<glm::vec2>(value.value)) {
                node2D->SetPosition(std::get<glm::vec2>(value.value));
                return true;
            }
        } else if (propertyName == "rotation" && value.type == ExportVariableType::Float) {
            if (std::holds_alternative<float>(value.value)) {
                node2D->SetRotation(std::get<float>(value.value));
                return true;
            }
        } else if (propertyName == "scale" && value.type == ExportVariableType::Vec2) {
            if (std::holds_alternative<glm::vec2>(value.value)) {
                node2D->SetScale(std::get<glm::vec2>(value.value));
                return true;
            }
        }
        // Note: Node2D doesn't have z_index or z_as_relative methods in current implementation
    } else if (node && node->IsValidNode()) {
        if (auto node3D = node->SafeCast<Node3D>()) {
            if (propertyName == "position" && value.type == ExportVariableType::Vec3) {
                if (std::holds_alternative<glm::vec3>(value.value)) {
                    node3D->SetPosition(std::get<glm::vec3>(value.value));
                    return true;
                }
            } else if (propertyName == "rotation" && value.type == ExportVariableType::Vec3) {
                if (std::holds_alternative<glm::vec3>(value.value)) {
                    // Convert Euler angles to quaternion
                    glm::vec3 eulerAngles = std::get<glm::vec3>(value.value);
                    node3D->SetRotation(eulerAngles.x, eulerAngles.y, eulerAngles.z);
                    return true;
                }
            } else if (propertyName == "scale" && value.type == ExportVariableType::Vec3) {
                if (std::holds_alternative<glm::vec3>(value.value)) {
                    node3D->SetScale(std::get<glm::vec3>(value.value));
                    return true;
                }
            }
        }
    } else if (node && node->IsValidNode()) {
        if (auto control = node->SafeCast<Control>()) {
            if (propertyName == "position" && value.type == ExportVariableType::Vec2) {
                if (std::holds_alternative<glm::vec2>(value.value)) {
                    control->SetPosition(std::get<glm::vec2>(value.value));
                    return true;
                }
            } else if (propertyName == "size" && value.type == ExportVariableType::Vec2) {
                if (std::holds_alternative<glm::vec2>(value.value)) {
                    control->SetSize(std::get<glm::vec2>(value.value));
                    return true;
                }
            }
            // Add more control properties as needed
        }
    }

    return false;
}

bool PropertyReflectionSystem::SetComponentPropertyValue(Component* component, const std::string& propertyName, const EnhancedAnimationValue& value) const {
    if (!component || !value.IsValid()) return false;

    const auto& exportVars = component->GetAllExportVariables();
    auto it = exportVars.find(propertyName);
    if (it == exportVars.end()) return false;

    // Convert EnhancedAnimationValue back to ExportValue
    ExportValue exportValue;
    switch (value.type) {
        case ExportVariableType::Bool:
            if (std::holds_alternative<bool>(value.value)) {
                exportValue = ExportValue(std::get<bool>(value.value));
            }
            break;
        case ExportVariableType::Int:
            if (std::holds_alternative<int>(value.value)) {
                exportValue = ExportValue(std::get<int>(value.value));
            }
            break;
        case ExportVariableType::Float:
            if (std::holds_alternative<float>(value.value)) {
                exportValue = ExportValue(std::get<float>(value.value));
            }
            break;
        case ExportVariableType::String:
            if (std::holds_alternative<std::string>(value.value)) {
                exportValue = ExportValue(std::get<std::string>(value.value));
            }
            break;
        case ExportVariableType::Vec2:
            if (std::holds_alternative<glm::vec2>(value.value)) {
                exportValue = ExportValue(std::get<glm::vec2>(value.value));
            }
            break;
        case ExportVariableType::Vec3:
            if (std::holds_alternative<glm::vec3>(value.value)) {
                exportValue = ExportValue(std::get<glm::vec3>(value.value));
            }
            break;
        case ExportVariableType::Vec4:
            if (std::holds_alternative<glm::vec4>(value.value)) {
                exportValue = ExportValue(std::get<glm::vec4>(value.value));
            }
            break;
        default:
            return false;
    }

    return component->SetExportVariable(propertyName, exportValue);
}

} // namespace Lupine
