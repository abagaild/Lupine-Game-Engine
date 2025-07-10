#include "GlobalsManagerDialog.h"
#include "lupine/core/Project.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <iostream>

GlobalsManagerDialog::GlobalsManagerDialog(Lupine::Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
    , m_globalsManager(&Lupine::GlobalsManager::Instance())
    , m_isTableView(true)
    , m_selectedAutoloadRow(-1)
    , m_selectedGlobalVariableRow(-1)
{
    setWindowTitle("Globals Manager");
    setModal(true);
    resize(1000, 700);
    
    setupUI();
    loadData();
}

void GlobalsManagerDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    m_mainLayout->addWidget(m_tabWidget);
    
    // Setup tabs
    setupAutoloadsTab();
    setupGlobalVariablesTab();
    
    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    m_applyButton = m_buttonBox->button(QDialogButtonBox::Apply);
    m_mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &GlobalsManagerDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &GlobalsManagerDialog::onRejected);
    connect(m_applyButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onApply);
}

void GlobalsManagerDialog::setupAutoloadsTab() {
    m_autoloadsTab = new QWidget();
    m_tabWidget->addTab(m_autoloadsTab, "Autoload Scripts");
    
    m_autoloadsLayout = new QHBoxLayout(m_autoloadsTab);
    
    // Left side - table
    auto leftWidget = new QWidget();
    m_autoloadsTableLayout = new QVBoxLayout(leftWidget);
    
    // Autoloads table
    m_autoloadsTable = new QTableWidget();
    m_autoloadsTable->setColumnCount(4);
    QStringList headers = {"Name", "Script Path", "Type", "Enabled"};
    m_autoloadsTable->setHorizontalHeaderLabels(headers);
    m_autoloadsTable->horizontalHeader()->setStretchLastSection(true);
    m_autoloadsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_autoloadsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_autoloadsTableLayout->addWidget(m_autoloadsTable);
    
    // Autoloads buttons
    m_autoloadsButtonsLayout = new QHBoxLayout();
    m_addAutoloadButton = new QPushButton("Add");
    m_removeAutoloadButton = new QPushButton("Remove");
    m_editAutoloadButton = new QPushButton("Edit");
    
    m_autoloadsButtonsLayout->addWidget(m_addAutoloadButton);
    m_autoloadsButtonsLayout->addWidget(m_removeAutoloadButton);
    m_autoloadsButtonsLayout->addWidget(m_editAutoloadButton);
    m_autoloadsButtonsLayout->addStretch();
    
    m_autoloadsTableLayout->addLayout(m_autoloadsButtonsLayout);
    m_autoloadsLayout->addWidget(leftWidget, 2);
    
    // Right side - form
    m_autoloadFormGroup = new QGroupBox("Autoload Details");
    m_autoloadFormLayout = new QFormLayout(m_autoloadFormGroup);
    
    m_autoloadNameEdit = new QLineEdit();
    m_autoloadFormLayout->addRow("Name:", m_autoloadNameEdit);
    
    auto scriptPathLayout = new QHBoxLayout();
    m_autoloadScriptPathEdit = new QLineEdit();
    m_browseAutoloadScriptButton = new QPushButton("Browse...");
    scriptPathLayout->addWidget(m_autoloadScriptPathEdit);
    scriptPathLayout->addWidget(m_browseAutoloadScriptButton);
    m_autoloadFormLayout->addRow("Script Path:", scriptPathLayout);
    
    m_autoloadScriptTypeCombo = new QComboBox();
    m_autoloadScriptTypeCombo->addItems({"python", "lua"});
    m_autoloadFormLayout->addRow("Script Type:", m_autoloadScriptTypeCombo);
    
    m_autoloadEnabledCheck = new QCheckBox();
    m_autoloadEnabledCheck->setChecked(true);
    m_autoloadFormLayout->addRow("Enabled:", m_autoloadEnabledCheck);
    
    m_autoloadDescriptionEdit = new QLineEdit();
    m_autoloadFormLayout->addRow("Description:", m_autoloadDescriptionEdit);
    
    m_autoloadsLayout->addWidget(m_autoloadFormGroup, 1);
    
    // Connect autoload signals
    connect(m_addAutoloadButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onAddAutoload);
    connect(m_removeAutoloadButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onRemoveAutoload);
    connect(m_editAutoloadButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onEditAutoload);
    connect(m_browseAutoloadScriptButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onBrowseAutoloadScript);
    connect(m_autoloadsTable, &QTableWidget::itemSelectionChanged, this, &GlobalsManagerDialog::onAutoloadSelectionChanged);
    
    // Initially disable form
    m_autoloadFormGroup->setEnabled(false);
}

