#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QMenuBar>
#include <QToolBar>
#include <memory>
#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include "lupine/scriptableobjects/ScriptableObjectInstance.h"

namespace Lupine {
class ScriptableObjectTemplate;
class ScriptableObjectInstance;
}

/**
 * @brief Dialog for managing scriptable object templates and instances
 *
 * Provides a two-tiered table editor interface for defining templates
 * and creating instances with field editing capabilities.
 */
class ScriptableObjectsDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    explicit ScriptableObjectsDialog(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    virtual ~ScriptableObjectsDialog();

private slots:
    // File operations
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();

    // Template operations
    void onNewTemplate();
    void onDeleteTemplate();
    void onTemplateSelectionChanged();
    void onTemplateNameChanged();
    void onTemplateDescriptionChanged();
    void onTemplateScriptLanguageChanged();
    void onTemplateCustomMethodsChanged();

    // Field operations
    void onAddField();
    void onDeleteField();
    void onFieldSelectionChanged();
    void onFieldDataChanged(int row, int column);

    // Instance operations
    void onNewInstance();
    void onDeleteInstance();
    void onInstanceSelectionChanged();
    void onInstanceNameChanged();
    void onInstanceDataChanged(int row, int column);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();

    // UI update methods
    void updateTemplateList();
    void updateTemplateDetails();
    void updateFieldList();
    void updateInstanceList();
    void updateInstanceDetails();
    void clearTemplateSelection();
    void clearInstanceSelection();

    // Helper methods
    std::shared_ptr<Lupine::ScriptableObjectTemplate> getSelectedTemplate() const;
    std::shared_ptr<Lupine::ScriptableObjectInstance> getSelectedInstance() const;
    void updateWindowTitle();
    void setModified(bool modified);
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void loadFromFile(const QString& filePath);
    void saveToFile(const QString& filePath);

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;

    // Left panel - Templates
    QWidget* m_templatesPanel;
    QVBoxLayout* m_templatesLayout;
    QTreeWidget* m_templatesTree;
    QPushButton* m_newTemplateButton;
    QPushButton* m_deleteTemplateButton;

    // Right panel - Details tabs
    QTabWidget* m_detailsTabs;

    // Template details tab
    QWidget* m_templateDetailsTab;
    QVBoxLayout* m_templateDetailsLayout;
    QLineEdit* m_templateNameEdit;
    QTextEdit* m_templateDescriptionEdit;
    QComboBox* m_templateScriptLanguageCombo;
    QTableWidget* m_fieldsTable;
    QPushButton* m_addFieldButton;
    QPushButton* m_deleteFieldButton;
    QTextEdit* m_customMethodsEdit;

    // Instance details tab
    QWidget* m_instanceDetailsTab;
    QVBoxLayout* m_instanceDetailsLayout;
    QTreeWidget* m_instancesTree;
    QPushButton* m_newInstanceButton;
    QPushButton* m_deleteInstanceButton;
    QLineEdit* m_instanceNameEdit;
    QTableWidget* m_instanceFieldsTable;

    // Data
    QString m_currentFilePath;
    bool m_modified;
    std::shared_ptr<Lupine::ScriptableObjectTemplate> m_selectedTemplate;
    std::shared_ptr<Lupine::ScriptableObjectInstance> m_selectedInstance;
};
