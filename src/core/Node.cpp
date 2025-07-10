#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/core/Scene.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include <algorithm>

namespace Lupine {

Node::Node(const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_parent(nullptr)
    , m_scene(nullptr)
    , m_active(true)
    , m_visible(true) {
}

Node::~Node() {
    // Call OnDestroy for safe cleanup before destruction
    try {
        OnDestroy();
    } catch (...) {
        // Ignore exceptions during destruction to prevent crashes
    }

    // Clear parent relationship safely
    if (m_parent) {
        m_parent = nullptr;
    }

    // Clear scene reference
    m_scene = nullptr;

    // Components and children will be automatically destroyed by unique_ptr
    // The unique_ptr destructors will handle the cleanup in reverse order
}

void Node::AddChild(std::unique_ptr<Node> child) {
    if (!child) return;
    
    // Remove from previous parent if any
    if (child->m_parent) {
        child->m_parent->RemoveChild(child->GetUUID());
    }
    
    // Set this node as parent
    child->SetParent(this);
    child->SetScene(m_scene);
    
    // Add to children list
    m_children.push_back(std::move(child));
}

std::unique_ptr<Node> Node::RemoveChild(const UUID& uuid) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [&uuid](const std::unique_ptr<Node>& child) {
            return child->GetUUID() == uuid;
        });
    
    if (it != m_children.end()) {
        std::unique_ptr<Node> child = std::move(*it);
        child->SetParent(nullptr);
        child->SetScene(nullptr);
        m_children.erase(it);
        return child;
    }
    
    return nullptr;
}

Node* Node::FindChild(const UUID& uuid, bool recursive) const {
    // Search direct children first
    for (const auto& child : m_children) {
        if (child->GetUUID() == uuid) {
            return child.get();
        }
    }
    
    // Search recursively if requested
    if (recursive) {
        for (const auto& child : m_children) {
            if (Node* found = child->FindChild(uuid, true)) {
                return found;
            }
        }
    }
    
    return nullptr;
}

Node* Node::FindChild(const std::string& name, bool recursive) const {
    // Search direct children first
    for (const auto& child : m_children) {
        if (child->GetName() == name) {
            return child.get();
        }
    }

    // Search recursively if requested
    if (recursive) {
        for (const auto& child : m_children) {
            if (Node* found = child->FindChild(name, true)) {
                return found;
            }
        }
    }

    return nullptr;
}

Node* Node::GetChild(size_t index) const {
    if (index >= m_children.size()) {
        return nullptr;
    }
    return m_children[index].get();
}

void Node::SetScene(Scene* scene) {
    m_scene = scene;
    
    // Propagate to children
    for (auto& child : m_children) {
        child->SetScene(scene);
    }
}

void Node::AddComponent(std::unique_ptr<Component> component) {
    if (!component) return;

    // Set the component's owner node
    component->SetOwner(this);

    // Call OnAwake for the component
    component->OnAwake();

    // Add to components list
    m_components.push_back(std::move(component));
}

std::unique_ptr<Component> Node::RemoveComponent(const UUID& uuid) {
    auto it = std::find_if(m_components.begin(), m_components.end(),
        [&uuid](const std::unique_ptr<Component>& component) {
            return component->GetUUID() == uuid;
        });

    if (it != m_components.end()) {
        std::unique_ptr<Component> component = std::move(*it);

        // Call OnDestroy before removing
        component->OnDestroy();
        component->SetOwner(nullptr);

        m_components.erase(it);
        return component;
    }

    return nullptr;
}

void Node::OnReady() {
    // Call OnReady for all components
    for (auto& component : m_components) {
        if (component->IsActive()) {
            component->OnReady();
        }
    }
    
    // Call OnReady for all children
    for (auto& child : m_children) {
        if (child->IsActive() && child->IsVisible()) {
            child->OnReady();
        }
    }
}

void Node::OnUpdate(float delta_time) {
    if (!m_active || !m_visible) return;
    
    // Call OnUpdate for all components
    for (auto& component : m_components) {
        if (component->IsActive()) {
            component->OnUpdate(delta_time);
        }
    }
    
    // Call OnUpdate for all children
    for (auto& child : m_children) {
        if (child->IsActive()) {
            child->OnUpdate(delta_time);
        }
    }
}

