#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace Lupine {

// Forward declarations
class VScriptNode;
class VScriptConnection;

/**
 * @brief Visual script graph containing nodes and connections
 * 
 * Represents a complete visual script as a graph of interconnected nodes.
 * Can be serialized to/from JSON (.vscript files) and converted to Python code.
 */
class VScriptGraph {
public:
    /**
     * @brief Constructor
     */
    VScriptGraph();

    /**
     * @brief Destructor
     */
    ~VScriptGraph();

    /**
     * @brief Add a node to the graph
     * @param node Node to add (takes ownership)
     * @return Pointer to the added node
     */
    VScriptNode* AddNode(std::unique_ptr<VScriptNode> node);

    /**
     * @brief Remove a node from the graph
     * @param node_id ID of the node to remove
     * @return True if node was found and removed
     */
    bool RemoveNode(const std::string& node_id);

    /**
     * @brief Get a node by ID
     * @param node_id ID of the node to find
     * @return Pointer to the node or nullptr if not found
     */
    VScriptNode* GetNode(const std::string& node_id) const;

    /**
     * @brief Get all nodes in the graph
     * @return Vector of node pointers
     */
    std::vector<VScriptNode*> GetNodes() const;

    /**
     * @brief Add a connection between two pins
     * @param connection Connection to add (takes ownership)
     * @return Pointer to the added connection
     */
    VScriptConnection* AddConnection(std::unique_ptr<VScriptConnection> connection);

    /**
     * @brief Remove a connection
     * @param from_node_id Source node ID
     * @param from_pin_name Source pin name
     * @param to_node_id Target node ID
     * @param to_pin_name Target pin name
     * @return True if connection was found and removed
     */
    bool RemoveConnection(const std::string& from_node_id, const std::string& from_pin_name,
                         const std::string& to_node_id, const std::string& to_pin_name);

    /**
     * @brief Get all connections in the graph
     * @return Vector of connection pointers
     */
    std::vector<VScriptConnection*> GetConnections() const;

    /**
     * @brief Get connections from a specific pin
     * @param node_id Node ID
     * @param pin_name Pin name
     * @return Vector of connections from this pin
     */
    std::vector<VScriptConnection*> GetConnectionsFromPin(const std::string& node_id, const std::string& pin_name) const;

    /**
     * @brief Get connections to a specific pin
     * @param node_id Node ID
     * @param pin_name Pin name
     * @return Vector of connections to this pin
     */
    std::vector<VScriptConnection*> GetConnectionsToPin(const std::string& node_id, const std::string& pin_name) const;

    /**
     * @brief Clear all nodes and connections
     */
    void Clear();

    /**
     * @brief Check if the graph is valid (no cycles, proper connections, etc.)
     * @return True if graph is valid
     */
    bool IsValid() const;

    /**
     * @brief Get entry points (nodes with no input execution pins)
     * @return Vector of entry point nodes
     */
    std::vector<VScriptNode*> GetEntryPoints() const;

    /**
     * @brief Load graph from JSON file
     * @param filepath Path to .vscript file
     * @return True if loaded successfully
     */
    bool LoadFromFile(const std::string& filepath);

    /**
     * @brief Save graph to JSON file
     * @param filepath Path to .vscript file
     * @return True if saved successfully
     */
    bool SaveToFile(const std::string& filepath) const;

    /**
     * @brief Serialize graph to JSON
     * @return JSON representation
     */
    nlohmann::json ToJson() const;

    /**
     * @brief Deserialize graph from JSON
     * @param json JSON data
     * @return True if deserialized successfully
     */
    bool FromJson(const nlohmann::json& json);

    /**
     * @brief Get graph metadata
     */
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    const std::string& GetDescription() const { return m_description; }
    void SetDescription(const std::string& description) { m_description = description; }

    const std::string& GetVersion() const { return m_version; }
    void SetVersion(const std::string& version) { m_version = version; }

private:
    /**
     * @brief Generate unique node ID
     * @return Unique node ID string
     */
    std::string GenerateNodeId() const;

    std::string m_name;                                                    ///< Graph name
    std::string m_description;                                             ///< Graph description
    std::string m_version;                                                 ///< Graph version
    
    std::vector<std::unique_ptr<VScriptNode>> m_nodes;                     ///< All nodes in the graph
    std::vector<std::unique_ptr<VScriptConnection>> m_connections;         ///< All connections in the graph
    
    std::unordered_map<std::string, VScriptNode*> m_node_lookup;           ///< Fast node lookup by ID
    
    mutable int m_next_node_id;                                            ///< Counter for generating unique node IDs
};

} // namespace Lupine
