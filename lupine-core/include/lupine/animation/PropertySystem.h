#pragma once

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "lupine/core/Component.h"
#include "lupine/core/Node.h"
#include "lupine/core/UUID.h"
#include "lupine/resources/AnimationResource.h"

namespace Lupine {

/**
 * @brief Property descriptor for dynamic property access
 */
struct PropertyDescriptor {
    std::string name;
    std::string displayName;
    std::string category;
    ExportVariableType type;
    std::string componentType;
    bool isNodeProperty;
    
    PropertyDescriptor() = default;
    PropertyDescriptor(const std::string& n, const std::string& display, const std::string& cat, 
                      ExportVariableType t, const std::string& comp = "", bool nodeProperty = false)
        : name(n), displayName(display), category(cat), type(t), componentType(comp), isNodeProperty(nodeProperty) {}
};

/**
 * @brief Enhanced animation value that works with ExportValue
 */
struct EnhancedAnimationValue {
    ExportValue value;
    ExportVariableType type;
    
    EnhancedAnimationValue() : value(0.0f), type(ExportVariableType::Float) {}
    EnhancedAnimationValue(const ExportValue& val, ExportVariableType t) : value(val), type(t) {}
    
    template<typename T>
    T GetValue() const {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }
        return T{};
    }
    
    bool IsValid() const {
        // Check if the variant holds a valid value (not default-constructed)
        return type != ExportVariableType::Float || std::get<float>(value) != 0.0f ||
               !std::holds_alternative<float>(value) ||
               std::holds_alternative<bool>(value) ||
               std::holds_alternative<int>(value) ||
               std::holds_alternative<std::string>(value) ||
               std::holds_alternative<glm::vec2>(value) ||
               std::holds_alternative<glm::vec3>(value) ||
               std::holds_alternative<glm::vec4>(value) ||
               std::holds_alternative<Lupine::FontPath>(value) ||
               std::holds_alternative<Lupine::UUID>(value);
    }
    
    std::string ToString() const;
    static EnhancedAnimationValue FromString(const std::string& str, ExportVariableType type);
};

/**
 * @brief Property state snapshot for a single object
 */
struct PropertySnapshot {
    std::string nodePath;
    std::unordered_map<std::string, EnhancedAnimationValue> properties;
    float timestamp;
    
    PropertySnapshot() : timestamp(0.0f) {}
    PropertySnapshot(const std::string& path, float time) : nodePath(path), timestamp(time) {}
};

/**
 * @brief Complete scene state snapshot
 */
struct SceneSnapshot {
    std::unordered_map<std::string, PropertySnapshot> nodeSnapshots;
    float timestamp;
    std::string description;
    
    SceneSnapshot() : timestamp(0.0f) {}
    SceneSnapshot(float time, const std::string& desc = "") : timestamp(time), description(desc) {}
};

/**
 * @brief Property change event for autokey detection
 */
struct PropertyChangeEvent {
    std::string nodePath;
    std::string propertyName;
    EnhancedAnimationValue oldValue;
    EnhancedAnimationValue newValue;
    float timestamp;
    
    PropertyChangeEvent() : timestamp(0.0f) {}
    PropertyChangeEvent(const std::string& path, const std::string& prop, 
                       const EnhancedAnimationValue& oldVal, const EnhancedAnimationValue& newVal, float time)
        : nodePath(path), propertyName(prop), oldValue(oldVal), newValue(newVal), timestamp(time) {}
};

/**
 * @brief Autokey recording modes
 */
enum class AutokeyMode {
    Disabled,           // No automatic keyframe recording
    AllProperties,      // Record keyframes for all property changes
    SelectedProperties, // Record keyframes only for selected properties
    ChangedProperties,  // Record keyframes only for properties that actually changed
    TransformOnly       // Record keyframes only for transform properties (position, rotation, scale)
};

/**
 * @brief Property filter for selective animation
 */
struct PropertyFilter {
    std::vector<std::string> includedProperties;
    std::vector<std::string> excludedProperties;
    std::vector<std::string> includedCategories;
    std::vector<std::string> excludedCategories;
    bool includeTransforms = true;
    bool includeVisibility = true;
    bool includeColors = true;
    bool includeCustomProperties = true;
    
    bool ShouldIncludeProperty(const PropertyDescriptor& desc) const;
};

/**
 * @brief Dynamic property discovery and access system
 */
class PropertyReflectionSystem {
public:
    PropertyReflectionSystem() = default;
    ~PropertyReflectionSystem() = default;
    
    // Property discovery
    std::vector<PropertyDescriptor> DiscoverProperties(Node* node) const;
    std::vector<PropertyDescriptor> DiscoverNodeProperties(Node* node) const;
    std::vector<PropertyDescriptor> DiscoverComponentProperties(Component* component) const;
    
    // Property access
    EnhancedAnimationValue GetPropertyValue(Node* node, const std::string& propertyName) const;
    bool SetPropertyValue(Node* node, const std::string& propertyName, const EnhancedAnimationValue& value) const;
    