void GlobalsManagerDialog::setupGlobalVariablesTab() {
    m_globalVariablesTab = new QWidget();
    m_tabWidget->addTab(m_globalVariablesTab, "Global Variables");
    
    m_globalVariablesLayout = new QVBoxLayout(m_globalVariablesTab);
    
    // View mode toggle
    m_variableViewToggleLayout = new QHBoxLayout();
    m_variableViewGroup = new QButtonGroup();
    
    m_tableViewButton = new QToolButton();
    m_tableViewButton->setText("Table View");
    m_tableViewButton->setCheckable(true);
    m_tableViewButton->setChecked(true);
    m_variableViewGroup->addButton(m_tableViewButton, 0);
    
    m_textViewButton = new QToolButton();
    m_textViewButton->setText("Text View");
    m_textViewButton->setCheckable(true);
    m_variableViewGroup->addButton(m_textViewButton, 1);
    
    m_parseTextButton = new QPushButton("Parse Text");
    m_parseTextButton->setVisible(false);
    
    m_variableViewToggleLayout->addWidget(m_tableViewButton);
    m_variableViewToggleLayout->addWidget(m_textViewButton);
    m_variableViewToggleLayout->addWidget(m_parseTextButton);
    m_variableViewToggleLayout->addStretch();
    
    m_globalVariablesLayout->addLayout(m_variableViewToggleLayout);
    
    // Create stacked widget for table/text views
    setupTableView();
    setupTextView();
    
    // Connect view toggle signals
    connect(m_variableViewGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &GlobalsManagerDialog::onToggleVariableView);
    connect(m_parseTextButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onParseVariableText);
}

void GlobalsManagerDialog::setupTableView() {
    m_tableViewWidget = new QWidget();
    m_tableViewLayout = new QHBoxLayout(m_tableViewWidget);
    
    // Left side - table
    auto leftWidget = new QWidget();
    m_variablesTableLayout = new QVBoxLayout(leftWidget);
    
    // Global variables table
    m_globalVariablesTable = new QTableWidget();
    m_globalVariablesTable->setColumnCount(5);
    QStringList headers = {"Name", "Type", "Value", "Default", "Exported"};
    m_globalVariablesTable->setHorizontalHeaderLabels(headers);
    m_globalVariablesTable->horizontalHeader()->setStretchLastSection(true);
    m_globalVariablesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_globalVariablesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_variablesTableLayout->addWidget(m_globalVariablesTable);
    
    // Variables buttons
    m_variablesButtonsLayout = new QHBoxLayout();
    m_addGlobalVariableButton = new QPushButton("Add");
    m_removeGlobalVariableButton = new QPushButton("Remove");
    m_editGlobalVariableButton = new QPushButton("Edit");
    m_resetGlobalVariableButton = new QPushButton("Reset");
    m_resetAllGlobalVariablesButton = new QPushButton("Reset All");
    
    m_variablesButtonsLayout->addWidget(m_addGlobalVariableButton);
    m_variablesButtonsLayout->addWidget(m_removeGlobalVariableButton);
    m_variablesButtonsLayout->addWidget(m_editGlobalVariableButton);
    m_variablesButtonsLayout->addWidget(m_resetGlobalVariableButton);
    m_variablesButtonsLayout->addWidget(m_resetAllGlobalVariablesButton);
    m_variablesButtonsLayout->addStretch();
    
    m_variablesTableLayout->addLayout(m_variablesButtonsLayout);
    m_tableViewLayout->addWidget(leftWidget, 2);
    
    // Right side - form
    m_globalVariableFormGroup = new QGroupBox("Variable Details");
    m_globalVariableFormLayout = new QFormLayout(m_globalVariableFormGroup);
    
    m_globalVariableNameEdit = new QLineEdit();
    m_globalVariableFormLayout->addRow("Name:", m_globalVariableNameEdit);
    
    m_globalVariableTypeCombo = new QComboBox();
    m_globalVariableTypeCombo->addItems({"bool", "int", "float", "string", "vec2", "vec3", "vec4"});
    m_globalVariableFormLayout->addRow("Type:", m_globalVariableTypeCombo);
    
    m_globalVariableValueEdit = new QLineEdit();
    m_globalVariableFormLayout->addRow("Value:", m_globalVariableValueEdit);
    
    m_globalVariableDefaultEdit = new QLineEdit();
    m_globalVariableFormLayout->addRow("Default:", m_globalVariableDefaultEdit);
    
    m_globalVariableExportedCheck = new QCheckBox();
    m_globalVariableExportedCheck->setChecked(true);
    m_globalVariableFormLayout->addRow("Exported:", m_globalVariableExportedCheck);
    
    m_globalVariableDescriptionEdit = new QLineEdit();
    m_globalVariableFormLayout->addRow("Description:", m_globalVariableDescriptionEdit);
    
    m_tableViewLayout->addWidget(m_globalVariableFormGroup, 1);
    
    m_globalVariablesLayout->addWidget(m_tableViewWidget);
    
    // Connect global variable signals
    connect(m_addGlobalVariableButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onAddGlobalVariable);
    connect(m_removeGlobalVariableButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onRemoveGlobalVariable);
    connect(m_editGlobalVariableButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onEditGlobalVariable);
    connect(m_resetGlobalVariableButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onResetGlobalVariable);
    connect(m_resetAllGlobalVariablesButton, &QPushButton::clicked, this, &GlobalsManagerDialog::onResetAllGlobalVariables);
    connect(m_globalVariablesTable, &QTableWidget::itemSelectionChanged, this, &GlobalsManagerDialog::onGlobalVariableSelectionChanged);
    
    // Initially disable form
    m_globalVariableFormGroup->setEnabled(false);
}

