#include "lupine/core/Scene.h"
#include "lupine/serialization/SceneSerializer.h"
#include <algorithm>
#include <iostream>

namespace Lupine {

Scene::Scene(const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_modified(false) {
}

void Scene::SetRootNode(std::unique_ptr<Node> root_node) {
    if (root_node) {
        root_node->SetScene(this);
    }
    m_root_node = std::move(root_node);
    MarkModified();
}

Node* Scene::FindNode(const UUID& uuid) const {
    if (!m_root_node) {
        return nullptr;
    }
    
    return FindNodeRecursive(m_root_node.get(), uuid);
}

Node* Scene::FindNode(const std::string& name) const {
    if (!m_root_node) {
        return nullptr;
    }

    return FindNodeRecursive(m_root_node.get(), name);
}

Node* Scene::FindNodeByUUID(const std::string& uuid_string) const {
    if (!m_root_node || uuid_string.empty()) {
        return nullptr;
    }

    UUID uuid = UUID::FromString(uuid_string);
    return FindNodeRecursive(m_root_node.get(), uuid);
}

std::vector<Node*> Scene::GetAllNodes() const {
    std::vector<Node*> nodes;
    if (m_root_node) {
        CollectNodes(m_root_node.get(), nodes);
    }
    return nodes;
}

bool Scene::LoadFromFile(const std::string& file_path) {
    auto loaded_scene = SceneSerializer::DeserializeFromFile(file_path);
    if (!loaded_scene) {
        return false;
    }

    // Copy loaded scene data to this scene
    m_name = loaded_scene->GetName();
    m_root_node = std::move(loaded_scene->m_root_node);
    if (m_root_node) {
        m_root_node->SetScene(this);
    }

    m_file_path = file_path;
    MarkSaved();
    return true;
}

bool Scene::SaveToFile(const std::string& file_path) {
    if (!file_path.empty()) {
        m_file_path = file_path;
    }

    if (m_file_path.empty()) {
        return false;
    }

    bool success = SceneSerializer::SerializeToFile(this, m_file_path);
    if (success) {
        MarkSaved();
    }
    return success;
}

void Scene::OnEnter() {
    if (m_root_node) {
        m_root_node->OnReady();
    }
}

void Scene::OnExit() {
    // Cleanup if needed
}

void Scene::OnUpdate(float delta_time) {
    if (m_root_node && m_root_node->IsActive()) {
        m_root_node->OnUpdate(delta_time);
    }
}

void Scene::OnPhysicsProcess(float delta_time) {
    if (m_root_node && m_root_node->IsActive()) {
        m_root_node->OnPhysicsProcess(delta_time);
    }
}

void Scene::OnInput(const void* event) {
    // TODO: Implement input handling
    (void)event;
}

void Scene::CollectNodes(Node* node, std::vector<Node*>& nodes) const {
    if (!node) return;
    
    nodes.push_back(node);
    
    for (const auto& child : node->GetChildren()) {
        CollectNodes(child.get(), nodes);
    }
}

Node* Scene::FindNodeRecursive(Node* node, const UUID& uuid) const {
    if (!node) return nullptr;
    
    if (node->GetUUID() == uuid) {
        return node;
    }
    
    for (const auto& child : node->GetChildren()) {
        if (Node* found = FindNodeRecursive(child.get(), uuid)) {
            return found;
        }
    }
    
    return nullptr;
}

Node* Scene::FindNodeRecursive(Node* node, const std::string& name) const {
    if (!node) return nullptr;
    
    if (node->GetName() == name) {
        return node;
    }
    
    for (const auto& child : node->GetChildren()) {
        if (Node* found = FindNodeRecursive(child.get(), name)) {
            return found;
        }
    }
    
    return nullptr;
}

} // namespace Lupine
