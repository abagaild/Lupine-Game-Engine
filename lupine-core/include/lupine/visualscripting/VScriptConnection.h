#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Lupine {

/**
 * @brief A connection between two pins in a visual script graph
 */
class VScriptConnection {
public:
    /**
     * @brief Constructor
     * @param from_node_id Source node ID
     * @param from_pin_name Source pin name
     * @param to_node_id Target node ID
     * @param to_pin_name Target pin name
     */
    VScriptConnection(const std::string& from_node_id, const std::string& from_pin_name,
                     const std::string& to_node_id, const std::string& to_pin_name);

    /**
     * @brief Get source node ID
     */
    const std::string& GetFromNodeId() const { return m_from_node_id; }

    /**
     * @brief Get source pin name
     */
    const std::string& GetFromPinName() const { return m_from_pin_name; }

    /**
     * @brief Get target node ID
     */
    const std::string& GetToNodeId() const { return m_to_node_id; }

    /**
     * @brief Get target pin name
     */
    const std::string& GetToPinName() const { return m_to_pin_name; }

    /**
     * @brief Check if this connection matches the given endpoints
     * @param from_node_id Source node ID
     * @param from_pin_name Source pin name
     * @param to_node_id Target node ID
     * @param to_pin_name Target pin name
     * @return True if connection matches
     */
    bool Matches(const std::string& from_node_id, const std::string& from_pin_name,
                const std::string& to_node_id, const std::string& to_pin_name) const;

    /**
     * @brief Check if this connection involves the given node
     * @param node_id Node ID to check
     * @return True if connection involves this node
     */
    bool InvolvesNode(const std::string& node_id) const;

    /**
     * @brief Check if this connection involves the given pin
     * @param node_id Node ID
     * @param pin_name Pin name
     * @return True if connection involves this pin
     */
    bool InvolvesPin(const std::string& node_id, const std::string& pin_name) const;

    /**
     * @brief Check if this connection starts from the given pin
     * @param node_id Node ID
     * @param pin_name Pin name
     * @return True if connection starts from this pin
     */
    bool StartsFromPin(const std::string& node_id, const std::string& pin_name) const;

    /**
     * @brief Check if this connection ends at the given pin
     * @param node_id Node ID
     * @param pin_name Pin name
     * @return True if connection ends at this pin
     */
    bool EndsAtPin(const std::string& node_id, const std::string& pin_name) const;

    /**
     * @brief Serialize connection to JSON
     */
    nlohmann::json ToJson() const;

    /**
     * @brief Deserialize connection from JSON
     */
    void FromJson(const nlohmann::json& json);

    /**
     * @brief Equality operator
     */
    bool operator==(const VScriptConnection& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const VScriptConnection& other) const;

private:
    std::string m_from_node_id;        ///< Source node ID
    std::string m_from_pin_name;       ///< Source pin name
    std::string m_to_node_id;          ///< Target node ID
    std::string m_to_pin_name;         ///< Target pin name
};

} // namespace Lupine
