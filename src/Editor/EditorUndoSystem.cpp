#include "EditorUndoSystem.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/serialization/SceneSerializer.h"
#include <QDebug>

EditorUndoSystem::EditorUndoSystem(QObject* parent)
    : QObject(parent)
    , m_undoIndex(0)
    , m_maxUndoSteps(25)
    , m_scene(nullptr)
    , m_recordingBulkOperation(false)
{
}

EditorUndoSystem::~EditorUndoSystem()
{
    clear();
}

bool EditorUndoSystem::canUndo() const
{
    return m_undoIndex > 0;
}

bool EditorUndoSystem::canRedo() const
{
    return m_undoIndex < static_cast<int>(m_undoStack.size());
}

void EditorUndoSystem::undo()
{
    if (!canUndo()) {
        return;
    }
    
    m_undoIndex--;
    const EditorUndoAction& action = m_undoStack[m_undoIndex];
    
    try {
        executeUndo(action);
        emit undoRedoStateChanged();
    } catch (const std::exception& e) {
        qWarning() << "Error during undo operation:" << e.what();
        // Move index back to maintain consistency
        m_undoIndex++;
    }
}

void EditorUndoSystem::redo()
{
    if (!canRedo()) {
        return;
    }
    
    const EditorUndoAction& action = m_undoStack[m_undoIndex];
    m_undoIndex++;
    
    try {
        executeRedo(action);
        emit undoRedoStateChanged();
    } catch (const std::exception& e) {
        qWarning() << "Error during redo operation:" << e.what();
        // Move index back to maintain consistency
        m_undoIndex--;
    }
}

void EditorUndoSystem::clear()
{
    m_undoStack.clear();
    m_undoIndex = 0;
    emit undoRedoStateChanged();
}

void EditorUndoSystem::recordNodeCreated(Lupine::Node* node, const QString& description)
{
    if (!node || !m_scene) return;
    
    EditorUndoAction action(EditorUndoActionType::NodeCreated, description);
    action.nodeId = node->GetUUID();
    action.nodeName = QString::fromStdString(node->GetName());
    action.nodeTypeName = node->GetTypeName();
    action.serializedNodeData = serializeNode(node);
    
    if (node->GetParent()) {
        action.parentNodeId = node->GetParent()->GetUUID();
    }
    
    addAction(std::move(action));
}

void EditorUndoSystem::recordNodeDeleted(Lupine::Node* node, const QString& description)
{
    if (!node || !m_scene) return;

    EditorUndoAction action(EditorUndoActionType::NodeDeleted, description);
    action.nodeId = node->GetUUID();
    action.nodeName = QString::fromStdString(node->GetName());
    action.nodeTypeName = node->GetTypeName();
    action.serializedNodeData = serializeNode(node);

    // Create a backup copy of the node for undo
    action.backupNode = node->Duplicate("");

    if (node->GetParent()) {
        action.parentNodeId = node->GetParent()->GetUUID();
    }

    addAction(std::move(action));
}

void EditorUndoSystem::recordNodeRenamed(Lupine::Node* node, const QString& oldName, const QString& newName)
{
    if (!node || !m_scene) return;

    EditorUndoAction action(EditorUndoActionType::NodeRenamed, QString("Rename '%1' to '%2'").arg(oldName, newName));
    action.nodeId = node->GetUUID();
    action.oldValue = Lupine::ExportValue(oldName.toStdString());
    action.newValue = Lupine::ExportValue(newName.toStdString());

    addAction(std::move(action));
}

void EditorUndoSystem::recordNodeReparented(Lupine::Node* node, Lupine::Node* oldParent, Lupine::Node* newParent)
{
    if (!node || !m_scene) return;
    
    QString oldParentName = oldParent ? QString::fromStdString(oldParent->GetName()) : "None";
    QString newParentName = newParent ? QString::fromStdString(newParent->GetName()) : "None";
    
    EditorUndoAction action(EditorUndoActionType::NodeReparented, 
                           QString("Move '%1' from '%2' to '%3'")
                           .arg(QString::fromStdString(node->GetName()), oldParentName, newParentName));
    action.nodeId = node->GetUUID();
    
    if (oldParent) {
        action.oldValue = Lupine::ExportValue(oldParent->GetUUID().ToString());
    }
    if (newParent) {
        action.newValue = Lupine::ExportValue(newParent->GetUUID().ToString());
    }
    
    addAction(std::move(action));
}

