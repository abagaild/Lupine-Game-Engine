#include "EditorClipboard.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/serialization/SceneSerializer.h"
#include <QDebug>
#include <algorithm>
#include <set>

EditorClipboard::EditorClipboard(QObject* parent)
    : QObject(parent)
    , m_clipboardCenter(0.0f)
    , m_isCutOperation(false)
    , m_scene(nullptr)
{
}

EditorClipboard::~EditorClipboard()
{
    clear();
}

void EditorClipboard::copyNodes(const std::vector<Lupine::Node*>& nodes, const QString& description)
{
    if (nodes.empty() || !m_scene) {
        return;
    }
    
    clear();
    m_description = description;
    m_isCutOperation = false;
    m_clipboardNodes.clear();
    
    // Calculate center position of all nodes
    glm::vec3 totalPosition(0.0f);
    int validNodeCount = 0;
    
    for (Lupine::Node* node : nodes) {
        if (node) {
            totalPosition += getNodePosition(node);
            validNodeCount++;
        }
    }
    
    if (validNodeCount > 0) {
        m_clipboardCenter = totalPosition / static_cast<float>(validNodeCount);
    }
    
    // Store each node and calculate relative position
    for (Lupine::Node* node : nodes) {
        if (node) {
            try {
                std::string serializedData = serializeNode(node);
                glm::vec3 nodePos = getNodePosition(node);
                glm::vec3 relativePos = nodePos - m_clipboardCenter;

                m_clipboardData.emplace_back(
                    serializedData,
                    node->GetTypeName(),
                    node->GetName(),
                    node->GetUUID(),
                    relativePos
                );

                // Store the actual node pointer for duplication
                m_clipboardNodes.push_back(node);
            } catch (const std::exception& e) {
                qWarning() << "Failed to serialize node for clipboard:" << e.what();
            }
        }
    }
    
    emit clipboardChanged();
    emit nodesCopied(description);
}

void EditorClipboard::cutNodes(const std::vector<Lupine::Node*>& nodes, const QString& description)
{
    if (nodes.empty() || !m_scene) {
        return;
    }
    
    // First copy the nodes
    copyNodes(nodes, description);
    
    // Mark as cut operation and store node IDs
    m_isCutOperation = true;
    m_cutNodeIds.clear();
    
    for (Lupine::Node* node : nodes) {
        if (node) {
            m_cutNodeIds.push_back(node->GetUUID());
        }
    }
    
    emit nodesCut(description);
}

std::vector<std::unique_ptr<Lupine::Node>> EditorClipboard::pasteNodes(Lupine::Node* targetParent, const glm::vec3& pastePosition)
{
    std::vector<std::unique_ptr<Lupine::Node>> pastedNodes;
    
    if (!hasData() || !targetParent || !m_scene) {
        return pastedNodes;
    }
    
    try {
        // Duplicate each node using the stored node pointers
        for (size_t i = 0; i < m_clipboardNodes.size() && i < m_clipboardData.size(); ++i) {
            Lupine::Node* originalNode = m_clipboardNodes[i];
            const auto& clipboardNode = m_clipboardData[i];

            if (originalNode) {
                // Use the node's Duplicate method to create a copy
                auto node = originalNode->Duplicate(" (Copy)");
                if (node) {
                    // Calculate final position
                    glm::vec3 finalPosition = pastePosition + clipboardNode.relativePosition;
                    setNodePosition(node.get(), finalPosition);

                    // Generate unique name if needed
                    generateUniqueNodeName(node.get(), targetParent);

                    pastedNodes.push_back(std::move(node));
                }
            }
        }
        
        // If this was a cut operation, we need to handle the original nodes
        if (m_isCutOperation) {
            // The cut nodes should be removed after successful paste
            // This will be handled by the calling code
        }
        
        emit nodesPasted(m_description, static_cast<int>(pastedNodes.size()));
        
    } catch (const std::exception& e) {
        qWarning() << "Error during paste operation:" << e.what();
        pastedNodes.clear();
    }
    
    return pastedNodes;
}

void EditorClipboard::copyNode(Lupine::Node* node, const QString& description)
{
    if (node) {
        copyNodes({node}, description);
    }
}

void EditorClipboard::cutNode(Lupine::Node* node, const QString& description)
{
    if (node) {
        cutNodes({node}, description);
    }
}

std::unique_ptr<Lupine::Node> EditorClipboard::pasteNode(Lupine::Node* targetParent, const glm::vec3& pastePosition)
{
    auto nodes = pasteNodes(targetParent, pastePosition);
    if (!nodes.empty()) {
        return std::move(nodes[0]);
    }
    return nullptr;
}

bool EditorClipboard::hasData() const
{
    return !m_clipboardData.empty();
}

void EditorClipboard::clear()
{
    m_clipboardData.clear();
    m_clipboardNodes.clear();
    m_cutNodeIds.clear();
    m_clipboardCenter = glm::vec3(0.0f);
    m_isCutOperation = false;
    m_description.clear();

    emit clipboardChanged();
}

void EditorClipboard::setScene(Lupine::Scene* scene)
{
    if (m_scene != scene) {
        clear(); // Clear clipboard when scene changes
        m_scene = scene;
    }
}

std::vector<QString> EditorClipboard::getNodeNames() const
{
    std::vector<QString> names;
    names.reserve(m_clipboardData.size());
    
    for (const auto& data : m_clipboardData) {
        names.push_back(QString::fromStdString(data.nodeName));
    }
    
    return names;
}

std::string EditorClipboard::serializeNode(Lupine::Node* node) const
{
    if (!node) return "";

    try {
        // For clipboard, we don't need actual serialization since we'll use duplication
        // Just store the node type name for validation
        return node->GetTypeName();
    } catch (const std::exception& e) {
        qWarning() << "Error getting node type:" << e.what();
        return "";
    }
}

std::unique_ptr<Lupine::Node> EditorClipboard::deserializeNode(const std::string& data) const
{
    if (data.empty()) return nullptr;

    try {
        // For clipboard, we use the stored node pointer directly via duplication
        // This method is not used in the new approach
        return nullptr;
    } catch (const std::exception& e) {
        qWarning() << "Error in deserializeNode:" << e.what();
        return nullptr;
    }
}

glm::vec3 EditorClipboard::getNodePosition(Lupine::Node* node) const
{
    if (!node) return glm::vec3(0.0f);
    
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        glm::vec2 pos = node2d->GetPosition();
        return glm::vec3(pos.x, pos.y, 0.0f);
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        return node3d->GetPosition();
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec2 pos = control->GetPosition();
        return glm::vec3(pos.x, pos.y, 0.0f);
    }
    
    return glm::vec3(0.0f);
}

void EditorClipboard::setNodePosition(Lupine::Node* node, const glm::vec3& position) const
{
    if (!node) return;
    
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        node2d->SetPosition(glm::vec2(position.x, position.y));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        node3d->SetPosition(position);
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        control->SetPosition(glm::vec2(position.x, position.y));
    }
}

void EditorClipboard::generateUniqueNodeName(Lupine::Node* node, Lupine::Node* parent) const
{
    if (!node || !parent) return;
    
    std::string baseName = node->GetName();
    std::string newName = baseName;
    
    // Get all existing child names
    std::set<std::string> existingNames;
    for (const auto& child : parent->GetChildren()) {
        if (child) {
            existingNames.insert(child->GetName());
        }
    }
    
    // Find unique name by appending numbers
    int counter = 1;
    while (existingNames.find(newName) != existingNames.end()) {
        newName = baseName + " (" + std::to_string(counter) + ")";
        counter++;
    }
    
    node->SetName(newName);
}
