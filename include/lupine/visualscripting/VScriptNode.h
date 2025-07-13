#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace Lupine {

// Forward declarations
class VScriptPin;

/**
 * @brief Data types for visual script pins
 */
enum class VScriptDataType {
    Execution,      ///< Execution flow (white)
    Boolean,        ///< Boolean value (red)
    Integer,        ///< Integer number (cyan)
    Float,          ///< Floating point number (green)
    String,         ///< String value (magenta)
    Vector2,        ///< 2D vector (yellow)
    Vector3,        ///< 3D vector (purple)
    Vector4,        ///< 4D vector (orange)
    Transform,      ///< Transform matrix (brown)
    Rotator,        ///< Rotation (pink)
    Color,          ///< Color value (rainbow)
    Object,         ///< Generic object reference (blue)
    Class,          ///< Class reference (dark blue)
    Enum,           ///< Enumeration (light blue)
    Struct,         ///< Structure (teal)
    Array,          ///< Array type (with element type)
    Map,            ///< Map/Dictionary type
    Set,            ///< Set type
    Delegate,       ///< Function delegate (red outline)
    Event,          ///< Event delegate (red filled)
    Wildcard,       ///< Wildcard/template type (gray diamond)
    Any             ///< Any type (gray)
};

/**
 * @brief Direction of a pin (input or output)
 */
enum class VScriptPinDirection {
    Input,          ///< Input pin (left side of node)
    Output          ///< Output pin (right side of node)
};

/**
 * @brief Categories for organizing nodes in the palette
 */
enum class VScriptNodeCategory {
    Event,          ///< Event nodes (OnReady, OnUpdate, etc.)
    FlowControl,    ///< Flow control (If, For, While, etc.)
    Variable,       ///< Variable access (Get, Set)
    Math,           ///< Mathematical operations
    Logic,          ///< Logical operations
    Function,       ///< Function calls
    Custom          ///< Custom snippet nodes
};

/**
 * @brief A pin on a visual script node for connections
 */
class VScriptPin {
public:
    /**
     * @brief Constructor
     * @param name Pin name
     * @param data_type Data type of the pin
     * @param direction Input or output direction
     * @param default_value Default value for the pin
     */
    VScriptPin(const std::string& name, VScriptDataType data_type,
               VScriptPinDirection direction, const std::string& default_value = "");

    /**
     * @brief Constructor for array pins
     * @param name Pin name
     * @param element_type Element type for arrays
     * @param direction Input or output direction
     * @param default_value Default value for the pin
     */
    VScriptPin(const std::string& name, VScriptDataType element_type,
               VScriptPinDirection direction, bool is_array, const std::string& default_value = "");

    /**
     * @brief Get pin name
     */
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    /**
     * @brief Get pin data type
     */
    VScriptDataType GetDataType() const { return m_data_type; }
    void SetDataType(VScriptDataType type) { m_data_type = type; }

    /**
     * @brief Get pin direction
     */
    VScriptPinDirection GetDirection() const { return m_direction; }
    void SetDirection(VScriptPinDirection direction) { m_direction = direction; }

    /**
     * @brief Get default value
     */
    const std::string& GetDefaultValue() const { return m_default_value; }
    void SetDefaultValue(const std::string& value) { m_default_value = value; }

    /**
     * @brief Get display label (can be different from name)
     */
    const std::string& GetLabel() const { return m_label.empty() ? m_name : m_label; }
    void SetLabel(const std::string& label) { m_label = label; }

    /**
     * @brief Get tooltip text
     */
    const std::string& GetTooltip() const { return m_tooltip; }
    void SetTooltip(const std::string& tooltip) { m_tooltip = tooltip; }

    /**
     * @brief Check if pin is an execution pin
     */
    bool IsExecutionPin() const { return m_data_type == VScriptDataType::Execution; }

    /**
     * @brief Check if this pin is an array pin
     */
    bool IsArrayPin() const { return m_is_array; }

    /**
     * @brief Check if this pin is a wildcard pin
     */
    bool IsWildcardPin() const { return m_data_type == VScriptDataType::Wildcard; }

    /**
     * @brief Check if this pin is a delegate pin
     */
    bool IsDelegatePin() const { return m_data_type == VScriptDataType::Delegate || m_data_type == VScriptDataType::Event; }

    /**
     * @brief Get the element type for array pins
     */
    VScriptDataType GetElementType() const { return m_element_type; }

    /**
     * @brief Set whether this pin is optional
     */
    void SetOptional(bool optional) { m_is_optional = optional; }

    /**
     * @brief Check if this pin is optional
     */
    bool IsOptional() const { return m_is_optional; }

    /**
     * @brief Set whether this pin can have multiple connections
     */
    void SetAllowMultipleConnections(bool allow) { m_allow_multiple_connections = allow; }