void EditorUndoSystem::recordNodeTransformChanged(Lupine::Node* node, const glm::vec3& oldPos, const glm::vec3& newPos,
                                                 const glm::vec3& oldRot, const glm::vec3& newRot,
                                                 const glm::vec3& oldScale, const glm::vec3& newScale,
                                                 const QString& description)
{
    if (!node || !m_scene) return;

    EditorUndoAction action(EditorUndoActionType::NodeTransformChanged, description);
    action.nodeId = node->GetUUID();
    action.oldPosition = oldPos;
    action.newPosition = newPos;
    action.oldRotation = oldRot;
    action.newRotation = newRot;
    action.oldScale = oldScale;
    action.newScale = newScale;

    addAction(std::move(action));
}

void EditorUndoSystem::recordNodePropertyChanged(Lupine::Node* node, const QString& propertyName,
                                                const Lupine::ExportValue& oldValue, const Lupine::ExportValue& newValue,
                                                const QString& description)
{
    if (!node || !m_scene) return;
    
    EditorUndoAction action(EditorUndoActionType::NodePropertyChanged, description);
    action.nodeId = node->GetUUID();
    action.propertyName = propertyName;
    action.oldValue = oldValue;
    action.newValue = newValue;
    
    addAction(std::move(action));
}

void EditorUndoSystem::recordComponentAdded(Lupine::Node* node, Lupine::Component* component, const QString& description)
{
    if (!node || !component || !m_scene) return;

    EditorUndoAction action(EditorUndoActionType::ComponentAdded, description);
    action.nodeId = node->GetUUID();
    action.componentId = component->GetUUID();
    action.componentTypeName = component->GetTypeName();

    addAction(std::move(action));
}

void EditorUndoSystem::recordComponentRemoved(Lupine::Node* node, Lupine::Component* component, const QString& description)
{
    if (!node || !component || !m_scene) return;

    EditorUndoAction action(EditorUndoActionType::ComponentRemoved, description);
    action.nodeId = node->GetUUID();
    action.componentId = component->GetUUID();
    action.componentTypeName = component->GetTypeName();

    addAction(std::move(action));
}

void EditorUndoSystem::recordComponentPropertyChanged(Lupine::Node* node, Lupine::Component* component, const QString& propertyName,
                                                     const Lupine::ExportValue& oldValue, const Lupine::ExportValue& newValue,
                                                     const QString& description)
{
    if (!node || !component || !m_scene) return;
    
    EditorUndoAction action(EditorUndoActionType::ComponentPropertyChanged, description);
    action.nodeId = node->GetUUID();
    action.componentId = component->GetUUID();
    action.propertyName = propertyName;
    action.oldValue = oldValue;
    action.newValue = newValue;
    
    addAction(std::move(action));
}

void EditorUndoSystem::beginBulkOperation(const QString& description)
{
    if (m_recordingBulkOperation) {
        endBulkOperation(); // End previous bulk operation
    }
    
    m_recordingBulkOperation = true;
    m_bulkActions.clear();
    m_bulkDescription = description;
}

void EditorUndoSystem::endBulkOperation()
{
    if (!m_recordingBulkOperation || m_bulkActions.empty()) {
        m_recordingBulkOperation = false;
        return;
    }
    
    EditorUndoAction bulkAction(EditorUndoActionType::BulkOperation, m_bulkDescription);
    
    // Create custom undo/redo functions for bulk operation
    // Move the bulk actions into the bulk action for storage
    bulkAction.bulkActions = std::move(m_bulkActions);

    bulkAction.customUndoFunction = [this, &bulkAction]() {
        // Execute undo for all actions in reverse order
        for (auto it = bulkAction.bulkActions.rbegin(); it != bulkAction.bulkActions.rend(); ++it) {
            executeUndo(*it);
        }
    };

    bulkAction.customRedoFunction = [this, &bulkAction]() {
        // Execute redo for all actions in forward order
        for (const auto& action : bulkAction.bulkActions) {
            executeRedo(action);
        }
    };
    
    // Remove any redo actions and add the bulk action
    if (m_undoIndex < static_cast<int>(m_undoStack.size())) {
        m_undoStack.erase(m_undoStack.begin() + m_undoIndex, m_undoStack.end());
    }
    
    m_undoStack.push_back(std::move(bulkAction));
    m_undoIndex = static_cast<int>(m_undoStack.size());
    
    // Limit stack size
    if (static_cast<int>(m_undoStack.size()) > m_maxUndoSteps) {
        m_undoStack.erase(m_undoStack.begin());
        m_undoIndex--;
    }
    
    m_recordingBulkOperation = false;
    m_bulkActions.clear();
    
    emit actionRecorded(m_bulkDescription);
    emit undoRedoStateChanged();
}

