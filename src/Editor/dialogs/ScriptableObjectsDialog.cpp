#include "ScriptableObjectsDialog.h"
#include "lupine/scriptableobjects/ScriptableObjectManager.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QGridLayout>
#include <QStandardPaths>

ScriptableObjectsDialog::ScriptableObjectsDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_templatesPanel(nullptr)
    , m_templatesLayout(nullptr)
    , m_templatesTree(nullptr)
    , m_newTemplateButton(nullptr)
    , m_deleteTemplateButton(nullptr)
    , m_detailsTabs(nullptr)
    , m_templateDetailsTab(nullptr)
    , m_templateDetailsLayout(nullptr)
    , m_templateNameEdit(nullptr)
    , m_templateDescriptionEdit(nullptr)
    , m_templateScriptLanguageCombo(nullptr)
    , m_fieldsTable(nullptr)
    , m_addFieldButton(nullptr)
    , m_deleteFieldButton(nullptr)
    , m_customMethodsEdit(nullptr)
    , m_instanceDetailsTab(nullptr)
    , m_instanceDetailsLayout(nullptr)
    , m_instancesTree(nullptr)
    , m_newInstanceButton(nullptr)
    , m_deleteInstanceButton(nullptr)
    , m_instanceNameEdit(nullptr)
    , m_instanceFieldsTable(nullptr)
    , m_modified(false)
    , m_selectedTemplate(nullptr)
    , m_selectedInstance(nullptr) {
    
    setWindowTitle("Scriptable Objects");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    setupUI();
    setupConnections();
    updateTemplateList();
    updateWindowTitle();
}

ScriptableObjectsDialog::~ScriptableObjectsDialog() {
}

void ScriptableObjectsDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    setupMenuBar();
    setupToolBar();
    
    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
    
    // Left panel - Templates
    m_templatesPanel = new QWidget();
    m_templatesPanel->setMinimumWidth(300);
    m_templatesPanel->setMaximumWidth(400);
    m_mainSplitter->addWidget(m_templatesPanel);
    
    m_templatesLayout = new QVBoxLayout(m_templatesPanel);
    
    QLabel* templatesLabel = new QLabel("Templates");
    templatesLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    m_templatesLayout->addWidget(templatesLabel);
    
    m_templatesTree = new QTreeWidget();
    m_templatesTree->setHeaderLabel("Template Name");
    m_templatesTree->setRootIsDecorated(false);
    m_templatesLayout->addWidget(m_templatesTree);
    
    QHBoxLayout* templateButtonsLayout = new QHBoxLayout();
    m_newTemplateButton = new QPushButton("New Template");
    m_deleteTemplateButton = new QPushButton("Delete Template");
    m_deleteTemplateButton->setEnabled(false);
    templateButtonsLayout->addWidget(m_newTemplateButton);
    templateButtonsLayout->addWidget(m_deleteTemplateButton);
    templateButtonsLayout->addStretch();
    m_templatesLayout->addLayout(templateButtonsLayout);
    
    // Right panel - Details tabs
    m_detailsTabs = new QTabWidget();
    m_mainSplitter->addWidget(m_detailsTabs);
    
    // Template details tab
    m_templateDetailsTab = new QWidget();
    m_detailsTabs->addTab(m_templateDetailsTab, "Template Details");
    
    m_templateDetailsLayout = new QVBoxLayout(m_templateDetailsTab);
    
    // Template basic info
    QGroupBox* templateInfoGroup = new QGroupBox("Template Information");
    m_templateDetailsLayout->addWidget(templateInfoGroup);
    
    QGridLayout* templateInfoLayout = new QGridLayout(templateInfoGroup);
    
    templateInfoLayout->addWidget(new QLabel("Name:"), 0, 0);
    m_templateNameEdit = new QLineEdit();
    m_templateNameEdit->setEnabled(false);
    templateInfoLayout->addWidget(m_templateNameEdit, 0, 1);
    
    templateInfoLayout->addWidget(new QLabel("Description:"), 1, 0);
    m_templateDescriptionEdit = new QTextEdit();
    m_templateDescriptionEdit->setMaximumHeight(80);
    m_templateDescriptionEdit->setEnabled(false);
    templateInfoLayout->addWidget(m_templateDescriptionEdit, 1, 1);
    
    templateInfoLayout->addWidget(new QLabel("Script Language:"), 2, 0);
    m_templateScriptLanguageCombo = new QComboBox();
    m_templateScriptLanguageCombo->addItems({"python", "lua"});
    m_templateScriptLanguageCombo->setEnabled(false);
    templateInfoLayout->addWidget(m_templateScriptLanguageCombo, 2, 1);
    
    // Fields section
    QGroupBox* fieldsGroup = new QGroupBox("Fields");
    m_templateDetailsLayout->addWidget(fieldsGroup);
    
    QVBoxLayout* fieldsLayout = new QVBoxLayout(fieldsGroup);
    
    m_fieldsTable = new QTableWidget();
    m_fieldsTable->setColumnCount(4);
    m_fieldsTable->setHorizontalHeaderLabels({"Name", "Type", "Default Value", "Description"});
    m_fieldsTable->horizontalHeader()->setStretchLastSection(true);
    m_fieldsTable->setEnabled(false);
    fieldsLayout->addWidget(m_fieldsTable);
    
    QHBoxLayout* fieldButtonsLayout = new QHBoxLayout();
    m_addFieldButton = new QPushButton("Add Field");
    m_deleteFieldButton = new QPushButton("Delete Field");
    m_addFieldButton->setEnabled(false);
    m_deleteFieldButton->setEnabled(false);
    fieldButtonsLayout->addWidget(m_addFieldButton);
    fieldButtonsLayout->addWidget(m_deleteFieldButton);
    fieldButtonsLayout->addStretch();
    fieldsLayout->addLayout(fieldButtonsLayout);
    
    // Custom methods section
    QGroupBox* methodsGroup = new QGroupBox("Custom Methods");
    m_templateDetailsLayout->addWidget(methodsGroup);
    
    QVBoxLayout* methodsLayout = new QVBoxLayout(methodsGroup);
    
    m_customMethodsEdit = new QTextEdit();
    m_customMethodsEdit->setPlaceholderText("Define custom methods here...");
    m_customMethodsEdit->setEnabled(false);
    methodsLayout->addWidget(m_customMethodsEdit);
    
    // Instance details tab
    m_instanceDetailsTab = new QWidget();
    m_detailsTabs->addTab(m_instanceDetailsTab, "Instances");
    
    m_instanceDetailsLayout = new QVBoxLayout(m_instanceDetailsTab);
    
    // Instance list
    QGroupBox* instancesGroup = new QGroupBox("Instances");
    m_instanceDetailsLayout->addWidget(instancesGroup);
    
    QVBoxLayout* instancesLayout = new QVBoxLayout(instancesGroup);
    
    m_instancesTree = new QTreeWidget();
    m_instancesTree->setHeaderLabel("Instance Name");
    m_instancesTree->setRootIsDecorated(false);
    instancesLayout->addWidget(m_instancesTree);
    
    QHBoxLayout* instanceButtonsLayout = new QHBoxLayout();
    m_newInstanceButton = new QPushButton("New Instance");
    m_deleteInstanceButton = new QPushButton("Delete Instance");
    m_newInstanceButton->setEnabled(false);
    m_deleteInstanceButton->setEnabled(false);
    instanceButtonsLayout->addWidget(m_newInstanceButton);
    instanceButtonsLayout->addWidget(m_deleteInstanceButton);
    instanceButtonsLayout->addStretch();
    instancesLayout->addLayout(instanceButtonsLayout);
    
    // Instance details
    QGroupBox* instanceDetailsGroup = new QGroupBox("Instance Details");
    m_instanceDetailsLayout->addWidget(instanceDetailsGroup);
    
    QVBoxLayout* instanceDetailsLayout = new QVBoxLayout(instanceDetailsGroup);
    
    QHBoxLayout* instanceNameLayout = new QHBoxLayout();
    instanceNameLayout->addWidget(new QLabel("Name:"));
    m_instanceNameEdit = new QLineEdit();
    m_instanceNameEdit->setEnabled(false);
    instanceNameLayout->addWidget(m_instanceNameEdit);
    instanceNameLayout->addStretch();
    instanceDetailsLayout->addLayout(instanceNameLayout);
    
    m_instanceFieldsTable = new QTableWidget();
    m_instanceFieldsTable->setColumnCount(3);
    m_instanceFieldsTable->setHorizontalHeaderLabels({"Field", "Type", "Value"});
    m_instanceFieldsTable->horizontalHeader()->setStretchLastSection(true);
    m_instanceFieldsTable->setEnabled(false);
    instanceDetailsLayout->addWidget(m_instanceFieldsTable);
    
    // Set splitter proportions
    m_mainSplitter->setSizes({300, 900});
}

void ScriptableObjectsDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setFixedHeight(50);
    m_mainLayout->addWidget(m_menuBar);
    
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New", this, &ScriptableObjectsDialog::onNew, QKeySequence::New);
    fileMenu->addAction("&Open...", this, &ScriptableObjectsDialog::onOpen, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", this, &ScriptableObjectsDialog::onSave, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &ScriptableObjectsDialog::onSaveAs, QKeySequence::SaveAs);
}

void ScriptableObjectsDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(50);
    m_mainLayout->addWidget(m_toolBar);
    
    m_toolBar->addAction("New", this, &ScriptableObjectsDialog::onNew);
    m_toolBar->addAction("Open", this, &ScriptableObjectsDialog::onOpen);
    m_toolBar->addAction("Save", this, &ScriptableObjectsDialog::onSave);
    m_toolBar->addSeparator();
    m_toolBar->addAction("New Template", this, &ScriptableObjectsDialog::onNewTemplate);
    m_toolBar->addAction("Delete Template", this, &ScriptableObjectsDialog::onDeleteTemplate);
}

void ScriptableObjectsDialog::setupConnections() {
    // Template connections
    connect(m_templatesTree, &QTreeWidget::itemSelectionChanged, this, &ScriptableObjectsDialog::onTemplateSelectionChanged);
    connect(m_newTemplateButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onNewTemplate);
    connect(m_deleteTemplateButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onDeleteTemplate);
    
    connect(m_templateNameEdit, &QLineEdit::textChanged, this, &ScriptableObjectsDialog::onTemplateNameChanged);
    connect(m_templateDescriptionEdit, &QTextEdit::textChanged, this, &ScriptableObjectsDialog::onTemplateDescriptionChanged);
    connect(m_templateScriptLanguageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScriptableObjectsDialog::onTemplateScriptLanguageChanged);
    connect(m_customMethodsEdit, &QTextEdit::textChanged, this, &ScriptableObjectsDialog::onTemplateCustomMethodsChanged);
    
    // Field connections
    connect(m_addFieldButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onAddField);
    connect(m_deleteFieldButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onDeleteField);
    connect(m_fieldsTable, &QTableWidget::cellChanged, this, &ScriptableObjectsDialog::onFieldDataChanged);
    
    // Instance connections
    connect(m_instancesTree, &QTreeWidget::itemSelectionChanged, this, &ScriptableObjectsDialog::onInstanceSelectionChanged);
    connect(m_newInstanceButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onNewInstance);
    connect(m_deleteInstanceButton, &QPushButton::clicked, this, &ScriptableObjectsDialog::onDeleteInstance);
    connect(m_instanceNameEdit, &QLineEdit::textChanged, this, &ScriptableObjectsDialog::onInstanceNameChanged);
    connect(m_instanceFieldsTable, &QTableWidget::cellChanged, this, &ScriptableObjectsDialog::onInstanceDataChanged);
}

