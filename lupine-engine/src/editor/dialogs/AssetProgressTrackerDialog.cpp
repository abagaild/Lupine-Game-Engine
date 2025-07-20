#include "AssetProgressTrackerDialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QHeaderView>
#include <QUuid>
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTreeWidget>
#include <QCheckBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QTimer>
#include <QScrollArea>
#include <QTime>

// AssetStage Implementation
AssetStage::AssetStage()
    : id(QUuid::createUuid().toString())
    , name("New Stage")
    , color(Qt::blue)
    , order(0)
    , isRequired(true)
    , estimatedDays(1)
{
}

AssetStage::AssetStage(const QString& name, const QColor& color)
    : id(QUuid::createUuid().toString())
    , name(name)
    , color(color)
    , order(0)
    , isRequired(true)
    , estimatedDays(1)
{
}

QJsonObject AssetStage::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["color"] = color.name();
    obj["order"] = order;
    obj["isRequired"] = isRequired;
    obj["estimatedDays"] = estimatedDays;
    return obj;
}

void AssetStage::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    name = json["name"].toString();
    description = json["description"].toString();
    color = QColor(json["color"].toString());
    order = json["order"].toInt();
    isRequired = json["isRequired"].toBool();
    estimatedDays = json["estimatedDays"].toInt();
}

// AssetType Implementation
AssetType::AssetType()
    : id(QUuid::createUuid().toString())
    , name("New Asset Type")
{
}

AssetType::AssetType(const QString& name)
    : id(QUuid::createUuid().toString())
    , name(name)
{
}

QJsonObject AssetType::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["defaultAssignee"] = defaultAssignee;
    
    QJsonArray stagesArray;
    for (const AssetStage& stage : stages) {
        stagesArray.append(stage.toJson());
    }
    obj["stages"] = stagesArray;
    
    QJsonArray tagsArray;
    for (const QString& tag : tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;
    
    return obj;
}

void AssetType::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    name = json["name"].toString();
    description = json["description"].toString();
    defaultAssignee = json["defaultAssignee"].toString();
    
    stages.clear();
    QJsonArray stagesArray = json["stages"].toArray();
    for (const QJsonValue& value : stagesArray) {
        AssetStage stage;
        stage.fromJson(value.toObject());
        stages.append(stage);
    }
    
    tags.clear();
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& value : tagsArray) {
        tags.append(value.toString());
    }
}

AssetStage* AssetType::findStage(const QString& stageId) {
    for (AssetStage& stage : stages) {
        if (stage.id == stageId) {
            return &stage;
        }
    }
    return nullptr;
}

int AssetType::getStageIndex(const QString& stageId) const {
    for (int i = 0; i < stages.size(); ++i) {
        if (stages[i].id == stageId) {
            return i;
        }
    }
    return -1;
}

// Asset Implementation
Asset::Asset()
    : id(QUuid::createUuid().toString())
    , name("New Asset")
    , createdDate(QDateTime::currentDateTime())
    , updatedDate(QDateTime::currentDateTime())
    , priority(3)
{
}

Asset::Asset(const QString& name, const QString& assetTypeId)
    : id(QUuid::createUuid().toString())
    , name(name)
    , assetTypeId(assetTypeId)
    , createdDate(QDateTime::currentDateTime())
    , updatedDate(QDateTime::currentDateTime())
    , priority(3)
{
}

QJsonObject Asset::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["assetTypeId"] = assetTypeId;
    obj["currentStageId"] = currentStageId;
    obj["assignee"] = assignee;
    obj["createdDate"] = createdDate.toString(Qt::ISODate);
    obj["updatedDate"] = updatedDate.toString(Qt::ISODate);
    obj["targetDate"] = targetDate.toString(Qt::ISODate);
    obj["filePath"] = filePath;
    obj["notes"] = notes;
    obj["priority"] = priority;
    
    QJsonArray tagsArray;
    for (const QString& tag : tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;
    
    QJsonObject completionDatesObj;
    for (auto it = stageCompletionDates.begin(); it != stageCompletionDates.end(); ++it) {
        completionDatesObj[it.key()] = it.value().toString(Qt::ISODate);
    }
    obj["stageCompletionDates"] = completionDatesObj;
    
    QJsonObject stageNotesObj;
    for (auto it = stageNotes.begin(); it != stageNotes.end(); ++it) {
        stageNotesObj[it.key()] = it.value();
    }
    obj["stageNotes"] = stageNotesObj;
    
    return obj;
}

void Asset::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    name = json["name"].toString();
    description = json["description"].toString();
    assetTypeId = json["assetTypeId"].toString();
    currentStageId = json["currentStageId"].toString();
    assignee = json["assignee"].toString();
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    updatedDate = QDateTime::fromString(json["updatedDate"].toString(), Qt::ISODate);
    targetDate = QDateTime::fromString(json["targetDate"].toString(), Qt::ISODate);
    filePath = json["filePath"].toString();
    notes = json["notes"].toString();
    priority = json["priority"].toInt();
    
    tags.clear();
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& value : tagsArray) {
        tags.append(value.toString());
    }
    
    stageCompletionDates.clear();
    QJsonObject completionDatesObj = json["stageCompletionDates"].toObject();
    for (auto it = completionDatesObj.begin(); it != completionDatesObj.end(); ++it) {
        stageCompletionDates[it.key()] = QDateTime::fromString(it.value().toString(), Qt::ISODate);
    }
    
    stageNotes.clear();
    QJsonObject stageNotesObj = json["stageNotes"].toObject();
    for (auto it = stageNotesObj.begin(); it != stageNotesObj.end(); ++it) {
        stageNotes[it.key()] = it.value().toString();
    }
}

bool Asset::isStageCompleted(const QString& stageId) const {
    return stageCompletionDates.contains(stageId);
}

int Asset::getCurrentStageIndex(const AssetType& assetType) const {
    return assetType.getStageIndex(currentStageId);
}

int Asset::getCompletionPercentage(const AssetType& assetType) const {
    if (assetType.stages.isEmpty()) return 0;
    
    int completedStages = 0;
    for (const AssetStage& stage : assetType.stages) {
        if (isStageCompleted(stage.id)) {
            completedStages++;
        }
    }
    
    return (completedStages * 100) / assetType.stages.size();
}

bool Asset::isOverdue() const {
    return targetDate.isValid() && targetDate < QDateTime::currentDateTime();
}

int Asset::getDaysInCurrentStage() const {
    if (currentStageId.isEmpty()) return 0;
    
    // Find when we entered the current stage
    QDateTime stageStartDate = updatedDate;
    for (auto it = stageCompletionDates.begin(); it != stageCompletionDates.end(); ++it) {
        if (it.value() > stageStartDate) {
            stageStartDate = it.value();
        }
    }
    
    return stageStartDate.daysTo(QDateTime::currentDateTime());
}

