#include "LocalizationTablesDialog.h"
#include <QStandardPaths>
#include <QApplication>
#include <QTextStream>
#include <QRegularExpression>
#include <QSortFilterProxyModel>

// SearchLineEdit implementation
SearchLineEdit::SearchLineEdit(QWidget* parent) : QLineEdit(parent) {
    setPlaceholderText("Search keys...");
    connect(this, &QLineEdit::textChanged, this, &SearchLineEdit::onTextChanged);
}

void SearchLineEdit::onTextChanged() {
    emit searchChanged(text());
}

// LocalizationTablesDialog implementation
LocalizationTablesDialog::LocalizationTablesDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_showEmptyKeys(true)
    , m_showMissingKeys(true)
    , m_dataChanged(false) {
    
    setWindowTitle("Localization Tables");
    setModal(true);
    resize(1000, 700);
    
    setupUI();
    loadLocalizationData();
}

LocalizationTablesDialog::~LocalizationTablesDialog() = default;

void LocalizationTablesDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    setupMenuBar();
    setupToolBar();
    setupMainContent();
    setupDialogButtons();
    
    setLayout(m_mainLayout);
}

void LocalizationTablesDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setMaximumHeight(50);
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Table", this, &LocalizationTablesDialog::onNewTable, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open Table...", this, &LocalizationTablesDialog::onOpenTable, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save Table", this, &LocalizationTablesDialog::onSaveTable, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save Table &As...", this, &LocalizationTablesDialog::onSaveTableAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_importCSVAction = fileMenu->addAction("&Import CSV...", this, &LocalizationTablesDialog::onImportCSV);
    m_exportCSVAction = fileMenu->addAction("&Export CSV...", this, &LocalizationTablesDialog::onExportCSV);
    
    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    m_addKeyAction = editMenu->addAction("&Add Key", this, &LocalizationTablesDialog::onAddKey, QKeySequence("Ctrl+K"));
    m_removeKeyAction = editMenu->addAction("&Remove Key", this, &LocalizationTablesDialog::onRemoveKey, QKeySequence::Delete);
    m_duplicateKeyAction = editMenu->addAction("&Duplicate Key", this, &LocalizationTablesDialog::onDuplicateKey, QKeySequence("Ctrl+D"));
    editMenu->addSeparator();
    m_addLocaleAction = editMenu->addAction("Add &Locale", this, &LocalizationTablesDialog::onAddLocale, QKeySequence("Ctrl+L"));
    m_removeLocaleAction = editMenu->addAction("Remove L&ocale", this, &LocalizationTablesDialog::onRemoveLocale);
    
    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_expandAllAction = viewMenu->addAction("&Expand All", this, &LocalizationTablesDialog::onExpandAll);
    m_collapseAllAction = viewMenu->addAction("&Collapse All", this, &LocalizationTablesDialog::onCollapseAll);
    viewMenu->addSeparator();
    m_refreshAction = viewMenu->addAction("&Refresh", this, &LocalizationTablesDialog::onRefresh, QKeySequence::Refresh);
    
    m_mainLayout->addWidget(m_menuBar);
}

void LocalizationTablesDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setMaximumHeight(50);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_addKeyAction);
    m_toolBar->addAction(m_removeKeyAction);
    m_toolBar->addAction(m_duplicateKeyAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_addLocaleAction);
    m_toolBar->addAction(m_removeLocaleAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_refreshAction);
    
    m_mainLayout->addWidget(m_toolBar);
}

void LocalizationTablesDialog::setupMainContent() {
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    setupKeyTree();
    setupLocalizationTable();
    
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setSizes({300, 700});
    
    m_mainLayout->addWidget(m_mainSplitter);
}

