#include "SceneTreePanel.h"
#include "../dialogs/AddNodeDialog.h"
#include "../IconManager.h"
#include "../MainWindow.h"
#include "../EditorUndoSystem.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "../../core/CrashHandler.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QDragMoveEvent>
#include <QMimeData>

// SceneTreeWidget implementation
SceneTreeWidget::SceneTreeWidget(SceneTreePanel* parent)
    : QTreeWidget(parent)
    , m_sceneTreePanel(parent)
{
}

void SceneTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void SceneTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void SceneTreeWidget::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* targetItem = itemAt(event->pos());
    QTreeWidgetItem* draggedItem = currentItem();

    if (!draggedItem || !targetItem || draggedItem == targetItem) {
        event->ignore();
        return;
    }

    // Get the nodes from the items
    Lupine::Node* draggedNode = static_cast<Lupine::Node*>(draggedItem->data(0, Qt::UserRole).value<void*>());
    Lupine::Node* targetNode = static_cast<Lupine::Node*>(targetItem->data(0, Qt::UserRole).value<void*>());

    if (!draggedNode || !targetNode) {
        event->ignore();
        return;
    }

    // Prevent dropping a node onto itself or its descendants
    Lupine::Node* checkParent = targetNode;
    while (checkParent) {
        if (checkParent == draggedNode) {
            event->ignore();
            return;
        }
        checkParent = checkParent->GetParent();
    }

    // Handle the reparenting through the scene tree panel
    if (m_sceneTreePanel->handleNodeReparenting(draggedNode, targetNode)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

SceneTreePanel::SceneTreePanel(QWidget *parent)
    : QWidget(parent)
    , m_scene(nullptr)
    , m_mainWindow(nullptr)
    , m_rootItem(nullptr)
    , m_deletionTimer(nullptr)
    , m_blockSelectionSignals(false)
{
    setupUI();
    setupContextMenu();

    // Setup safe deletion mechanism
    m_deletionTimer = new QTimer(this);
    m_deletionTimer->setSingleShot(true);
    m_deletionTimer->setInterval(0); // Process on next event loop iteration
    connect(m_deletionTimer, &QTimer::timeout, this, &SceneTreePanel::onDeferredNodeDeletion);
}

SceneTreePanel::~SceneTreePanel()
{
    // Stop any pending deletion timer
    if (m_deletionTimer && m_deletionTimer->isActive()) {
        m_deletionTimer->stop();
    }

    // Clear our tracking sets
    m_nodesToDelete.clear();
    m_deletedNodes.clear();
}

void SceneTreePanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    m_toolbarLayout = new QHBoxLayout();
    m_addNodeButton = new QToolButton();
    m_addNodeButton->setText("+");
    m_addNodeButton->setToolTip("Add Node");
    
    m_deleteNodeButton = new QToolButton();
    m_deleteNodeButton->setText("-");
    m_deleteNodeButton->setToolTip("Delete Node");
    m_deleteNodeButton->setEnabled(false);
    
    m_toolbarLayout->addWidget(m_addNodeButton);
    m_toolbarLayout->addWidget(m_deleteNodeButton);
    m_toolbarLayout->addStretch();
    
    // Tree widget
    m_treeWidget = new SceneTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Scene" << "Visible");
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_treeWidget->setDefaultDropAction(Qt::MoveAction);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setColumnWidth(0, 200); // Set width for name column
    m_treeWidget->setColumnWidth(1, 50);  // Set width for visibility column

    // Make visibility column fixed width and non-resizable
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_treeWidget->header()->setMinimumSectionSize(50);
    m_treeWidget->header()->resizeSection(1, 50);

    // Right-justify the visibility column header
    m_treeWidget->headerItem()->setTextAlignment(1, Qt::AlignRight);
    
    m_layout->addLayout(m_toolbarLayout);
    m_layout->addWidget(m_treeWidget);
    
    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, 
            this, &SceneTreePanel::onItemSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, 
            this, &SceneTreePanel::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::itemChanged,
            this, &SceneTreePanel::onItemChanged);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &SceneTreePanel::onCustomContextMenuRequested);
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &SceneTreePanel::onToggleNodeVisibility);
    
    connect(m_addNodeButton, &QToolButton::clicked, this, &SceneTreePanel::onAddNode);
    connect(m_deleteNodeButton, &QToolButton::clicked, this, &SceneTreePanel::onDeleteNode);
}

