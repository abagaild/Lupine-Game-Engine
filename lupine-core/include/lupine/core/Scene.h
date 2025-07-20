#pragma once

#include "lupine/core/UUID.h"
#include "lupine/core/Node.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace Lupine {

/**
 * @brief Scene class for managing scene trees
 * 
 * A scene contains a tree of nodes and provides functionality for
 * loading, saving, and managing the scene hierarchy.
 */
class Scene {
public:
    /**
     * @brief Constructor
     * @param name Name of the scene
     */
    explicit Scene(const std::string& name = "Scene");
    
    /**
     * @brief Destructor
     */
    ~Scene() = default;
    
    // Disable copy constructor and assignment operator
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    
    // Enable move constructor and assignment operator
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;
    
    /**
     * @brief Get the scene's UUID
     * @return UUID of the scene
     */
    const UUID& GetUUID() const { return m_uuid; }
    
    /**
     * @brief Get the scene's name
     * @return Name of the scene
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set the scene's name
     * @param name New name for the scene
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get the scene's file path
     * @return File path of the scene
     */
    const std::string& GetFilePath() const { return m_file_path; }
    
    /**
     * @brief Set the scene's file path
     * @param file_path New file path
     */
    void SetFilePath(const std::string& file_path) { m_file_path = file_path; }
    
    /**
     * @brief Get the root node of the scene
     * @return Pointer to root node, nullptr if no root
     */
    Node* GetRootNode() const { return m_root_node.get(); }
    
    /**
     * @brief Set the root node of the scene
     * @param root_node Unique pointer to root node
     */
    void SetRootNode(std::unique_ptr<Node> root_node);
    
    /**
     * @brief Create a new root node
     * @tparam T Node type
     * @param name Name of the root node
     * @return Pointer to created root node
     */
    template<typename T = Node>
    T* CreateRootNode(const std::string& name = "Root");
    
    /**
     * @brief Find a node by UUID (searches entire scene tree)
     * @param uuid UUID to search for
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindNode(const UUID& uuid) const;
    
    /**
     * @brief Find a node by name (searches entire scene tree)
     * @param name Name to search for
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindNode(const std::string& name) const;

    /**
     * @brief Find a node by UUID string (searches entire scene tree)
     * @param uuid_string UUID string to search for
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindNodeByUUID(const std::string& uuid_string) const;
    
    /**
     * @brief Get all nodes in the scene
     * @return Vector of node pointers
     */
    std::vector<Node*> GetAllNodes() const;
    
    /**
     * @brief Check if scene has been modified since last save
     * @return True if modified
     */
    bool IsModified() const { return m_modified; }
    
    /**
     * @brief Mark scene as modified
     */
    void MarkModified() { m_modified = true; }
    
    /**
     * @brief Mark scene as saved (not modified)
     */
    void MarkSaved() { m_modified = false; }
    
    /**
     * @brief Load scene from file
     * @param file_path Path to scene file
     * @return True if loaded successfully
     */
    bool LoadFromFile(const std::string& file_path);
    
    /**
     * @brief Save scene to file
     * @param file_path Path to save to (optional, uses current path if empty)
     * @return True if saved successfully
     */
    bool SaveToFile(const std::string& file_path = "");
    
    /**
     * @brief Called when scene becomes active
     */
    void OnEnter();
    
    /**
     * @brief Called when scene becomes inactive
     */
    void OnExit();
    
    /**
     * @brief Called every frame
     * @param delta_time Time since last frame
     */
    void OnUpdate(float delta_time);
    
    /**
     * @brief Called during physics processing
     * @param delta_time Time since last physics update
     */
    void OnPhysicsProcess(float delta_time);
    
    /**
     * @brief Called when input events occur
     * @param event Input event
     */
    void OnInput(const void* event);

private:
    UUID m_uuid;
    std::string m_name;
    std::string m_file_path;
    std::unique_ptr<Node> m_root_node;
    bool m_modified;
    
    /**
     * @brief Recursively collect all nodes from a subtree
     * @param node Root of subtree
     * @param nodes Vector to collect nodes into
     */
    void CollectNodes(Node* node, std::vector<Node*>& nodes) const;
    
    /**
     * @brief Recursively search for node by UUID
     * @param node Root of subtree to search
     * @param uuid UUID to search for
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindNodeRecursive(Node* node, const UUID& uuid) const;
    
    /**
     * @brief Recursively search for node by name
     * @param node Root of subtree to search
     * @param name Name to search for
     * @return Pointer to found node, nullptr if not found
     */
    Node* FindNodeRecursive(Node* node, const std::string& name) const;
};

// Template implementation
template<typename T>
T* Scene::CreateRootNode(const std::string& name) {
    static_assert(std::is_base_of_v<Node, T>, "T must be derived from Node");
    
    auto root = std::make_unique<T>(name);
    T* root_ptr = root.get();
    SetRootNode(std::move(root));
    return root_ptr;
}

} // namespace Lupine