void EditorUndoSystem::recordCustomOperation(const QString& description, 
                                            std::function<void()> undoFunction, 
                                            std::function<void()> redoFunction)
{
    EditorUndoAction action(EditorUndoActionType::BulkOperation, description);
    action.customUndoFunction = undoFunction;
    action.customRedoFunction = redoFunction;
    
    addAction(std::move(action));
}

void EditorUndoSystem::setMaxUndoSteps(int maxSteps)
{
    m_maxUndoSteps = maxSteps;
    
    // Trim stack if necessary
    while (static_cast<int>(m_undoStack.size()) > m_maxUndoSteps) {
        m_undoStack.erase(m_undoStack.begin());
        if (m_undoIndex > 0) {
            m_undoIndex--;
        }
    }
    
    emit undoRedoStateChanged();
}

void EditorUndoSystem::setScene(Lupine::Scene* scene)
{
    if (m_scene != scene) {
        clear(); // Clear undo stack when scene changes
        m_scene = scene;
    }
}

QString EditorUndoSystem::getUndoDescription() const
{
    if (canUndo()) {
        return m_undoStack[m_undoIndex - 1].description;
    }
    return QString();
}

QString EditorUndoSystem::getRedoDescription() const
{
    if (canRedo()) {
        return m_undoStack[m_undoIndex].description;
    }
    return QString();
}

void EditorUndoSystem::addAction(EditorUndoAction&& action)
{
    if (m_recordingBulkOperation) {
        m_bulkActions.push_back(std::move(action));
        return;
    }

    // Remove any redo actions
    if (m_undoIndex < static_cast<int>(m_undoStack.size())) {
        m_undoStack.erase(m_undoStack.begin() + m_undoIndex, m_undoStack.end());
    }

    QString description = action.description; // Save description before moving
    m_undoStack.push_back(std::move(action));
    m_undoIndex = static_cast<int>(m_undoStack.size());

    // Limit stack size
    if (static_cast<int>(m_undoStack.size()) > m_maxUndoSteps) {
        m_undoStack.erase(m_undoStack.begin());
        m_undoIndex--;
    }

    emit actionRecorded(description);
    emit undoRedoStateChanged();
}

void EditorUndoSystem::executeUndo(const EditorUndoAction& action)
{
    if (!m_scene) return;

    switch (action.type) {
        case EditorUndoActionType::NodeCreated: {
            // Remove the created node
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && node->GetParent()) {
                node->GetParent()->RemoveChild(node->GetUUID());
            }
            break;
        }

        case EditorUndoActionType::NodeDeleted: {
            // Recreate the deleted node using the backup copy
            if (action.backupNode) {
                // Create a new copy from the backup
                auto recreatedNode = action.backupNode->Duplicate("");
                if (recreatedNode) {
                    Lupine::Node* parent = findNodeById(action.parentNodeId);
                    if (parent) {
                        parent->AddChild(std::move(recreatedNode));
                    } else {
                        // Add to root if parent not found
                        m_scene->GetRootNode()->AddChild(std::move(recreatedNode));
                    }
                }
            }
            break;
        }

        case EditorUndoActionType::NodeRenamed: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && std::holds_alternative<std::string>(action.oldValue)) {
                node->SetName(std::get<std::string>(action.oldValue));
            }
            break;
        }

        case EditorUndoActionType::NodeReparented: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && std::holds_alternative<std::string>(action.oldValue)) {
                Lupine::UUID oldParentId = Lupine::UUID::FromString(std::get<std::string>(action.oldValue));
                Lupine::Node* oldParent = findNodeById(oldParentId);
                if (oldParent && node->GetParent()) {
                    auto removedNode = node->GetParent()->RemoveChild(node->GetUUID());
                    if (removedNode) {
                        oldParent->AddChild(std::move(removedNode));
                    }
                }
            }
            break;
        }

        case EditorUndoActionType::NodeTransformChanged: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node) {
                // Apply old transform values based on node type
                if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
                    node2d->SetPosition(glm::vec2(action.oldPosition.x, action.oldPosition.y));
                    node2d->SetRotation(action.oldRotation.z);
                    node2d->SetScale(glm::vec2(action.oldScale.x, action.oldScale.y));
                } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
                    node3d->SetPosition(action.oldPosition);
                    node3d->SetRotation(action.oldRotation);
                    node3d->SetScale(action.oldScale);
                } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
                    control->SetPosition(glm::vec2(action.oldPosition.x, action.oldPosition.y));
                    control->SetSize(glm::vec2(action.oldScale.x, action.oldScale.y));
                }
            }
            break;
        }

        case EditorUndoActionType::BulkOperation: {
            if (action.customUndoFunction) {
                action.customUndoFunction();
            }
            break;
        }

        default:
            qWarning() << "Unhandled undo action type:" << static_cast<int>(action.type);
            break;
    }
}