// AssetItem Implementation
AssetItem::AssetItem(const Asset& asset, const AssetType& assetType, QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , m_asset(asset)
{
    setupItem(assetType);
}

void AssetItem::setupItem(const AssetType& assetType) {
    updateFromAsset(m_asset, assetType);
}

void AssetItem::updateFromAsset(const Asset& asset, const AssetType& assetType) {
    m_asset = asset;
    
    setText(0, asset.name);
    setText(1, assetType.name);
    
    // Find current stage name
    AssetStage* currentStage = const_cast<AssetType&>(assetType).findStage(asset.currentStageId);
    setText(2, currentStage ? currentStage->name : "Not Started");
    
    setText(3, asset.assignee);
    setText(4, asset.targetDate.isValid() ? asset.targetDate.toString("yyyy-MM-dd") : "");
    setText(5, QString::number(asset.priority));
    setText(6, QString::number(asset.getCompletionPercentage(assetType)) + "%");
    
    // Set colors based on stage
    if (currentStage) {
        setForeground(2, QBrush(currentStage->color));
    }
    
    // Mark overdue assets
    if (asset.isOverdue()) {
        setBackground(0, QBrush(QColor(255, 200, 200)));
    }
}

// AssetProgressTrackerDialog Implementation
AssetProgressTrackerDialog::AssetProgressTrackerDialog(QWidget* parent)
    : QDialog(parent)
    , m_modified(false)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
{
    setWindowTitle("Asset Progress Tracker");
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    m_settings = new QSettings("LupineEngine", "AssetProgressTracker", this);
    
    setupUI();
    loadSettings();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(60000);
    connect(m_updateTimer, &QTimer::timeout, this, &AssetProgressTrackerDialog::updateProgress);
    m_updateTimer->start();

    // Initialize with a new project after UI is fully set up
    QTimer::singleShot(0, this, &AssetProgressTrackerDialog::newProject);
}

AssetProgressTrackerDialog::~AssetProgressTrackerDialog() {
    saveSettings();
}

void AssetProgressTrackerDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    setupMenuBar();
    setupToolBar();
    
    // Main tab widget
    m_mainTabWidget = new QTabWidget(this);
    m_mainLayout->addWidget(m_mainTabWidget);
    
    setupAssetTypeManager();
    setupAssetList();
    setupPipelineView();
    setupStatusBar();
    
    // Connect signals
    connect(m_mainTabWidget, &QTabWidget::currentChanged, this, &AssetProgressTrackerDialog::onTabChanged);
}

// Stub implementations for now
void AssetProgressTrackerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setFixedHeight(24);  // Lock menu bar height to 24px
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Project", this, &AssetProgressTrackerDialog::onNewProject, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &AssetProgressTrackerDialog::onOpenProject, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &AssetProgressTrackerDialog::onSaveProject, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &AssetProgressTrackerDialog::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_generateReportAction = fileMenu->addAction("&Generate Report...", this, &AssetProgressTrackerDialog::onGenerateReport);
    m_exportAction = fileMenu->addAction("&Export to CSV...", this, &AssetProgressTrackerDialog::onExportCSV);
    m_importAction = fileMenu->addAction("&Import from CSV...", this, &AssetProgressTrackerDialog::onImportCSV);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close);

    // Asset Type menu
    QMenu* assetTypeMenu = m_menuBar->addMenu("Asset &Type");
    m_addAssetTypeAction = assetTypeMenu->addAction("&Add Asset Type", this, &AssetProgressTrackerDialog::onAddAssetType, QKeySequence("Ctrl+T"));
    m_editAssetTypeAction = assetTypeMenu->addAction("&Edit Asset Type", this, &AssetProgressTrackerDialog::onEditAssetType, QKeySequence("F3"));
    m_deleteAssetTypeAction = assetTypeMenu->addAction("&Delete Asset Type", this, &AssetProgressTrackerDialog::onDeleteAssetType, QKeySequence("Shift+Delete"));
    m_duplicateAssetTypeAction = assetTypeMenu->addAction("D&uplicate Asset Type", this, &AssetProgressTrackerDialog::onDuplicateAssetType, QKeySequence("Ctrl+Shift+T"));

    // Asset menu
    QMenu* assetMenu = m_menuBar->addMenu("&Asset");
    m_addAssetAction = assetMenu->addAction("&Add Asset", this, &AssetProgressTrackerDialog::onAddAsset, QKeySequence("Ctrl+N"));
    m_editAssetAction = assetMenu->addAction("&Edit Asset", this, &AssetProgressTrackerDialog::onEditAsset, QKeySequence("F2"));
    m_deleteAssetAction = assetMenu->addAction("&Delete Asset", this, &AssetProgressTrackerDialog::onDeleteAsset, QKeySequence::Delete);
    m_duplicateAssetAction = assetMenu->addAction("D&uplicate Asset", this, &AssetProgressTrackerDialog::onDuplicateAsset, QKeySequence("Ctrl+D"));
    assetMenu->addSeparator();
    m_nextStageAction = assetMenu->addAction("Move to &Next Stage", this, &AssetProgressTrackerDialog::onMoveToNextStage, QKeySequence("Ctrl+Right"));
    m_previousStageAction = assetMenu->addAction("Move to &Previous Stage", this, &AssetProgressTrackerDialog::onMoveToPreviousStage, QKeySequence("Ctrl+Left"));
    m_setStageAction = assetMenu->addAction("&Set Stage...", this, &AssetProgressTrackerDialog::onSetStage, QKeySequence("Ctrl+S"));

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_pipelineViewAction = viewMenu->addAction("&Pipeline View", this, &AssetProgressTrackerDialog::onShowPipeline);
}

void AssetProgressTrackerDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(26);  // Reduce toolbar height to 26px
    m_toolBar->setIconSize(QSize(16, 16));  // Smaller icons for compact layout
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Asset type operations
    m_toolBar->addAction(m_addAssetTypeAction);
    m_toolBar->addAction(m_editAssetTypeAction);
    m_toolBar->addAction(m_deleteAssetTypeAction);
    m_toolBar->addSeparator();

    // Asset operations
    m_toolBar->addAction(m_addAssetAction);
    m_toolBar->addAction(m_editAssetAction);
    m_toolBar->addAction(m_deleteAssetAction);
    m_toolBar->addSeparator();

    // Stage operations
    m_toolBar->addAction(m_previousStageAction);
    m_toolBar->addAction(m_nextStageAction);
}

void AssetProgressTrackerDialog::setupAssetTypeManager() {
    m_assetTypesTab = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(m_assetTypesTab);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Create splitter for asset types list and details
    m_assetTypesSplitter = new QSplitter(Qt::Horizontal, m_assetTypesTab);
    mainLayout->addWidget(m_assetTypesSplitter);

    // Left panel - Asset Types List
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Asset types list header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerLabel = new QLabel("Asset Types");
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    // Asset type management buttons
    QPushButton* addAssetTypeBtn = new QPushButton("Add");
    addAssetTypeBtn->setMaximumWidth(60);
    QPushButton* deleteAssetTypeBtn = new QPushButton("Delete");
    deleteAssetTypeBtn->setMaximumWidth(60);
    QPushButton* duplicateAssetTypeBtn = new QPushButton("Duplicate");
    duplicateAssetTypeBtn->setMaximumWidth(80);

    headerLayout->addWidget(addAssetTypeBtn);
    headerLayout->addWidget(deleteAssetTypeBtn);
    headerLayout->addWidget(duplicateAssetTypeBtn);

    leftLayout->addLayout(headerLayout);

    // Asset types list
    m_assetTypesList = new QListWidget();
    m_assetTypesList->setMinimumWidth(250);
    leftLayout->addWidget(m_assetTypesList);

    m_assetTypesSplitter->addWidget(leftPanel);

    // Right panel - Asset Type Details
    m_assetTypeDetailsWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(m_assetTypeDetailsWidget);
    rightLayout->setContentsMargins(10, 0, 0, 0);

    // Asset type details header
    QLabel* detailsHeader = new QLabel("Asset Type Details");
    detailsHeader->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(detailsHeader);

    // Asset type name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    m_assetTypeNameEdit = new QLineEdit();
    nameLayout->addWidget(m_assetTypeNameEdit);
    rightLayout->addLayout(nameLayout);

    // Asset type description
    rightLayout->addWidget(new QLabel("Description:"));
    m_assetTypeDescriptionEdit = new QTextEdit();
    m_assetTypeDescriptionEdit->setMaximumHeight(80);
    rightLayout->addWidget(m_assetTypeDescriptionEdit);

    // Stages section
    QLabel* stagesHeader = new QLabel("Production Stages");
    stagesHeader->setStyleSheet("font-weight: bold; margin-top: 10px;");
    rightLayout->addWidget(stagesHeader);

    // Stages list and controls
    QHBoxLayout* stagesLayout = new QHBoxLayout();

    // Stages list
    m_stagesList = new QListWidget();
    m_stagesList->setMinimumHeight(200);
    stagesLayout->addWidget(m_stagesList);

    // Stage control buttons
    QVBoxLayout* stageButtonsLayout = new QVBoxLayout();
    m_addStageButton = new QPushButton("Add Stage");
    m_editStageButton = new QPushButton("Edit Stage");
    m_deleteStageButton = new QPushButton("Delete Stage");
    m_moveStageUpButton = new QPushButton("Move Up");
    m_moveStageDownButton = new QPushButton("Move Down");

    stageButtonsLayout->addWidget(m_addStageButton);
    stageButtonsLayout->addWidget(m_editStageButton);
    stageButtonsLayout->addWidget(m_deleteStageButton);
    stageButtonsLayout->addSpacing(10);
    stageButtonsLayout->addWidget(m_moveStageUpButton);
    stageButtonsLayout->addWidget(m_moveStageDownButton);
    stageButtonsLayout->addStretch();

    stagesLayout->addLayout(stageButtonsLayout);
    rightLayout->addLayout(stagesLayout);

    rightLayout->addStretch();

    m_assetTypesSplitter->addWidget(m_assetTypeDetailsWidget);

    // Set splitter proportions
    m_assetTypesSplitter->setSizes({300, 500});

    // Connect signals
    connect(addAssetTypeBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onAddAssetType);
    connect(deleteAssetTypeBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onDeleteAssetType);
    connect(duplicateAssetTypeBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onDuplicateAssetType);
    connect(m_assetTypesList, &QListWidget::currentRowChanged, this, &AssetProgressTrackerDialog::onAssetTypeChanged);
    connect(m_assetTypeNameEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        int currentRow = m_assetTypesList->currentRow();
        if (currentRow >= 0 && currentRow < m_assetTypes.size()) {
            m_assetTypes[currentRow].name = text;
            m_assetTypesList->item(currentRow)->setText(text);
            setModified(true);
        }
    });
    connect(m_assetTypeDescriptionEdit, &QTextEdit::textChanged, this, [this]() {
        int currentRow = m_assetTypesList->currentRow();
        if (currentRow >= 0 && currentRow < m_assetTypes.size()) {
            m_assetTypes[currentRow].description = m_assetTypeDescriptionEdit->toPlainText();
            setModified(true);
        }
    });

    // Stage button connections
    connect(m_addStageButton, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onAddStage);
    connect(m_editStageButton, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onEditStage);
    connect(m_deleteStageButton, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onDeleteStage);
    connect(m_moveStageUpButton, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onMoveStageUp);
    connect(m_moveStageDownButton, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onMoveStageDown);
    connect(m_stagesList, &QListWidget::currentRowChanged, this, &AssetProgressTrackerDialog::onStageSelectionChanged);

    m_mainTabWidget->addTab(m_assetTypesTab, "Asset Types");
}

