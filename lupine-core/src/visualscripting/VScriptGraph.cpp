#include "lupine/visualscripting/VScriptGraph.h"
#include "lupine/visualscripting/VScriptNode.h"
#include "lupine/visualscripting/VScriptConnection.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace Lupine {

VScriptGraph::VScriptGraph()
    : m_name("Untitled Graph")
    , m_description("")
    , m_version("1.0")
    , m_next_node_id(1)
{
}

VScriptGraph::~VScriptGraph() {
    Clear();
}

VScriptNode* VScriptGraph::AddNode(std::unique_ptr<VScriptNode> node) {
    if (!node) {
        return nullptr;
    }

    VScriptNode* node_ptr = node.get();
    m_node_lookup[node->GetId()] = node_ptr;
    m_nodes.push_back(std::move(node));
    
    return node_ptr;
}

bool VScriptGraph::RemoveNode(const std::string& node_id) {
    // Remove all connections involving this node
    auto it = m_connections.begin();
    while (it != m_connections.end()) {
        if ((*it)->InvolvesNode(node_id)) {
            it = m_connections.erase(it);
        } else {
            ++it;
        }
    }

    // Remove the node
    auto node_it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&node_id](const std::unique_ptr<VScriptNode>& node) {
            return node->GetId() == node_id;
        });

    if (node_it != m_nodes.end()) {
        m_node_lookup.erase(node_id);
        m_nodes.erase(node_it);
        return true;
    }

    return false;
}

VScriptNode* VScriptGraph::GetNode(const std::string& node_id) const {
    auto it = m_node_lookup.find(node_id);
    return (it != m_node_lookup.end()) ? it->second : nullptr;
}

std::vector<VScriptNode*> VScriptGraph::GetNodes() const {
    std::vector<VScriptNode*> nodes;
    nodes.reserve(m_nodes.size());
    
    for (const auto& node : m_nodes) {
        nodes.push_back(node.get());
    }
    
    return nodes;
}

VScriptConnection* VScriptGraph::AddConnection(std::unique_ptr<VScriptConnection> connection) {
    if (!connection) {
        return nullptr;
    }

    // Validate that the nodes and pins exist
    VScriptNode* from_node = GetNode(connection->GetFromNodeId());
    VScriptNode* to_node = GetNode(connection->GetToNodeId());
    
    if (!from_node || !to_node) {
        return nullptr;
    }

    VScriptPin* from_pin = from_node->GetPin(connection->GetFromPinName());
    VScriptPin* to_pin = to_node->GetPin(connection->GetToPinName());
    
    if (!from_pin || !to_pin) {
        return nullptr;
    }

    // Check pin compatibility
    if (!from_pin->IsCompatibleWith(*to_pin)) {
        return nullptr;
    }

    VScriptConnection* connection_ptr = connection.get();
    m_connections.push_back(std::move(connection));
    
    return connection_ptr;
}

bool VScriptGraph::RemoveConnection(const std::string& from_node_id, const std::string& from_pin_name,
                                   const std::string& to_node_id, const std::string& to_pin_name) {
    auto it = std::find_if(m_connections.begin(), m_connections.end(),
        [&](const std::unique_ptr<VScriptConnection>& conn) {
            return conn->Matches(from_node_id, from_pin_name, to_node_id, to_pin_name);
        });

    if (it != m_connections.end()) {
        m_connections.erase(it);
        return true;
    }

    return false;
}

std::vector<VScriptConnection*> VScriptGraph::GetConnections() const {
    std::vector<VScriptConnection*> connections;
    connections.reserve(m_connections.size());
    
    for (const auto& connection : m_connections) {
        connections.push_back(connection.get());
    }
    
    return connections;
}

std::vector<VScriptConnection*> VScriptGraph::GetConnectionsFromPin(const std::string& node_id, const std::string& pin_name) const {
    std::vector<VScriptConnection*> connections;
    
    for (const auto& connection : m_connections) {
        if (connection->StartsFromPin(node_id, pin_name)) {
            connections.push_back(connection.get());
        }
    }
    
    return connections;
}

std::vector<VScriptConnection*> VScriptGraph::GetConnectionsToPin(const std::string& node_id, const std::string& pin_name) const {
    std::vector<VScriptConnection*> connections;
    
    for (const auto& connection : m_connections) {
        if (connection->EndsAtPin(node_id, pin_name)) {
            connections.push_back(connection.get());
        }
    }
    
    return connections;
}

void VScriptGraph::Clear() {
    m_connections.clear();
    m_nodes.clear();
    m_node_lookup.clear();
    m_next_node_id = 1;
}

bool VScriptGraph::IsValid() const {
    // TODO: Implement graph validation
    // - Check for cycles in execution flow
    // - Validate all connections
    // - Check for orphaned nodes
    return true;
}

std::vector<VScriptNode*> VScriptGraph::GetEntryPoints() const {
    std::vector<VScriptNode*> entry_points;
    
    for (const auto& node : m_nodes) {
        auto exec_inputs = node->GetExecutionInputPins();
        bool has_input_connections = false;
        
        for (const auto& pin : exec_inputs) {
            auto connections = GetConnectionsToPin(node->GetId(), pin->GetName());
            if (!connections.empty()) {
                has_input_connections = true;
                break;
            }
        }
        
        if (!has_input_connections && !exec_inputs.empty()) {
            entry_points.push_back(node.get());
        }
    }
    
    return entry_points;
}

bool VScriptGraph::LoadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[VScriptGraph] Failed to open file: " << filepath << std::endl;
            return false;
        }

        nlohmann::json json;
        file >> json;
        file.close();

        return FromJson(json);
    } catch (const std::exception& e) {
        std::cerr << "[VScriptGraph] Error loading file: " << e.what() << std::endl;
        return false;
    }
}

bool VScriptGraph::SaveToFile(const std::string& filepath) const {
    try {
        nlohmann::json json = ToJson();
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[VScriptGraph] Failed to create file: " << filepath << std::endl;
            return false;
        }

        file << json.dump(2);  // Pretty print with 2-space indentation
        file.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[VScriptGraph] Error saving file: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::json VScriptGraph::ToJson() const {
    nlohmann::json json;
    
    json["name"] = m_name;
    json["description"] = m_description;
    json["version"] = m_version;
    
    // Serialize nodes
    json["nodes"] = nlohmann::json::array();
    for (const auto& node : m_nodes) {
        json["nodes"].push_back(node->ToJson());
    }
    
    // Serialize connections
    json["connections"] = nlohmann::json::array();
    for (const auto& connection : m_connections) {
        json["connections"].push_back(connection->ToJson());
    }
    
    return json;
}

bool VScriptGraph::FromJson(const nlohmann::json& json) {
    try {
        Clear();
        
        if (json.contains("name")) {
            m_name = json["name"].get<std::string>();
        }
        
        if (json.contains("description")) {
            m_description = json["description"].get<std::string>();
        }
        
        if (json.contains("version")) {
            m_version = json["version"].get<std::string>();
        }
        
        // TODO: Deserialize nodes and connections
        // This will require a node factory to create the correct node types
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[VScriptGraph] Error deserializing JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string VScriptGraph::GenerateNodeId() const {
    return "node_" + std::to_string(m_next_node_id++);
}

} // namespace Lupine
