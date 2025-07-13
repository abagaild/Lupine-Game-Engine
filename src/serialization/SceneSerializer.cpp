#include "lupine/serialization/SceneSerializer.h"
#include "lupine/serialization/JsonUtils.h"
#include "lupine/serialization/SerializationUtils.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"

#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Label.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/scripting/LuaScriptComponent.h"
#include "lupine/scripting/PythonScriptComponent.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

namespace Lupine {

bool SceneSerializer::SerializeToFile(const Scene* scene, const std::string& file_path) {
    if (!scene) {
        std::cerr << "Cannot serialize null scene" << std::endl;
        return false;
    }

    try {
        JsonNode sceneJson;
        sceneJson["scene"]["name"] = scene->GetName();
        sceneJson["scene"]["uuid"] = scene->GetUUID().ToString();
        sceneJson["scene"]["version"] = "1.0";
        sceneJson["scene"]["created"] = std::to_string(std::time(nullptr));

        if (scene->GetRootNode()) {
            sceneJson["scene"]["root_node"] = SerializeNode(scene->GetRootNode());
        }

        return JsonUtils::SaveToFile(sceneJson, file_path, true);
    } catch (const std::exception& e) {
        std::cerr << "Error serializing scene: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<Scene> SceneSerializer::DeserializeFromFile(const std::string& file_path) {
    try {
        JsonNode sceneJson = JsonUtils::LoadFromFile(file_path);
        return DeserializeFromString(JsonUtils::Stringify(sceneJson));
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing scene from file: " << e.what() << std::endl;
        return nullptr;
    }
}

std::string SceneSerializer::SerializeToString(const Scene* scene) {
    if (!scene) {
        return "{}";
    }

    try {
        JsonNode sceneJson;
        sceneJson["scene"]["name"] = scene->GetName();
        sceneJson["scene"]["uuid"] = scene->GetUUID().ToString();
        sceneJson["scene"]["version"] = "1.0";
        sceneJson["scene"]["created"] = std::to_string(std::time(nullptr));

        if (scene->GetRootNode()) {
            sceneJson["scene"]["root_node"] = SerializeNode(scene->GetRootNode());
        }

        return JsonUtils::Stringify(sceneJson, true);
    } catch (const std::exception& e) {
        std::cerr << "Error serializing scene to string: " << e.what() << std::endl;
        return "{}";
    }
}

std::unique_ptr<Scene> SceneSerializer::DeserializeFromString(const std::string& json_string) {
    try {
        JsonNode sceneJson = JsonUtils::Parse(json_string);

        if (!sceneJson.HasKey("scene")) {
            std::cerr << "Invalid scene JSON: missing 'scene' key" << std::endl;
            return nullptr;
        }

        const JsonNode& sceneData = sceneJson["scene"];

        std::string sceneName = sceneData.HasKey("name") ? sceneData["name"].AsString() : "Untitled Scene";
        auto scene = std::make_unique<Scene>(sceneName);

        // Note: Scene UUID is generated in constructor, we don't set it from file

        if (sceneData.HasKey("root_node")) {
            auto rootNode = DeserializeNode(sceneData["root_node"]);
            if (rootNode) {
                scene->SetRootNode(std::move(rootNode));
            }
        }

        std::cout << "Successfully deserialized scene: " << sceneName << std::endl;

        // Debug: Check what was actually loaded
        if (scene->GetRootNode()) {
            std::cout << "Root node: " << scene->GetRootNode()->GetName() << std::endl;
            std::cout << "Root node children count: " << scene->GetRootNode()->GetChildren().size() << std::endl;
            for (const auto& child : scene->GetRootNode()->GetChildren()) {
                std::cout << "  Child: " << child->GetName() << " (components: " << child->GetAllComponents().size() << ")" << std::endl;
            }
        } else {
            std::cout << "Warning: No root node in deserialized scene!" << std::endl;
        }

        return scene;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing scene from string: " << e.what() << std::endl;
        return nullptr;
    }
}

JsonNode SceneSerializer::SerializeNode(const Node* node) {
    if (!node) {
        return JsonNode();
    }

    JsonNode nodeJson;
    nodeJson["type"] = node->GetTypeName();
    nodeJson["name"] = node->GetName();
    nodeJson["uuid"] = node->GetUUID().ToString();
    nodeJson["active"] = node->IsActive();
    nodeJson["visible"] = node->IsVisible();

    // Serialize transform properties for Node2D, Node3D, and Control
    if (auto* node2d = dynamic_cast<const Node2D*>(node)) {
        JsonNode transform;
        transform["position"] = SerializationUtils::SerializeVec2(node2d->GetPosition());
        transform["rotation"] = node2d->GetRotation();
        transform["scale"] = SerializationUtils::SerializeVec2(node2d->GetScale());
        nodeJson["transform"] = transform;
    } else if (auto* node3d = dynamic_cast<const Node3D*>(node)) {
        JsonNode transform;
        transform["position"] = SerializationUtils::SerializeVec3(node3d->GetPosition());
        transform["rotation"] = SerializationUtils::SerializeQuat(node3d->GetRotation());
        transform["scale"] = SerializationUtils::SerializeVec3(node3d->GetScale());
        nodeJson["transform"] = transform;
    } else if (auto* control = dynamic_cast<const Control*>(node)) {
        JsonNode controlData;
        controlData["position"] = SerializationUtils::SerializeVec2(control->GetPosition());
        controlData["size"] = SerializationUtils::SerializeVec2(control->GetSize());
        controlData["world_space"] = control->GetWorldSpace();
        nodeJson["control_data"] = controlData;
    }

    // Serialize components
    const auto& components = node->GetAllComponents();
    if (!components.empty()) {
        JsonNode componentsArray;
        componentsArray.value = std::vector<JsonNode>();
        for (const auto& component : components) {
            componentsArray.Push(SerializeComponent(component.get()));
        }
        nodeJson["components"] = componentsArray;
    }

    // Serialize children
    const auto& children = node->GetChildren();
    if (!children.empty()) {
        JsonNode childrenArray;
        childrenArray.value = std::vector<JsonNode>();
        for (const auto& child : children) {
            childrenArray.Push(SerializeNode(child.get()));
        }
        nodeJson["children"] = childrenArray;
    }

    return nodeJson;
}

JsonNode SceneSerializer::SerializeComponent(const Component* component) {
    if (!component) {
        return JsonNode();
    }

    JsonNode componentJson;
    componentJson["type"] = component->GetTypeName();
    componentJson["name"] = component->GetName();
    componentJson["uuid"] = component->GetUUID().ToString();
    componentJson["active"] = component->IsActive();
    componentJson["category"] = component->GetCategory();

    // Serialize export variables
    const auto& exportVars = component->GetExportVariables();
    if (!exportVars.empty()) {
        JsonNode exportVarsJson;
        exportVarsJson.value = std::map<std::string, JsonNode>();
        for (const auto& [name, var] : exportVars) {
            JsonNode varJson;
            varJson["type"] = SerializationUtils::ExportVariableTypeToString(var.type);
            varJson["value"] = SerializationUtils::SerializeExportValue(var.value);
            varJson["description"] = var.description;
            exportVarsJson.AsObject()[name] = varJson;
        }
        componentJson["export_variables"] = exportVarsJson;
    }

    return componentJson;
}

std::unique_ptr<Node> SceneSerializer::CreateNodeByType(const std::string& type_name, const std::string& name) {
    if (type_name == "Node2D") {
        return std::make_unique<Node2D>(name);
    } else if (type_name == "Node3D") {
        return std::make_unique<Node3D>(name);
    } else if (type_name == "Control") {
        return std::make_unique<Control>(name);
    } else {
        return std::make_unique<Node>(name);
    }
}

std::unique_ptr<Node> SceneSerializer::DeserializeNode(const JsonNode& nodeData) {
    if (!nodeData.HasKey("type") || !nodeData.HasKey("name")) {
        std::cerr << "Error: Node missing required type or name" << std::endl;
        return nullptr;
    }

    std::string nodeType = nodeData["type"].AsString();
    std::string nodeName = nodeData["name"].AsString();

    std::cout << "Deserializing node: " << nodeName << " (type: " << nodeType << ")" << std::endl;

    // Create node by type
    auto node = CreateNodeByType(nodeType, nodeName);
    if (!node) {
        std::cerr << "Error: Failed to create node of type: " << nodeType << std::endl;
        return nullptr;
    }

    // Set UUID from file if present
    if (nodeData.HasKey("uuid")) {
        std::string uuidStr = nodeData["uuid"].AsString();
        node->SetUUID(UUID::FromString(uuidStr));
        std::cout << "Set node UUID: " << uuidStr << " for node: " << nodeName << std::endl;
    }

    // Set active state
    if (nodeData.HasKey("active")) {
        node->SetActive(nodeData["active"].AsBool());
    }

    // Set visibility state
    if (nodeData.HasKey("visible")) {
        node->SetVisible(nodeData["visible"].AsBool());
    }

    // Deserialize transform properties for Node2D and Node3D
    if (nodeData.HasKey("transform")) {
        const JsonNode& transformData = nodeData["transform"];

        if (auto* node2d = dynamic_cast<Node2D*>(node.get())) {
            if (transformData.HasKey("position")) {
                const JsonNode& posNode = transformData["position"];
                if (posNode.IsArray()) {
                    const auto& arr = posNode.AsArray();
                    if (arr.size() >= 2) {
                        node2d->SetPosition(glm::vec2(arr[0].AsDouble(), arr[1].AsDouble()));
                    }
                } else if (posNode.IsString()) {
                    node2d->SetPosition(SerializationUtils::DeserializeVec2(posNode.AsString()));
                }
            }
            if (transformData.HasKey("rotation")) {
                node2d->SetRotation(static_cast<float>(transformData["rotation"].AsDouble()));
            }
            if (transformData.HasKey("scale")) {
                const JsonNode& scaleNode = transformData["scale"];
                if (scaleNode.IsArray()) {
                    const auto& arr = scaleNode.AsArray();
                    if (arr.size() >= 2) {
                        node2d->SetScale(glm::vec2(arr[0].AsDouble(), arr[1].AsDouble()));
                    }
                } else if (scaleNode.IsString()) {
                    node2d->SetScale(SerializationUtils::DeserializeVec2(scaleNode.AsString()));
                }
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node.get())) {
            if (transformData.HasKey("position")) {
                const JsonNode& posNode = transformData["position"];
                if (posNode.IsArray()) {
                    const auto& arr = posNode.AsArray();
                    if (arr.size() >= 3) {
                        node3d->SetPosition(glm::vec3(arr[0].AsDouble(), arr[1].AsDouble(), arr[2].AsDouble()));
                    }
                } else if (posNode.IsString()) {
                    node3d->SetPosition(SerializationUtils::DeserializeVec3(posNode.AsString()));
                }
            }
            if (transformData.HasKey("rotation")) {
                const JsonNode& rotNode = transformData["rotation"];
                if (rotNode.IsArray()) {
                    const auto& arr = rotNode.AsArray();
                    if (arr.size() >= 4) {
                        node3d->SetRotation(glm::quat(arr[0].AsDouble(), arr[1].AsDouble(), arr[2].AsDouble(), arr[3].AsDouble()));
                    }
                } else if (rotNode.IsString()) {
                    node3d->SetRotation(SerializationUtils::DeserializeQuat(rotNode.AsString()));
                }
            }
            if (transformData.HasKey("scale")) {
                const JsonNode& scaleNode = transformData["scale"];
                if (scaleNode.IsArray()) {
                    const auto& arr = scaleNode.AsArray();
                    if (arr.size() >= 3) {
                        node3d->SetScale(glm::vec3(arr[0].AsDouble(), arr[1].AsDouble(), arr[2].AsDouble()));
                    }
                } else if (scaleNode.IsString()) {
                    node3d->SetScale(SerializationUtils::DeserializeVec3(scaleNode.AsString()));
                }
            }
        }
    }

    // Deserialize Control properties
    if (nodeData.HasKey("control_data")) {
        const JsonNode& controlData = nodeData["control_data"];

        if (auto* control = dynamic_cast<Control*>(node.get())) {
            if (controlData.HasKey("position")) {
                const JsonNode& posNode = controlData["position"];
                if (posNode.IsArray()) {
                    const auto& arr = posNode.AsArray();
                    if (arr.size() >= 2) {
                        control->SetPosition(glm::vec2(arr[0].AsDouble(), arr[1].AsDouble()));
                    }
                } else if (posNode.IsString()) {
                    control->SetPosition(SerializationUtils::DeserializeVec2(posNode.AsString()));
                }
            }
            if (controlData.HasKey("size")) {
                const JsonNode& sizeNode = controlData["size"];
                if (sizeNode.IsArray()) {
                    const auto& arr = sizeNode.AsArray();
                    if (arr.size() >= 2) {
                        control->SetSize(glm::vec2(arr[0].AsDouble(), arr[1].AsDouble()));
                    }
                } else if (sizeNode.IsString()) {
                    control->SetSize(SerializationUtils::DeserializeVec2(sizeNode.AsString()));
                }
            }
            if (controlData.HasKey("world_space")) {
                control->SetWorldSpace(controlData["world_space"].AsBool());
            }
        }
    }

    // Deserialize components
    if (nodeData.HasKey("components") && nodeData["components"].IsArray()) {
        const auto& componentsArray = nodeData["components"].AsArray();
        std::cout << "Found " << componentsArray.size() << " components for node: " << nodeName << std::endl;

        for (const auto& componentData : componentsArray) {
            if (componentData.HasKey("type")) {
                std::string componentType = componentData["type"].AsString();
                auto& registry = ComponentRegistry::Instance();
                auto component = registry.CreateComponent(componentType);
                if (component) {
                    // Set component properties
                    if (componentData.HasKey("name")) {
                        component->SetName(componentData["name"].AsString());
                    }
                    // Set UUID from file if present
                    if (componentData.HasKey("uuid")) {
                        std::string uuidStr = componentData["uuid"].AsString();
                        component->SetUUID(UUID::FromString(uuidStr));
                    }
                    if (componentData.HasKey("active")) {
                        component->SetActive(componentData["active"].AsBool());
                    }

                    // Deserialize export variables
                    if (componentData.HasKey("export_variables") && componentData["export_variables"].IsObject()) {
                        const auto& exportVarsJson = componentData["export_variables"].AsObject();
                        for (const auto& [varName, varData] : exportVarsJson) {
                            // Handle both old format (with type/value) and new format (direct values)
                            if (varData.HasKey("type") && varData.HasKey("value")) {
                                // Old format
                                std::string typeStr = varData["type"].AsString();
                                std::string valueStr = varData["value"].AsString();

                                ExportVariableType varType = SerializationUtils::StringToExportVariableType(typeStr);
                                ExportValue varValue = SerializationUtils::ParseExportValue(valueStr, varType);

                                component->SetExportVariable(varName, varValue);
                            } else {
                                // New format - direct values
                                ExportValue varValue;
                                if (varData.IsBool()) {
                                    varValue = varData.AsBool();
                                } else if (varData.IsInt()) {
                                    varValue = static_cast<int>(varData.AsInt());
                                } else if (varData.IsDouble()) {
                                    varValue = static_cast<float>(varData.AsDouble());
                                } else if (varData.IsString()) {
                                    varValue = varData.AsString();
                                } else if (varData.IsArray()) {
                                    const auto& arr = varData.AsArray();
                                    if (arr.size() == 2) {
                                        varValue = glm::vec2(arr[0].AsDouble(), arr[1].AsDouble());
                                    } else if (arr.size() == 3) {
                                        varValue = glm::vec3(arr[0].AsDouble(), arr[1].AsDouble(), arr[2].AsDouble());
                                    } else if (arr.size() == 4) {
                                        varValue = glm::vec4(arr[0].AsDouble(), arr[1].AsDouble(), arr[2].AsDouble(), arr[3].AsDouble());
                                    }
                                }
                                component->SetExportVariable(varName, varValue);
                            }
                        }
                    }

                    node->AddComponent(std::move(component));
                    std::cout << "Added component: " << componentType << " to node: " << nodeName << std::endl;
                } else {
                    std::cout << "Failed to create component of type: " << componentType << std::endl;
                }
            }
        }
    }

    // Deserialize children
    if (nodeData.HasKey("children") && nodeData["children"].IsArray()) {
        const auto& childrenArray = nodeData["children"].AsArray();
        std::cout << "Found " << childrenArray.size() << " children for node: " << nodeName << std::endl;

        for (const auto& childData : childrenArray) {
            auto childNode = DeserializeNode(childData);
            if (childNode) {
                std::cout << "Adding child node: " << childNode->GetName() << " to parent: " << nodeName << std::endl;
                node->AddChild(std::move(childNode));
            } else {
                std::cout << "Failed to deserialize child node for parent: " << nodeName << std::endl;
            }
        }
    } else {
        std::cout << "No children found for node: " << nodeName << std::endl;
    }

    return node;
}

} // namespace Lupine