void AssetProgressTrackerDialog::setupAssetList() {
    m_assetsTab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(m_assetsTab);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Create filter widget
    m_filterWidget = new QWidget();
    m_filterWidget->setFixedHeight(50);  // Lock filter bar height to 50px
    QHBoxLayout* filterLayout = new QHBoxLayout(m_filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);

    // Filter controls
    filterLayout->addWidget(new QLabel("Type:"));
    m_assetTypeFilter = new QComboBox();
    m_assetTypeFilter->addItem("All Types");
    filterLayout->addWidget(m_assetTypeFilter);

    filterLayout->addWidget(new QLabel("Stage:"));
    m_stageFilter = new QComboBox();
    m_stageFilter->addItem("All Stages");
    filterLayout->addWidget(m_stageFilter);

    filterLayout->addWidget(new QLabel("Assignee:"));
    m_assigneeFilter = new QComboBox();
    m_assigneeFilter->addItem("All Assignees");
    filterLayout->addWidget(m_assigneeFilter);

    filterLayout->addWidget(new QLabel("Search:"));
    m_searchFilter = new QLineEdit();
    m_searchFilter->setPlaceholderText("Search assets...");
    filterLayout->addWidget(m_searchFilter);

    m_showCompletedCheck = new QCheckBox("Show Completed");
    m_showCompletedCheck->setChecked(true);
    filterLayout->addWidget(m_showCompletedCheck);

    filterLayout->addWidget(new QLabel("Sort:"));
    m_sortCombo = new QComboBox();
    m_sortCombo->addItems({"Name", "Type", "Stage", "Priority", "Target Date", "Progress"});
    filterLayout->addWidget(m_sortCombo);

    filterLayout->addStretch();

    // Asset management buttons
    QPushButton* addAssetBtn = new QPushButton("Add Asset");
    QPushButton* editAssetBtn = new QPushButton("Edit Asset");
    QPushButton* deleteAssetBtn = new QPushButton("Delete Asset");
    QPushButton* duplicateAssetBtn = new QPushButton("Duplicate");

    filterLayout->addWidget(addAssetBtn);
    filterLayout->addWidget(editAssetBtn);
    filterLayout->addWidget(deleteAssetBtn);
    filterLayout->addWidget(duplicateAssetBtn);

    mainLayout->addWidget(m_filterWidget);

    // Create splitter for asset list and details
    m_assetsSplitter = new QSplitter(Qt::Horizontal, m_assetsTab);
    mainLayout->addWidget(m_assetsSplitter);

    // Asset tree widget
    m_assetTree = new QTreeWidget();
    m_assetTree->setHeaderLabels({"Name", "Type", "Stage", "Assignee", "Target Date", "Priority", "Progress"});
    m_assetTree->setMinimumWidth(600);
    m_assetTree->setAlternatingRowColors(true);
    m_assetTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assetTree->setSortingEnabled(true);

    // Set column widths
    m_assetTree->setColumnWidth(0, 150); // Name
    m_assetTree->setColumnWidth(1, 120); // Type
    m_assetTree->setColumnWidth(2, 100); // Stage
    m_assetTree->setColumnWidth(3, 100); // Assignee
    m_assetTree->setColumnWidth(4, 100); // Target Date
    m_assetTree->setColumnWidth(5, 80);  // Priority
    m_assetTree->setColumnWidth(6, 80);  // Progress

    m_assetsSplitter->addWidget(m_assetTree);

    // Asset details widget
    setupAssetDetails();
    m_assetsSplitter->addWidget(m_assetDetailsWidget);

    // Set splitter proportions
    m_assetsSplitter->setSizes({600, 400});

    // Connect signals
    connect(addAssetBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onAddAsset);
    connect(editAssetBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onEditAsset);
    connect(deleteAssetBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onDeleteAsset);
    connect(duplicateAssetBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::onDuplicateAsset);

    connect(m_assetTree, &QTreeWidget::currentItemChanged, this, &AssetProgressTrackerDialog::onAssetSelectionChanged);
    connect(m_assetTree, &QTreeWidget::itemDoubleClicked, this, &AssetProgressTrackerDialog::onAssetDoubleClicked);
    connect(m_assetTree, &QTreeWidget::itemChanged, this, &AssetProgressTrackerDialog::onAssetItemChanged);

    // Filter connections
    connect(m_assetTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetProgressTrackerDialog::onFilterChanged);
    connect(m_stageFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetProgressTrackerDialog::onFilterChanged);
    connect(m_assigneeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetProgressTrackerDialog::onFilterChanged);
    connect(m_searchFilter, &QLineEdit::textChanged, this, &AssetProgressTrackerDialog::onFilterChanged);
    connect(m_showCompletedCheck, &QCheckBox::toggled, this, &AssetProgressTrackerDialog::onShowCompletedToggled);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetProgressTrackerDialog::onSortChanged);

    m_mainTabWidget->addTab(m_assetsTab, "Assets");
}

