#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <memory>
#include "lupine/core/Node.h"

namespace Lupine {
    class Scene;
}

/**
 * @brief Data structure for storing clipboard node information
 */
struct ClipboardNodeData {
    std::string serializedData;
    std::string nodeTypeName;
    std::string nodeName;
    Lupine::UUID originalNodeId;
    glm::vec3 relativePosition; // Position relative to clipboard center
    
    ClipboardNodeData() = default;
    ClipboardNodeData(const std::string& data, const std::string& typeName, 
                     const std::string& name, const Lupine::UUID& id, 
                     const glm::vec3& pos)
        : serializedData(data), nodeTypeName(typeName), nodeName(name), 
          originalNodeId(id), relativePosition(pos) {}
};

/**
 * @brief Global clipboard system for the editor
 */
class EditorClipboard : public QObject
{
    Q_OBJECT

public:
    explicit EditorClipboard(QObject* parent = nullptr);
    ~EditorClipboard();

    // Clipboard operations
    void copyNodes(const std::vector<Lupine::Node*>& nodes, const QString& description = "Copy Nodes");
    void cutNodes(const std::vector<Lupine::Node*>& nodes, const QString& description = "Cut Nodes");
    std::vector<std::unique_ptr<Lupine::Node>> pasteNodes(Lupine::Node* targetParent, const glm::vec3& pastePosition = glm::vec3(0.0f));
    
    // Single node operations
    void copyNode(Lupine::Node* node, const QString& description = "Copy Node");
    void cutNode(Lupine::Node* node, const QString& description = "Cut Node");
    std::unique_ptr<Lupine::Node> pasteNode(Lupine::Node* targetParent, const glm::vec3& pastePosition = glm::vec3(0.0f));
    
    // Clipboard state
    bool hasData() const;
    bool isCutOperation() const { return m_isCutOperation; }
    int getNodeCount() const { return static_cast<int>(m_clipboardData.size()); }
    void clear();
    
    // Scene management
    void setScene(Lupine::Scene* scene);
    Lupine::Scene* getScene() const { return m_scene; }
    
    // Information
    QString getDescription() const { return m_description; }
    std::vector<QString> getNodeNames() const;

signals:
    void clipboardChanged();
    void nodesCopied(const QString& description);
    void nodesCut(const QString& description);
    void nodesPasted(const QString& description, int count);

private:
    void calculateClipboardCenter();
    std::string serializeNode(Lupine::Node* node) const;
    std::unique_ptr<Lupine::Node> deserializeNode(const std::string& data) const;
    glm::vec3 getNodePosition(Lupine::Node* node) const;
    void setNodePosition(Lupine::Node* node, const glm::vec3& position) const;
    void generateUniqueNodeName(Lupine::Node* node, Lupine::Node* parent) const;
    
    std::vector<ClipboardNodeData> m_clipboardData;
    std::vector<Lupine::Node*> m_clipboardNodes; // Store actual node pointers for duplication
    glm::vec3 m_clipboardCenter;
    bool m_isCutOperation;
    QString m_description;

    Lupine::Scene* m_scene;
    
    // Cut operation tracking
    std::vector<Lupine::UUID> m_cutNodeIds;
};
