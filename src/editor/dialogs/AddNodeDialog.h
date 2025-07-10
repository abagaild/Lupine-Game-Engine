#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QTimer>
#include <QProgressBar>
#include <QShowEvent>
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"

/**
 * @brief Node type information for the dialog
 */
struct NodeTypeInfo {
    QString name;
    QString category;
    QString description;
    QString typeName;  // For creating the actual node
    bool isTemplate;   // Whether this is a template node with components
    QStringList components; // Components to add for template nodes
    
    NodeTypeInfo(const QString& n, const QString& cat, const QString& desc, 
                 const QString& type, bool tmpl = false, const QStringList& comps = {})
        : name(n), category(cat), description(desc), typeName(type), isTemplate(tmpl), components(comps) {}
};

/**
 * @brief Dialog for selecting and adding nodes to the scene
 */
class AddNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddNodeDialog(QWidget* parent = nullptr);
    ~AddNodeDialog() = default;

    /**
     * @brief Get the selected node type info
     * @return NodeTypeInfo or nullptr if none selected
     */
    const NodeTypeInfo* getSelectedNodeType() const;

    /**
     * @brief Create a node based on NodeTypeInfo
     * @param nodeType Node type information
     * @param name Name for the new node
     * @return Created node with components if it's a template
     */
    static std::unique_ptr<Lupine::Node> createNode(const NodeTypeInfo& nodeType, const std::string& name);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onNodeSelectionChanged();
    void onNodeDoubleClicked(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onAddButtonClicked();
    void onCancelButtonClicked();
    void onLoadingTimer();

private:
    void setupUI();
    void loadNodesAsync();
    void populateNodeTree();
    void filterNodes(const QString& filter);
    QTreeWidgetItem* findOrCreateCategoryItem(const QString& category);
    void initializeNodeTypes();
    void updateLoadingProgress();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchLayout;
    QHBoxLayout* m_buttonLayout;
    QSplitter* m_splitter;
    
    QLineEdit* m_searchEdit;
    QTreeWidget* m_nodeTree;
    QLabel* m_descriptionLabel;
    QTextEdit* m_descriptionText;
    QProgressBar* m_loadingProgress;
    QLabel* m_loadingLabel;

    QPushButton* m_addButton;
    QPushButton* m_cancelButton;
    
    // Data
    const NodeTypeInfo* m_selectedNodeType;
    std::vector<NodeTypeInfo> m_nodeTypes;
    std::unordered_map<std::string, QTreeWidgetItem*> m_categoryItems;

    // Loading state
    bool m_nodesLoaded = false;
    bool m_isLoading = false;
    QTimer* m_loadingTimer;
    size_t m_loadingIndex = 0;
};