void LocalizationTablesDialog::setupKeyTree() {
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    
    // Search and filter controls
    QGroupBox* filterGroup = new QGroupBox("Search & Filter");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterGroup);
    
    m_searchEdit = new SearchLineEdit();
    filterLayout->addWidget(m_searchEdit);
    
    m_showEmptyKeysCheck = new QCheckBox("Show empty keys");
    m_showEmptyKeysCheck->setChecked(m_showEmptyKeys);
    filterLayout->addWidget(m_showEmptyKeysCheck);
    
    m_showMissingKeysCheck = new QCheckBox("Show missing keys");
    m_showMissingKeysCheck->setChecked(m_showMissingKeys);
    filterLayout->addWidget(m_showMissingKeysCheck);
    
    m_leftLayout->addWidget(filterGroup);
    
    // Key tree
    QLabel* treeLabel = new QLabel("Localization Keys");
    m_leftLayout->addWidget(treeLabel);
    
    m_keyTree = new QTreeWidget();
    m_keyTree->setHeaderLabel("Keys");
    m_keyTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_keyTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_leftLayout->addWidget(m_keyTree);
    
    // Connect signals
    connect(m_searchEdit, &SearchLineEdit::searchChanged, this, &LocalizationTablesDialog::onSearchChanged);
    connect(m_showEmptyKeysCheck, &QCheckBox::toggled, this, &LocalizationTablesDialog::onShowEmptyKeysToggled);
    connect(m_showMissingKeysCheck, &QCheckBox::toggled, this, &LocalizationTablesDialog::onShowMissingKeysToggled);
    connect(m_keyTree, &QTreeWidget::itemSelectionChanged, this, &LocalizationTablesDialog::onKeySelectionChanged);
    connect(m_keyTree, &QTreeWidget::customContextMenuRequested, this, &LocalizationTablesDialog::onTableContextMenu);
}

void LocalizationTablesDialog::setupLocalizationTable() {
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    
    m_tableLabel = new QLabel("Localization Values");
    m_rightLayout->addWidget(m_tableLabel);
    
    m_localizationTable = new QTableWidget();
    m_localizationTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_localizationTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_localizationTable->horizontalHeader()->setStretchLastSection(true);
    m_localizationTable->verticalHeader()->setVisible(false);
    m_rightLayout->addWidget(m_localizationTable);
    
    // Connect signals
    connect(m_localizationTable, &QTableWidget::cellChanged, this, &LocalizationTablesDialog::onTableCellChanged);
    connect(m_localizationTable, &QTableWidget::customContextMenuRequested, this, &LocalizationTablesDialog::onTableContextMenu);
}

void LocalizationTablesDialog::setupDialogButtons() {
    m_buttonLayout = new QHBoxLayout();
    
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    m_applyButton = new QPushButton("Apply");
    
    m_okButton->setDefault(true);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_okButton, &QPushButton::clicked, this, &LocalizationTablesDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LocalizationTablesDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &LocalizationTablesDialog::onApplyClicked);
}

void LocalizationTablesDialog::loadLocalizationData() {
    auto& locManager = Lupine::LocalizationManager::Instance();
    
    // Get all keys and locales
    m_allKeys.clear();
    auto keys = locManager.GetAllKeys();
    for (const auto& key : keys) {
        m_allKeys.push_back(key);
    }
    
    m_tableLocales = locManager.GetSupportedLocales();
    
    // Refresh UI
    refreshKeyTree();
    refreshLocalizationTable();
    updateTableColumns();
}

void LocalizationTablesDialog::refreshKeyTree() {
    m_keyTree->clear();
    
    // Group keys by category (everything before first dot)
    std::map<std::string, std::vector<std::string>> categorizedKeys;
    
    for (const auto& key : m_allKeys) {
        std::string category = "General";
        size_t dotPos = key.find('.');
        if (dotPos != std::string::npos) {
            category = key.substr(0, dotPos);
        }
        categorizedKeys[category].push_back(key);
    }
    
    // Add categories and keys to tree
    for (const auto& [category, keys] : categorizedKeys) {
        QTreeWidgetItem* categoryItem = findOrCreateCategory(QString::fromStdString(category));
        
        for (const auto& key : keys) {
            QTreeWidgetItem* keyItem = new QTreeWidgetItem(categoryItem);
            keyItem->setText(0, QString::fromStdString(key));
            keyItem->setData(0, Qt::UserRole, QString::fromStdString(key));
        }
    }
    
    m_keyTree->expandAll();
    applySearchFilter();
}

QTreeWidgetItem* LocalizationTablesDialog::findOrCreateCategory(const QString& category) {
    // Look for existing category
    for (int i = 0; i < m_keyTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_keyTree->topLevelItem(i);
        if (item->text(0) == category) {
            return item;
        }
    }
    
    // Create new category
    QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_keyTree);
    categoryItem->setText(0, category);
    categoryItem->setData(0, Qt::UserRole, "CATEGORY");
    QFont font = categoryItem->font(0);
    font.setBold(true);
    categoryItem->setFont(0, font);
    
    return categoryItem;
}