void GlobalsManagerDialog::setupTextView() {
    m_textViewWidget = new QWidget();
    m_textViewLayout = new QVBoxLayout(m_textViewWidget);
    
    m_textViewLabel = new QLabel("Global Variables (Text Format):");
    m_textViewLayout->addWidget(m_textViewLabel);
    
    m_globalVariablesTextEdit = new QTextEdit();
    m_globalVariablesTextEdit->setFont(QFont("Consolas", 10));
    m_textViewLayout->addWidget(m_globalVariablesTextEdit);
    
    m_globalVariablesLayout->addWidget(m_textViewWidget);
    m_textViewWidget->setVisible(false);
    
    connect(m_globalVariablesTextEdit, &QTextEdit::textChanged, this, &GlobalsManagerDialog::onVariableTextChanged);
}

void GlobalsManagerDialog::loadData() {
    // Load globals data from project settings
    if (m_project) {
        auto globals_setting = m_project->GetSetting("globals");
        if (globals_setting) {
            // Try to get as string (JSON)
            if (std::holds_alternative<std::string>(*globals_setting)) {
                std::string json_str = std::get<std::string>(*globals_setting);
                try {
                    auto json = nlohmann::json::parse(json_str);
                    m_globalsManager->DeserializeFromJson(json);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing globals JSON: " << e.what() << std::endl;
                }
            }
        }
    }
    
    refreshAutoloadsTable();
    refreshGlobalVariablesTable();
    refreshGlobalVariablesText();
}

void GlobalsManagerDialog::saveData() {
    // Save globals data to project settings
    if (m_project) {
        auto json = m_globalsManager->SerializeToJson();
        std::string json_str = json.dump();
        m_project->SetSetting("globals", json_str);
    }
}

// === Dialog Actions ===

void GlobalsManagerDialog::onAccepted() {
    saveData();
    accept();
}

void GlobalsManagerDialog::onRejected() {
    reject();
}

void GlobalsManagerDialog::onApply() {
    saveData();
}

// === Autoload Scripts ===

void GlobalsManagerDialog::onAddAutoload() {
    AutoloadEditDialog dialog(Lupine::AutoloadScript(), this);
    if (dialog.exec() == QDialog::Accepted) {
        auto autoload = dialog.getAutoload();
        if (m_globalsManager->RegisterAutoload(autoload)) {
            refreshAutoloadsTable();
        } else {
            QMessageBox::warning(this, "Error", "Failed to add autoload script. Check that the name is unique.");
        }
    }
}

void GlobalsManagerDialog::onRemoveAutoload() {
    int row = m_autoloadsTable->currentRow();
    if (row >= 0) {
        auto item = m_autoloadsTable->item(row, 0);
        if (item) {
            QString name = item->text();
            if (QMessageBox::question(this, "Confirm",
                QString("Remove autoload script '%1'?").arg(name)) == QMessageBox::Yes) {
                if (m_globalsManager->UnregisterAutoload(name.toStdString())) {
                    refreshAutoloadsTable();
                    clearAutoloadForm();
                }
            }
        }
    }
}

void GlobalsManagerDialog::onEditAutoload() {
    int row = m_autoloadsTable->currentRow();
    if (row >= 0) {
        auto item = m_autoloadsTable->item(row, 0);
        if (item) {
            QString name = item->text();
            auto autoload = m_globalsManager->GetAutoload(name.toStdString());
            if (autoload) {
                AutoloadEditDialog dialog(*autoload, this);
                if (dialog.exec() == QDialog::Accepted) {
                    auto updated_autoload = dialog.getAutoload();
                    // Remove old and add new (in case name changed)
                    m_globalsManager->UnregisterAutoload(name.toStdString());
                    if (m_globalsManager->RegisterAutoload(updated_autoload)) {
                        refreshAutoloadsTable();
                    } else {
                        // Re-add the original if new one failed
                        m_globalsManager->RegisterAutoload(*autoload);
                        QMessageBox::warning(this, "Error", "Failed to update autoload script.");
                    }
                }
            }
        }
    }
}

