#include "lupine/animation/PropertySystem.h"
#include "lupine/core/Scene.h"
#include "lupine/serialization/JsonUtils.h"
#include <chrono>
#include <sstream>

namespace Lupine {

// PropertyStateManager implementation
PropertySnapshot PropertyStateManager::CaptureNodeState(Node* node, const PropertyFilter& filter) const {
    if (!node) return PropertySnapshot();
    
    PropertySnapshot snapshot;
    snapshot.nodePath = node->GetName(); // In a full implementation, this would be the full path
    snapshot.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    // Discover all properties for this node
    auto properties = m_reflectionSystem.DiscoverProperties(node);
    
    // Capture values for properties that pass the filter
    for (const auto& propDesc : properties) {
        if (filter.ShouldIncludeProperty(propDesc)) {
            EnhancedAnimationValue value = m_reflectionSystem.GetPropertyValue(node, propDesc.name);
            if (value.IsValid()) {
                snapshot.properties[propDesc.name] = value;
            }
        }
    }
    
    return snapshot;
}

SceneSnapshot PropertyStateManager::CaptureSceneState(Scene* scene, const PropertyFilter& filter) const {
    if (!scene) return SceneSnapshot();
    
    SceneSnapshot snapshot;
    snapshot.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    snapshot.description = "Scene state at " + std::to_string(snapshot.timestamp);
    
    // Capture state for all nodes in the scene
    std::function<void(Node*)> captureNodeRecursive = [&](Node* node) {
        if (!node) return;

        PropertySnapshot nodeSnapshot = CaptureNodeState(node, filter);
        if (!nodeSnapshot.properties.empty()) {
            snapshot.nodeSnapshots[nodeSnapshot.nodePath] = nodeSnapshot;
        }

        // Recursively capture child nodes
        for (const auto& child : node->GetChildren()) {
            captureNodeRecursive(child.get());
        }
    };
    
    if (scene->GetRootNode()) {
        captureNodeRecursive(scene->GetRootNode());
    }
    
    return snapshot;
}

bool PropertyStateManager::RestoreNodeState(Node* node, const PropertySnapshot& snapshot) const {
    if (!node || snapshot.properties.empty()) return false;
    
    bool success = true;
    for (const auto& [propertyName, value] : snapshot.properties) {
        if (!m_reflectionSystem.SetPropertyValue(node, propertyName, value)) {
            success = false;
        }
    }
    
    return success;
}

bool PropertyStateManager::RestoreSceneState(Scene* scene, const SceneSnapshot& snapshot) const {
    if (!scene || snapshot.nodeSnapshots.empty()) return false;
    
    bool success = true;
    
    // Find nodes by path and restore their states
    std::function<void(Node*)> restoreNodeRecursive = [&](Node* node) {
        if (!node) return;

        auto it = snapshot.nodeSnapshots.find(node->GetName());
        if (it != snapshot.nodeSnapshots.end()) {
            if (!RestoreNodeState(node, it->second)) {
                success = false;
            }
        }

        // Recursively restore child nodes
        for (const auto& child : node->GetChildren()) {
            restoreNodeRecursive(child.get());
        }
    };
    
    if (scene->GetRootNode()) {
        restoreNodeRecursive(scene->GetRootNode());
    }
    
    return success;
}

std::vector<PropertyChangeEvent> PropertyStateManager::CompareStates(const PropertySnapshot& oldState, const PropertySnapshot& newState) const {
    std::vector<PropertyChangeEvent> changes;
    
    // Check for changed properties
    for (const auto& [propertyName, newValue] : newState.properties) {
        auto oldIt = oldState.properties.find(propertyName);
        if (oldIt != oldState.properties.end()) {
            // Property exists in both states, check if it changed
            const auto& oldValue = oldIt->second;
            if (oldValue.ToString() != newValue.ToString()) {
                changes.emplace_back(newState.nodePath, propertyName, oldValue, newValue, newState.timestamp);
            }
        } else {
            // Property is new
            EnhancedAnimationValue defaultValue;
            changes.emplace_back(newState.nodePath, propertyName, defaultValue, newValue, newState.timestamp);
        }
    }
    
    // Check for removed properties
    for (const auto& [propertyName, oldValue] : oldState.properties) {
        auto newIt = newState.properties.find(propertyName);
        if (newIt == newState.properties.end()) {
            // Property was removed
            EnhancedAnimationValue defaultValue;
            changes.emplace_back(newState.nodePath, propertyName, oldValue, defaultValue, newState.timestamp);
        }
    }
    
    return changes;
}

std::vector<PropertyChangeEvent> PropertyStateManager::CompareSceneStates(const SceneSnapshot& oldState, const SceneSnapshot& newState) const {
    std::vector<PropertyChangeEvent> allChanges;
    
    // Compare each node's state
    for (const auto& [nodePath, newNodeState] : newState.nodeSnapshots) {
        auto oldIt = oldState.nodeSnapshots.find(nodePath);
        if (oldIt != oldState.nodeSnapshots.end()) {
            auto nodeChanges = CompareStates(oldIt->second, newNodeState);
            allChanges.insert(allChanges.end(), nodeChanges.begin(), nodeChanges.end());
        } else {
            // Node is new, all properties are changes
            for (const auto& [propertyName, value] : newNodeState.properties) {
                EnhancedAnimationValue defaultValue;
                allChanges.emplace_back(nodePath, propertyName, defaultValue, value, newState.timestamp);
            }
        }
    }
    
    // Check for removed nodes
    for (const auto& [nodePath, oldNodeState] : oldState.nodeSnapshots) {
        auto newIt = newState.nodeSnapshots.find(nodePath);
        if (newIt == newState.nodeSnapshots.end()) {
            // Node was removed, all properties are changes
            for (const auto& [propertyName, value] : oldNodeState.properties) {
                EnhancedAnimationValue defaultValue;
                allChanges.emplace_back(nodePath, propertyName, value, defaultValue, newState.timestamp);
            }
        }
    }
    
    return allChanges;
}

std::string PropertyStateManager::SerializeSnapshot(const PropertySnapshot& snapshot) const {
    JsonNode json;
    json["node_path"] = snapshot.nodePath;
    json["timestamp"] = snapshot.timestamp;
    
    JsonNode propertiesJson;
    for (const auto& [propertyName, value] : snapshot.properties) {
        JsonNode propJson;
        propJson["type"] = static_cast<int>(value.type);
        propJson["value"] = value.ToString();
        propertiesJson[propertyName] = propJson;
    }
    json["properties"] = propertiesJson;
    
    return JsonUtils::Stringify(json, true);
}

std::string PropertyStateManager::SerializeSceneSnapshot(const SceneSnapshot& snapshot) const {
    JsonNode json;
    json["timestamp"] = snapshot.timestamp;
    json["description"] = snapshot.description;
    
    JsonNode nodesJson;
    for (const auto& [nodePath, nodeSnapshot] : snapshot.nodeSnapshots) {
        JsonNode nodeJson;
        nodeJson["timestamp"] = nodeSnapshot.timestamp;
        
        JsonNode propertiesJson;
        for (const auto& [propertyName, value] : nodeSnapshot.properties) {
            JsonNode propJson;
            propJson["type"] = static_cast<int>(value.type);
            propJson["value"] = value.ToString();
            propertiesJson[propertyName] = propJson;
        }
        nodeJson["properties"] = propertiesJson;
        nodesJson[nodePath] = nodeJson;
    }
    json["nodes"] = nodesJson;
    
    return JsonUtils::Stringify(json, true);
}

PropertySnapshot PropertyStateManager::DeserializeSnapshot(const std::string& data) const {
    PropertySnapshot snapshot;
    
    try {
        JsonNode json = JsonUtils::Parse(data);
        
        snapshot.nodePath = json["node_path"].AsString();
        snapshot.timestamp = static_cast<float>(json["timestamp"].AsDouble());
        
        if (json.HasKey("properties")) {
            JsonNode propertiesJson = json["properties"];
            for (const auto& [propertyName, propJson] : propertiesJson.AsObject()) {
                ExportVariableType type = static_cast<ExportVariableType>(propJson["type"].AsInt());
                std::string valueStr = propJson["value"].AsString();
                
                EnhancedAnimationValue value = EnhancedAnimationValue::FromString(valueStr, type);
                snapshot.properties[propertyName] = value;
            }
        }
    } catch (const std::exception&) {
        // Return empty snapshot on parse error
        snapshot = PropertySnapshot();
    }
    
    return snapshot;
}

SceneSnapshot PropertyStateManager::DeserializeSceneSnapshot(const std::string& data) const {
    SceneSnapshot snapshot;
    
    try {
        JsonNode json = JsonUtils::Parse(data);
        
        snapshot.timestamp = static_cast<float>(json["timestamp"].AsDouble());
        snapshot.description = json["description"].AsString();

        if (json.HasKey("nodes")) {
            JsonNode nodesJson = json["nodes"];
            for (const auto& [nodePath, nodeJson] : nodesJson.AsObject()) {
                PropertySnapshot nodeSnapshot;
                nodeSnapshot.nodePath = nodePath;
                nodeSnapshot.timestamp = static_cast<float>(nodeJson["timestamp"].AsDouble());
                
                if (nodeJson.HasKey("properties")) {
                    JsonNode propertiesJson = nodeJson["properties"];
                    for (const auto& [propertyName, propJson] : propertiesJson.AsObject()) {
                        ExportVariableType type = static_cast<ExportVariableType>(propJson["type"].AsInt());
                        std::string valueStr = propJson["value"].AsString();
                        
                        EnhancedAnimationValue value = EnhancedAnimationValue::FromString(valueStr, type);
                        nodeSnapshot.properties[propertyName] = value;
                    }
                }
                
                snapshot.nodeSnapshots[nodePath] = nodeSnapshot;
            }
        }
    } catch (const std::exception&) {
        // Return empty snapshot on parse error
        snapshot = SceneSnapshot();
    }
    
    return snapshot;
}

} // namespace Lupine