void AssetProgressTrackerDialog::setupAssetDetails() {
    m_assetDetailsWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_assetDetailsWidget);
    layout->setContentsMargins(10, 0, 0, 0);

    // Asset details header
    QLabel* detailsHeader = new QLabel("Asset Details");
    detailsHeader->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(detailsHeader);

    // Asset name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    m_assetNameEdit = new QLineEdit();
    nameLayout->addWidget(m_assetNameEdit);
    layout->addLayout(nameLayout);

    // Asset description
    layout->addWidget(new QLabel("Description:"));
    m_assetDescriptionEdit = new QTextEdit();
    m_assetDescriptionEdit->setMaximumHeight(80);
    layout->addWidget(m_assetDescriptionEdit);

    // Asset type and stage
    QHBoxLayout* typeStageLayout = new QHBoxLayout();
    typeStageLayout->addWidget(new QLabel("Type:"));
    m_assetTypeCombo = new QComboBox();
    typeStageLayout->addWidget(m_assetTypeCombo);

    typeStageLayout->addWidget(new QLabel("Stage:"));
    m_currentStageCombo = new QComboBox();
    typeStageLayout->addWidget(m_currentStageCombo);
    layout->addLayout(typeStageLayout);

    // Assignee and priority
    QHBoxLayout* assigneePriorityLayout = new QHBoxLayout();
    assigneePriorityLayout->addWidget(new QLabel("Assignee:"));
    m_assigneeEdit = new QLineEdit();
    assigneePriorityLayout->addWidget(m_assigneeEdit);

    assigneePriorityLayout->addWidget(new QLabel("Priority:"));
    m_prioritySpinBox = new QSpinBox();
    m_prioritySpinBox->setRange(1, 5);
    m_prioritySpinBox->setValue(3);
    assigneePriorityLayout->addWidget(m_prioritySpinBox);
    layout->addLayout(assigneePriorityLayout);

    // Target date
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Target Date:"));
    m_targetDateEdit = new QDateEdit();
    m_targetDateEdit->setCalendarPopup(true);
    m_targetDateEdit->setDate(QDate::currentDate().addDays(30));
    dateLayout->addWidget(m_targetDateEdit);
    layout->addLayout(dateLayout);

    // File path
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel("File Path:"));
    m_filePathEdit = new QLineEdit();
    m_browseFileButton = new QPushButton("Browse...");
    m_browseFileButton->setMaximumWidth(80);
    fileLayout->addWidget(m_filePathEdit);
    fileLayout->addWidget(m_browseFileButton);
    layout->addLayout(fileLayout);

    // Progress bar
    layout->addWidget(new QLabel("Progress:"));
    m_assetProgressBar = new QProgressBar();
    m_assetProgressBar->setRange(0, 100);
    layout->addWidget(m_assetProgressBar);

    // Status labels
    m_createdLabel = new QLabel("Created: -");
    m_updatedLabel = new QLabel("Updated: -");
    m_daysInStageLabel = new QLabel("Days in stage: -");
    layout->addWidget(m_createdLabel);
    layout->addWidget(m_updatedLabel);
    layout->addWidget(m_daysInStageLabel);

    // Notes
    layout->addWidget(new QLabel("Notes:"));
    m_notesEdit = new QTextEdit();
    m_notesEdit->setMaximumHeight(100);
    layout->addWidget(m_notesEdit);

    layout->addStretch();

    // Connect asset detail change signals
    connect(m_assetNameEdit, &QLineEdit::textChanged, this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_assetDescriptionEdit, &QTextEdit::textChanged, this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_assetTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_currentStageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_assigneeEdit, &QLineEdit::textChanged, this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_prioritySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_targetDateEdit, &QDateEdit::dateChanged, this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_filePathEdit, &QLineEdit::textChanged, this, [this]() { updateCurrentAssetFromUI(); });
    connect(m_notesEdit, &QTextEdit::textChanged, this, [this]() { updateCurrentAssetFromUI(); });

    connect(m_browseFileButton, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Asset File", "", "All Files (*.*)");
        if (!fileName.isEmpty()) {
            m_filePathEdit->setText(fileName);
        }
    });
}

void AssetProgressTrackerDialog::setupPipelineView() {
    m_pipelineTab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(m_pipelineTab);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Pipeline view header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerLabel = new QLabel("Asset Pipeline View");
    headerLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    // Pipeline view controls
    QPushButton* refreshBtn = new QPushButton("Refresh");
    QPushButton* exportBtn = new QPushButton("Export View");
    QComboBox* assetTypeFilterCombo = new QComboBox();
    assetTypeFilterCombo->addItem("All Asset Types");

    headerLayout->addWidget(new QLabel("Filter:"));
    headerLayout->addWidget(assetTypeFilterCombo);
    headerLayout->addWidget(refreshBtn);
    headerLayout->addWidget(exportBtn);

    mainLayout->addLayout(headerLayout);

    // Create scroll area for pipeline view
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_pipelineViewWidget = new QWidget();
    m_pipelineViewWidget->setMinimumSize(800, 600);
    scrollArea->setWidget(m_pipelineViewWidget);

    mainLayout->addWidget(scrollArea);

    // Connect signals
    connect(refreshBtn, &QPushButton::clicked, this, &AssetProgressTrackerDialog::updatePipelineView);
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Export Pipeline View", "", "PNG Images (*.png)");
        if (!fileName.isEmpty()) {
            exportPipelineView(fileName);
        }
    });
    connect(assetTypeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetProgressTrackerDialog::updatePipelineView);

    m_mainTabWidget->addTab(m_pipelineTab, "Pipeline");
}

void AssetProgressTrackerDialog::setupFilters() { /* TODO */ }
void AssetProgressTrackerDialog::setupStatusBar() {
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel("Status - Coming Soon"));
    
    QWidget* statusWidget = new QWidget();
    statusWidget->setLayout(statusLayout);
    statusWidget->setMaximumHeight(30);
    m_mainLayout->addWidget(statusWidget);
}

// Stub implementations
void AssetProgressTrackerDialog::newProject() {
    m_projectName = "New Project";

    // Clear existing data
    m_assetTypes.clear();
    m_assets.clear();

    // Create a default asset type with basic stages
    AssetType defaultType("Default Asset Type");
    defaultType.description = "Default asset type for general assets";

    // Add basic production stages
    AssetStage conceptStage("Concept");
    conceptStage.description = "Initial concept and planning";
    conceptStage.color = QColor(255, 200, 100);
    conceptStage.order = 0;
    defaultType.stages.append(conceptStage);

    AssetStage productionStage("Production");
    productionStage.description = "Main production work";
    productionStage.color = QColor(100, 200, 255);
    productionStage.order = 1;
    defaultType.stages.append(productionStage);

    AssetStage reviewStage("Review");
    reviewStage.description = "Review and feedback";
    reviewStage.color = QColor(255, 255, 100);
    reviewStage.order = 2;
    defaultType.stages.append(reviewStage);

    AssetStage completeStage("Complete");
    completeStage.description = "Asset completed";
    completeStage.color = QColor(100, 255, 100);
    completeStage.order = 3;
    defaultType.stages.append(completeStage);

    m_assetTypes.append(defaultType);

    updateWindowTitle();
    updateAssetTypeList();
    updateAssetList();
    setModified(false);
}
void AssetProgressTrackerDialog::openProject() { /* TODO */ }
void AssetProgressTrackerDialog::saveProject() { /* TODO */ }
void AssetProgressTrackerDialog::saveProjectAs() { /* TODO */ }
void AssetProgressTrackerDialog::addAssetType() { /* TODO */ }
void AssetProgressTrackerDialog::editAssetType() {
    // Switch to asset types tab and focus on the name field for editing
    m_mainTabWidget->setCurrentIndex(0); // Asset Types tab
    if (m_assetTypesList->currentRow() >= 0) {
        m_assetTypeNameEdit->setFocus();
        m_assetTypeNameEdit->selectAll();
    }
}
void AssetProgressTrackerDialog::deleteAssetType() { /* TODO */ }
void AssetProgressTrackerDialog::duplicateAssetType() { /* TODO */ }
void AssetProgressTrackerDialog::addAsset() { /* TODO */ }
void AssetProgressTrackerDialog::editAsset() { /* TODO */ }
void AssetProgressTrackerDialog::deleteAsset() { /* TODO */ }
void AssetProgressTrackerDialog::duplicateAsset() { /* TODO */ }
void AssetProgressTrackerDialog::moveAssetToNextStage() { /* TODO */ }
void AssetProgressTrackerDialog::moveAssetToPreviousStage() { /* TODO */ }
void AssetProgressTrackerDialog::setAssetStage() { /* TODO */ }
void AssetProgressTrackerDialog::generateReport() { /* TODO */ }
void AssetProgressTrackerDialog::exportToCSV() { /* TODO */ }
void AssetProgressTrackerDialog::importFromCSV() { /* TODO */ }
void AssetProgressTrackerDialog::showPipelineView() { /* TODO */ }