void LocalizationTablesDialog::refreshLocalizationTable() {
    updateTableColumns();

    // Clear existing data
    m_localizationTable->setRowCount(0);

    if (m_allKeys.empty()) {
        return;
    }

    // Set row count
    m_localizationTable->setRowCount(static_cast<int>(m_allKeys.size()));

    auto& locManager = Lupine::LocalizationManager::Instance();

    // Populate table
    for (size_t row = 0; row < m_allKeys.size(); ++row) {
        const std::string& key = m_allKeys[row];

        // Key column (read-only)
        QTableWidgetItem* keyItem = new QTableWidgetItem(QString::fromStdString(key));
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
        keyItem->setBackground(QColor(240, 240, 240));
        m_localizationTable->setItem(static_cast<int>(row), 0, keyItem);

        // Locale columns
        for (size_t col = 0; col < m_tableLocales.size(); ++col) {
            const Lupine::Locale& locale = m_tableLocales[col];
            auto table = locManager.GetTable(locale);

            QString value;
            if (table && table->HasKey(key)) {
                value = QString::fromStdString(table->GetString(key));
            }

            QTableWidgetItem* valueItem = new QTableWidgetItem(value);
            m_localizationTable->setItem(static_cast<int>(row), static_cast<int>(col + 1), valueItem);
        }
    }

    applySearchFilter();
}

void LocalizationTablesDialog::updateTableColumns() {
    // Set column count (key + locales)
    int columnCount = static_cast<int>(m_tableLocales.size() + 1);
    m_localizationTable->setColumnCount(columnCount);

    // Set headers
    QStringList headers;
    headers << "Key";
    for (const auto& locale : m_tableLocales) {
        headers << QString::fromStdString(locale.display_name.empty() ?
                                         locale.GetIdentifier() : locale.display_name);
    }
    m_localizationTable->setHorizontalHeaderLabels(headers);

    // Set column widths
    m_localizationTable->setColumnWidth(0, 200); // Key column
    for (int i = 1; i < columnCount; ++i) {
        m_localizationTable->setColumnWidth(i, 150); // Locale columns
    }
}

void LocalizationTablesDialog::applySearchFilter() {
    if (m_searchFilter.isEmpty()) {
        // Show all items
        for (int i = 0; i < m_keyTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* categoryItem = m_keyTree->topLevelItem(i);
            categoryItem->setHidden(false);

            for (int j = 0; j < categoryItem->childCount(); ++j) {
                categoryItem->child(j)->setHidden(false);
            }
        }

        for (int i = 0; i < m_localizationTable->rowCount(); ++i) {
            m_localizationTable->setRowHidden(i, false);
        }
    } else {
        // Filter items
        QRegularExpression regex(m_searchFilter, QRegularExpression::CaseInsensitiveOption);

        // Filter tree
        for (int i = 0; i < m_keyTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* categoryItem = m_keyTree->topLevelItem(i);
            bool categoryHasVisibleChildren = false;

            for (int j = 0; j < categoryItem->childCount(); ++j) {
                QTreeWidgetItem* keyItem = categoryItem->child(j);
                bool matches = keyItem->text(0).contains(regex);
                keyItem->setHidden(!matches);
                if (matches) {
                    categoryHasVisibleChildren = true;
                }
            }

            categoryItem->setHidden(!categoryHasVisibleChildren);
        }

        // Filter table
        for (int i = 0; i < m_localizationTable->rowCount(); ++i) {
            QTableWidgetItem* keyItem = m_localizationTable->item(i, 0);
            if (keyItem) {
                bool matches = keyItem->text().contains(regex);
                m_localizationTable->setRowHidden(i, !matches);
            }
        }
    }
}

void LocalizationTablesDialog::onSearchChanged(const QString& text) {
    m_searchFilter = text;
    applySearchFilter();
}

void LocalizationTablesDialog::onShowEmptyKeysToggled(bool show) {
    m_showEmptyKeys = show;
    // TODO: Implement filtering for empty keys
    refreshLocalizationTable();
}

void LocalizationTablesDialog::onShowMissingKeysToggled(bool show) {
    m_showMissingKeys = show;
    // TODO: Implement filtering for missing keys
    refreshLocalizationTable();
}

