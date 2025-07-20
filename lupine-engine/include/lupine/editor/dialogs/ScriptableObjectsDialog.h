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
    /**
     * @brief Setup the user interface
     */
    void setupUI();
    
    /**
     * @brief Setup menu bar
     */
    void setupMenuBar();
    
    /**
     * @brief Setup toolbar
     */
    void setupToolBar();
    
    /**
     * @brief Setup connections
     */
    void setupConnections();
    
    /**
     * @brief Update template list
     */
    void updateTemplateList();
    
    /**
     * @brief Update template details
     */
    void updateTemplateDetails();
    
    /**
     * @brief Update field list for current template
     */
    void updateFieldList();
    
    /**
     * @brief Update instance list for current template
     */
    void updateInstanceList();
    
    /**
     * @brief Update instance details
     */
    void updateInstanceDetails();
    
    /**
     * @brief Clear template selection
     */
    void clearTemplateSelection();
    
    /**
     * @brief Clear instance selection
     */
    void clearInstanceSelection();
    
    /**
     * @brief Get selected template
     */
    std::shared_ptr<Lupine::ScriptableObjectTemplate> getSelectedTemplate() const;
    
    /**
     * @brief Get selected instance
     */
    std::shared_ptr<Lupine::ScriptableObjectInstance> getSelectedInstance() const;
    
    /**
     * @brief Update window title
     */
    void updateWindowTitle();
    
    /**
     * @brief Set modified state
     */
    void setModified(bool modified);
    
    /**
     * @brief Check if there are unsaved changes
     */
    bool hasUnsavedChanges() const;
    
    /**
     * @brief Prompt to save changes
     */
    bool promptSaveChanges();
    
    /**
     * @brief Load data from file
     */
    void loadFromFile(const QString& filePath);
    
    /**
     * @brief Save data to file
     */
    void saveToFile(const QString& filePath);

private:
    // UI Layout
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
    
    // Middle panel - Template/Instance details
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
    
    // State
    QString m_currentFilePath;
    bool m_modified;
    std::shared_ptr<Lupine::ScriptableObjectTemplate> m_selectedTemplate;
    std::shared_ptr<Lupine::ScriptableObjectInstance> m_selectedInstance;
};
