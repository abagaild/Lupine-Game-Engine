#pragma once

#include "lupine/core/UUID.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <functional>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Lupine {

class Node;
class Component;

/**
 * @brief Font path structure for font selection
 */
struct FontPath {
    std::string path;           // File path or system font family name
    bool is_system_font = false; // True if this is a system font
    std::string style_name = "Regular"; // Font style for system fonts

    FontPath() = default;
    FontPath(const std::string& p, bool is_system = false, const std::string& style = "Regular")
        : path(p), is_system_font(is_system), style_name(style) {}

    // For serialization/comparison
    bool operator==(const FontPath& other) const {
        return path == other.path && is_system_font == other.is_system_font && style_name == other.style_name;
    }

    bool operator!=(const FontPath& other) const {
        return !(*this == other);
    }

    // Get display name for UI
    std::string GetDisplayName() const {
        if (is_system_font) {
            if (style_name == "Regular" || style_name.empty()) {
                return path;
            }
            return path + " " + style_name;
        } else {
            // Extract filename from path
            size_t last_slash = path.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                return path.substr(last_slash + 1);
            }
            return path;
        }
    }

    // Get actual file path for loading
    std::string GetFilePath() const {
        if (is_system_font) {
            // This will be resolved by ResourceManager
            return "";
        }
        return path;
    }
};

/**
 * @brief Export variable types for specialized editor widgets
 */
enum class ExportVariableType {
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Vec4,
    FilePath,
    FontPath,     // For font selection (system fonts + file browser)
    NodeReference,
    Color,
    Enum  // For dropdown selection from predefined values
};

/**
 * @brief Variant type for export variables
 */
using ExportValue = std::variant<
    bool,
    int,
    float,
    std::string,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    FontPath, // For font selection
    UUID      // For node references
>;

/**
 * @brief Export variable metadata
 */
struct ExportVariable {
    std::string name;
    ExportValue value;
    ExportValue defaultValue; // Store default value for reset functionality
    std::string description;
    ExportVariableType type;
    std::vector<std::string> enumOptions; // For enum types, list of possible values

    ExportVariable() = default;
    ExportVariable(const std::string& n, const ExportValue& v, const std::string& desc = "", ExportVariableType t = ExportVariableType::String)
        : name(n), value(v), defaultValue(v), description(desc), type(t) {}
    ExportVariable(const std::string& n, const ExportValue& v, const std::string& desc, ExportVariableType t, const std::vector<std::string>& options)
        : name(n), value(v), defaultValue(v), description(desc), type(t), enumOptions(options) {}
};

/**
 * @brief Base class for all components
 * 
 * Components provide functionality to nodes and can be written in C++, Lua, or Python.
 * They have export variables that can be edited in the editor and lifecycle methods
 * that are called by the engine.
 */
class Component {
public:
    /**
     * @brief Constructor
     * @param name Name of the component
     */
    explicit Component(const std::string& name = "Component");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Component() = default;
    
    // Disable copy constructor and assignment operator
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    
    // Enable move constructor and assignment operator
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;
    
    /**
     * @brief Get the component's UUID
     * @return UUID of the component
     */
    const UUID& GetUUID() const { return m_uuid; }

    /**
     * @brief Set the component's UUID (used during deserialization)
     * @param uuid The UUID to set
     */
    void SetUUID(const UUID& uuid) { m_uuid = uuid; }
    
    /**
     * @brief Get the component's name
     * @return Name of the component
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set the component's name
     * @param name New name for the component
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get the owner node
     * @return Pointer to owner node, nullptr if not attached
     */
    Node* GetOwner() const { return m_owner; }
    
    /**
     * @brief Set the owner node (internal use)
     * @param owner Pointer to owner node
     */
    void SetOwner(Node* owner) { m_owner = owner; }
    
    /**
     * @brief Check if component is active
     * @return True if active
     */
    bool IsActive() const { return m_active; }
    