void LocalizationTablesDialog::onKeySelectionChanged() {
    QString selectedKey = getSelectedKey();
    if (!selectedKey.isEmpty()) {
        // Find and select corresponding row in table
        for (int i = 0; i < m_localizationTable->rowCount(); ++i) {
            QTableWidgetItem* keyItem = m_localizationTable->item(i, 0);
            if (keyItem && keyItem->text() == selectedKey) {
                m_localizationTable->selectRow(i);
                m_localizationTable->scrollToItem(keyItem);
                break;
            }
        }
    }
}

void LocalizationTablesDialog::onTableCellChanged(int row, int column) {
    if (column == 0) {
        return; // Key column is read-only
    }

    QTableWidgetItem* keyItem = m_localizationTable->item(row, 0);
    QTableWidgetItem* valueItem = m_localizationTable->item(row, column);

    if (!keyItem || !valueItem) {
        return;
    }

    std::string key = keyItem->text().toStdString();
    std::string value = valueItem->text().toStdString();

    // Get corresponding locale
    size_t localeIndex = static_cast<size_t>(column - 1);
    if (localeIndex >= m_tableLocales.size()) {
        return;
    }

    const Lupine::Locale& locale = m_tableLocales[localeIndex];

    // Update localization manager
    auto& locManager = Lupine::LocalizationManager::Instance();
    auto table = locManager.GetTable(locale);
    if (!table) {
        table = locManager.CreateTable(locale);
    }

    if (table) {
        table->SetString(key, value);
        m_dataChanged = true;
    }
}

QString LocalizationTablesDialog::getSelectedKey() const {
    auto selectedItems = m_keyTree->selectedItems();
    if (selectedItems.isEmpty()) {
        return QString();
    }

    QTreeWidgetItem* item = selectedItems.first();
    QString userData = item->data(0, Qt::UserRole).toString();

    // Skip category items
    if (userData == "CATEGORY") {
        return QString();
    }

    return item->text(0);
}

void LocalizationTablesDialog::onAddKey() {
    showAddKeyDialog();
}

void LocalizationTablesDialog::onRemoveKey() {
    QString selectedKey = getSelectedKey();
    if (selectedKey.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a key to remove.");
        return;
    }

    int ret = QMessageBox::question(this, "Remove Key",
        QString("Are you sure you want to remove the key '%1'?\nThis will delete all localization data for this key.")
        .arg(selectedKey),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        locManager.RemoveKeyFromAllLocales(selectedKey.toStdString());

        // Remove from local list
        auto it = std::find(m_allKeys.begin(), m_allKeys.end(), selectedKey.toStdString());
        if (it != m_allKeys.end()) {
            m_allKeys.erase(it);
        }

        refreshKeyTree();
        refreshLocalizationTable();
        m_dataChanged = true;
    }
}

void LocalizationTablesDialog::onDuplicateKey() {
    QString selectedKey = getSelectedKey();
    if (selectedKey.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a key to duplicate.");
        return;
    }

    QString newKey = generateUniqueKey(selectedKey + "_copy");

    auto& locManager = Lupine::LocalizationManager::Instance();

    // Copy values from selected key to new key
    for (const auto& locale : m_tableLocales) {
        auto table = locManager.GetTable(locale);
        if (table && table->HasKey(selectedKey.toStdString())) {
            std::string value = table->GetString(selectedKey.toStdString());
            if (!locManager.GetTable(locale)) {
                locManager.CreateTable(locale);
            }
            locManager.GetTable(locale)->SetString(newKey.toStdString(), value);
        }
    }

    m_allKeys.push_back(newKey.toStdString());
    refreshKeyTree();
    refreshLocalizationTable();
    m_dataChanged = true;
}

QString LocalizationTablesDialog::generateUniqueKey(const QString& baseName) {
    QString newKey = baseName;
    int counter = 1;

    while (std::find(m_allKeys.begin(), m_allKeys.end(), newKey.toStdString()) != m_allKeys.end()) {
        newKey = QString("%1_%2").arg(baseName).arg(counter++);
    }

    return newKey;
}

