#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <memory>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace Lupine {

// Forward declarations
class Node;
class Component;

/**
 * @brief Type for global variable values
 */
using GlobalVariableValue = std::variant<bool, int, float, std::string, glm::vec2, glm::vec3, glm::vec4>;

/**
 * @brief Global variable definition
 */
struct GlobalVariable {
    std::string name;
    std::string type;
    GlobalVariableValue value;
    GlobalVariableValue default_value;
    std::string description;
    bool is_exported = true; // Whether this variable is accessible from scripts
    
    GlobalVariable() = default;
    GlobalVariable(const std::string& name, const std::string& type, const GlobalVariableValue& value, const std::string& description = "")
        : name(name), type(type), value(value), default_value(value), description(description) {}
};

/**
 * @brief Autoload script definition
 */
struct AutoloadScript {
    std::string name;           // Name to access the script by (e.g., "GameManager")
    std::string script_path;    // Path to the script file
    std::string script_type;    // "python" or "lua"
    bool enabled = true;        // Whether this autoload is active
    std::string description;    // Optional description
    
    // Runtime data
    Node* instance_node = nullptr;      // Node that holds the script component
    Component* script_component = nullptr; // The actual script component
    
    AutoloadScript() = default;
    AutoloadScript(const std::string& name, const std::string& script_path, const std::string& script_type, const std::string& description = "")
        : name(name), script_path(script_path), script_type(script_type), description(description) {}
};

/**
 * @brief GlobalsManager - Singleton class for managing global variables and autoload scripts
 * 
 * This class provides Godot-like functionality for:
 * - Registering autoload scripts that can be accessed by name from anywhere
 * - Managing global variables with strict typing and default values
 * - Runtime access from Python and Lua scripts
 */
class GlobalsManager {
public:
    /**
     * @brief Get the singleton instance
     */
    static GlobalsManager& Instance();
    
    /**
     * @brief Initialize the globals manager
     */
    static bool Initialize();
    
    /**
     * @brief Shutdown the globals manager
     */
    static void Shutdown();

    /**
     * @brief Destructor
     */
    ~GlobalsManager() = default;

    // === Autoload Scripts ===
    
    /**
     * @brief Register an autoload script
     * @param autoload The autoload script definition
     * @return True if registered successfully
     */
    bool RegisterAutoload(const AutoloadScript& autoload);
    
    /**
     * @brief Unregister an autoload script
     * @param name Name of the autoload to remove
     * @return True if removed successfully
     */
    bool UnregisterAutoload(const std::string& name);
    
    /**
     * @brief Get an autoload script by name
     * @param name Name of the autoload
     * @return Pointer to autoload or nullptr if not found
     */
    AutoloadScript* GetAutoload(const std::string& name);
    
    /**
     * @brief Get all registered autoloads
     * @return Map of all autoloads
     */
    const std::unordered_map<std::string, AutoloadScript>& GetAllAutoloads() const { return m_autoloads; }
    
    /**
     * @brief Initialize all autoload scripts (called at scene start)
     * @param scene Scene to add autoload nodes to
     */
    void InitializeAutoloads(class Scene* scene);
    
    /**
     * @brief Cleanup autoload scripts (called at scene end)
     */
    void CleanupAutoloads();
    
    /**
     * @brief Get autoload script component by name (for runtime access)
     * @param name Name of the autoload
     * @return Pointer to script component or nullptr
     */
    Component* GetAutoloadComponent(const std::string& name);
    
    // === Global Variables ===
    
    /**
     * @brief Register a global variable
     * @param variable The global variable definition
     * @return True if registered successfully
     */
    bool RegisterGlobalVariable(const GlobalVariable& variable);
    
    /**
     * @brief Unregister a global variable
     * @param name Name of the variable to remove
     * @return True if removed successfully
     */
    bool UnregisterGlobalVariable(const std::string& name);
    
    /**
     * @brief Set global variable value
     * @param name Variable name
     * @param value New value
     * @return True if set successfully
     */
    bool SetGlobalVariable(const std::string& name, const GlobalVariableValue& value);
    
    /**
     * @brief Get global variable value
     * @param name Variable name
     * @return Pointer to value or nullptr if not found
     */
    const GlobalVariableValue* GetGlobalVariable(const std::string& name) const;
    
    /**
     * @brief Get global variable definition
     * @param name Variable name
     * @return Pointer to variable definition or nullptr if not found
     */
    const GlobalVariable* GetGlobalVariableDefinition(const std::string& name) const;
    
    /**
     * @brief Get all global variables
     * @return Map of all global variables
     */
    const std::unordered_map<std::string, GlobalVariable>& GetAllGlobalVariables() const { return m_global_variables; }
    
    /**
     * @brief Reset global variable to default value
     * @param name Variable name
     * @return True if reset successfully
     */
    bool ResetGlobalVariable(const std::string& name);
    
    /**
     * @brief Reset all global variables to default values
     */
    void ResetAllGlobalVariables();
    
    // === Typed Getters for Global Variables ===
    
    /**
     * @brief Get typed global variable value
     * @tparam T Type to get
     * @param name Variable name
     * @param default_value Default value if not found or wrong type
     * @return Variable value or default
     */
    template<typename T>
    T GetGlobalVariableValue(const std::string& name, const T& default_value = T{}) const;
    
    // === Serialization ===
    
    /**
     * @brief Serialize globals configuration to JSON
     * @return JSON object containing all globals data
     */
    nlohmann::json SerializeToJson() const;
    
    /**
     * @brief Deserialize globals configuration from JSON
     * @param json JSON object to deserialize from
     * @return True if deserialized successfully
     */
    bool DeserializeFromJson(const nlohmann::json& json);
    
    /**
     * @brief Clear all globals data
     */
    void Clear();

    /**
     * @brief Parse a variable value from string
     * @param type The variable type
     * @param value_str The string representation of the value
     * @return The parsed value
     */
    GlobalVariableValue ParseVariableValue(const std::string& type, const std::string& value_str) const;

private:
    GlobalsManager() = default;
    
    // Disable copy and move
    GlobalsManager(const GlobalsManager&) = delete;
    GlobalsManager& operator=(const GlobalsManager&) = delete;
    GlobalsManager(GlobalsManager&&) = delete;
    GlobalsManager& operator=(GlobalsManager&&) = delete;
    
    // Helper methods
    std::string GetVariableTypeString(const GlobalVariableValue& value) const;
    bool ValidateVariableType(const std::string& type, const GlobalVariableValue& value) const;
    
    // Data
    std::unordered_map<std::string, AutoloadScript> m_autoloads;
    std::unordered_map<std::string, GlobalVariable> m_global_variables;
    
    static std::unique_ptr<GlobalsManager> s_instance;
    static bool s_initialized;
};

// Template implementation
template<typename T>
T GlobalsManager::GetGlobalVariableValue(const std::string& name, const T& default_value) const {
    auto it = m_global_variables.find(name);
    if (it != m_global_variables.end()) {
        if (std::holds_alternative<T>(it->second.value)) {
            return std::get<T>(it->second.value);
        }

        // Handle type conversions between int and float
        if constexpr (std::is_same_v<T, int>) {
            if (std::holds_alternative<float>(it->second.value)) {
                return static_cast<int>(std::get<float>(it->second.value));
            }
        } else if constexpr (std::is_same_v<T, float>) {
            if (std::holds_alternative<int>(it->second.value)) {
                return static_cast<float>(std::get<int>(it->second.value));
            }
        }
    }
    return default_value;
}

} // namespace Lupine