    // Property validation
    bool IsPropertyAnimatable(const PropertyDescriptor& desc) const;
    bool IsPropertyValid(Node* node, const std::string& propertyName) const;
    
    // Utility methods
    std::string GetPropertyDisplayName(const std::string& propertyName) const;
    std::string GetPropertyCategory(const std::string& propertyName) const;
    ExportVariableType GetPropertyType(Node* node, const std::string& propertyName) const;
    
private:
    // Helper methods for specific node types
    std::vector<PropertyDescriptor> GetNode2DProperties() const;
    std::vector<PropertyDescriptor> GetNode3DProperties() const;
    std::vector<PropertyDescriptor> GetControlProperties() const;
    
    // Helper methods for property access
    EnhancedAnimationValue GetNodePropertyValue(Node* node, const std::string& propertyName) const;
    EnhancedAnimationValue GetComponentPropertyValue(Component* component, const std::string& propertyName) const;
    bool SetNodePropertyValue(Node* node, const std::string& propertyName, const EnhancedAnimationValue& value) const;
    bool SetComponentPropertyValue(Component* component, const std::string& propertyName, const EnhancedAnimationValue& value) const;
};

/**
 * @brief Property state management system
 */
class PropertyStateManager {
public:
    PropertyStateManager() = default;
    ~PropertyStateManager() = default;
    
    // State capture
    PropertySnapshot CaptureNodeState(Node* node, const PropertyFilter& filter = PropertyFilter{}) const;
    SceneSnapshot CaptureSceneState(Scene* scene, const PropertyFilter& filter = PropertyFilter{}) const;
    
    // State restoration
    bool RestoreNodeState(Node* node, const PropertySnapshot& snapshot) const;
    bool RestoreSceneState(Scene* scene, const SceneSnapshot& snapshot) const;
    
    // State comparison
    std::vector<PropertyChangeEvent> CompareStates(const PropertySnapshot& oldState, const PropertySnapshot& newState) const;
    std::vector<PropertyChangeEvent> CompareSceneStates(const SceneSnapshot& oldState, const SceneSnapshot& newState) const;
    
    // State serialization
    std::string SerializeSnapshot(const PropertySnapshot& snapshot) const;
    std::string SerializeSceneSnapshot(const SceneSnapshot& snapshot) const;
    PropertySnapshot DeserializeSnapshot(const std::string& data) const;
    SceneSnapshot DeserializeSceneSnapshot(const std::string& data) const;
    
private:
    PropertyReflectionSystem m_reflectionSystem;
};

/**
 * @brief Enhanced interpolation system for all property types
 */
class PropertyInterpolator {
public:
    PropertyInterpolator() = default;
    ~PropertyInterpolator() = default;
    
    // Interpolation methods
    EnhancedAnimationValue Interpolate(const EnhancedAnimationValue& a, const EnhancedAnimationValue& b, 
                                     float t, InterpolationType interpolation = InterpolationType::Linear) const;
    
    // Type-specific interpolation
    EnhancedAnimationValue InterpolateFloat(float a, float b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateVec2(const glm::vec2& a, const glm::vec2& b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateVec3(const glm::vec3& a, const glm::vec3& b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateVec4(const glm::vec4& a, const glm::vec4& b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateQuaternion(const glm::quat& a, const glm::quat& b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateBool(bool a, bool b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateInt(int a, int b, float t, InterpolationType interpolation) const;
    EnhancedAnimationValue InterpolateString(const std::string& a, const std::string& b, float t, InterpolationType interpolation) const;
    
    // Easing functions
    float ApplyEasing(float t, InterpolationType interpolation) const;
    
private:
    // Easing function implementations
    float EaseLinear(float t) const { return t; }
    float EaseInSine(float t) const;
    float EaseOutSine(float t) const;
    float EaseInOutSine(float t) const;
    float EaseInQuad(float t) const;
    float EaseOutQuad(float t) const;
    float EaseInOutQuad(float t) const;
    float EaseInCubic(float t) const;
    float EaseOutCubic(float t) const;
    float EaseInOutCubic(float t) const;
    float EaseInQuart(float t) const;
    float EaseOutQuart(float t) const;
    float EaseInOutQuart(float t) const;
    float EaseInQuint(float t) const;
    float EaseOutQuint(float t) const;
    float EaseInOutQuint(float t) const;
    float EaseInExpo(float t) const;
    float EaseOutExpo(float t) const;
    float EaseInOutExpo(float t) const;
    float EaseInCirc(float t) const;
    float EaseOutCirc(float t) const;
    float EaseInOutCirc(float t) const;
    float EaseInBack(float t) const;
    float EaseOutBack(float t) const;
    float EaseInOutBack(float t) const;
    float EaseInElastic(float t) const;
    float EaseOutElastic(float t) const;
    float EaseInOutElastic(float t) const;
    float EaseInBounce(float t) const;
    float EaseOutBounce(float t) const;
    float EaseInOutBounce(float t) const;
};

} // namespace Lupine