void LocalizationTablesDialog::showAddKeyDialog() {
    QStringList existingKeys;
    for (const auto& key : m_allKeys) {
        existingKeys << QString::fromStdString(key);
    }

    AddKeyDialog dialog(this, existingKeys);
    if (dialog.exec() == QDialog::Accepted) {
        QString newKey = dialog.getKey();
        QString defaultValue = dialog.getDefaultValue();

        auto& locManager = Lupine::LocalizationManager::Instance();
        locManager.AddKeyToAllLocales(newKey.toStdString(), defaultValue.toStdString());

        m_allKeys.push_back(newKey.toStdString());
        refreshKeyTree();
        refreshLocalizationTable();
        m_dataChanged = true;
    }
}

// File operation slots
void LocalizationTablesDialog::onNewTable() {
    if (m_dataChanged) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save them first?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            onSaveTable();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    auto& locManager = Lupine::LocalizationManager::Instance();
    locManager.Clear();
    locManager.Initialize();

    m_currentFilePath.clear();
    loadLocalizationData();
    m_dataChanged = false;
}

void LocalizationTablesDialog::onOpenTable() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open Localization Table",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        if (locManager.LoadFromFile(fileName.toStdString())) {
            m_currentFilePath = fileName;
            loadLocalizationData();
            m_dataChanged = false;
        } else {
            QMessageBox::warning(this, "Open Failed",
                "Failed to open localization table. Please check the file format.");
        }
    }
}

void LocalizationTablesDialog::onSaveTable() {
    if (m_currentFilePath.isEmpty()) {
        onSaveTableAs();
        return;
    }

    auto& locManager = Lupine::LocalizationManager::Instance();
    if (locManager.SaveToFile(m_currentFilePath.toStdString())) {
        m_dataChanged = false;
    } else {
        QMessageBox::warning(this, "Save Failed", "Failed to save localization table.");
    }
}

void LocalizationTablesDialog::onSaveTableAs() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Localization Table",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/localization.json",
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        if (locManager.SaveToFile(fileName.toStdString())) {
            m_currentFilePath = fileName;
            m_dataChanged = false;
        } else {
            QMessageBox::warning(this, "Save Failed", "Failed to save localization table.");
        }
    }
}

void LocalizationTablesDialog::onImportCSV() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import CSV",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "CSV Files (*.csv)");

    if (!fileName.isEmpty()) {
        // TODO: Implement CSV import
        QMessageBox::information(this, "Not Implemented", "CSV import is not yet implemented.");
    }
}

void LocalizationTablesDialog::onExportCSV() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export CSV",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/localization.csv",
        "CSV Files (*.csv)");

    if (!fileName.isEmpty()) {
        // TODO: Implement CSV export
        QMessageBox::information(this, "Not Implemented", "CSV export is not yet implemented.");
    }
}

void LocalizationTablesDialog::onAddLocale() {
    // TODO: Implement add locale dialog
    QMessageBox::information(this, "Not Implemented", "Add locale is not yet implemented. Use Localization Settings dialog.");
}

void LocalizationTablesDialog::onRemoveLocale() {
    // TODO: Implement remove locale
    QMessageBox::information(this, "Not Implemented", "Remove locale is not yet implemented. Use Localization Settings dialog.");
}

void LocalizationTablesDialog::onExpandAll() {
    m_keyTree->expandAll();
}

void LocalizationTablesDialog::onCollapseAll() {
    m_keyTree->collapseAll();
}

void LocalizationTablesDialog::onRefresh() {
    loadLocalizationData();
}

void LocalizationTablesDialog::onTableContextMenu(const QPoint& pos) {
    QMenu contextMenu(this);
    contextMenu.addAction(m_addKeyAction);
    contextMenu.addAction(m_removeKeyAction);
    contextMenu.addAction(m_duplicateKeyAction);
    contextMenu.exec(mapToGlobal(pos));
}

void LocalizationTablesDialog::onOkClicked() {
    if (validateData()) {
        saveLocalizationData();
        accept();
    }
}

void LocalizationTablesDialog::onCancelClicked() {
    reject();
}

void LocalizationTablesDialog::onApplyClicked() {
    if (validateData()) {
        saveLocalizationData();
    }
}

bool LocalizationTablesDialog::validateData() {
    // Basic validation - could be extended
    return true;
}

void LocalizationTablesDialog::saveLocalizationData() {
    // Data is already saved to LocalizationManager through table cell changes
    m_dataChanged = false;
}

