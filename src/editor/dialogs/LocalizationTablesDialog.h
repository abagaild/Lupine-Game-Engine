#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QToolBar>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QCloseEvent>
#include "lupine/localization/LocalizationManager.h"

// Forward declarations
namespace Lupine {
    class LocalizationManager;
}

/**
 * @brief Custom search line edit widget
 */
class SearchLineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit SearchLineEdit(QWidget* parent = nullptr);

signals:
    void searchChanged(const QString& text);

private slots:
    void onTextChanged();

private:
};

/**
 * @brief Dialog for editing localization tables
 * 
 * This dialog provides a Unity-like interface for editing localization data:
 * - Left panel: Tree view of localization keys organized by categories
 * - Right panel: Table view showing key-value pairs for all locales
 * - Toolbar with actions for adding/removing keys and locales
 * - Search functionality to filter keys
 */
class LocalizationTablesDialog : public QDialog {
    Q_OBJECT

public:
    explicit LocalizationTablesDialog(QWidget* parent = nullptr);
    ~LocalizationTablesDialog();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File operations
    void onNewTable();
    void onOpenTable();
    void onSaveTable();
    void onSaveTableAs();
    void onImportCSV();
    void onExportCSV();
    
    // Edit operations
    void onAddKey();
    void onRemoveKey();
    void onDuplicateKey();
    void onAddLocale();
    void onRemoveLocale();
    
    // View operations
    void onExpandAll();
    void onCollapseAll();
    void onRefresh();
    
    // Search and filter
    void onSearchChanged(const QString& text);
    void onShowEmptyKeysToggled(bool show);
    void onShowMissingKeysToggled(bool show);
    
    // Selection and editing
    void onKeySelectionChanged();
    void onTableCellChanged(int row, int column);
    void onTableContextMenu(const QPoint& pos);
    
    // Dialog buttons
    void onOkClicked();
    void onCancelClicked();
    void onApplyClicked();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainContent();
    void setupKeyTree();
    void setupLocalizationTable();
    void setupDialogButtons();
    
    void loadLocalizationData();
    void saveLocalizationData();
    void refreshKeyTree();
    void refreshLocalizationTable();
    void updateTableColumns();
    void applySearchFilter();
    
    bool validateData();
    void showAddKeyDialog();
    void showAddLocaleDialog();
    QString generateUniqueKey(const QString& baseName = "new_key");
    
    // Helper methods
    QTreeWidgetItem* findOrCreateCategory(const QString& category);
    void addKeyToTree(const QString& key);
    void removeKeyFromTree(const QString& key);
    QString getSelectedKey() const;
    std::vector<Lupine::Locale> getTableLocales() const;
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // Left panel - Key tree
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;
    SearchLineEdit* m_searchEdit;
    QCheckBox* m_showEmptyKeysCheck;
    QCheckBox* m_showMissingKeysCheck;
    QTreeWidget* m_keyTree;
    
    // Right panel - Localization table
    QWidget* m_rightPanel;
    QVBoxLayout* m_rightLayout;
    QLabel* m_tableLabel;
    QTableWidget* m_localizationTable;
    
    // Dialog buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_importCSVAction;
    QAction* m_exportCSVAction;
    QAction* m_addKeyAction;
    QAction* m_removeKeyAction;
    QAction* m_duplicateKeyAction;
    QAction* m_addLocaleAction;
    QAction* m_removeLocaleAction;
    QAction* m_expandAllAction;
    QAction* m_collapseAllAction;
    QAction* m_refreshAction;
    
    // Data
    std::vector<std::string> m_allKeys;
    std::vector<Lupine::Locale> m_tableLocales;
    QString m_currentFilePath;
    QString m_searchFilter;
    bool m_showEmptyKeys;
    bool m_showMissingKeys;
    bool m_dataChanged;
};

/**
 * @brief Dialog for adding a new localization key
 */
class AddKeyDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddKeyDialog(QWidget* parent = nullptr, const QStringList& existingKeys = QStringList());
    
    QString getKey() const;
    QString getCategory() const;
    QString getDefaultValue() const;

private slots:
    void onKeyChanged();
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUI();
    bool validateInput();
    
    QVBoxLayout* m_mainLayout;
    QGridLayout* m_formLayout;
    
    QLabel* m_keyLabel;
    QLineEdit* m_keyEdit;
    QLabel* m_categoryLabel;
    QComboBox* m_categoryCombo;
    QLabel* m_defaultValueLabel;
    QLineEdit* m_defaultValueEdit;
    
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    QStringList m_existingKeys;
};