// Slot stubs
// File operation slots
void AssetProgressTrackerDialog::onNewProject() { newProject(); }
void AssetProgressTrackerDialog::onOpenProject() { openProject(); }
void AssetProgressTrackerDialog::onSaveProject() { saveProject(); }
void AssetProgressTrackerDialog::onSaveProjectAs() { saveProjectAs(); }
void AssetProgressTrackerDialog::onGenerateReport() { generateReport(); }
void AssetProgressTrackerDialog::onExportCSV() { exportToCSV(); }
void AssetProgressTrackerDialog::onImportCSV() { importFromCSV(); }
void AssetProgressTrackerDialog::onShowPipeline() { showPipelineView(); }

// Asset type operation slots (these call the actual implementations)
void AssetProgressTrackerDialog::onEditAssetType() { editAssetType(); }

// Other slots
void AssetProgressTrackerDialog::onMoveToNextStage() { moveAssetToNextStage(); }
void AssetProgressTrackerDialog::onMoveToPreviousStage() { moveAssetToPreviousStage(); }
void AssetProgressTrackerDialog::onSetStage() { setAssetStage(); }
void AssetProgressTrackerDialog::onTabChanged(int) { /* TODO */ }
void AssetProgressTrackerDialog::updateProgress() { /* TODO */ }

void AssetProgressTrackerDialog::updateAssetTypeList() {
    if (!m_assetTypesList) return; // UI not initialized yet

    m_assetTypesList->clear();

    for (const AssetType& assetType : m_assetTypes) {
        QListWidgetItem* item = new QListWidgetItem(assetType.name);
        item->setData(Qt::UserRole, assetType.id);
        m_assetTypesList->addItem(item);
    }

    // Update asset details if we have a selection
    if (m_assetTypesList->count() > 0 && m_assetTypesList->currentRow() < 0) {
        m_assetTypesList->setCurrentRow(0);
    }
}

void AssetProgressTrackerDialog::updateAssetList() {
    if (!m_assetTree) return; // UI not initialized yet

    m_assetTree->clear();

    // Update filter combo boxes first (only if UI is initialized)
    if (m_assetTypeFilter) {
        updateFilterComboBoxes();
    }

    // Get filter values (with null checks)
    QString selectedAssetType = m_assetTypeFilter ? m_assetTypeFilter->currentData().toString() : QString();
    QString selectedStage = m_stageFilter ? m_stageFilter->currentData().toString() : QString();
    QString selectedAssignee = m_assigneeFilter ? m_assigneeFilter->currentData().toString() : QString();
    QString searchText = m_searchFilter ? m_searchFilter->text().toLower() : QString();
    bool showCompleted = m_showCompletedCheck ? m_showCompletedCheck->isChecked() : true;

    for (const Asset& asset : m_assets) {
        // Apply filters
        if (!selectedAssetType.isEmpty() && asset.assetTypeId != selectedAssetType) continue;
        if (!selectedStage.isEmpty() && asset.currentStageId != selectedStage) continue;
        if (!selectedAssignee.isEmpty() && asset.assignee != selectedAssignee) continue;
        if (!searchText.isEmpty() && !asset.name.toLower().contains(searchText)) continue;

        // Find asset type for completion check
        AssetType* assetType = nullptr;
        for (AssetType& type : m_assetTypes) {
            if (type.id == asset.assetTypeId) {
                assetType = &type;
                break;
            }
        }

        if (assetType) {
            int completionPercentage = asset.getCompletionPercentage(*assetType);
            if (!showCompleted && completionPercentage >= 100) continue;

            AssetItem* item = new AssetItem(asset, *assetType, m_assetTree);
            item->setData(0, Qt::UserRole, asset.id);
        }
    }

    // Apply sorting (only if UI is initialized)
    if (m_sortCombo) {
        int sortColumn = m_sortCombo->currentIndex();
        m_assetTree->sortItems(sortColumn, Qt::AscendingOrder);
    }
}

void AssetProgressTrackerDialog::updateAssetDetails() {
    int currentRow = m_assetTypesList->currentRow();
    bool hasSelection = currentRow >= 0 && currentRow < m_assetTypes.size();

    // Enable/disable controls based on selection
    m_assetTypeNameEdit->setEnabled(hasSelection);
    m_assetTypeDescriptionEdit->setEnabled(hasSelection);
    m_stagesList->setEnabled(hasSelection);
    m_addStageButton->setEnabled(hasSelection);

    if (hasSelection) {
        const AssetType& assetType = m_assetTypes[currentRow];

        // Block signals to prevent triggering setModified
        m_assetTypeNameEdit->blockSignals(true);
        m_assetTypeDescriptionEdit->blockSignals(true);

        m_assetTypeNameEdit->setText(assetType.name);
        m_assetTypeDescriptionEdit->setPlainText(assetType.description);

        m_assetTypeNameEdit->blockSignals(false);
        m_assetTypeDescriptionEdit->blockSignals(false);

        updateStagesList();
    } else {
        m_assetTypeNameEdit->clear();
        m_assetTypeDescriptionEdit->clear();
        m_stagesList->clear();
    }

    // Update stage button states
    onStageSelectionChanged();
}

void AssetProgressTrackerDialog::updateStagesList() {
    m_stagesList->clear();

    int currentAssetTypeRow = m_assetTypesList->currentRow();
    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size()) {
        const AssetType& assetType = m_assetTypes[currentAssetTypeRow];

        for (const AssetStage& stage : assetType.stages) {
            QListWidgetItem* item = new QListWidgetItem(stage.name);
            item->setData(Qt::UserRole, stage.id);

            // Set background color based on stage color
            item->setBackground(QBrush(stage.color.lighter(180)));
            item->setForeground(QBrush(stage.color.darker(150)));

            m_stagesList->addItem(item);
        }
    }

    // Update stage button states
    onStageSelectionChanged();
}