void SceneTreePanel::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_addNodeAction = m_contextMenu->addAction("Add Node");
    m_contextMenu->addSeparator();
    m_renameNodeAction = m_contextMenu->addAction("Rename");
    m_duplicateNodeAction = m_contextMenu->addAction("Duplicate");
    m_contextMenu->addSeparator();
    m_deleteNodeAction = m_contextMenu->addAction("Delete");
    
    connect(m_addNodeAction, &QAction::triggered, this, &SceneTreePanel::onAddNode);
    connect(m_renameNodeAction, &QAction::triggered, this, &SceneTreePanel::onRenameNode);
    connect(m_duplicateNodeAction, &QAction::triggered, this, &SceneTreePanel::onDuplicateNode);
    connect(m_deleteNodeAction, &QAction::triggered, this, &SceneTreePanel::onDeleteNode);
}

void SceneTreePanel::setScene(Lupine::Scene* scene)
{
    // Stop any pending deletions when changing scenes
    if (m_deletionTimer && m_deletionTimer->isActive()) {
        m_deletionTimer->stop();
    }

    // Clear deletion tracking when changing scenes
    m_nodesToDelete.clear();
    m_deletedNodes.clear();

    m_scene = scene;

    // Debug: Log scene information
    if (scene) {
        auto* rootNode = scene->GetRootNode();
        if (rootNode) {
            qDebug() << "SceneTreePanel: Setting scene with root node:" << QString::fromStdString(rootNode->GetName())
                     << "children count:" << rootNode->GetChildren().size();
        } else {
            qDebug() << "SceneTreePanel: Setting scene but no root node found";
        }
    } else {
        qDebug() << "SceneTreePanel: Setting null scene";
    }

    refreshTree();
}

void SceneTreePanel::setMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void SceneTreePanel::refreshTree()
{
    try {
        // Block signals during refresh to prevent cascading events
        m_treeWidget->blockSignals(true);

        m_treeWidget->clear();
        m_rootItem = nullptr;

        if (!m_scene) {
            m_treeWidget->blockSignals(false);
            return;
        }

        populateTree();
        m_treeWidget->expandAll();

        // Re-enable signals
        m_treeWidget->blockSignals(false);
    } catch (...) {
        // Ensure signals are re-enabled even if an exception occurs
        m_treeWidget->blockSignals(false);
        throw;
    }
}

void SceneTreePanel::populateTree()
{
    if (!m_scene || !m_scene->GetRootNode()) {
        qDebug() << "SceneTreePanel::populateTree: No scene or root node";
        return;
    }

    Lupine::Node* rootNode = m_scene->GetRootNode();
    qDebug() << "SceneTreePanel::populateTree: Creating root item for" << QString::fromStdString(rootNode->GetName());

    m_rootItem = new QTreeWidgetItem(m_treeWidget);
    m_rootItem->setText(0, QString::fromStdString(rootNode->GetName()));
    m_rootItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(rootNode)));
    m_rootItem->setFlags(m_rootItem->flags() | Qt::ItemIsEditable);

    // Add visibility icon for root node
    QIcon rootVisibilityIcon;
    if (rootNode->IsVisible()) {
        rootVisibilityIcon = QIcon("icons/visible.png");
    } else {
        rootVisibilityIcon = QIcon("icons/invisible.png");
    }
    m_rootItem->setIcon(1, rootVisibilityIcon);
    m_rootItem->setTextAlignment(1, Qt::AlignRight);

    // Add child nodes
    qDebug() << "SceneTreePanel::populateTree: Adding" << rootNode->GetChildren().size() << "children";
    for (const auto& child : rootNode->GetChildren()) {
        qDebug() << "SceneTreePanel::populateTree: Adding child" << QString::fromStdString(child->GetName());
        addNodeToTree(child.get(), m_rootItem);
    }
}