void Node::OnPhysicsProcess(float delta_time) {
    if (!m_active || !m_visible) return;

    // Call OnPhysicsProcess for all components
    for (auto& component : m_components) {
        if (component->IsActive()) {
            component->OnPhysicsProcess(delta_time);
        }
    }

    // Call OnPhysicsProcess for all children
    for (auto& child : m_children) {
        if (child->IsActive()) {
            child->OnPhysicsProcess(delta_time);
        }
    }
}

std::unique_ptr<Node> Node::Duplicate(const std::string& name_suffix) const {
    // Create a new node of the same type with a new name
    std::unique_ptr<Node> duplicated_node;

    // Use the SceneSerializer to create a node of the correct type
    std::string node_type;
    std::string new_name;

    try {
        node_type = GetTypeName();
        new_name = m_name + name_suffix;
    } catch (...) {
        // If we can't get the type name or name, use defaults
        node_type = "Node";
        new_name = "Duplicated Node" + name_suffix;
    }

    // Create a node of the appropriate type with safety checks
    try {
        if (node_type == "Node2D") {
            duplicated_node = std::make_unique<Node2D>(new_name);
        } else if (node_type == "Node3D") {
            duplicated_node = std::make_unique<Node3D>(new_name);
        } else if (node_type == "Control") {
            duplicated_node = std::make_unique<Control>(new_name);
        } else {
            // Default to base Node
            duplicated_node = std::make_unique<Node>(new_name);
        }
    } catch (...) {
        // If node creation fails, try with a basic Node
        try {
            duplicated_node = std::make_unique<Node>(new_name);
        } catch (...) {
            return nullptr;
        }
    }

    if (!duplicated_node) {
        return nullptr;
    }

    // Copy basic properties
    duplicated_node->SetActive(m_active);
    duplicated_node->SetVisible(m_visible);

    // Copy type-specific properties with safety check
    try {
        CopyTypeSpecificProperties(duplicated_node.get());
    } catch (...) {
        // If type-specific property copying fails, continue with default values
        // This prevents crashes from problematic property copying
    }

    // Duplicate all components with safety checks
    try {
        for (const auto& component : m_components) {
            try {
                auto duplicated_component = DuplicateComponent(component.get());
                if (duplicated_component) {
                    duplicated_node->AddComponent(std::move(duplicated_component));
                }
            } catch (...) {
                // Skip this component if duplication fails
                // This prevents crashes from problematic components
                continue;
            }
        }
    } catch (...) {
        // If component iteration fails, continue without components
    }

    // Recursively duplicate all children with safety checks
    try {
        for (const auto& child : m_children) {
            try {
                auto duplicated_child = child->Duplicate(name_suffix);
                if (duplicated_child) {
                    duplicated_node->AddChild(std::move(duplicated_child));
                }
            } catch (...) {
                // Skip this child if duplication fails
                // This prevents crashes from problematic child nodes
                continue;
            }
        }
    } catch (...) {
        // If child iteration fails, continue without children
    }

    return duplicated_node;
}

void Node::CopyTypeSpecificProperties(Node* target) const {
    // Base Node has no type-specific properties to copy
    // This method will be overridden in derived classes
    (void)target; // Suppress unused parameter warning
}

std::unique_ptr<Component> Node::DuplicateComponent(const Component* component) const {
    if (!component) {
        return nullptr;
    }

    try {
        // Create a new component of the same type using the ComponentRegistry
        auto& registry = ComponentRegistry::Instance();

        // Get the component type name safely
        std::string component_type;
        try {
            component_type = component->GetTypeName();
        } catch (...) {
            // If we can't get the type name, we can't duplicate the component
            return nullptr;
        }

        auto duplicated_component = registry.CreateComponent(component_type);

        if (!duplicated_component) {
            // Component type not registered or creation failed
            return nullptr;
        }

        // Copy basic properties
        duplicated_component->SetName(component->GetName());
        duplicated_component->SetActive(component->IsActive());

        // Copy export variables with safety checks
        try {
            const auto& export_vars = component->GetExportVariables();
            for (const auto& [name, var] : export_vars) {
                try {
                    duplicated_component->SetExportVariable(name, var.value);
                } catch (...) {
                    // Skip this export variable if it fails to copy
                    // This prevents crashes from invalid or corrupted export variable data
                    continue;
                }
            }
        } catch (...) {
            // If export variables can't be accessed, continue without them
            // The component will use its default export variable values
        }

        return duplicated_component;
    } catch (...) {
        // If component creation fails completely, return nullptr
        return nullptr;
    }
}

} // namespace Lupine