void GlobalsManagerDialog::onAutoloadSelectionChanged() {
    int row = m_autoloadsTable->currentRow();
    m_selectedAutoloadRow = row;

    if (row >= 0) {
        auto item = m_autoloadsTable->item(row, 0);
        if (item) {
            QString name = item->text();
            auto autoload = m_globalsManager->GetAutoload(name.toStdString());
            if (autoload) {
                populateAutoloadForm(*autoload);
                m_autoloadFormGroup->setEnabled(true);
                return;
            }
        }
    }

    clearAutoloadForm();
    m_autoloadFormGroup->setEnabled(false);
}

void GlobalsManagerDialog::onBrowseAutoloadScript() {
    QString projectDir = "";
    if (m_project) {
        projectDir = QString::fromStdString(m_project->GetProjectDirectory());
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Script File", projectDir,
        "Script Files (*.py *.lua);;Python Files (*.py);;Lua Files (*.lua);;All Files (*)");

    if (!fileName.isEmpty()) {
        // Make path relative to project directory if possible
        if (!projectDir.isEmpty()) {
            QDir dir(projectDir);
            QString relativePath = dir.relativeFilePath(fileName);
            if (!relativePath.startsWith("..")) {
                fileName = relativePath;
            }
        }
        m_autoloadScriptPathEdit->setText(fileName);

        // Auto-detect script type from extension
        if (fileName.endsWith(".py", Qt::CaseInsensitive)) {
            m_autoloadScriptTypeCombo->setCurrentText("python");
        } else if (fileName.endsWith(".lua", Qt::CaseInsensitive)) {
            m_autoloadScriptTypeCombo->setCurrentText("lua");
        }
    }
}

// === Global Variables ===

void GlobalsManagerDialog::onAddGlobalVariable() {
    GlobalVariableEditDialog dialog(Lupine::GlobalVariable(), this);
    if (dialog.exec() == QDialog::Accepted) {
        auto variable = dialog.getGlobalVariable();
        if (m_globalsManager->RegisterGlobalVariable(variable)) {
            refreshGlobalVariablesTable();
            refreshGlobalVariablesText();
        } else {
            QMessageBox::warning(this, "Error", "Failed to add global variable. Check that the name is unique.");
        }
    }
}

void GlobalsManagerDialog::onRemoveGlobalVariable() {
    int row = m_globalVariablesTable->currentRow();
    if (row >= 0) {
        auto item = m_globalVariablesTable->item(row, 0);
        if (item) {
            QString name = item->text();
            if (QMessageBox::question(this, "Confirm",
                QString("Remove global variable '%1'?").arg(name)) == QMessageBox::Yes) {
                if (m_globalsManager->UnregisterGlobalVariable(name.toStdString())) {
                    refreshGlobalVariablesTable();
                    refreshGlobalVariablesText();
                    clearGlobalVariableForm();
                }
            }
        }
    }
}

void GlobalsManagerDialog::onEditGlobalVariable() {
    int row = m_globalVariablesTable->currentRow();
    if (row >= 0) {
        auto item = m_globalVariablesTable->item(row, 0);
        if (item) {
            QString name = item->text();
            auto variable = m_globalsManager->GetGlobalVariableDefinition(name.toStdString());
            if (variable) {
                GlobalVariableEditDialog dialog(*variable, this);
                if (dialog.exec() == QDialog::Accepted) {
                    auto updated_variable = dialog.getGlobalVariable();
                    // Remove old and add new (in case name changed)
                    m_globalsManager->UnregisterGlobalVariable(name.toStdString());
                    if (m_globalsManager->RegisterGlobalVariable(updated_variable)) {
                        refreshGlobalVariablesTable();
                        refreshGlobalVariablesText();
                    } else {
                        // Re-add the original if new one failed
                        m_globalsManager->RegisterGlobalVariable(*variable);
                        QMessageBox::warning(this, "Error", "Failed to update global variable.");
                    }
                }
            }
        }
    }
}

void GlobalsManagerDialog::onGlobalVariableSelectionChanged() {
    int row = m_globalVariablesTable->currentRow();
    m_selectedGlobalVariableRow = row;

    if (row >= 0) {
        auto item = m_globalVariablesTable->item(row, 0);
        if (item) {
            QString name = item->text();
            auto variable = m_globalsManager->GetGlobalVariableDefinition(name.toStdString());
            if (variable) {
                populateGlobalVariableForm(*variable);
                m_globalVariableFormGroup->setEnabled(true);
                return;
            }
        }
    }

    clearGlobalVariableForm();
    m_globalVariableFormGroup->setEnabled(false);
}

void GlobalsManagerDialog::onGlobalVariableValueChanged() {
    // Handle real-time value changes if needed
}