// File operations
void ScriptableObjectsDialog::onNew() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    manager.Clear();

    m_currentFilePath.clear();
    setModified(false);
    updateTemplateList();
    clearTemplateSelection();
    clearInstanceSelection();
    updateWindowTitle();
}

void ScriptableObjectsDialog::onOpen() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this,
        "Open Scriptable Objects",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Scriptable Objects Files (*.so.json)");

    if (!filePath.isEmpty()) {
        loadFromFile(filePath);
    }
}

void ScriptableObjectsDialog::onSave() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
    } else {
        saveToFile(m_currentFilePath);
    }
}

void ScriptableObjectsDialog::onSaveAs() {
    QString filePath = QFileDialog::getSaveFileName(this,
        "Save Scriptable Objects",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Scriptable Objects Files (*.so.json)");

    if (!filePath.isEmpty()) {
        if (!filePath.endsWith(".so.json")) {
            filePath += ".so.json";
        }
        saveToFile(filePath);
    }
}

// Template operations
void ScriptableObjectsDialog::onNewTemplate() {
    auto& manager = Lupine::ScriptableObjectManager::Instance();

    QString baseName = "NewTemplate";
    QString templateName = baseName;
    int counter = 1;

    while (manager.TemplateNameExists(templateName.toStdString())) {
        templateName = baseName + QString::number(counter++);
    }

    auto template_obj = manager.CreateTemplate(templateName.toStdString());
    if (template_obj) {
        setModified(true);
        updateTemplateList();

        // Select the new template
        for (int i = 0; i < m_templatesTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_templatesTree->topLevelItem(i);
            if (item->text(0) == templateName) {
                m_templatesTree->setCurrentItem(item);
                break;
            }
        }
    }
}

void ScriptableObjectsDialog::onDeleteTemplate() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    int ret = QMessageBox::question(this, "Delete Template",
        QString("Are you sure you want to delete template '%1' and all its instances?")
        .arg(QString::fromStdString(template_obj->GetName())),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& manager = Lupine::ScriptableObjectManager::Instance();
        manager.RemoveTemplate(template_obj->GetUUID());

        setModified(true);
        updateTemplateList();
        clearTemplateSelection();
        clearInstanceSelection();
    }
}

void ScriptableObjectsDialog::onTemplateSelectionChanged() {
    updateTemplateDetails();
    updateInstanceList();
    clearInstanceSelection();
}

void ScriptableObjectsDialog::onTemplateNameChanged() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    QString newName = m_templateNameEdit->text();
    if (newName.isEmpty()) return;

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    if (newName.toStdString() != template_obj->GetName() &&
        manager.TemplateNameExists(newName.toStdString())) {
        QMessageBox::warning(this, "Name Conflict", "A template with this name already exists.");
        m_templateNameEdit->setText(QString::fromStdString(template_obj->GetName()));
        return;
    }

    template_obj->SetName(newName.toStdString());
    setModified(true);
    updateTemplateList();
}

void ScriptableObjectsDialog::onTemplateDescriptionChanged() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    template_obj->SetDescription(m_templateDescriptionEdit->toPlainText().toStdString());
    setModified(true);
}

void ScriptableObjectsDialog::onTemplateScriptLanguageChanged() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    template_obj->SetScriptLanguage(m_templateScriptLanguageCombo->currentText().toStdString());
    setModified(true);
}

void ScriptableObjectsDialog::onTemplateCustomMethodsChanged() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    template_obj->SetCustomMethods(m_customMethodsEdit->toPlainText().toStdString());
    setModified(true);
}

// Field operations
void ScriptableObjectsDialog::onAddField() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    Lupine::ScriptableObjectField field;
    field.name = "new_field";
    field.type = Lupine::ScriptableObjectFieldType::String;
    field.default_value = std::string("");
    field.description = "";

    template_obj->AddField(field);
    setModified(true);
    updateFieldList();
}

