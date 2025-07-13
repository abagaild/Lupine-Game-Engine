#include "NodeSelectionDialog.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include <QHeaderView>
#include <QStyle>

NodeSelectionDialog::NodeSelectionDialog(Lupine::Scene* scene, QWidget* parent)
    : QDialog(parent)
    , m_scene(scene)
    , m_selectedNode(nullptr)
{
    setupUI();
    populateTree();
}

Lupine::Node* NodeSelectionDialog::getSelectedNode() const
{
    return m_selectedNode;
}

void NodeSelectionDialog::setSelectedNode(const Lupine::UUID& nodeUUID)
{
    if (!m_scene) return;
    
    // Find node by UUID in the scene
    std::function<Lupine::Node*(Lupine::Node*)> findNodeByUUID = [&](Lupine::Node* node) -> Lupine::Node* {
        if (node->GetUUID() == nodeUUID) {
            return node;
        }
        
        for (const auto& child : node->GetChildren()) {
            if (auto* found = findNodeByUUID(child.get())) {
                return found;
            }
        }
        return nullptr;
    };
    
    if (auto* rootNode = m_scene->GetRootNode()) {
        if (auto* foundNode = findNodeByUUID(rootNode)) {
            // Find the corresponding tree item and select it
            if (auto* item = findItemForNode(foundNode)) {
                m_treeWidget->setCurrentItem(item);
                m_selectedNode = foundNode;
            }
        }
    }
}

void NodeSelectionDialog::setupUI()
{
    setWindowTitle("Select Node");
    setModal(true);
    resize(400, 500);
    
    m_layout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("Select a node from the scene:");
    m_titleLabel->setStyleSheet("font-weight: bold; margin-bottom: 8px;");
    m_layout->addWidget(m_titleLabel);
    
    // Search box
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search nodes...");
    m_layout->addWidget(m_searchEdit);
    
    // Tree widget
    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabel("Scene Tree");
    m_treeWidget->header()->hide();
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layout->addWidget(m_treeWidget);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_selectButton = new QPushButton("Select");
    m_selectButton->setEnabled(false);
    m_selectButton->setDefault(true);
    m_buttonLayout->addWidget(m_selectButton);
    
    m_cancelButton = new QPushButton("Cancel");
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_layout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &NodeSelectionDialog::onItemSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &NodeSelectionDialog::onItemDoubleClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NodeSelectionDialog::onSearchTextChanged);
    connect(m_selectButton, &QPushButton::clicked, this, &NodeSelectionDialog::onSelectButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &NodeSelectionDialog::onCancelButtonClicked);
}

void NodeSelectionDialog::populateTree()
{
    m_treeWidget->clear();
    
    if (!m_scene || !m_scene->GetRootNode()) return;
    
    Lupine::Node* rootNode = m_scene->GetRootNode();
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString::fromStdString(rootNode->GetName()));
    rootItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(rootNode)));
    
    // Set icon based on node type
    QString nodeType = QString::fromStdString(rootNode->GetTypeName());
    if (nodeType == "Node2D") {
        rootItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    } else if (nodeType == "Node3D") {
        rootItem->setIcon(0, style()->standardIcon(QStyle::SP_ComputerIcon));
    } else if (nodeType == "Control") {
        rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DialogOkButton));
    } else {
        rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    }
    
    // Add child nodes
    for (const auto& child : rootNode->GetChildren()) {
        addNodeToTree(child.get(), rootItem);
    }
    
    m_treeWidget->expandAll();
}

void NodeSelectionDialog::addNodeToTree(Lupine::Node* node, QTreeWidgetItem* parentItem)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
    item->setText(0, QString::fromStdString(node->GetName()));
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(node)));
    
    // Set icon based on node type
    QString nodeType = QString::fromStdString(node->GetTypeName());
    if (nodeType == "Node2D") {
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    } else if (nodeType == "Node3D") {
        item->setIcon(0, style()->standardIcon(QStyle::SP_ComputerIcon));
    } else if (nodeType == "Control") {
        item->setIcon(0, style()->standardIcon(QStyle::SP_DialogOkButton));
    } else {
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    }
    
    // Add child nodes recursively
    for (const auto& child : node->GetChildren()) {
        addNodeToTree(child.get(), item);
    }
}

void NodeSelectionDialog::onItemSelectionChanged()
{
    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();
    m_selectedNode = getNodeFromItem(currentItem);
    m_selectButton->setEnabled(m_selectedNode != nullptr);
}

void NodeSelectionDialog::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    if (item && getNodeFromItem(item)) {
        accept();
    }
}

void NodeSelectionDialog::onSearchTextChanged(const QString& text)
{
    filterTree(text);
}

void NodeSelectionDialog::onSelectButtonClicked()
{
    if (m_selectedNode) {
        accept();
    }
}

void NodeSelectionDialog::onCancelButtonClicked()
{
    reject();
}

void NodeSelectionDialog::filterTree(const QString& searchText)
{
    if (searchText.isEmpty()) {
        // Show all items
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
            expandFilteredItems(item, true);
        }
        m_treeWidget->expandAll();
    } else {
        // Filter items
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
            bool hasMatch = itemMatchesFilter(item, searchText);
            expandFilteredItems(item, hasMatch);
        }
    }
}

void NodeSelectionDialog::expandFilteredItems(QTreeWidgetItem* item, bool hasMatch)
{
    if (!item) return;
    
    bool childHasMatch = false;
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        bool childMatches = itemMatchesFilter(child, m_searchEdit->text());
        expandFilteredItems(child, childMatches);
        if (childMatches) {
            childHasMatch = true;
        }
    }
    
    bool shouldShow = hasMatch || childHasMatch;
    item->setHidden(!shouldShow);
    if (shouldShow && (hasMatch || childHasMatch)) {
        item->setExpanded(true);
    }
}

bool NodeSelectionDialog::itemMatchesFilter(QTreeWidgetItem* item, const QString& filter)
{
    if (!item || filter.isEmpty()) return true;
    
    return item->text(0).contains(filter, Qt::CaseInsensitive);
}

QTreeWidgetItem* NodeSelectionDialog::findItemForNode(Lupine::Node* node) const
{
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> searchItem = [&](QTreeWidgetItem* item) -> QTreeWidgetItem* {
        if (getNodeFromItem(item) == node) {
            return item;
        }
        
        for (int i = 0; i < item->childCount(); ++i) {
            if (auto* found = searchItem(item->child(i))) {
                return found;
            }
        }
        return nullptr;
    };
    
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        if (auto* found = searchItem(m_treeWidget->topLevelItem(i))) {
            return found;
        }
    }
    return nullptr;
}

Lupine::Node* NodeSelectionDialog::getNodeFromItem(QTreeWidgetItem* item) const
{
    if (!item) return nullptr;
    
    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        return static_cast<Lupine::Node*>(data.value<void*>());
    }
    return nullptr;
}