void GlobalsManagerDialog::onResetGlobalVariable() {
    int row = m_globalVariablesTable->currentRow();
    if (row >= 0) {
        auto item = m_globalVariablesTable->item(row, 0);
        if (item) {
            QString name = item->text();
            if (m_globalsManager->ResetGlobalVariable(name.toStdString())) {
                refreshGlobalVariablesTable();
                refreshGlobalVariablesText();
                // Update form if this variable is selected
                if (row == m_selectedGlobalVariableRow) {
                    auto variable = m_globalsManager->GetGlobalVariableDefinition(name.toStdString());
                    if (variable) {
                        populateGlobalVariableForm(*variable);
                    }
                }
            }
        }
    }
}

void GlobalsManagerDialog::onResetAllGlobalVariables() {
    if (QMessageBox::question(this, "Confirm",
        "Reset all global variables to their default values?") == QMessageBox::Yes) {
        m_globalsManager->ResetAllGlobalVariables();
        refreshGlobalVariablesTable();
        refreshGlobalVariablesText();
        // Update form if a variable is selected
        if (m_selectedGlobalVariableRow >= 0) {
            onGlobalVariableSelectionChanged();
        }
    }
}

// === View Mode Toggle ===

void GlobalsManagerDialog::onToggleVariableView() {
    int buttonId = m_variableViewGroup->checkedId();
    m_isTableView = (buttonId == 0);

    m_tableViewWidget->setVisible(m_isTableView);
    m_textViewWidget->setVisible(!m_isTableView);
    m_parseTextButton->setVisible(!m_isTableView);

    if (!m_isTableView) {
        refreshGlobalVariablesText();
    }
}

void GlobalsManagerDialog::onVariableTextChanged() {
    // Enable parse button when text changes
    if (!m_isTableView) {
        m_parseTextButton->setEnabled(true);
    }
}

void GlobalsManagerDialog::onParseVariableText() {
    if (parseGlobalVariablesFromText()) {
        refreshGlobalVariablesTable();
        QMessageBox::information(this, "Success", "Global variables parsed successfully from text.");
    } else {
        QMessageBox::warning(this, "Error", "Failed to parse global variables from text. Check the format.");
    }
    m_parseTextButton->setEnabled(false);
}

// === Helper Methods ===

void GlobalsManagerDialog::refreshAutoloadsTable() {
    m_autoloadsTable->setRowCount(0);

    const auto& autoloads = m_globalsManager->GetAllAutoloads();
    m_autoloadsTable->setRowCount(autoloads.size());

    int row = 0;
    for (const auto& [name, autoload] : autoloads) {
        m_autoloadsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(autoload.name)));
        m_autoloadsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(autoload.script_path)));
        m_autoloadsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(autoload.script_type)));
        m_autoloadsTable->setItem(row, 3, new QTableWidgetItem(autoload.enabled ? "Yes" : "No"));
        row++;
    }

    m_autoloadsTable->resizeColumnsToContents();
}

void GlobalsManagerDialog::populateAutoloadForm(const Lupine::AutoloadScript& autoload) {
    m_autoloadNameEdit->setText(QString::fromStdString(autoload.name));
    m_autoloadScriptPathEdit->setText(QString::fromStdString(autoload.script_path));
    m_autoloadScriptTypeCombo->setCurrentText(QString::fromStdString(autoload.script_type));
    m_autoloadEnabledCheck->setChecked(autoload.enabled);
    m_autoloadDescriptionEdit->setText(QString::fromStdString(autoload.description));
}

void GlobalsManagerDialog::clearAutoloadForm() {
    m_autoloadNameEdit->clear();
    m_autoloadScriptPathEdit->clear();
    m_autoloadScriptTypeCombo->setCurrentIndex(0);
    m_autoloadEnabledCheck->setChecked(true);
    m_autoloadDescriptionEdit->clear();
}

bool GlobalsManagerDialog::validateAutoloadForm() {
    return !m_autoloadNameEdit->text().isEmpty() && !m_autoloadScriptPathEdit->text().isEmpty();
}

Lupine::AutoloadScript GlobalsManagerDialog::getAutoloadFromForm() {
    Lupine::AutoloadScript autoload;
    autoload.name = m_autoloadNameEdit->text().toStdString();
    autoload.script_path = m_autoloadScriptPathEdit->text().toStdString();
    autoload.script_type = m_autoloadScriptTypeCombo->currentText().toStdString();
    autoload.enabled = m_autoloadEnabledCheck->isChecked();
    autoload.description = m_autoloadDescriptionEdit->text().toStdString();
    return autoload;
}

void GlobalsManagerDialog::refreshGlobalVariablesTable() {
    m_globalVariablesTable->setRowCount(0);

    const auto& variables = m_globalsManager->GetAllGlobalVariables();
    m_globalVariablesTable->setRowCount(variables.size());

    int row = 0;
    for (const auto& [name, variable] : variables) {
        m_globalVariablesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(variable.name)));
        m_globalVariablesTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(variable.type)));
        m_globalVariablesTable->setItem(row, 2, new QTableWidgetItem(formatGlobalVariableValue(variable.value, variable.type)));
        m_globalVariablesTable->setItem(row, 3, new QTableWidgetItem(formatGlobalVariableValue(variable.default_value, variable.type)));
        m_globalVariablesTable->setItem(row, 4, new QTableWidgetItem(variable.is_exported ? "Yes" : "No"));
        row++;
    }

    m_globalVariablesTable->resizeColumnsToContents();
}