    /**
     * @brief Set component active state
     * @param active New active state
     */
    void SetActive(bool active) { m_active = active; }
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    virtual std::string GetTypeName() const { return "Component"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    virtual std::string GetCategory() const { return "General"; }
    
    /**
     * @brief Add an export variable
     * @param name Variable name
     * @param value Initial value
     * @param description Variable description
     * @param type Variable type for editor widgets
     */
    void AddExportVariable(const std::string& name, const ExportValue& value, const std::string& description = "", ExportVariableType type = ExportVariableType::String);

    /**
     * @brief Add enum export variable
     * @param name Variable name
     * @param value Initial value (should be int index)
     * @param description Variable description
     * @param options List of enum option names
     */
    void AddEnumExportVariable(const std::string& name, int value, const std::string& description, const std::vector<std::string>& options);
    
    /**
     * @brief Set export variable value
     * @param name Variable name
     * @param value New value
     * @return True if variable exists and was set
     */
    bool SetExportVariable(const std::string& name, const ExportValue& value);
    
    /**
     * @brief Get export variable value
     * @param name Variable name
     * @return Pointer to value, nullptr if not found
     */
    const ExportValue* GetExportVariable(const std::string& name) const;

    /**
     * @brief Reset export variable to its default value
     * @param name Variable name
     * @return True if variable exists and was reset
     */
    bool ResetExportVariable(const std::string& name);

    /**
     * @brief Get all export variables
     * @return Map of all export variables
     */
    const std::unordered_map<std::string, ExportVariable>& GetAllExportVariables() const { return m_export_variables; }
    
    /**
     * @brief Get all export variables
     * @return Map of export variables
     */
    const std::unordered_map<std::string, ExportVariable>& GetExportVariables() const { return m_export_variables; }
    
    /**
     * @brief Helper to get typed export variable value
     * @tparam T Type to get
     * @param name Variable name
     * @param default_value Default value if not found or wrong type
     * @return Variable value or default
     */
    template<typename T>
    T GetExportVariableValue(const std::string& name, const T& default_value = T{}) const;
    
    // Lifecycle methods
    
    /**
     * @brief Called when component is first created and added to a node
     */
    virtual void OnAwake() {}
    
    /**
     * @brief Called when the node enters the scene tree
     */
    virtual void OnReady() {}
    
    /**
     * @brief Called every frame
     * @param delta_time Time since last frame
     */
    virtual void OnUpdate(float delta_time) { (void)delta_time; }
    
    /**
     * @brief Called during physics processing
     * @param delta_time Time since last physics update
     */
    virtual void OnPhysicsProcess(float delta_time) { (void)delta_time; }
    
    /**
     * @brief Called when input events occur
     * @param event Input event (placeholder for now)
     */
    virtual void OnInput(const void* event) { (void)event; }
    
    /**
     * @brief Called when component is about to be destroyed
     */
    virtual void OnDestroy() {}

protected:
    UUID m_uuid;
    std::string m_name;
    Node* m_owner;
    bool m_active;
    std::unordered_map<std::string, ExportVariable> m_export_variables;
    
    /**
     * @brief Initialize export variables (called in constructor of derived classes)
     */
    virtual void InitializeExportVariables() {}
};

/**
 * @brief Helper functions to determine export variable type from value
 */
inline ExportVariableType GetExportVariableType(const ExportValue& value) {
    if (std::holds_alternative<bool>(value)) return ExportVariableType::Bool;
    if (std::holds_alternative<int>(value)) return ExportVariableType::Int;
    if (std::holds_alternative<float>(value)) return ExportVariableType::Float;
    if (std::holds_alternative<std::string>(value)) return ExportVariableType::String;
    if (std::holds_alternative<glm::vec2>(value)) return ExportVariableType::Vec2;
    if (std::holds_alternative<glm::vec3>(value)) return ExportVariableType::Vec3;
    if (std::holds_alternative<glm::vec4>(value)) return ExportVariableType::Vec4;
    if (std::holds_alternative<FontPath>(value)) return ExportVariableType::FontPath;
    if (std::holds_alternative<UUID>(value)) return ExportVariableType::NodeReference;
    return ExportVariableType::String;
}

/**
 * @brief Component factory function type
 */
using ComponentFactory = std::function<std::unique_ptr<Component>()>;

/**
 * @brief Component information for registration
 */
struct ComponentInfo {
    std::string name;
    std::string category;
    std::string description;
    ComponentFactory factory;

    ComponentInfo() = default;
    ComponentInfo(const std::string& n, const std::string& cat, const std::string& desc, ComponentFactory f)
        : name(n), category(cat), description(desc), factory(f) {}
};

/**
 * @brief Component registry for managing available component types
 */
class ComponentRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static ComponentRegistry& Instance();

    /**
     * @brief Register a component type
     * @param name Component type name
     * @param info Component information
     */
    void RegisterComponent(const std::string& name, const ComponentInfo& info);

    /**
     * @brief Create a component by name
     * @param name Component type name
     * @return Created component or nullptr if not found
     */
    std::unique_ptr<Component> CreateComponent(const std::string& name);

    /**
     * @brief Get all registered component names
     * @return Vector of component names
     */
    std::vector<std::string> GetComponentNames() const;

    /**
     * @brief Get component info by name
     * @param name Component type name
     * @return Component info or nullptr if not found
     */
    const ComponentInfo* GetComponentInfo(const std::string& name) const;

    /**
     * @brief Get components by category
     * @param category Category name
     * @return Vector of component names in the category
     */
    std::vector<std::string> GetComponentsByCategory(const std::string& category) const;

    /**
     * @brief Get all categories
     * @return Vector of category names
     */
    std::vector<std::string> GetCategories() const;

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentInfo> m_components;
};

/**
 * @brief Helper macro for registering components
 */
#define REGISTER_COMPONENT(ComponentClass, Category, Description) \
    namespace { \
        struct ComponentClass##Registrar { \
            ComponentClass##Registrar() { \
                ::Lupine::ComponentRegistry::Instance().RegisterComponent( \
                    #ComponentClass, \
                    ::Lupine::ComponentInfo(#ComponentClass, Category, Description, \
                        []() -> std::unique_ptr<::Lupine::Component> { \
                            return std::make_unique<::Lupine::ComponentClass>(); \
                        }) \
                ); \
            } \
        }; \
        static ComponentClass##Registrar g_##ComponentClass##Registrar; \
    }

// Template implementation
template<typename T>
T Component::GetExportVariableValue(const std::string& name, const T& default_value) const {
    auto it = m_export_variables.find(name);
    if (it != m_export_variables.end()) {
        if (std::holds_alternative<T>(it->second.value)) {
            return std::get<T>(it->second.value);
        }
    }
    return default_value;
}

} // namespace Lupine