void ScriptableObjectsDialog::onDeleteField() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    int currentRow = m_fieldsTable->currentRow();
    if (currentRow < 0) return;

    QString fieldName = m_fieldsTable->item(currentRow, 0)->text();
    template_obj->RemoveField(fieldName.toStdString());

    setModified(true);
    updateFieldList();
    updateInstanceList(); // Update instances as field structure changed
}

void ScriptableObjectsDialog::onFieldSelectionChanged() {
    bool hasSelection = m_fieldsTable->currentRow() >= 0;
    m_deleteFieldButton->setEnabled(hasSelection);
}

void ScriptableObjectsDialog::onFieldDataChanged(int row, int column) {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    if (row >= static_cast<int>(template_obj->GetFields().size())) return;

    auto fields = template_obj->GetFields();
    auto& field = const_cast<Lupine::ScriptableObjectField&>(fields[row]);

    QTableWidgetItem* item = m_fieldsTable->item(row, column);
    if (!item) return;

    switch (column) {
        case 0: // Name
            field.name = item->text().toStdString();
            break;
        case 1: // Type
            field.type = Lupine::ScriptableObjectTemplate::StringToFieldType(item->text().toStdString());
            break;
        case 2: // Default Value
            field.default_value = Lupine::ScriptableObjectTemplate::StringToFieldValue(item->text().toStdString(), field.type);
            break;
        case 3: // Description
            field.description = item->text().toStdString();
            break;
    }

    template_obj->RemoveField(field.name);
    template_obj->AddField(field);
    setModified(true);
}

// Instance operations
void ScriptableObjectsDialog::onNewInstance() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    auto& manager = Lupine::ScriptableObjectManager::Instance();

    QString baseName = "NewInstance";
    QString instanceName = baseName;
    int counter = 1;

    while (manager.InstanceNameExists(template_obj->GetName(), instanceName.toStdString())) {
        instanceName = baseName + QString::number(counter++);
    }

    auto instance = manager.CreateInstance(template_obj->GetUUID(), instanceName.toStdString());
    if (instance) {
        setModified(true);
        updateInstanceList();

        // Select the new instance
        for (int i = 0; i < m_instancesTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_instancesTree->topLevelItem(i);
            if (item->text(0) == instanceName) {
                m_instancesTree->setCurrentItem(item);
                break;
            }
        }
    }
}

void ScriptableObjectsDialog::onDeleteInstance() {
    auto instance = getSelectedInstance();
    if (!instance) return;

    int ret = QMessageBox::question(this, "Delete Instance",
        QString("Are you sure you want to delete instance '%1'?")
        .arg(QString::fromStdString(instance->GetName())),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& manager = Lupine::ScriptableObjectManager::Instance();
        manager.RemoveInstance(instance->GetUUID());

        setModified(true);
        updateInstanceList();
        clearInstanceSelection();
    }
}

void ScriptableObjectsDialog::onInstanceSelectionChanged() {
    updateInstanceDetails();
}

void ScriptableObjectsDialog::onInstanceNameChanged() {
    auto instance = getSelectedInstance();
    if (!instance) return;

    QString newName = m_instanceNameEdit->text();
    if (newName.isEmpty()) return;

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    auto template_obj = instance->GetTemplate();
    if (template_obj && newName.toStdString() != instance->GetName() &&
        manager.InstanceNameExists(template_obj->GetName(), newName.toStdString())) {
        QMessageBox::warning(this, "Name Conflict", "An instance with this name already exists for this template.");
        m_instanceNameEdit->setText(QString::fromStdString(instance->GetName()));
        return;
    }

    instance->SetName(newName.toStdString());
    setModified(true);
    updateInstanceList();
}

void ScriptableObjectsDialog::onInstanceDataChanged(int row, int column) {
    auto instance = getSelectedInstance();
    if (!instance || !instance->GetTemplate()) return;

    const auto& fields = instance->GetTemplate()->GetFields();
    if (row >= static_cast<int>(fields.size())) return;

    if (column != 2) return; // Only value column is editable

    QTableWidgetItem* item = m_instanceFieldsTable->item(row, column);
    if (!item) return;

    const auto& field = fields[row];
    instance->SetFieldValueFromString(field.name, item->text().toStdString());
    setModified(true);
}

