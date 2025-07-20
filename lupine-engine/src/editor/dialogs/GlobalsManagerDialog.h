#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolButton>
#include <QButtonGroup>
#include "lupine/core/GlobalsManager.h"

namespace Lupine {
    class Project;
}

/**
 * @brief Globals Manager Dialog
 * 
 * Provides a comprehensive interface for managing autoload scripts and global variables.
 * Features tabbed interface with table/text view toggle for global variables.
 */
class GlobalsManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit GlobalsManagerDialog(Lupine::Project* project, QWidget* parent = nullptr);

private slots:
    // Dialog actions
    void onAccepted();
    void onRejected();
    void onApply();
    
    // Autoload scripts
    void onAddAutoload();
    void onRemoveAutoload();
    void onEditAutoload();
    void onAutoloadSelectionChanged();
    void onBrowseAutoloadScript();
    
    // Global variables
    void onAddGlobalVariable();
    void onRemoveGlobalVariable();
    void onEditGlobalVariable();
    void onGlobalVariableSelectionChanged();
    void onGlobalVariableValueChanged();
    void onResetGlobalVariable();
    void onResetAllGlobalVariables();
    
    // View mode toggle
    void onToggleVariableView();
    void onVariableTextChanged();
    void onParseVariableText();

private:
    void setupUI();
    void setupAutoloadsTab();
    void setupGlobalVariablesTab();
    void setupTableView();
    void setupTextView();
    void loadData();
    void saveData();
    
    // Autoload helpers
    void refreshAutoloadsTable();
    void populateAutoloadForm(const Lupine::AutoloadScript& autoload);
    void clearAutoloadForm();
    bool validateAutoloadForm();
    Lupine::AutoloadScript getAutoloadFromForm();
    
    // Global variables helpers
    void refreshGlobalVariablesTable();
    void refreshGlobalVariablesText();
    void populateGlobalVariableForm(const Lupine::GlobalVariable& variable);
    void clearGlobalVariableForm();
    bool validateGlobalVariableForm();
    Lupine::GlobalVariable getGlobalVariableFromForm();
    bool parseGlobalVariablesFromText();
    QString formatGlobalVariableValue(const Lupine::GlobalVariableValue& value, const std::string& type);
    Lupine::GlobalVariableValue parseGlobalVariableValue(const QString& value_str, const std::string& type);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_applyButton;
    
    // === Autoloads Tab ===
    QWidget* m_autoloadsTab;
    QHBoxLayout* m_autoloadsLayout;
    
    // Autoloads table
    QVBoxLayout* m_autoloadsTableLayout;
    QTableWidget* m_autoloadsTable;
    QHBoxLayout* m_autoloadsButtonsLayout;
    QPushButton* m_addAutoloadButton;
    QPushButton* m_removeAutoloadButton;
    QPushButton* m_editAutoloadButton;
    
    // Autoload form
    QGroupBox* m_autoloadFormGroup;
    QFormLayout* m_autoloadFormLayout;
    QLineEdit* m_autoloadNameEdit;
    QLineEdit* m_autoloadScriptPathEdit;
    QPushButton* m_browseAutoloadScriptButton;
    QComboBox* m_autoloadScriptTypeCombo;
    QCheckBox* m_autoloadEnabledCheck;
    QLineEdit* m_autoloadDescriptionEdit;
    
    // === Global Variables Tab ===
    QWidget* m_globalVariablesTab;
    QVBoxLayout* m_globalVariablesLayout;
    
    // View mode toggle
    QHBoxLayout* m_variableViewToggleLayout;
    QButtonGroup* m_variableViewGroup;
    QToolButton* m_tableViewButton;
    QToolButton* m_textViewButton;
    QPushButton* m_parseTextButton;
    
    // Table view
    QWidget* m_tableViewWidget;
    QHBoxLayout* m_tableViewLayout;
    
    // Variables table
    QVBoxLayout* m_variablesTableLayout;
    QTableWidget* m_globalVariablesTable;
    QHBoxLayout* m_variablesButtonsLayout;
    QPushButton* m_addGlobalVariableButton;
    QPushButton* m_removeGlobalVariableButton;
    QPushButton* m_editGlobalVariableButton;
    QPushButton* m_resetGlobalVariableButton;
    QPushButton* m_resetAllGlobalVariablesButton;
    
    // Variable form
    QGroupBox* m_globalVariableFormGroup;
    QFormLayout* m_globalVariableFormLayout;
    QLineEdit* m_globalVariableNameEdit;
    QComboBox* m_globalVariableTypeCombo;
    QLineEdit* m_globalVariableValueEdit;
    QLineEdit* m_globalVariableDefaultEdit;
    QCheckBox* m_globalVariableExportedCheck;
    QLineEdit* m_globalVariableDescriptionEdit;
    
    // Text view
    QWidget* m_textViewWidget;
    QVBoxLayout* m_textViewLayout;
    QLabel* m_textViewLabel;
    QTextEdit* m_globalVariablesTextEdit;
    
    // Data
    Lupine::Project* m_project;
    Lupine::GlobalsManager* m_globalsManager;
    
    // State
    bool m_isTableView;
    int m_selectedAutoloadRow;
    int m_selectedGlobalVariableRow;
};

/**
 * @brief Helper dialog for editing individual autoload scripts
 */
class AutoloadEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit AutoloadEditDialog(const Lupine::AutoloadScript& autoload, QWidget* parent = nullptr);
    Lupine::AutoloadScript getAutoload() const;

private slots:
    void onBrowseScript();

private:
    void setupUI();
    
    QVBoxLayout* m_layout;
    QFormLayout* m_formLayout;
    QLineEdit* m_nameEdit;
    QLineEdit* m_scriptPathEdit;
    QPushButton* m_browseButton;
    QComboBox* m_scriptTypeCombo;
    QCheckBox* m_enabledCheck;
    QLineEdit* m_descriptionEdit;
    QDialogButtonBox* m_buttonBox;
};

/**
 * @brief Helper dialog for editing individual global variables
 */
class GlobalVariableEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit GlobalVariableEditDialog(const Lupine::GlobalVariable& variable, QWidget* parent = nullptr);
    Lupine::GlobalVariable getGlobalVariable() const;

private slots:
    void onTypeChanged();

private:
    void setupUI();
    void updateValueEditors();
    
    QVBoxLayout* m_layout;
    QFormLayout* m_formLayout;
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QLineEdit* m_valueEdit;
    QLineEdit* m_defaultEdit;
    QCheckBox* m_exportedCheck;
    QLineEdit* m_descriptionEdit;
    QDialogButtonBox* m_buttonBox;
};