void GlobalsManagerDialog::refreshGlobalVariablesText() {
    QString text;
    const auto& variables = m_globalsManager->GetAllGlobalVariables();

    for (const auto& [name, variable] : variables) {
        text += QString("%1:%2=%3").arg(
            QString::fromStdString(variable.name),
            QString::fromStdString(variable.type),
            formatGlobalVariableValue(variable.value, variable.type)
        );

        if (!variable.description.empty()) {
            text += QString(" # %1").arg(QString::fromStdString(variable.description));
        }

        text += "\n";
    }

    m_globalVariablesTextEdit->setPlainText(text);
}

void GlobalsManagerDialog::populateGlobalVariableForm(const Lupine::GlobalVariable& variable) {
    m_globalVariableNameEdit->setText(QString::fromStdString(variable.name));
    m_globalVariableTypeCombo->setCurrentText(QString::fromStdString(variable.type));
    m_globalVariableValueEdit->setText(formatGlobalVariableValue(variable.value, variable.type));
    m_globalVariableDefaultEdit->setText(formatGlobalVariableValue(variable.default_value, variable.type));
    m_globalVariableExportedCheck->setChecked(variable.is_exported);
    m_globalVariableDescriptionEdit->setText(QString::fromStdString(variable.description));
}

void GlobalsManagerDialog::clearGlobalVariableForm() {
    m_globalVariableNameEdit->clear();
    m_globalVariableTypeCombo->setCurrentIndex(0);
    m_globalVariableValueEdit->clear();
    m_globalVariableDefaultEdit->clear();
    m_globalVariableExportedCheck->setChecked(true);
    m_globalVariableDescriptionEdit->clear();
}

bool GlobalsManagerDialog::validateGlobalVariableForm() {
    return !m_globalVariableNameEdit->text().isEmpty() && !m_globalVariableValueEdit->text().isEmpty();
}

Lupine::GlobalVariable GlobalsManagerDialog::getGlobalVariableFromForm() {
    Lupine::GlobalVariable variable;
    variable.name = m_globalVariableNameEdit->text().toStdString();
    variable.type = m_globalVariableTypeCombo->currentText().toStdString();
    variable.value = parseGlobalVariableValue(m_globalVariableValueEdit->text(), variable.type);
    variable.default_value = parseGlobalVariableValue(m_globalVariableDefaultEdit->text(), variable.type);
    variable.is_exported = m_globalVariableExportedCheck->isChecked();
    variable.description = m_globalVariableDescriptionEdit->text().toStdString();
    return variable;
}

QString GlobalsManagerDialog::formatGlobalVariableValue(const Lupine::GlobalVariableValue& value, const std::string& type) {
    if (type == "bool") {
        return std::get<bool>(value) ? "true" : "false";
    } else if (type == "int") {
        return QString::number(std::get<int>(value));
    } else if (type == "float") {
        return QString::number(std::get<float>(value));
    } else if (type == "string") {
        return QString::fromStdString(std::get<std::string>(value));
    } else if (type == "vec2") {
        auto vec = std::get<glm::vec2>(value);
        return QString("%1 %2").arg(vec.x).arg(vec.y);
    } else if (type == "vec3") {
        auto vec = std::get<glm::vec3>(value);
        return QString("%1 %2 %3").arg(vec.x).arg(vec.y).arg(vec.z);
    } else if (type == "vec4") {
        auto vec = std::get<glm::vec4>(value);
        return QString("%1 %2 %3 %4").arg(vec.x).arg(vec.y).arg(vec.z).arg(vec.w);
    }
    return "";
}

Lupine::GlobalVariableValue GlobalsManagerDialog::parseGlobalVariableValue(const QString& value_str, const std::string& type) {
    return m_globalsManager->ParseVariableValue(type, value_str.toStdString());
}

bool GlobalsManagerDialog::parseGlobalVariablesFromText() {
    QString text = m_globalVariablesTextEdit->toPlainText();
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    // Clear existing variables
    m_globalsManager->Clear();

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }

        // Parse format: name:type=value # description
        QStringList parts = trimmed.split('#');
        QString main_part = parts[0].trimmed();
        QString description = parts.size() > 1 ? parts[1].trimmed() : "";

        QStringList name_type_value = main_part.split('=');
        if (name_type_value.size() != 2) {
            continue;
        }

        QString name_type = name_type_value[0].trimmed();
        QString value_str = name_type_value[1].trimmed();

        QStringList name_type_parts = name_type.split(':');
        if (name_type_parts.size() != 2) {
            continue;
        }

        QString name = name_type_parts[0].trimmed();
        QString type = name_type_parts[1].trimmed();

        if (name.isEmpty() || type.isEmpty()) {
            continue;
        }

        Lupine::GlobalVariable variable;
        variable.name = name.toStdString();
        variable.type = type.toStdString();
        variable.value = parseGlobalVariableValue(value_str, variable.type);
        variable.default_value = variable.value;
        variable.description = description.toStdString();
        variable.is_exported = true;

        if (!m_globalsManager->RegisterGlobalVariable(variable)) {
            return false;
        }
    }

    return true;
}

