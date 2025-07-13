#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

namespace Lupine {
    class Scene;
    class Node;
    class UUID;
}

/**
 * @brief Dialog for selecting a node from the scene tree
 */
class NodeSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NodeSelectionDialog(Lupine::Scene* scene, QWidget* parent = nullptr);
    
    /**
     * @brief Get the selected node
     * @return Selected node or nullptr if none selected
     */
    Lupine::Node* getSelectedNode() const;
    
    /**
     * @brief Set the currently selected node by UUID
     * @param nodeUUID UUID of the node to select
     */
    void setSelectedNode(const Lupine::UUID& nodeUUID);

private slots:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onSelectButtonClicked();
    void onCancelButtonClicked();

private:
    void setupUI();
    void populateTree();
    void addNodeToTree(Lupine::Node* node, QTreeWidgetItem* parentItem = nullptr);
    void filterTree(const QString& searchText);
    void expandFilteredItems(QTreeWidgetItem* item, bool hasMatch);
    bool itemMatchesFilter(QTreeWidgetItem* item, const QString& filter);
    QTreeWidgetItem* findItemForNode(Lupine::Node* node) const;
    Lupine::Node* getNodeFromItem(QTreeWidgetItem* item) const;
    
    // UI Components
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QLineEdit* m_searchEdit;
    QTreeWidget* m_treeWidget;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_selectButton;
    QPushButton* m_cancelButton;
    
    // Data
    Lupine::Scene* m_scene;
    Lupine::Node* m_selectedNode;
};
