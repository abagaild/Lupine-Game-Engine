#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <vector>
#include <memory>
#include <functional>
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"

namespace Lupine {
    class Scene;
    class Component;
}

/**
 * @brief Types of operations that can be undone/redone
 */
enum class EditorUndoActionType {
    NodeCreated,
    NodeDeleted,
    NodeRenamed,
    NodeReparented,
    NodeTransformChanged,
    NodePropertyChanged,
    ComponentAdded,
    ComponentRemoved,
    ComponentPropertyChanged,
    BulkOperation
};

/**
 * @brief Data structure for storing undo/redo action information
 */
struct EditorUndoAction {
    EditorUndoActionType type;
    QString description;
    
    // Node-related data
    Lupine::UUID nodeId;
    QString nodeName;
    Lupine::UUID parentNodeId;
    std::string nodeTypeName;
    
    // Property-related data
    QString propertyName;
    Lupine::ExportValue oldValue;
    Lupine::ExportValue newValue;
    
    // Component-related data
    Lupine::UUID componentId;
    std::string componentTypeName;
    
    // Transform data
    glm::vec3 oldPosition;
    glm::vec3 newPosition;
    glm::vec3 oldRotation;
    glm::vec3 newRotation;
    glm::vec3 oldScale;
    glm::vec3 newScale;
    
    // Serialized node data for creation/deletion
    std::string serializedNodeData;
    std::unique_ptr<Lupine::Node> backupNode; // For node deletion undo

    // Custom undo/redo functions for complex operations
    std::function<void()> customUndoFunction;
    std::function<void()> customRedoFunction;

    // For bulk operations - store the individual actions
    std::vector<EditorUndoAction> bulkActions;
    
    EditorUndoAction(EditorUndoActionType actionType, const QString& desc)
        : type(actionType), description(desc) {}

    // Make it movable but not copyable
    EditorUndoAction(const EditorUndoAction&) = delete;
    EditorUndoAction& operator=(const EditorUndoAction&) = delete;
    EditorUndoAction(EditorUndoAction&&) = default;
    EditorUndoAction& operator=(EditorUndoAction&&) = default;
};

/**
 * @brief Global undo/redo system for the editor
 */
class EditorUndoSystem : public QObject
{
    Q_OBJECT

public:
    explicit EditorUndoSystem(QObject* parent = nullptr);
    ~EditorUndoSystem();

    // Undo/Redo operations
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clear();
    
    // Action recording
    void recordNodeCreated(Lupine::Node* node, const QString& description = "Create Node");
    void recordNodeDeleted(Lupine::Node* node, const QString& description = "Delete Node");
    void recordNodeRenamed(Lupine::Node* node, const QString& oldName, const QString& newName);
    void recordNodeReparented(Lupine::Node* node, Lupine::Node* oldParent, Lupine::Node* newParent);
    void recordNodeTransformChanged(Lupine::Node* node, const glm::vec3& oldPos, const glm::vec3& newPos,
                                   const glm::vec3& oldRot, const glm::vec3& newRot,
                                   const glm::vec3& oldScale, const glm::vec3& newScale,
                                   const QString& description = "Transform Changed");
    void recordNodePropertyChanged(Lupine::Node* node, const QString& propertyName,
                                  const Lupine::ExportValue& oldValue, const Lupine::ExportValue& newValue,
                                  const QString& description = "Property Changed");
    void recordComponentAdded(Lupine::Node* node, Lupine::Component* component, const QString& description = "Add Component");
    void recordComponentRemoved(Lupine::Node* node, Lupine::Component* component, const QString& description = "Remove Component");
    void recordComponentPropertyChanged(Lupine::Node* node, Lupine::Component* component, const QString& propertyName,
                                       const Lupine::ExportValue& oldValue, const Lupine::ExportValue& newValue,
                                       const QString& description = "Component Property Changed");
    
    // Bulk operations
    void beginBulkOperation(const QString& description);
    void endBulkOperation();
    
    // Custom operations
    void recordCustomOperation(const QString& description, 
                              std::function<void()> undoFunction, 
                              std::function<void()> redoFunction);
    
    // Configuration
    void setMaxUndoSteps(int maxSteps);
    int getMaxUndoSteps() const { return m_maxUndoSteps; }
    
    // Scene management
    void setScene(Lupine::Scene* scene);
    Lupine::Scene* getScene() const { return m_scene; }
    
    // Status information
    QString getUndoDescription() const;
    QString getRedoDescription() const;
    int getUndoStackSize() const { return static_cast<int>(m_undoStack.size()); }
    int getCurrentUndoIndex() const { return m_undoIndex; }

signals:
    void undoRedoStateChanged();
    void actionRecorded(const QString& description);

private:
    void addAction(EditorUndoAction&& action);
    void executeUndo(const EditorUndoAction& action);
    void executeRedo(const EditorUndoAction& action);
    
    // Helper methods
    Lupine::Node* findNodeById(const Lupine::UUID& nodeId) const;
    Lupine::Component* findComponentById(Lupine::Node* node, const Lupine::UUID& componentId) const;
    std::string serializeNode(Lupine::Node* node) const;
    std::unique_ptr<Lupine::Node> deserializeNode(const std::string& data) const;
    
    std::vector<EditorUndoAction> m_undoStack;
    int m_undoIndex;
    int m_maxUndoSteps;
    
    Lupine::Scene* m_scene;
    
    // Bulk operation state
    bool m_recordingBulkOperation;
    std::vector<EditorUndoAction> m_bulkActions;
    QString m_bulkDescription;
};