// UI update methods
void ScriptableObjectsDialog::updateTemplateList() {
    m_templatesTree->clear();

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    const auto& templates = manager.GetTemplates();

    for (const auto& pair : templates) {
        auto template_obj = pair.second;
        QTreeWidgetItem* item = new QTreeWidgetItem(m_templatesTree);
        item->setText(0, QString::fromStdString(template_obj->GetName()));
        item->setData(0, Qt::UserRole, QString::fromStdString(template_obj->GetUUID().ToString()));
    }
}

void ScriptableObjectsDialog::updateTemplateDetails() {
    auto template_obj = getSelectedTemplate();

    bool hasTemplate = (template_obj != nullptr);
    m_templateNameEdit->setEnabled(hasTemplate);
    m_templateDescriptionEdit->setEnabled(hasTemplate);
    m_templateScriptLanguageCombo->setEnabled(hasTemplate);
    m_fieldsTable->setEnabled(hasTemplate);
    m_addFieldButton->setEnabled(hasTemplate);
    m_customMethodsEdit->setEnabled(hasTemplate);
    m_newInstanceButton->setEnabled(hasTemplate);

    if (template_obj) {
        m_templateNameEdit->setText(QString::fromStdString(template_obj->GetName()));
        m_templateDescriptionEdit->setPlainText(QString::fromStdString(template_obj->GetDescription()));

        QString language = QString::fromStdString(template_obj->GetScriptLanguage());
        int index = m_templateScriptLanguageCombo->findText(language);
        if (index >= 0) {
            m_templateScriptLanguageCombo->setCurrentIndex(index);
        }

        m_customMethodsEdit->setPlainText(QString::fromStdString(template_obj->GetCustomMethods()));

        updateFieldList();
    } else {
        m_templateNameEdit->clear();
        m_templateDescriptionEdit->clear();
        m_templateScriptLanguageCombo->setCurrentIndex(0);
        m_customMethodsEdit->clear();
        m_fieldsTable->setRowCount(0);
    }

    m_deleteTemplateButton->setEnabled(hasTemplate);
}

void ScriptableObjectsDialog::updateFieldList() {
    auto template_obj = getSelectedTemplate();
    if (!template_obj) {
        m_fieldsTable->setRowCount(0);
        return;
    }

    const auto& fields = template_obj->GetFields();
    m_fieldsTable->setRowCount(static_cast<int>(fields.size()));

    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];

        // Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(field.name));
        m_fieldsTable->setItem(static_cast<int>(i), 0, nameItem);

        // Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(QString::fromStdString(
            Lupine::ScriptableObjectTemplate::FieldTypeToString(field.type)));
        m_fieldsTable->setItem(static_cast<int>(i), 1, typeItem);

        // Default Value
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::fromStdString(
            Lupine::ScriptableObjectTemplate::FieldValueToString(field.default_value, field.type)));
        m_fieldsTable->setItem(static_cast<int>(i), 2, valueItem);

        // Description
        QTableWidgetItem* descItem = new QTableWidgetItem(QString::fromStdString(field.description));
        m_fieldsTable->setItem(static_cast<int>(i), 3, descItem);
    }

    bool hasSelection = m_fieldsTable->currentRow() >= 0;
    m_deleteFieldButton->setEnabled(hasSelection);
}

void ScriptableObjectsDialog::updateInstanceList() {
    m_instancesTree->clear();

    auto template_obj = getSelectedTemplate();
    if (!template_obj) return;

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    auto instances = manager.GetInstancesForTemplate(template_obj->GetUUID());

    for (auto instance : instances) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_instancesTree);
        item->setText(0, QString::fromStdString(instance->GetName()));
        item->setData(0, Qt::UserRole, QString::fromStdString(instance->GetUUID().ToString()));
    }
}