void SceneTreePanel::addNodeToTree(Lupine::Node* node, QTreeWidgetItem* parentItem)
{
    if (!node) return;

    // Skip nodes that are marked for deletion
    if (m_deletedNodes.contains(node)) {
        return;
    }

    try {
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, QString::fromStdString(node->GetName()));
        item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(node)));
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // Set icon using the new icon system with fallback
        QString nodeType = QString::fromStdString(node->GetTypeName());
        std::string category = "";

        // Determine category based on node type
        if (nodeType.contains("2D")) category = "2D";
        else if (nodeType.contains("3D")) category = "3D";
        else if (nodeType == "Control" || nodeType.contains("UI")) category = "UI";
        else category = "Core";

        QIcon icon = IconManager::Instance().GetNodeIcon(node->GetTypeName(), category);
        item->setIcon(0, icon);

        // Add visibility icon in the second column
        QIcon visibilityIcon;
        if (node->IsVisible()) {
            visibilityIcon = QIcon("icons/visible.png");
        } else {
            visibilityIcon = QIcon("icons/invisible.png");
        }
        item->setIcon(1, visibilityIcon);

        // Right-align the visibility icon
        item->setTextAlignment(1, Qt::AlignRight);

        // Make the visibility column clickable but not checkable (we handle clicks manually)
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

        // Add child nodes recursively
        for (const auto& child : node->GetChildren()) {
            addNodeToTree(child.get(), item);
        }
    } catch (...) {
        // Ignore exceptions during tree population to prevent crashes
        // This can happen if a node is being deleted while we're building the tree
    }
}

void SceneTreePanel::selectNode(Lupine::Node* node)
{
    LUPINE_AUTO_TRACK_FUNCTION();

    // Block signals to prevent circular signal emissions
    m_blockSelectionSignals = true;

    QTreeWidgetItem* item = findItemForNode(node);
    if (item) {
        m_treeWidget->setCurrentItem(item);
    }

    // Re-enable signals
    m_blockSelectionSignals = false;
}

Lupine::Node* SceneTreePanel::getSelectedNode() const
{
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    return getNodeFromItem(item);
}

QTreeWidgetItem* SceneTreePanel::findItemForNode(Lupine::Node* node) const
{
    if (!node) return nullptr;
    
    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        if (getNodeFromItem(*it) == node) {
            return *it;
        }
        ++it;
    }
    return nullptr;
}

Lupine::Node* SceneTreePanel::getNodeFromItem(QTreeWidgetItem* item) const
{
    if (!item) return nullptr;

    // Get the raw pointer from the item data
    Lupine::Node* node = static_cast<Lupine::Node*>(item->data(0, Qt::UserRole).value<void*>());

    // Validate that the node is still valid (not deleted)
    if (!isNodeValid(node)) {
        return nullptr;
    }

    return node;
}

bool SceneTreePanel::isNodeValid(Lupine::Node* node) const
{
    if (!node) return false;

    // Check if the node is in our deleted nodes set
    if (m_deletedNodes.contains(node)) {
        return false;
    }

    try {
        // First check if the node itself is in a valid state
        if (!node->IsValidNode()) {
            return false;
        }

        // Additional validation: check if the node exists in the scene hierarchy
        if (m_scene && m_scene->GetRootNode()) {
            // For root node, check directly
            if (node == m_scene->GetRootNode()) {
                return true;
            }

            // For other nodes, use recursive search without creating vectors
            return isNodeInHierarchy(m_scene->GetRootNode(), node);
        }
    } catch (...) {
        // If any exception occurs during validation, consider the node invalid
        return false;
    }

    return false;
}

bool SceneTreePanel::isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const
{
    if (!root || !target) return false;

    try {
        // Check if this is the target node
        if (root == target) {
            return true;
        }

        // Check children recursively
        for (const auto& child : root->GetChildren()) {
            if (child.get() == target || isNodeInHierarchy(child.get(), target)) {
                return true;
            }
        }
    } catch (...) {
        // If any exception occurs during traversal, return false
        return false;
    }

    return false;
}

void SceneTreePanel::updateNodeName(Lupine::Node* node, const QString& newName)
{
    LUPINE_AUTO_TRACK_FUNCTION();

    if (!node) return;

    QTreeWidgetItem* item = findItemForNode(node);
    if (item) {
        // Block signals to prevent triggering onItemChanged
        m_treeWidget->blockSignals(true);
        item->setText(0, newName);
        m_treeWidget->blockSignals(false);
    }
}

void SceneTreePanel::onItemSelectionChanged()
{
    LUPINE_AUTO_TRACK_FUNCTION();

    // Don't emit signals if we're in the middle of programmatic selection
    if (m_blockSelectionSignals) {
        return;
    }

    try {
        Lupine::Node* selectedNode = getSelectedNode();
        m_deleteNodeButton->setEnabled(selectedNode != nullptr && selectedNode != m_scene->GetRootNode());
        emit nodeSelected(selectedNode);
    } catch (...) {
        // Ignore exceptions during selection changes to prevent crashes
        // This can happen when accessing deleted nodes
        m_deleteNodeButton->setEnabled(false);
        emit nodeSelected(nullptr);
    }
}