// === AutoloadEditDialog ===

AutoloadEditDialog::AutoloadEditDialog(const Lupine::AutoloadScript& autoload, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Autoload Script");
    setModal(true);
    resize(500, 300);

    setupUI();

    // Populate with existing data
    m_nameEdit->setText(QString::fromStdString(autoload.name));
    m_scriptPathEdit->setText(QString::fromStdString(autoload.script_path));
    m_scriptTypeCombo->setCurrentText(QString::fromStdString(autoload.script_type));
    m_enabledCheck->setChecked(autoload.enabled);
    m_descriptionEdit->setText(QString::fromStdString(autoload.description));
}

void AutoloadEditDialog::setupUI() {
    m_layout = new QVBoxLayout(this);

    m_formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit();
    m_formLayout->addRow("Name:", m_nameEdit);

    auto scriptPathLayout = new QHBoxLayout();
    m_scriptPathEdit = new QLineEdit();
    m_browseButton = new QPushButton("Browse...");
    scriptPathLayout->addWidget(m_scriptPathEdit);
    scriptPathLayout->addWidget(m_browseButton);
    m_formLayout->addRow("Script Path:", scriptPathLayout);

    m_scriptTypeCombo = new QComboBox();
    m_scriptTypeCombo->addItems({"python", "lua"});
    m_formLayout->addRow("Script Type:", m_scriptTypeCombo);

    m_enabledCheck = new QCheckBox();
    m_enabledCheck->setChecked(true);
    m_formLayout->addRow("Enabled:", m_enabledCheck);

    m_descriptionEdit = new QLineEdit();
    m_formLayout->addRow("Description:", m_descriptionEdit);

    m_layout->addLayout(m_formLayout);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_layout->addWidget(m_buttonBox);

    connect(m_browseButton, &QPushButton::clicked, this, &AutoloadEditDialog::onBrowseScript);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AutoloadEditDialog::onBrowseScript() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select Script File", "",
        "Script Files (*.py *.lua);;Python Files (*.py);;Lua Files (*.lua);;All Files (*)");

    if (!fileName.isEmpty()) {
        m_scriptPathEdit->setText(fileName);

        // Auto-detect script type from extension
        if (fileName.endsWith(".py", Qt::CaseInsensitive)) {
            m_scriptTypeCombo->setCurrentText("python");
        } else if (fileName.endsWith(".lua", Qt::CaseInsensitive)) {
            m_scriptTypeCombo->setCurrentText("lua");
        }
    }
}

Lupine::AutoloadScript AutoloadEditDialog::getAutoload() const {
    Lupine::AutoloadScript autoload;
    autoload.name = m_nameEdit->text().toStdString();
    autoload.script_path = m_scriptPathEdit->text().toStdString();
    autoload.script_type = m_scriptTypeCombo->currentText().toStdString();
    autoload.enabled = m_enabledCheck->isChecked();
    autoload.description = m_descriptionEdit->text().toStdString();
    return autoload;
}

// === GlobalVariableEditDialog ===