void ScriptableObjectsDialog::updateInstanceDetails() {
    auto instance = getSelectedInstance();

    bool hasInstance = (instance != nullptr);
    m_instanceNameEdit->setEnabled(hasInstance);
    m_instanceFieldsTable->setEnabled(hasInstance);
    m_deleteInstanceButton->setEnabled(hasInstance);

    if (instance && instance->GetTemplate()) {
        m_instanceNameEdit->setText(QString::fromStdString(instance->GetName()));

        const auto& fields = instance->GetTemplate()->GetFields();
        m_instanceFieldsTable->setRowCount(static_cast<int>(fields.size()));

        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];

            // Field name
            QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(field.name));
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            m_instanceFieldsTable->setItem(static_cast<int>(i), 0, nameItem);

            // Field type
            QTableWidgetItem* typeItem = new QTableWidgetItem(QString::fromStdString(
                Lupine::ScriptableObjectTemplate::FieldTypeToString(field.type)));
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
            m_instanceFieldsTable->setItem(static_cast<int>(i), 1, typeItem);

            // Field value
            QString valueStr = QString::fromStdString(instance->GetFieldValueAsString(field.name));
            QTableWidgetItem* valueItem = new QTableWidgetItem(valueStr);
            m_instanceFieldsTable->setItem(static_cast<int>(i), 2, valueItem);
        }
    } else {
        m_instanceNameEdit->clear();
        m_instanceFieldsTable->setRowCount(0);
    }
}

void ScriptableObjectsDialog::clearTemplateSelection() {
    m_templatesTree->clearSelection();
    m_selectedTemplate = nullptr;
    updateTemplateDetails();
}

void ScriptableObjectsDialog::clearInstanceSelection() {
    m_instancesTree->clearSelection();
    m_selectedInstance = nullptr;
    updateInstanceDetails();
}

std::shared_ptr<Lupine::ScriptableObjectTemplate> ScriptableObjectsDialog::getSelectedTemplate() const {
    QTreeWidgetItem* item = m_templatesTree->currentItem();
    if (!item) return nullptr;

    QString uuidStr = item->data(0, Qt::UserRole).toString();
    Lupine::UUID uuid = Lupine::UUID::FromString(uuidStr.toStdString());

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    return manager.GetTemplate(uuid);
}

std::shared_ptr<Lupine::ScriptableObjectInstance> ScriptableObjectsDialog::getSelectedInstance() const {
    QTreeWidgetItem* item = m_instancesTree->currentItem();
    if (!item) return nullptr;

    QString uuidStr = item->data(0, Qt::UserRole).toString();
    Lupine::UUID uuid = Lupine::UUID::FromString(uuidStr.toStdString());

    auto& manager = Lupine::ScriptableObjectManager::Instance();
    return manager.GetInstance(uuid);
}

void ScriptableObjectsDialog::updateWindowTitle() {
    QString title = "Scriptable Objects";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.fileName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void ScriptableObjectsDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

bool ScriptableObjectsDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool ScriptableObjectsDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    int ret = QMessageBox::question(this, "Unsaved Changes",
        "You have unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
        case QMessageBox::Save:
            onSave();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void ScriptableObjectsDialog::loadFromFile(const QString& filePath) {
    auto& manager = Lupine::ScriptableObjectManager::Instance();

    if (manager.LoadFromFile(filePath.toStdString())) {
        m_currentFilePath = filePath;
        setModified(false);
        updateTemplateList();
        clearTemplateSelection();
        clearInstanceSelection();
        QMessageBox::information(this, "Success", "Scriptable objects loaded successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to load scriptable objects from file.");
    }
}

void ScriptableObjectsDialog::saveToFile(const QString& filePath) {
    auto& manager = Lupine::ScriptableObjectManager::Instance();

    if (manager.SaveToFile(filePath.toStdString())) {
        m_currentFilePath = filePath;
        setModified(false);
        QMessageBox::information(this, "Success", "Scriptable objects saved successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to save scriptable objects to file.");
    }
}