void SceneTreePanel::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    try {
        Lupine::Node* node = getNodeFromItem(item);
        if (node) {
            emit nodeDoubleClicked(node);
        }
    } catch (...) {
        // Ignore exceptions during double-click to prevent crashes
    }
}

void SceneTreePanel::onItemChanged(QTreeWidgetItem* item, int column)
{
    LUPINE_AUTO_TRACK_FUNCTION();

    if (column == 0) {
        try {
            Lupine::Node* node = getNodeFromItem(item);
            if (node) {
                QString newName = item->text(0);
                // Only update if the name actually changed to prevent unnecessary signals
                if (newName.toStdString() != node->GetName()) {
                    QString oldName = QString::fromStdString(node->GetName());

                    // Record undo action before changing the name
                    if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                        m_mainWindow->getUndoSystem()->recordNodeRenamed(node, oldName, newName);
                    }

                    node->SetName(newName.toStdString());
                    emit nodeRenamed(node, newName);
                }
            }
        } catch (...) {
            // Ignore exceptions during item changes to prevent crashes
        }
    }
}

void SceneTreePanel::onCustomContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    Lupine::Node* node = getNodeFromItem(item);
    
    // Enable/disable actions based on selection
    bool hasSelection = (node != nullptr);
    bool canDelete = hasSelection && (node != m_scene->GetRootNode());
    
    m_renameNodeAction->setEnabled(hasSelection);
    m_duplicateNodeAction->setEnabled(hasSelection);
    m_deleteNodeAction->setEnabled(canDelete);
    
    m_contextMenu->exec(m_treeWidget->mapToGlobal(pos));
}

void SceneTreePanel::onAddNode()
{
    if (!m_scene) return;

    // Show dialog to choose node type
    AddNodeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        const NodeTypeInfo* nodeType = dialog.getSelectedNodeType();
        if (nodeType) {
            // Create the node using the dialog's factory method
            auto newNode = AddNodeDialog::createNode(*nodeType, "New " + nodeType->name.toStdString());
            Lupine::Node* nodePtr = newNode.get();

            // Add to selected node or root
            Lupine::Node* selectedNode = getSelectedNode();
            if (selectedNode) {
                selectedNode->AddChild(std::move(newNode));
            } else if (m_scene->GetRootNode()) {
                m_scene->GetRootNode()->AddChild(std::move(newNode));
            }

            // Record undo action
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeCreated(nodePtr, "Add Node");
            }

            refreshTree();
            selectNode(nodePtr);
        }
    }
}

void SceneTreePanel::onDeleteNode()
{
    Lupine::Node* selectedNode = getSelectedNode();
    if (!selectedNode || selectedNode == m_scene->GetRootNode()) return;

    int ret = QMessageBox::question(this, "Delete Node",
        QString("Are you sure you want to delete '%1' and all its children?")
        .arg(QString::fromStdString(selectedNode->GetName())));

    if (ret == QMessageBox::Yes) {
        // Record undo action before deletion
        if (m_mainWindow && m_mainWindow->getUndoSystem()) {
            m_mainWindow->getUndoSystem()->recordNodeDeleted(selectedNode, "Delete Node");
        }

        // Mark the node and all its children as deleted immediately
        // to prevent access during the deletion process
        std::function<void(Lupine::Node*)> markDeleted = [&](Lupine::Node* node) {
            if (node) {
                m_deletedNodes.insert(node);
                for (const auto& child : node->GetChildren()) {
                    markDeleted(child.get());
                }
            }
        };
        markDeleted(selectedNode);

        // Add to deferred deletion queue
        m_nodesToDelete.insert(selectedNode);

        // Emit signal before deletion so handlers can still access the node
        // (but they should check isNodeValid first)
        emit nodeDeleted(selectedNode);

        // Clear the current selection to prevent further access
        m_treeWidget->clearSelection();

        // Refresh the tree immediately to remove the visual representation
        refreshTree();

        // Schedule the actual deletion for the next event loop iteration
        // This ensures all pending Qt events are processed first
        if (!m_deletionTimer->isActive()) {
            m_deletionTimer->start();
        }
    }
}

void SceneTreePanel::onRenameNode()
{
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (item) {
        m_treeWidget->editItem(item, 0);
    }
}

