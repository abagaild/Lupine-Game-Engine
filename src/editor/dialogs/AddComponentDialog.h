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
#include <memory>
#include "lupine/core/Component.h"

// Cached component information for faster access
struct CachedComponentInfo {
    std::string name;
    std::string category;
    std::string description;
    QString displayName;
    QString categoryName;
    QString descriptionText;
    QTreeWidgetItem* treeItem = nullptr;
};

/**
 * @brief Dialog for selecting and adding components to nodes
 */
class AddComponentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddComponentDialog(QWidget* parent = nullptr);
    ~AddComponentDialog() = default;

    /**
     * @brief Get the selected component name
     * @return Component name or empty string if none selected
     */
    QString getSelectedComponentName() const;

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onComponentSelectionChanged();
    void onComponentDoubleClicked(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onAddButtonClicked();
    void onCancelButtonClicked();
    void onLoadingTimer();

private:
    void setupUI();
    void loadComponentsAsync();
    void populateComponentTree();
    void filterComponents(const QString& filter);
    QTreeWidgetItem* findOrCreateCategoryItem(const QString& category);
    void cacheComponentInfo();
    void updateLoadingProgress();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchLayout;
    QHBoxLayout* m_buttonLayout;
    QSplitter* m_splitter;
    
    QLineEdit* m_searchEdit;
    QTreeWidget* m_componentTree;
    QLabel* m_descriptionLabel;
    QTextEdit* m_descriptionText;
    QProgressBar* m_loadingProgress;
    QLabel* m_loadingLabel;

    QPushButton* m_addButton;
    QPushButton* m_cancelButton;
    
    // Data
    QString m_selectedComponentName;
    std::vector<std::string> m_allComponentNames;
    std::unordered_map<std::string, QTreeWidgetItem*> m_categoryItems;
    std::vector<CachedComponentInfo> m_cachedComponents;
    std::unordered_map<std::string, std::shared_ptr<CachedComponentInfo>> m_componentCache;

    // Loading state
    bool m_componentsLoaded = false;
    bool m_isLoading = false;
    QTimer* m_loadingTimer;
    size_t m_loadingIndex = 0;
};