void EditorUndoSystem::executeRedo(const EditorUndoAction& action)
{
    if (!m_scene) return;

    switch (action.type) {
        case EditorUndoActionType::NodeCreated: {
            // Recreate the node
            auto recreatedNode = deserializeNode(action.serializedNodeData);
            if (recreatedNode) {
                Lupine::Node* parent = findNodeById(action.parentNodeId);
                if (parent) {
                    parent->AddChild(std::move(recreatedNode));
                } else {
                    m_scene->GetRootNode()->AddChild(std::move(recreatedNode));
                }
            }
            break;
        }

        case EditorUndoActionType::NodeDeleted: {
            // Remove the node again
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && node->GetParent()) {
                node->GetParent()->RemoveChild(node->GetUUID());
            }
            break;
        }

        case EditorUndoActionType::NodeRenamed: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && std::holds_alternative<std::string>(action.newValue)) {
                node->SetName(std::get<std::string>(action.newValue));
            }
            break;
        }

        case EditorUndoActionType::NodeReparented: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node && std::holds_alternative<std::string>(action.newValue)) {
                Lupine::UUID newParentId = Lupine::UUID::FromString(std::get<std::string>(action.newValue));
                Lupine::Node* newParent = findNodeById(newParentId);
                if (newParent && node->GetParent()) {
                    auto removedNode = node->GetParent()->RemoveChild(node->GetUUID());
                    if (removedNode) {
                        newParent->AddChild(std::move(removedNode));
                    }
                }
            }
            break;
        }

        case EditorUndoActionType::NodeTransformChanged: {
            Lupine::Node* node = findNodeById(action.nodeId);
            if (node) {
                // Apply new transform values based on node type
                if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
                    node2d->SetPosition(glm::vec2(action.newPosition.x, action.newPosition.y));
                    node2d->SetRotation(action.newRotation.z);
                    node2d->SetScale(glm::vec2(action.newScale.x, action.newScale.y));
                } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
                    node3d->SetPosition(action.newPosition);
                    node3d->SetRotation(action.newRotation);
                    node3d->SetScale(action.newScale);
                } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
                    control->SetPosition(glm::vec2(action.newPosition.x, action.newPosition.y));
                    control->SetSize(glm::vec2(action.newScale.x, action.newScale.y));
                }
            }
            break;
        }

        case EditorUndoActionType::BulkOperation: {
            if (action.customRedoFunction) {
                action.customRedoFunction();
            }
            break;
        }

        default:
            qWarning() << "Unhandled redo action type:" << static_cast<int>(action.type);
            break;
    }
}

Lupine::Node* EditorUndoSystem::findNodeById(const Lupine::UUID& nodeId) const
{
    if (!m_scene || !m_scene->GetRootNode()) {
        return nullptr;
    }

    // Recursive search function
    std::function<Lupine::Node*(Lupine::Node*)> searchNode = [&](Lupine::Node* node) -> Lupine::Node* {
        if (node->GetUUID() == nodeId) {
            return node;
        }

        for (const auto& child : node->GetChildren()) {
            if (auto* found = searchNode(child.get())) {
                return found;
            }
        }

        return nullptr;
    };

    return searchNode(m_scene->GetRootNode());
}

Lupine::Component* EditorUndoSystem::findComponentById(Lupine::Node* node, const Lupine::UUID& componentId) const
{
    if (!node) return nullptr;

    const auto& components = node->GetAllComponents();
    for (const auto& component : components) {
        if (component && component->GetUUID() == componentId) {
            return component.get();
        }
    }

    return nullptr;
}

std::string EditorUndoSystem::serializeNode(Lupine::Node* node) const
{
    if (!node) return "";

    try {
        // Use the SceneSerializer to serialize the node to JSON
        Lupine::JsonNode nodeJson = Lupine::SceneSerializer::SerializeNode(node);
        return Lupine::JsonUtils::Stringify(nodeJson, false);
    } catch (const std::exception& e) {
        qWarning() << "Error serializing node:" << e.what();
        return "";
    }
}

std::unique_ptr<Lupine::Node> EditorUndoSystem::deserializeNode(const std::string& data) const
{
    if (data.empty()) return nullptr;

    try {
        // Parse the JSON data and use SceneSerializer to deserialize the node
        Lupine::JsonNode nodeJson = Lupine::JsonUtils::Parse(data);
        return Lupine::SceneSerializer::DeserializeNode(nodeJson);
    } catch (const std::exception& e) {
        qWarning() << "Error deserializing node:" << e.what();
        return nullptr;
    }
}