void AssetProgressTrackerDialog::updatePipelineView() {
    if (!m_pipelineViewWidget) return;

    // Clear existing layout
    QLayout* existingLayout = m_pipelineViewWidget->layout();
    if (existingLayout) {
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete existingLayout;
    }

    // Create new layout for pipeline stages
    QHBoxLayout* pipelineLayout = new QHBoxLayout(m_pipelineViewWidget);
    pipelineLayout->setContentsMargins(10, 10, 10, 10);
    pipelineLayout->setSpacing(20);

    // Group assets by asset type and stage
    QMap<QString, QMap<QString, QList<Asset*>>> assetsByTypeAndStage;

    for (Asset& asset : m_assets) {
        AssetType* assetType = findAssetType(asset.assetTypeId);
        if (assetType) {
            assetsByTypeAndStage[assetType->name][asset.currentStageId].append(&asset);
        }
    }

    // Create pipeline columns for each stage
    QSet<QString> allStageIds;
    QMap<QString, QString> stageIdToName;
    QMap<QString, QColor> stageIdToColor;

    // Collect all unique stages
    for (const AssetType& assetType : m_assetTypes) {
        for (const AssetStage& stage : assetType.stages) {
            allStageIds.insert(stage.id);
            stageIdToName[stage.id] = stage.name;
            stageIdToColor[stage.id] = stage.color;
        }
    }

    // Create columns for each stage
    for (const QString& stageId : allStageIds) {
        QVBoxLayout* stageLayout = new QVBoxLayout();

        // Stage header
        QLabel* stageHeader = new QLabel(stageIdToName[stageId]);
        stageHeader->setStyleSheet(QString("font-weight: bold; font-size: 14px; "
                                          "background-color: %1; color: white; "
                                          "padding: 8px; border-radius: 4px;")
                                  .arg(stageIdToColor[stageId].name()));
        stageHeader->setAlignment(Qt::AlignCenter);
        stageLayout->addWidget(stageHeader);

        // Stage content area
        QScrollArea* stageScrollArea = new QScrollArea();
        stageScrollArea->setWidgetResizable(true);
        stageScrollArea->setMinimumWidth(200);
        stageScrollArea->setMaximumWidth(250);
        stageScrollArea->setMinimumHeight(400);

        QWidget* stageContent = new QWidget();
        QVBoxLayout* stageContentLayout = new QVBoxLayout(stageContent);
        stageContentLayout->setContentsMargins(5, 5, 5, 5);
        stageContentLayout->setSpacing(5);

        // Add assets to this stage
        int assetCount = 0;
        for (const auto& typeEntry : assetsByTypeAndStage) {
            const auto& stageMap = typeEntry;
            if (stageMap.contains(stageId)) {
                for (Asset* asset : stageMap[stageId]) {
                    QWidget* assetCard = createAssetCard(*asset);
                    stageContentLayout->addWidget(assetCard);
                    assetCount++;
                }
            }
        }

        // Add stretch to push cards to top
        stageContentLayout->addStretch();

        // Update stage header with count
        stageHeader->setText(QString("%1 (%2)").arg(stageIdToName[stageId]).arg(assetCount));

        stageScrollArea->setWidget(stageContent);
        stageLayout->addWidget(stageScrollArea);

        // Add stage column to pipeline
        QWidget* stageWidget = new QWidget();
        stageWidget->setLayout(stageLayout);
        pipelineLayout->addWidget(stageWidget);
    }

    pipelineLayout->addStretch();
}

QWidget* AssetProgressTrackerDialog::createAssetCard(const Asset& asset) {
    QWidget* card = new QWidget();
    card->setStyleSheet("QWidget { background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 6px; margin: 2px; }");
    card->setMinimumHeight(80);
    card->setMaximumHeight(120);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(8, 6, 8, 6);
    cardLayout->setSpacing(4);

    // Asset name
    QLabel* nameLabel = new QLabel(asset.name);
    nameLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    nameLabel->setWordWrap(true);
    cardLayout->addWidget(nameLabel);

    // Asset type
    AssetType* assetType = const_cast<AssetProgressTrackerDialog*>(this)->findAssetType(asset.assetTypeId);
    if (assetType) {
        QLabel* typeLabel = new QLabel(assetType->name);
        typeLabel->setStyleSheet("font-size: 10px; color: #666;");
        cardLayout->addWidget(typeLabel);
    }

    // Progress bar
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setMaximumHeight(12);
    progressBar->setRange(0, 100);
    if (assetType) {
        progressBar->setValue(asset.getCompletionPercentage(*assetType));
    }
    cardLayout->addWidget(progressBar);

    // Assignee and priority
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    if (!asset.assignee.isEmpty()) {
        QLabel* assigneeLabel = new QLabel(asset.assignee);
        assigneeLabel->setStyleSheet("font-size: 9px; color: #888;");
        bottomLayout->addWidget(assigneeLabel);
    }

    bottomLayout->addStretch();

    // Priority indicator
    QLabel* priorityLabel = new QLabel(QString("P%1").arg(asset.priority));
    QString priorityColor = asset.priority <= 2 ? "#ff4444" : (asset.priority <= 3 ? "#ffaa00" : "#44aa44");
    priorityLabel->setStyleSheet(QString("font-size: 9px; font-weight: bold; color: %1;").arg(priorityColor));
    bottomLayout->addWidget(priorityLabel);

    cardLayout->addLayout(bottomLayout);

    // Make card clickable
    card->setProperty("assetId", asset.id);
    card->installEventFilter(const_cast<AssetProgressTrackerDialog*>(this));

    // Add overdue indicator
    if (asset.isOverdue()) {
        card->setStyleSheet("QWidget { background-color: #ffe6e6; border: 2px solid #ff6666; border-radius: 6px; margin: 2px; }");
    }

    return card;
}

void AssetProgressTrackerDialog::exportPipelineView(const QString& fileName) {
    if (!m_pipelineViewWidget) return;

    // Create a pixmap of the pipeline view
    QPixmap pixmap(m_pipelineViewWidget->size());
    m_pipelineViewWidget->render(&pixmap);

    // Save the pixmap
    if (!pixmap.save(fileName, "PNG")) {
        QMessageBox::warning(this, "Export Failed", "Failed to export pipeline view to " + fileName);
    } else {
        QMessageBox::information(this, "Export Successful", "Pipeline view exported to " + fileName);
    }
}

void AssetProgressTrackerDialog::updateFilterComboBoxes() {
    // Update asset type filter
    m_assetTypeFilter->clear();
    m_assetTypeFilter->addItem("All Types", "");
    for (const AssetType& assetType : m_assetTypes) {
        m_assetTypeFilter->addItem(assetType.name, assetType.id);
    }

    // Update stage filter
    m_stageFilter->clear();
    m_stageFilter->addItem("All Stages", "");
    QSet<QString> addedStages;
    for (const AssetType& assetType : m_assetTypes) {
        for (const AssetStage& stage : assetType.stages) {
            if (!addedStages.contains(stage.id)) {
                m_stageFilter->addItem(stage.name, stage.id);
                addedStages.insert(stage.id);
            }
        }
    }

    // Update assignee filter
    m_assigneeFilter->clear();
    m_assigneeFilter->addItem("All Assignees", "");
    QSet<QString> assignees;
    for (const Asset& asset : m_assets) {
        if (!asset.assignee.isEmpty()) {
            assignees.insert(asset.assignee);
        }
    }
    for (const QString& assignee : assignees) {
        m_assigneeFilter->addItem(assignee, assignee);
    }
}

void AssetProgressTrackerDialog::updateCurrentAssetFromUI() {
    QTreeWidgetItem* currentItem = m_assetTree->currentItem();
    if (currentItem) {
        QString assetId = currentItem->data(0, Qt::UserRole).toString();
        Asset* asset = findAsset(assetId);

        if (asset) {
            asset->name = m_assetNameEdit->text();
            asset->description = m_assetDescriptionEdit->toPlainText();
            asset->assignee = m_assigneeEdit->text();
            asset->priority = m_prioritySpinBox->value();
            asset->targetDate = QDateTime(m_targetDateEdit->date(), QTime());
            asset->filePath = m_filePathEdit->text();
            asset->notes = m_notesEdit->toPlainText();
            asset->updatedDate = QDateTime::currentDateTime();

            // Update asset type and stage
            QString assetTypeId = m_assetTypeCombo->currentData().toString();
            QString stageId = m_currentStageCombo->currentData().toString();
            if (!assetTypeId.isEmpty()) {
                asset->assetTypeId = assetTypeId;
            }
            if (!stageId.isEmpty()) {
                asset->currentStageId = stageId;
            }

            // Update the tree item
            AssetType* assetType = findAssetType(asset->assetTypeId);
            if (assetType) {
                AssetItem* assetItem = dynamic_cast<AssetItem*>(currentItem);
                if (assetItem) {
                    assetItem->updateFromAsset(*asset, *assetType);
                }
            }

            setModified(true);
        }
    }
}

