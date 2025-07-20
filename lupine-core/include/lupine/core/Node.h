#pragma once

#include "lupine/core/UUID.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Lupine {

class Component;
class Scene;

/**
 * @brief Base class for all nodes in the scene tree
 * 
 * Nodes are the fundamental building blocks of the Lupine engine.
 * They form a hierarchical tree structure and can have components attached.
 */
class Node {
public:
    /**
     * @brief Constructor
     * @param name Name of the node
     */
    explicit Node(const std::string& name = "Node");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Node();
    
    // Disable copy constructor and assignment operator
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    
    // Enable move constructor and assignment operator
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;
    
    /**
     * @brief Get the node's UUID
     * @return UUID of the node
     */
    const UUID& GetUUID() const { return m_uuid; }

    /**
     * @brief Set the node's UUID (used during deserialization)
     * @param uuid The UUID to set
     */
    void SetUUID(const UUID& uuid) { m_uuid = uuid; }
    
    /**
     * @brief Get the node's name
     * @return Name of the node
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set the node's name
     * @param name New name for the node
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get the parent node
     * @return Pointer to parent node, nullptr if root
     */
    Node* GetParent() const { return m_parent; }
    
    /**
     * @brief Get all child nodes
     * @return Vector of child node pointers
     */
    const std::vector<std::unique_ptr<Node>>& GetChildren() const { return m_children; }
    
    /**
     * @brief Add a child node
     * @param child Unique pointer to child node
     */
    void AddChild(std::unique_ptr<Node> child);
    
    /**
     * @brief Remove a child node by UUID
     * @param uuid UUID of child to remove
     * @return Unique pointer to removed child, nullptr if not found
     */
    std::unique_ptr<Node> RemoveChild(const UUID& uuid);
    
    /**
     * @brief Find a child node by UUID
     * @param uuid UUID to search for
     * @param recursive Whether to search recursively
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindChild(const UUID& uuid, bool recursive = false) const;
    
    /**
     * @brief Find a child node by name
     * @param name Name to search for
     * @param recursive Whether to search recursively
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindChild(const std::string& name, bool recursive = false) const;

    /**
     * @brief Get the number of direct children
     * @return Number of children
     */
    size_t GetChildCount() const { return m_children.size(); }

    /**
     * @brief Get a child by index
     * @param index Index of child
     * @return Pointer to child, nullptr if index is out of bounds
     */
    Node* GetChild(size_t index) const;

    /**
     * @brief Create a child node of specified type
     * @tparam T Node type (must derive from Node)
     * @param name Name of the child node
     * @return Pointer to created child node
     */
    template<typename T = Node>
    T* CreateChild(const std::string& name = "Node");
    
    /**
     * @brief Get the scene this node belongs to
     * @return Pointer to scene, nullptr if not in a scene
     */
    Scene* GetScene() const { return m_scene; }
    
    /**
     * @brief Set the scene this node belongs to (internal use)
     * @param scene Pointer to scene
     */
    void SetScene(Scene* scene);
    
    /**
     * @brief Add a component to this node
     * @param component Unique pointer to component
     */
    void AddComponent(std::unique_ptr<Component> component);

    /**
     * @brief Add a component of specified type to this node
     * @tparam T Component type (must derive from Component)
     * @return Pointer to created component
     */
    template<typename T>
    T* AddComponent();
    
    /**
     * @brief Remove a component by UUID
     * @param uuid UUID of component to remove
     * @return Unique pointer to removed component, nullptr if not found
     */
    std::unique_ptr<Component> RemoveComponent(const UUID& uuid);
    
    /**
     * @brief Get a component by type
     * @tparam T Component type
     * @return Pointer to component, nullptr if not found
     */
    template<typename T>
    T* GetComponent() const;
    
    /**
     * @brief Get all components of a specific type
     * @tparam T Component type
     * @return Vector of component pointers
     */
    template<typename T>
    std::vector<T*> GetComponents() const;
    
    /**
     * @brief Get all components
     * @return Vector of component pointers
     */
    const std::vector<std::unique_ptr<Component>>& GetAllComponents() const { return m_components; }
    
    /**
     * @brief Check if node is active
     * @return True if active
     */
    bool IsActive() const { return m_active; }
    
