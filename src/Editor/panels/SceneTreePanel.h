#pragma once

#ifndef SCENETREEPANEL_H
#define SCENETREEPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>
#include <QSet>

namespace Lupine {
    class Scene;
    class Node;
}

// Forward declaration
class SceneTreePanel;

/**
 * @brief Custom tree widget that handles drag and drop for scene nodes
 */
class SceneTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit SceneTreeWidget(SceneTreePanel* parent);

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;

private:
    SceneTreePanel* m_sceneTreePanel;
};

class SceneTreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTreePanel(QWidget *parent = nullptr);
    ~SceneTreePanel();
    
    void setScene(Lupine::Scene* scene);
    void setMainWindow(class MainWindow* mainWindow);
    void refreshTree();
    void selectNode(Lupine::Node* node);
    Lupine::Node* getSelectedNode() const;
    void updateNodeName(Lupine::Node* node, const QString& newName);

    // Node operations
    void onDuplicateNode();

    // Drag and drop support
    bool handleNodeReparenting(Lupine::Node* draggedNode, Lupine::Node* newParent);

signals:
    void nodeSelected(Lupine::Node* node);
    void nodeDoubleClicked(Lupine::Node* node);
    void nodeRenamed(Lupine::Node* node, const QString& newName);
    void nodeDeleted(Lupine::Node* node);
    void nodeParentChanged(Lupine::Node* node, Lupine::Node* newParent);
    void nodeDuplicated(Lupine::Node* originalNode, Lupine::Node* duplicatedNode);

private slots:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onAddNode();
    void onDeleteNode();
    void onRenameNode();
    void onDeferredNodeDeletion();
    void onToggleNodeVisibility(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void setupContextMenu();
    void populateTree();
    void addNodeToTree(Lupine::Node* node, QTreeWidgetItem* parentItem = nullptr);
    QTreeWidgetItem* findItemForNode(Lupine::Node* node) const;
    Lupine::Node* getNodeFromItem(QTreeWidgetItem* item) const;
    bool isNodeValid(Lupine::Node* node) const;
    bool isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const;
    
    // UI Components
    QVBoxLayout* m_layout;
    QHBoxLayout* m_toolbarLayout;
    SceneTreeWidget* m_treeWidget;
    QToolButton* m_addNodeButton;
    QToolButton* m_deleteNodeButton;
    
    // Context menu
    QMenu* m_contextMenu;
    QAction* m_addNodeAction;
    QAction* m_deleteNodeAction;
    QAction* m_renameNodeAction;
    QAction* m_duplicateNodeAction;
    
    // Data
    Lupine::Scene* m_scene;
    class MainWindow* m_mainWindow;
    QTreeWidgetItem* m_rootItem;

    // Safe deletion mechanism
    QTimer* m_deletionTimer;
    QSet<Lupine::Node*> m_nodesToDelete;
    QSet<Lupine::Node*> m_deletedNodes;

    // Signal recursion guard
    bool m_blockSelectionSignals;
};

#endif // SCENETREEPANEL_H