void AssetProgressTrackerDialog::updateAssetDetailsFromAsset(const Asset& asset) {
    // Block signals to prevent triggering updateCurrentAssetFromUI
    m_assetNameEdit->blockSignals(true);
    m_assetDescriptionEdit->blockSignals(true);
    m_assigneeEdit->blockSignals(true);
    m_prioritySpinBox->blockSignals(true);
    m_targetDateEdit->blockSignals(true);
    m_filePathEdit->blockSignals(true);
    m_notesEdit->blockSignals(true);
    m_assetTypeCombo->blockSignals(true);
    m_currentStageCombo->blockSignals(true);

    // Update UI controls
    m_assetNameEdit->setText(asset.name);
    m_assetDescriptionEdit->setPlainText(asset.description);
    m_assigneeEdit->setText(asset.assignee);
    m_prioritySpinBox->setValue(asset.priority);
    m_targetDateEdit->setDate(asset.targetDate.date());
    m_filePathEdit->setText(asset.filePath);
    m_notesEdit->setPlainText(asset.notes);

    // Update asset type combo
    updateAssetTypeCombo();
    for (int i = 0; i < m_assetTypeCombo->count(); ++i) {
        if (m_assetTypeCombo->itemData(i).toString() == asset.assetTypeId) {
            m_assetTypeCombo->setCurrentIndex(i);
            break;
        }
    }

    // Update stage combo based on selected asset type
    updateStageCombo(asset.assetTypeId);
    for (int i = 0; i < m_currentStageCombo->count(); ++i) {
        if (m_currentStageCombo->itemData(i).toString() == asset.currentStageId) {
            m_currentStageCombo->setCurrentIndex(i);
            break;
        }
    }

    // Update progress bar
    AssetType* assetType = findAssetType(asset.assetTypeId);
    if (assetType) {
        int progress = asset.getCompletionPercentage(*assetType);
        m_assetProgressBar->setValue(progress);
    }

    // Update status labels
    m_createdLabel->setText("Created: " + asset.createdDate.toString("yyyy-MM-dd hh:mm"));
    m_updatedLabel->setText("Updated: " + asset.updatedDate.toString("yyyy-MM-dd hh:mm"));
    m_daysInStageLabel->setText("Days in stage: " + QString::number(asset.getDaysInCurrentStage()));

    // Restore signals
    m_assetNameEdit->blockSignals(false);
    m_assetDescriptionEdit->blockSignals(false);
    m_assigneeEdit->blockSignals(false);
    m_prioritySpinBox->blockSignals(false);
    m_targetDateEdit->blockSignals(false);
    m_filePathEdit->blockSignals(false);
    m_notesEdit->blockSignals(false);
    m_assetTypeCombo->blockSignals(false);
    m_currentStageCombo->blockSignals(false);
}

void AssetProgressTrackerDialog::clearAssetDetails() {
    m_assetNameEdit->clear();
    m_assetDescriptionEdit->clear();
    m_assigneeEdit->clear();
    m_prioritySpinBox->setValue(3);
    m_targetDateEdit->setDate(QDate::currentDate().addDays(30));
    m_filePathEdit->clear();
    m_notesEdit->clear();
    m_assetProgressBar->setValue(0);
    m_createdLabel->setText("Created: -");
    m_updatedLabel->setText("Updated: -");
    m_daysInStageLabel->setText("Days in stage: -");
}

void AssetProgressTrackerDialog::updateAssetTypeCombo() {
    m_assetTypeCombo->clear();
    for (const AssetType& assetType : m_assetTypes) {
        m_assetTypeCombo->addItem(assetType.name, assetType.id);
    }
}

void AssetProgressTrackerDialog::updateStageCombo(const QString& assetTypeId) {
    m_currentStageCombo->clear();

    AssetType* assetType = findAssetType(assetTypeId);
    if (assetType) {
        for (const AssetStage& stage : assetType->stages) {
            m_currentStageCombo->addItem(stage.name, stage.id);
        }
    }
}

void AssetProgressTrackerDialog::updateWindowTitle() { setWindowTitle("Asset Progress Tracker - " + m_projectName); }
void AssetProgressTrackerDialog::updateStatistics() {
    int totalAssets = m_assets.size();
    int completedAssets = 0;
    int inProgressAssets = 0;
    int overdueAssets = 0;

    for (const Asset& asset : m_assets) {
        AssetType* assetType = const_cast<AssetProgressTrackerDialog*>(this)->findAssetType(asset.assetTypeId);
        if (assetType) {
            int completion = asset.getCompletionPercentage(*assetType);
            if (completion >= 100) {
                completedAssets++;
            } else if (completion > 0) {
                inProgressAssets++;
            }
        }

        if (asset.isOverdue()) {
            overdueAssets++;
        }
    }

    // Update window title with statistics
    QString statsText = QString("Asset Progress Tracker - %1 total, %2 completed, %3 in progress")
        .arg(totalAssets).arg(completedAssets).arg(inProgressAssets);
    if (overdueAssets > 0) {
        statsText += QString(", %1 overdue").arg(overdueAssets);
    }
    setWindowTitle(statsText);
}
bool AssetProgressTrackerDialog::hasUnsavedChanges() const { return m_modified; }
bool AssetProgressTrackerDialog::promptSaveChanges() { return true; }
void AssetProgressTrackerDialog::setModified(bool modified) { m_modified = modified; }
AssetType* AssetProgressTrackerDialog::findAssetType(const QString& id) {
    for (AssetType& assetType : m_assetTypes) {
        if (assetType.id == id) {
            return &assetType;
        }
    }
    return nullptr;
}

Asset* AssetProgressTrackerDialog::findAsset(const QString& id) {
    for (Asset& asset : m_assets) {
        if (asset.id == id) {
            return &asset;
        }
    }
    return nullptr;
}

AssetItem* AssetProgressTrackerDialog::findAssetItem(const QString& id) {
    for (int i = 0; i < m_assetTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_assetTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == id) {
            return dynamic_cast<AssetItem*>(item);
        }
    }
    return nullptr;
}
void AssetProgressTrackerDialog::addAssetToTree(const Asset&) { /* TODO */ }
void AssetProgressTrackerDialog::removeAssetFromTree(const QString&) { /* TODO */ }
void AssetProgressTrackerDialog::loadSettings() { /* TODO */ }
void AssetProgressTrackerDialog::saveSettings() { /* TODO */ }
bool AssetProgressTrackerDialog::loadFromFile(const QString&) { return false; }
bool AssetProgressTrackerDialog::saveToFile(const QString&) { return false; }

void AssetProgressTrackerDialog::closeEvent(QCloseEvent* event) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

// Asset Type Management Slots
void AssetProgressTrackerDialog::onAddAssetType() {
    AssetType newAssetType("New Asset Type");
    m_assetTypes.append(newAssetType);
    updateAssetTypeList();

    // Select the new asset type
    m_assetTypesList->setCurrentRow(m_assetTypes.size() - 1);
    setModified(true);
}

void AssetProgressTrackerDialog::onDeleteAssetType() {
    int currentRow = m_assetTypesList->currentRow();
    if (currentRow >= 0 && currentRow < m_assetTypes.size()) {
        // Confirm deletion
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Delete Asset Type",
            QString("Are you sure you want to delete the asset type '%1'?").arg(m_assetTypes[currentRow].name),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_assetTypes.removeAt(currentRow);
            updateAssetTypeList();
            setModified(true);
        }
    }
}

void AssetProgressTrackerDialog::onDuplicateAssetType() {
    int currentRow = m_assetTypesList->currentRow();
    if (currentRow >= 0 && currentRow < m_assetTypes.size()) {
        AssetType duplicated = m_assetTypes[currentRow];
        duplicated.id = QUuid::createUuid().toString();
        duplicated.name += " (Copy)";

        m_assetTypes.append(duplicated);
        updateAssetTypeList();

        // Select the duplicated asset type
        m_assetTypesList->setCurrentRow(m_assetTypes.size() - 1);
        setModified(true);
    }
}

void AssetProgressTrackerDialog::onAssetTypeChanged() {
    updateAssetDetails();
}