    /**
     * @brief Check if this pin allows multiple connections
     */
    bool AllowsMultipleConnections() const { return m_allow_multiple_connections; }

    /**
     * @brief Check if pin is compatible with another pin for connections
     * @param other Other pin to check compatibility with
     * @return True if pins can be connected
     */
    bool IsCompatibleWith(const VScriptPin& other) const;

    /**
     * @brief Serialize pin to JSON
     */
    nlohmann::json ToJson() const;

    /**
     * @brief Deserialize pin from JSON
     */
    void FromJson(const nlohmann::json& json);

private:
    std::string m_name;                    ///< Pin name (identifier)
    std::string m_label;                   ///< Display label (optional)
    std::string m_tooltip;                 ///< Tooltip text
    VScriptDataType m_data_type;           ///< Data type
    VScriptPinDirection m_direction;       ///< Input or output
    std::string m_default_value;           ///< Default value as string
    bool m_is_array;                       ///< Whether this is an array pin
    VScriptDataType m_element_type;        ///< Element type for array pins
    bool m_is_optional;                    ///< Whether this pin is optional
    bool m_allow_multiple_connections;     ///< Whether this pin allows multiple connections
    std::string m_sub_category;            ///< Sub-category for organization
};

/**
 * @brief A node in a visual script graph
 */
class VScriptNode {
public:
    /**
     * @brief Constructor
     * @param id Unique node ID
     * @param type Node type name
     * @param category Node category
     */
    VScriptNode(const std::string& id, const std::string& type, VScriptNodeCategory category);

    /**
     * @brief Virtual destructor
     */
    virtual ~VScriptNode() = default;

    /**
     * @brief Get node ID
     */
    const std::string& GetId() const { return m_id; }

    /**
     * @brief Get node type
     */
    const std::string& GetType() const { return m_type; }

    /**
     * @brief Get node category
     */
    VScriptNodeCategory GetCategory() const { return m_category; }

    /**
     * @brief Get display name
     */
    const std::string& GetDisplayName() const { return m_display_name.empty() ? m_type : m_display_name; }
    void SetDisplayName(const std::string& name) { m_display_name = name; }

    /**
     * @brief Get node description
     */
    const std::string& GetDescription() const { return m_description; }
    void SetDescription(const std::string& description) { m_description = description; }

    /**
     * @brief Position in the graph editor
     */
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    void SetPosition(float x, float y) { m_x = x; m_y = y; }

    /**
     * @brief Node properties (key-value pairs for configuration)
     */
    const std::unordered_map<std::string, std::string>& GetProperties() const { return m_properties; }
    void SetProperty(const std::string& key, const std::string& value) { m_properties[key] = value; }
    std::string GetProperty(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Pin management
     */
    void AddPin(std::unique_ptr<VScriptPin> pin);
    VScriptPin* GetPin(const std::string& name) const;
    std::vector<VScriptPin*> GetInputPins() const;
    std::vector<VScriptPin*> GetOutputPins() const;
    std::vector<VScriptPin*> GetExecutionInputPins() const;
    std::vector<VScriptPin*> GetExecutionOutputPins() const;
    std::vector<VScriptPin*> GetDataInputPins() const;
    std::vector<VScriptPin*> GetDataOutputPins() const;

    /**
     * @brief Generate Python code for this node
     * @param indent_level Current indentation level
     * @return Generated Python code lines
     */
    virtual std::vector<std::string> GenerateCode(int indent_level = 0) const;

    /**
     * @brief Check if this is a comment node (for special rendering)
     */
    virtual bool IsCommentNode() const { return false; }

    /**
     * @brief Get the Python template for this node type
     * @return Python code template with placeholders
     */
    virtual std::string GetCodeTemplate() const;

    /**
     * @brief Serialize node to JSON
     */
    virtual nlohmann::json ToJson() const;

    /**
     * @brief Deserialize node from JSON
     */
    virtual void FromJson(const nlohmann::json& json);

protected:
    /**
     * @brief Initialize pins for this node type (called in constructor)
     */
    virtual void InitializePins() {}

    std::string m_id;                                              ///< Unique node ID
    std::string m_type;                                            ///< Node type name
    std::string m_display_name;                                    ///< Display name (optional)
    std::string m_description;                                     ///< Node description
    VScriptNodeCategory m_category;                                ///< Node category
    
    float m_x, m_y;                                                ///< Position in graph editor
    
    std::vector<std::unique_ptr<VScriptPin>> m_pins;               ///< All pins on this node
    std::unordered_map<std::string, VScriptPin*> m_pin_lookup;     ///< Fast pin lookup by name
    std::unordered_map<std::string, std::string> m_properties;     ///< Node properties
};

} // namespace Lupine