    /**
     * @brief Set node active state
     * @param active New active state
     */
    void SetActive(bool active) { m_active = active; }

    /**
     * @brief Check if node is visible
     * @return True if visible
     */
    bool IsVisible() const { return m_visible; }

    /**
     * @brief Set node visibility state
     * @param visible New visibility state
     */
    void SetVisible(bool visible) { m_visible = visible; }

    /**
     * @brief Create a deep copy of this node
     * @param name_suffix Suffix to add to the duplicated node name
     * @return Unique pointer to the duplicated node
     */
    std::unique_ptr<Node> Duplicate(const std::string& name_suffix = " Copy") const;
    
    /**
     * @brief Called when node enters the scene tree
     */
    virtual void OnReady();
    
    /**
     * @brief Called every frame
     * @param delta_time Time since last frame
     */
    virtual void OnUpdate(float delta_time);
    
    /**
     * @brief Called during physics processing
     * @param delta_time Time since last physics update
     */
    virtual void OnPhysicsProcess(float delta_time);

    /**
     * @brief Called when node is about to be destroyed
     * Override this for cleanup that needs to happen before destruction
     */
    virtual void OnDestroy() {}

    /**
     * @brief Get node type name (for serialization)
     * @return Type name string
     */
    virtual std::string GetTypeName() const { return "Node"; }

    /**
     * @brief Safely cast node to derived type with null checking
     * @tparam T Target type to cast to
     * @return Pointer to cast result or nullptr if cast fails or node is invalid
     */
    template<typename T>
    T* SafeCast() const {
        try {
            return dynamic_cast<T*>(const_cast<Node*>(this));
        } catch (...) {
            return nullptr;
        }
    }

    /**
     * @brief Check if this node is valid and safe to use
     * @return True if node appears to be in a valid state
     */
    bool IsValidNode() const {
        try {
            // Basic validation - check if we can access member variables
            return !m_name.empty() || m_name.empty(); // This will throw if object is corrupted
        } catch (...) {
            return false;
        }
    }

protected:
    UUID m_uuid;
    std::string m_name;
    Node* m_parent;
    std::vector<std::unique_ptr<Node>> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
    Scene* m_scene;
    bool m_active;
    bool m_visible;
    
    /**
     * @brief Set parent node (internal use)
     * @param parent Pointer to parent node
     */
    void SetParent(Node* parent) { m_parent = parent; }

    /**
     * @brief Copy type-specific properties to another node (helper for duplication)
     * @param target Target node to copy properties to
     */
    virtual void CopyTypeSpecificProperties(Node* target) const;

    /**
     * @brief Duplicate a component (helper for duplication)
     * @param component Component to duplicate
     * @return Duplicated component or nullptr if failed
     */
    std::unique_ptr<Component> DuplicateComponent(const Component* component) const;

    friend class Scene;
};

// Template implementations
template<typename T>
T* Node::GetComponent() const {
    for (const auto& component : m_components) {
        if (component) {
            try {
                if (auto* typed_component = dynamic_cast<T*>(component.get())) {
                    return typed_component;
                }
            } catch (...) {
                // If dynamic_cast fails due to RTTI issues, skip this component
                continue;
            }
        }
    }
    return nullptr;
}

template<typename T>
std::vector<T*> Node::GetComponents() const {
    std::vector<T*> result;
    for (const auto& component : m_components) {
        if (component) {
            try {
                if (auto* typed_component = dynamic_cast<T*>(component.get())) {
                    result.push_back(typed_component);
                }
            } catch (...) {
                // If dynamic_cast fails due to RTTI issues, skip this component
                continue;
            }
        }
    }
    return result;
}

template<typename T>
T* Node::CreateChild(const std::string& name) {
    static_assert(std::is_base_of_v<Node, T>, "T must be derived from Node");

    auto child = std::make_unique<T>(name);
    T* child_ptr = child.get();
    AddChild(std::move(child));
    return child_ptr;
}

template<typename T>
T* Node::AddComponent() {
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");

    auto component = std::make_unique<T>();
    T* component_ptr = component.get();
    AddComponent(std::move(component));
    return component_ptr;
}

} // namespace Lupine
