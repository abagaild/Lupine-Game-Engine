#pragma once

#include "lupine/input/InputManager.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace Lupine {

/**
 * @brief Represents a single input binding for an action
 */
struct ActionBinding {
    InputDevice device;
    int code;
    InputActionType action_type;
    
    ActionBinding() : device(InputDevice::Keyboard), code(0), action_type(InputActionType::Pressed) {}
    ActionBinding(InputDevice dev, int c, InputActionType type) 
        : device(dev), code(c), action_type(type) {}
    
    // Convert to InputBinding for InputManager
    InputBinding ToInputBinding() const {
        return InputBinding(device, code, action_type);
    }
    
    // Equality operator for comparison
    bool operator==(const ActionBinding& other) const {
        return device == other.device && code == other.code && action_type == other.action_type;
    }
};

/**
 * @brief Represents an action with multiple possible bindings
 */
struct Action {
    std::string name;
    std::string description;
    std::vector<ActionBinding> bindings;
    
    Action() = default;
    Action(const std::string& n, const std::string& desc = "") 
        : name(n), description(desc) {}
    
    void AddBinding(const ActionBinding& binding) {
        // Check if binding already exists
        for (const auto& existing : bindings) {
            if (existing == binding) {
                return; // Don't add duplicate bindings
            }
        }
        bindings.push_back(binding);
    }
    
    void RemoveBinding(const ActionBinding& binding) {
        bindings.erase(
            std::remove(bindings.begin(), bindings.end(), binding),
            bindings.end()
        );
    }
    
    bool HasBinding(const ActionBinding& binding) const {
        return std::find(bindings.begin(), bindings.end(), binding) != bindings.end();
    }
};

/**
 * @brief Action map configuration system
 * 
 * Manages action mappings with support for multiple bindings per action,
 * JSON serialization/deserialization, and integration with InputManager.
 */
class ActionMap {
public:
    /**
     * @brief Constructor
     */
    ActionMap();
    
    /**
     * @brief Destructor
     */
    ~ActionMap();
    
    /**
     * @brief Add a new action
     * @param name Action name
     * @param description Action description
     * @return Reference to the created action
     */
    Action& AddAction(const std::string& name, const std::string& description = "");
    
    /**
     * @brief Remove an action
     * @param name Action name
     */
    void RemoveAction(const std::string& name);
    
    /**
     * @brief Get an action by name
     * @param name Action name
     * @return Pointer to action or nullptr if not found
     */
    Action* GetAction(const std::string& name);
    
    /**
     * @brief Get an action by name (const version)
     * @param name Action name
     * @return Pointer to action or nullptr if not found
     */
    const Action* GetAction(const std::string& name) const;
    
    /**
     * @brief Get all actions
     * @return Map of action names to actions
     */
    const std::unordered_map<std::string, Action>& GetActions() const { return m_actions; }
    
    /**
     * @brief Add a binding to an action
     * @param action_name Action name
     * @param binding Binding to add
     * @return True if successful
     */
    bool AddBinding(const std::string& action_name, const ActionBinding& binding);
    
    /**
     * @brief Remove a binding from an action
     * @param action_name Action name
     * @param binding Binding to remove
     * @return True if successful
     */
    bool RemoveBinding(const std::string& action_name, const ActionBinding& binding);
    
    /**
     * @brief Check if an action has a specific binding
     * @param action_name Action name
     * @param binding Binding to check
     * @return True if action has the binding
     */
    bool HasBinding(const std::string& action_name, const ActionBinding& binding) const;
    
    /**
     * @brief Find which action (if any) uses a specific binding
     * @param binding Binding to search for
     * @return Action name or empty string if not found
     */
    std::string FindActionForBinding(const ActionBinding& binding) const;
    
    /**
     * @brief Clear all actions and bindings
     */
    void Clear();
    
    /**
     * @brief Apply this action map to the InputManager
     * 
     * This will register all actions with the InputManager using their bindings.
     * Note: This will clear existing InputManager actions first.
     */
    void ApplyToInputManager();
    
    /**
     * @brief Load action map from JSON file
     * @param filepath Path to JSON file
     * @return True if successful
     */
    bool LoadFromFile(const std::string& filepath);
    
    /**
     * @brief Save action map to JSON file
     * @param filepath Path to JSON file
     * @return True if successful
     */
    bool SaveToFile(const std::string& filepath) const;
    
    /**
     * @brief Load action map from JSON data
     * @param json JSON data
     * @return True if successful
     */
    bool LoadFromJson(const nlohmann::json& json);
    
    /**
     * @brief Convert action map to JSON
     * @return JSON representation
     */
    nlohmann::json ToJson() const;
    
    /**
     * @brief Create default action map with common actions
     * @return Default action map
     */
    static ActionMap CreateDefault();

private:
    std::unordered_map<std::string, Action> m_actions;
    
    /**
     * @brief Convert device enum to string
     */
    static std::string DeviceToString(InputDevice device);
    
    /**
     * @brief Convert string to device enum
     */
    static InputDevice StringToDevice(const std::string& str);
    
    /**
     * @brief Convert action type enum to string
     */
    static std::string ActionTypeToString(InputActionType type);
    
    /**
     * @brief Convert string to action type enum
     */
    static InputActionType StringToActionType(const std::string& str);
};

} // namespace Lupine
