#include "lupine/visualscripting/VScriptConnection.h"

namespace Lupine {

VScriptConnection::VScriptConnection(const std::string& from_node_id, const std::string& from_pin_name,
                                   const std::string& to_node_id, const std::string& to_pin_name)
    : m_from_node_id(from_node_id)
    , m_from_pin_name(from_pin_name)
    , m_to_node_id(to_node_id)
    , m_to_pin_name(to_pin_name)
{
}

bool VScriptConnection::Matches(const std::string& from_node_id, const std::string& from_pin_name,
                               const std::string& to_node_id, const std::string& to_pin_name) const {
    return m_from_node_id == from_node_id && 
           m_from_pin_name == from_pin_name &&
           m_to_node_id == to_node_id && 
           m_to_pin_name == to_pin_name;
}

bool VScriptConnection::InvolvesNode(const std::string& node_id) const {
    return m_from_node_id == node_id || m_to_node_id == node_id;
}

bool VScriptConnection::InvolvesPin(const std::string& node_id, const std::string& pin_name) const {
    return (m_from_node_id == node_id && m_from_pin_name == pin_name) ||
           (m_to_node_id == node_id && m_to_pin_name == pin_name);
}

bool VScriptConnection::StartsFromPin(const std::string& node_id, const std::string& pin_name) const {
    return m_from_node_id == node_id && m_from_pin_name == pin_name;
}

bool VScriptConnection::EndsAtPin(const std::string& node_id, const std::string& pin_name) const {
    return m_to_node_id == node_id && m_to_pin_name == pin_name;
}

nlohmann::json VScriptConnection::ToJson() const {
    nlohmann::json json;
    json["from_node_id"] = m_from_node_id;
    json["from_pin_name"] = m_from_pin_name;
    json["to_node_id"] = m_to_node_id;
    json["to_pin_name"] = m_to_pin_name;
    return json;
}

void VScriptConnection::FromJson(const nlohmann::json& json) {
    if (json.contains("from_node_id")) {
        m_from_node_id = json["from_node_id"].get<std::string>();
    }
    if (json.contains("from_pin_name")) {
        m_from_pin_name = json["from_pin_name"].get<std::string>();
    }
    if (json.contains("to_node_id")) {
        m_to_node_id = json["to_node_id"].get<std::string>();
    }
    if (json.contains("to_pin_name")) {
        m_to_pin_name = json["to_pin_name"].get<std::string>();
    }
}

bool VScriptConnection::operator==(const VScriptConnection& other) const {
    return Matches(other.m_from_node_id, other.m_from_pin_name, 
                  other.m_to_node_id, other.m_to_pin_name);
}

bool VScriptConnection::operator!=(const VScriptConnection& other) const {
    return !(*this == other);
}

} // namespace Lupine