void LocalizationTablesDialog::closeEvent(QCloseEvent* event) {
    if (m_dataChanged) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save them?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            saveLocalizationData();
            event->accept();
        } else if (ret == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

// AddKeyDialog implementation
AddKeyDialog::AddKeyDialog(QWidget* parent, const QStringList& existingKeys)
    : QDialog(parent), m_existingKeys(existingKeys) {

    setWindowTitle("Add Localization Key");
    setModal(true);
    resize(400, 200);

    setupUI();
}

void AddKeyDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    // Form layout
    QWidget* formWidget = new QWidget();
    m_formLayout = new QGridLayout(formWidget);

    // Key field
    m_keyLabel = new QLabel("Key:");
    m_keyEdit = new QLineEdit();
    m_keyEdit->setPlaceholderText("e.g., ui.button.start_game");
    m_formLayout->addWidget(m_keyLabel, 0, 0);
    m_formLayout->addWidget(m_keyEdit, 0, 1);

    // Category field
    m_categoryLabel = new QLabel("Category:");
    m_categoryCombo = new QComboBox();
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({"ui", "menu", "game", "dialog", "error", "general"});
    m_formLayout->addWidget(m_categoryLabel, 1, 0);
    m_formLayout->addWidget(m_categoryCombo, 1, 1);

    // Default value field
    m_defaultValueLabel = new QLabel("Default Value:");
    m_defaultValueEdit = new QLineEdit();
    m_defaultValueEdit->setPlaceholderText("Default text for this key");
    m_formLayout->addWidget(m_defaultValueLabel, 2, 0);
    m_formLayout->addWidget(m_defaultValueEdit, 2, 1);

    m_mainLayout->addWidget(formWidget);

    // Dialog buttons
    m_buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");

    m_okButton->setDefault(true);
    m_okButton->setEnabled(false); // Disabled until valid key is entered

    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);

    m_mainLayout->addLayout(m_buttonLayout);

    // Connect signals
    connect(m_keyEdit, &QLineEdit::textChanged, this, &AddKeyDialog::onKeyChanged);
    connect(m_okButton, &QPushButton::clicked, this, &AddKeyDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AddKeyDialog::onCancelClicked);

    // Auto-generate key when category changes
    connect(m_categoryCombo, &QComboBox::currentTextChanged, [this]() {
        if (m_keyEdit->text().isEmpty()) {
            QString category = m_categoryCombo->currentText();
            if (!category.isEmpty()) {
                m_keyEdit->setText(category + ".");
                m_keyEdit->setFocus();
                m_keyEdit->setCursorPosition(m_keyEdit->text().length());
            }
        }
    });
}

void AddKeyDialog::onKeyChanged() {
    QString key = m_keyEdit->text();
    bool isValid = !key.isEmpty() &&
                   !key.contains(QRegularExpression("[^a-zA-Z0-9._]")) &&
                   !m_existingKeys.contains(key);

    m_okButton->setEnabled(isValid);

    // Update key color based on validity
    if (key.isEmpty()) {
        m_keyEdit->setStyleSheet("");
    } else if (m_existingKeys.contains(key)) {
        m_keyEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }"); // Light red
    } else if (!key.contains(QRegularExpression("[^a-zA-Z0-9._]"))) {
        m_keyEdit->setStyleSheet("QLineEdit { background-color: #ccffcc; }"); // Light green
    } else {
        m_keyEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }"); // Light red
    }
}

bool AddKeyDialog::validateInput() {
    QString key = m_keyEdit->text();

    if (key.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Key cannot be empty.");
        m_keyEdit->setFocus();
        return false;
    }

    if (key.contains(QRegularExpression("[^a-zA-Z0-9._]"))) {
        QMessageBox::warning(this, "Validation Error",
            "Key can only contain letters, numbers, dots, and underscores.");
        m_keyEdit->setFocus();
        return false;
    }

    if (m_existingKeys.contains(key)) {
        QMessageBox::warning(this, "Validation Error",
            "A key with this name already exists.");
        m_keyEdit->setFocus();
        return false;
    }

    return true;
}

void AddKeyDialog::onOkClicked() {
    if (validateInput()) {
        accept();
    }
}

void AddKeyDialog::onCancelClicked() {
    reject();
}

QString AddKeyDialog::getKey() const {
    return m_keyEdit->text();
}

QString AddKeyDialog::getCategory() const {
    return m_categoryCombo->currentText();
}

QString AddKeyDialog::getDefaultValue() const {
    return m_defaultValueEdit->text();
}

#include "moc_LocalizationTablesDialog.cpp"