// Stage Management Slots
void AssetProgressTrackerDialog::onAddStage() {
    int currentAssetTypeRow = m_assetTypesList->currentRow();
    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size()) {
        AssetStage newStage("New Stage");
        newStage.order = m_assetTypes[currentAssetTypeRow].stages.size();

        m_assetTypes[currentAssetTypeRow].stages.append(newStage);
        updateStagesList();

        // Select the new stage
        m_stagesList->setCurrentRow(m_assetTypes[currentAssetTypeRow].stages.size() - 1);
        setModified(true);
    }
}

void AssetProgressTrackerDialog::onEditStage() {
    int currentAssetTypeRow = m_assetTypesList->currentRow();
    int currentStageRow = m_stagesList->currentRow();

    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size() &&
        currentStageRow >= 0 && currentStageRow < m_assetTypes[currentAssetTypeRow].stages.size()) {

        AssetStage& stage = m_assetTypes[currentAssetTypeRow].stages[currentStageRow];

        // Simple edit dialog for now
        bool ok;
        QString newName = QInputDialog::getText(this, "Edit Stage", "Stage name:", QLineEdit::Normal, stage.name, &ok);
        if (ok && !newName.isEmpty()) {
            stage.name = newName;
            updateStagesList();
            setModified(true);
        }
    }
}

void AssetProgressTrackerDialog::onDeleteStage() {
    int currentAssetTypeRow = m_assetTypesList->currentRow();
    int currentStageRow = m_stagesList->currentRow();

    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size() &&
        currentStageRow >= 0 && currentStageRow < m_assetTypes[currentAssetTypeRow].stages.size()) {

        AssetStage& stage = m_assetTypes[currentAssetTypeRow].stages[currentStageRow];

        // Confirm deletion
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Delete Stage",
            QString("Are you sure you want to delete the stage '%1'?").arg(stage.name),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_assetTypes[currentAssetTypeRow].stages.removeAt(currentStageRow);
            updateStagesList();
            setModified(true);
        }
    }
}

void AssetProgressTrackerDialog::onMoveStageUp() {
    int currentAssetTypeRow = m_assetTypesList->currentRow();
    int currentStageRow = m_stagesList->currentRow();

    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size() &&
        currentStageRow > 0 && currentStageRow < m_assetTypes[currentAssetTypeRow].stages.size()) {

        // Swap stages
        m_assetTypes[currentAssetTypeRow].stages.swapItemsAt(currentStageRow, currentStageRow - 1);

        // Update order values
        for (int i = 0; i < m_assetTypes[currentAssetTypeRow].stages.size(); ++i) {
            m_assetTypes[currentAssetTypeRow].stages[i].order = i;
        }

        updateStagesList();
        m_stagesList->setCurrentRow(currentStageRow - 1);
        setModified(true);
    }
}

void AssetProgressTrackerDialog::onMoveStageDown() {
    int currentAssetTypeRow = m_assetTypesList->currentRow();
    int currentStageRow = m_stagesList->currentRow();

    if (currentAssetTypeRow >= 0 && currentAssetTypeRow < m_assetTypes.size() &&
        currentStageRow >= 0 && currentStageRow < m_assetTypes[currentAssetTypeRow].stages.size() - 1) {

        // Swap stages
        m_assetTypes[currentAssetTypeRow].stages.swapItemsAt(currentStageRow, currentStageRow + 1);

        // Update order values
        for (int i = 0; i < m_assetTypes[currentAssetTypeRow].stages.size(); ++i) {
            m_assetTypes[currentAssetTypeRow].stages[i].order = i;
        }

        updateStagesList();
        m_stagesList->setCurrentRow(currentStageRow + 1);
        setModified(true);
    }
}

void AssetProgressTrackerDialog::onStageSelectionChanged() {
    int currentStageRow = m_stagesList->currentRow();
    bool hasSelection = currentStageRow >= 0;

    m_editStageButton->setEnabled(hasSelection);
    m_deleteStageButton->setEnabled(hasSelection);
    m_moveStageUpButton->setEnabled(hasSelection && currentStageRow > 0);
    m_moveStageDownButton->setEnabled(hasSelection && currentStageRow < m_stagesList->count() - 1);
}

// Asset Management Slots
void AssetProgressTrackerDialog::onAddAsset() {
    if (m_assetTypes.isEmpty()) {
        QMessageBox::information(this, "No Asset Types", "Please create at least one asset type before adding assets.");
        return;
    }

    // Find the first asset type with stages
    AssetType* validAssetType = nullptr;
    for (AssetType& assetType : m_assetTypes) {
        if (!assetType.stages.isEmpty()) {
            validAssetType = &assetType;
            break;
        }
    }

    if (!validAssetType) {
        QMessageBox::information(this, "No Stages Available",
            "The asset type has no stages defined. Please add stages to the asset type first.");
        return;
    }

    Asset newAsset("New Asset", validAssetType->id);
    newAsset.currentStageId = validAssetType->stages.first().id;

    m_assets.append(newAsset);
    updateAssetList();
    setModified(true);
    updateStatistics();

    // Select the new asset
    for (int i = 0; i < m_assetTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_assetTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == newAsset.id) {
            m_assetTree->setCurrentItem(item);
            break;
        }
    }
}

void AssetProgressTrackerDialog::onEditAsset() {
    QTreeWidgetItem* currentItem = m_assetTree->currentItem();
    if (currentItem) {
        // Asset editing is handled through the details panel
        // This could open a dedicated edit dialog in the future
    }
}

void AssetProgressTrackerDialog::onDeleteAsset() {
    QTreeWidgetItem* currentItem = m_assetTree->currentItem();
    if (currentItem) {
        QString assetId = currentItem->data(0, Qt::UserRole).toString();
        Asset* asset = findAsset(assetId);

        if (asset) {
            QMessageBox::StandardButton reply = QMessageBox::question(this,
                "Delete Asset",
                QString("Are you sure you want to delete the asset '%1'?").arg(asset->name),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                for (int i = 0; i < m_assets.size(); ++i) {
                    if (m_assets[i].id == assetId) {
                        m_assets.removeAt(i);
                        break;
                    }
                }
                updateAssetList();
                setModified(true);
            }
        }
    }
}

void AssetProgressTrackerDialog::onDuplicateAsset() {
    QTreeWidgetItem* currentItem = m_assetTree->currentItem();
    if (currentItem) {
        QString assetId = currentItem->data(0, Qt::UserRole).toString();
        Asset* asset = findAsset(assetId);

        if (asset) {
            Asset duplicated = *asset;
            duplicated.id = QUuid::createUuid().toString();
            duplicated.name += " (Copy)";
            duplicated.createdDate = QDateTime::currentDateTime();
            duplicated.updatedDate = QDateTime::currentDateTime();

            m_assets.append(duplicated);
            updateAssetList();
            setModified(true);
        }
    }
}

void AssetProgressTrackerDialog::onAssetSelectionChanged() {
    QTreeWidgetItem* currentItem = m_assetTree->currentItem();
    if (currentItem) {
        QString assetId = currentItem->data(0, Qt::UserRole).toString();
        Asset* asset = findAsset(assetId);

        if (asset) {
            updateAssetDetailsFromAsset(*asset);
        }
    } else {
        clearAssetDetails();
    }
}

void AssetProgressTrackerDialog::onAssetDoubleClicked(QTreeWidgetItem* item, int column) {
    if (item && column == 0) { // Only allow renaming the name column
        m_assetTree->editItem(item, 0);
    }
}

void AssetProgressTrackerDialog::onAssetItemChanged(QTreeWidgetItem* item, int column) {
    if (column == 0) { // Name changed
        QString assetId = item->data(0, Qt::UserRole).toString();
        Asset* asset = findAsset(assetId);
        if (asset) {
            asset->name = item->text(0);
            setModified(true);
            updateAssetDetails(); // Refresh the details panel
        }
    }
}

void AssetProgressTrackerDialog::onFilterChanged() {
    updateAssetList();
}

void AssetProgressTrackerDialog::onSortChanged() {
    updateAssetList();
}

void AssetProgressTrackerDialog::onShowCompletedToggled(bool show) {
    Q_UNUSED(show)
    updateAssetList();
}


