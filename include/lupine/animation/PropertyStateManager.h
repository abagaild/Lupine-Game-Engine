#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <variant>
#include <functional>

namespace lupine {

class Node;

/**
 * @brief Property state for animation system
 */
struct PropertyState {
    std::variant<float, int, bool, std::string> value;
    std::variant<float, int, bool, std::string> targetValue;
    float duration = 1.0f;
    float currentTime = 0.0f;
    bool isAnimating = false;
    std::function<void()> onComplete;
};

/**
 * @brief Manages property states for animation system
 */
class PropertyStateManager {
public:
    PropertyStateManager();
    ~PropertyStateManager();

    /**
     * @brief Register a property for animation
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     * @param initialValue Initial value of the property
     */
    void RegisterProperty(const std::string& nodeId, const std::string& propertyName, 
                         const std::variant<float, int, bool, std::string>& initialValue);

    /**
     * @brief Unregister a property
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     */
    void UnregisterProperty(const std::string& nodeId, const std::string& propertyName);

    /**
     * @brief Start animating a property
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     * @param targetValue Target value to animate to
     * @param duration Animation duration in seconds
     * @param onComplete Callback when animation completes
     */
    void AnimateProperty(const std::string& nodeId, const std::string& propertyName,
                        const std::variant<float, int, bool, std::string>& targetValue,
                        float duration, std::function<void()> onComplete = nullptr);

    /**
     * @brief Stop animating a property
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     */
    void StopAnimation(const std::string& nodeId, const std::string& propertyName);

    /**
     * @brief Update all property animations
     * @param deltaTime Time elapsed since last update
     */
    void Update(float deltaTime);

    /**
     * @brief Get current value of a property
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     * @return Current property value
     */
    std::variant<float, int, bool, std::string> GetPropertyValue(const std::string& nodeId, 
                                                                const std::string& propertyName);

    /**
     * @brief Set property value directly (stops any ongoing animation)
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     * @param value New value
     */
    void SetPropertyValue(const std::string& nodeId, const std::string& propertyName,
                         const std::variant<float, int, bool, std::string>& value);

    /**
     * @brief Check if a property is currently animating
     * @param nodeId Node identifier
     * @param propertyName Name of the property
     * @return True if property is animating
     */
    bool IsAnimating(const std::string& nodeId, const std::string& propertyName);

    /**
     * @brief Clear all property states for a node
     * @param nodeId Node identifier
     */
    void ClearNodeProperties(const std::string& nodeId);

    /**
     * @brief Clear all property states
     */
    void Clear();

private:
    std::unordered_map<std::string, std::unordered_map<std::string, PropertyState>> m_propertyStates;

    std::string GetPropertyKey(const std::string& nodeId, const std::string& propertyName);
    void ApplyPropertyToNode(const std::string& nodeId, const std::string& propertyName, 
                           const std::variant<float, int, bool, std::string>& value);
};

} // namespace lupine