void SceneTreePanel::onDuplicateNode()
{
    Lupine::Node* selectedNode = getSelectedNode();
    if (!selectedNode || !m_scene) {
        return;
    }

    // Don't allow duplicating the root node
    if (selectedNode == m_scene->GetRootNode()) {
        QMessageBox::information(this, "Cannot Duplicate", "Cannot duplicate the root node.");
        return;
    }

    // Create a duplicate of the selected node
    auto duplicatedNode = selectedNode->Duplicate();
    if (!duplicatedNode) {
        QMessageBox::warning(this, "Duplication Failed", "Failed to duplicate the selected node.");
        return;
    }

    // Add the duplicated node to the same parent as the original
    Lupine::Node* parent = selectedNode->GetParent();
    if (parent) {
        Lupine::Node* duplicatedPtr = duplicatedNode.get();
        parent->AddChild(std::move(duplicatedNode));

        // Record undo action
        if (m_mainWindow && m_mainWindow->getUndoSystem()) {
            m_mainWindow->getUndoSystem()->recordNodeCreated(duplicatedPtr, "Duplicate Node");
        }

        // Refresh the tree and select the new node
        refreshTree();
        selectNode(duplicatedPtr);

        // Emit signal for node duplication
        emit nodeDuplicated(selectedNode, duplicatedPtr);

        // Mark scene as modified
        if (m_scene) {
            m_scene->MarkModified();
        }
    }
}

void SceneTreePanel::onToggleNodeVisibility(QTreeWidgetItem* item, int column)
{
    // Only handle clicks on the visibility column (column 1)
    if (column != 1 || !item) {
        return;
    }

    Lupine::Node* node = getNodeFromItem(item);
    if (!node) {
        return;
    }

    // Toggle visibility
    bool newVisibility = !node->IsVisible();
    node->SetVisible(newVisibility);

    // Update the icon
    QIcon visibilityIcon;
    if (newVisibility) {
        visibilityIcon = QIcon("icons/visible.png");
    } else {
        visibilityIcon = QIcon("icons/invisible.png");
    }
    item->setIcon(1, visibilityIcon);

    // Mark scene as modified
    if (m_scene) {
        m_scene->MarkModified();
    }
}

bool SceneTreePanel::handleNodeReparenting(Lupine::Node* draggedNode, Lupine::Node* newParent)
{
    if (!draggedNode || !newParent || !m_scene) {
        return false;
    }

    // Don't allow reparenting to self or descendants
    Lupine::Node* checkParent = newParent;
    while (checkParent) {
        if (checkParent == draggedNode) {
            return false;
        }
        checkParent = checkParent->GetParent();
    }

    // Don't allow reparenting the root node
    if (draggedNode == m_scene->GetRootNode()) {
        return false;
    }

    // Get the current parent
    Lupine::Node* currentParent = draggedNode->GetParent();
    if (!currentParent) {
        return false;
    }

    // Remove from current parent and add to new parent
    auto nodePtr = currentParent->RemoveChild(draggedNode->GetUUID());
    if (nodePtr) {
        newParent->AddChild(std::move(nodePtr));

        // Emit signal for parent change
        emit nodeParentChanged(draggedNode, newParent);

        // Refresh the tree to reflect the changes
        refreshTree();

        // Select the moved node
        selectNode(draggedNode);

        return true;
    }

    return false;
}

void SceneTreePanel::onDeferredNodeDeletion()
{
    // Process all nodes scheduled for deletion
    for (Lupine::Node* nodeToDelete : m_nodesToDelete) {
        if (!nodeToDelete) continue;

        try {
            // Double-check that the node is still valid and has a parent
            if (nodeToDelete->IsValidNode() && nodeToDelete->GetParent()) {
                // Verify the parent is also still valid
                Lupine::Node* parent = nodeToDelete->GetParent();
                if (parent && parent->IsValidNode()) {
                    // Actually remove the node from its parent
                    // This will destroy the node and all its children
                    parent->RemoveChild(nodeToDelete->GetUUID());
                }
            }
        } catch (...) {
            // Ignore exceptions during deletion to prevent crashes
            // The node might have already been deleted by a parent deletion
            // or the object might be in an invalid state
        }
    }

    // Clear the deletion queue
    m_nodesToDelete.clear();

    // Note: We don't clear m_deletedNodes here because the pointers are now invalid
    // and we want to keep them marked as deleted for safety
}