GlobalVariableEditDialog::GlobalVariableEditDialog(const Lupine::GlobalVariable& variable, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Global Variable");
    setModal(true);
    resize(400, 300);

    setupUI();

    // Populate with existing data
    m_nameEdit->setText(QString::fromStdString(variable.name));
    m_typeCombo->setCurrentText(QString::fromStdString(variable.type));
    m_exportedCheck->setChecked(variable.is_exported);
    m_descriptionEdit->setText(QString::fromStdString(variable.description));

    // Format values based on type
    if (variable.type == "bool") {
        m_valueEdit->setText(std::get<bool>(variable.value) ? "true" : "false");
        m_defaultEdit->setText(std::get<bool>(variable.default_value) ? "true" : "false");
    } else if (variable.type == "int") {
        m_valueEdit->setText(QString::number(std::get<int>(variable.value)));
        m_defaultEdit->setText(QString::number(std::get<int>(variable.default_value)));
    } else if (variable.type == "float") {
        m_valueEdit->setText(QString::number(std::get<float>(variable.value)));
        m_defaultEdit->setText(QString::number(std::get<float>(variable.default_value)));
    } else if (variable.type == "string") {
        m_valueEdit->setText(QString::fromStdString(std::get<std::string>(variable.value)));
        m_defaultEdit->setText(QString::fromStdString(std::get<std::string>(variable.default_value)));
    } else if (variable.type == "vec2") {
        auto vec = std::get<glm::vec2>(variable.value);
        m_valueEdit->setText(QString("%1 %2").arg(vec.x).arg(vec.y));
        auto default_vec = std::get<glm::vec2>(variable.default_value);
        m_defaultEdit->setText(QString("%1 %2").arg(default_vec.x).arg(default_vec.y));
    } else if (variable.type == "vec3") {
        auto vec = std::get<glm::vec3>(variable.value);
        m_valueEdit->setText(QString("%1 %2 %3").arg(vec.x).arg(vec.y).arg(vec.z));
        auto default_vec = std::get<glm::vec3>(variable.default_value);
        m_defaultEdit->setText(QString("%1 %2 %3").arg(default_vec.x).arg(default_vec.y).arg(default_vec.z));
    } else if (variable.type == "vec4") {
        auto vec = std::get<glm::vec4>(variable.value);
        m_valueEdit->setText(QString("%1 %2 %3 %4").arg(vec.x).arg(vec.y).arg(vec.z).arg(vec.w));
        auto default_vec = std::get<glm::vec4>(variable.default_value);
        m_defaultEdit->setText(QString("%1 %2 %3 %4").arg(default_vec.x).arg(default_vec.y).arg(default_vec.z).arg(default_vec.w));
    }
}

void GlobalVariableEditDialog::setupUI() {
    m_layout = new QVBoxLayout(this);

    m_formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit();
    m_formLayout->addRow("Name:", m_nameEdit);

    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"bool", "int", "float", "string", "vec2", "vec3", "vec4"});
    m_formLayout->addRow("Type:", m_typeCombo);

    m_valueEdit = new QLineEdit();
    m_formLayout->addRow("Value:", m_valueEdit);

    m_defaultEdit = new QLineEdit();
    m_formLayout->addRow("Default:", m_defaultEdit);

    m_exportedCheck = new QCheckBox();
    m_exportedCheck->setChecked(true);
    m_formLayout->addRow("Exported:", m_exportedCheck);

    m_descriptionEdit = new QLineEdit();
    m_formLayout->addRow("Description:", m_descriptionEdit);

    m_layout->addLayout(m_formLayout);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_layout->addWidget(m_buttonBox);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlobalVariableEditDialog::onTypeChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void GlobalVariableEditDialog::onTypeChanged() {
    updateValueEditors();
}

void GlobalVariableEditDialog::updateValueEditors() {
    QString type = m_typeCombo->currentText();

    // Set placeholder text based on type
    if (type == "bool") {
        m_valueEdit->setPlaceholderText("true or false");
        m_defaultEdit->setPlaceholderText("true or false");
    } else if (type == "int") {
        m_valueEdit->setPlaceholderText("Integer value (e.g., 42)");
        m_defaultEdit->setPlaceholderText("Integer value (e.g., 0)");
    } else if (type == "float") {
        m_valueEdit->setPlaceholderText("Float value (e.g., 3.14)");
        m_defaultEdit->setPlaceholderText("Float value (e.g., 0.0)");
    } else if (type == "string") {
        m_valueEdit->setPlaceholderText("Text value");
        m_defaultEdit->setPlaceholderText("Default text");
    } else if (type == "vec2") {
        m_valueEdit->setPlaceholderText("x y (e.g., 1.0 2.0)");
        m_defaultEdit->setPlaceholderText("x y (e.g., 0.0 0.0)");
    } else if (type == "vec3") {
        m_valueEdit->setPlaceholderText("x y z (e.g., 1.0 2.0 3.0)");
        m_defaultEdit->setPlaceholderText("x y z (e.g., 0.0 0.0 0.0)");
    } else if (type == "vec4") {
        m_valueEdit->setPlaceholderText("x y z w (e.g., 1.0 2.0 3.0 4.0)");
        m_defaultEdit->setPlaceholderText("x y z w (e.g., 0.0 0.0 0.0 0.0)");
    }
}

Lupine::GlobalVariable GlobalVariableEditDialog::getGlobalVariable() const {
    Lupine::GlobalVariable variable;
    variable.name = m_nameEdit->text().toStdString();
    variable.type = m_typeCombo->currentText().toStdString();
    variable.is_exported = m_exportedCheck->isChecked();
    variable.description = m_descriptionEdit->text().toStdString();

    // Parse values based on type
    QString value_str = m_valueEdit->text();
    QString default_str = m_defaultEdit->text().isEmpty() ? value_str : m_defaultEdit->text();

    auto& globals_manager = Lupine::GlobalsManager::Instance();
    variable.value = globals_manager.ParseVariableValue(variable.type, value_str.toStdString());
    variable.default_value = globals_manager.ParseVariableValue(variable.type, default_str.toStdString());

    return variable;
}
