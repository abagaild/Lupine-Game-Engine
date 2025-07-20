#include "MilestoneTrackerDialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QHeaderView>
#include <QUuid>

// Milestone Implementation
Milestone::Milestone()
    : id(QUuid::createUuid().toString())
    , title("New Milestone")
    , status(MilestoneStatus::NotStarted)
    , priority(MilestonePriority::Normal)
    , createdDate(QDateTime::currentDateTime())
    , progress(0)
    , budget(0.0)
    , actualCost(0.0)
{
}

Milestone::Milestone(const QString& title, const QString& description)
    : id(QUuid::createUuid().toString())
    , title(title)
    , description(description)
    , status(MilestoneStatus::NotStarted)
    , priority(MilestonePriority::Normal)
    , createdDate(QDateTime::currentDateTime())
    , progress(0)
    , budget(0.0)
    , actualCost(0.0)
{
}

QJsonObject Milestone::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["description"] = description;
    obj["status"] = static_cast<int>(status);
    obj["priority"] = static_cast<int>(priority);
    obj["createdDate"] = createdDate.toString(Qt::ISODate);
    obj["startDate"] = startDate.toString(Qt::ISODate);
    obj["targetDate"] = targetDate.toString(Qt::ISODate);
    obj["completedDate"] = completedDate.toString(Qt::ISODate);
    obj["owner"] = owner;
    obj["progress"] = progress;
    obj["budget"] = budget;
    obj["actualCost"] = actualCost;
    return obj;
}

void Milestone::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    title = json["title"].toString();
    description = json["description"].toString();
    status = static_cast<MilestoneStatus>(json["status"].toInt());
    priority = static_cast<MilestonePriority>(json["priority"].toInt());
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    startDate = QDateTime::fromString(json["startDate"].toString(), Qt::ISODate);
    targetDate = QDateTime::fromString(json["targetDate"].toString(), Qt::ISODate);
    completedDate = QDateTime::fromString(json["completedDate"].toString(), Qt::ISODate);
    owner = json["owner"].toString();
    progress = json["progress"].toInt();
    budget = json["budget"].toDouble();
    actualCost = json["actualCost"].toDouble();
}

QString Milestone::getStatusString() const {
    switch (status) {
        case MilestoneStatus::NotStarted: return "Not Started";
        case MilestoneStatus::InProgress: return "In Progress";
        case MilestoneStatus::Completed: return "Completed";
        case MilestoneStatus::Delayed: return "Delayed";
        case MilestoneStatus::Cancelled: return "Cancelled";
        default: return "Not Started";
    }
}

QString Milestone::getPriorityString() const {
    switch (priority) {
        case MilestonePriority::Low: return "Low";
        case MilestonePriority::Normal: return "Normal";
        case MilestonePriority::High: return "High";
        case MilestonePriority::Critical: return "Critical";
        default: return "Normal";
    }
}

QColor Milestone::getStatusColor() const {
    switch (status) {
        case MilestoneStatus::NotStarted: return QColor(128, 128, 128);
        case MilestoneStatus::InProgress: return QColor(255, 165, 0);
        case MilestoneStatus::Completed: return QColor(0, 128, 0);
        case MilestoneStatus::Delayed: return QColor(255, 0, 0);
        case MilestoneStatus::Cancelled: return QColor(128, 0, 128);
        default: return QColor(128, 128, 128);
    }
}

bool Milestone::isOverdue() const {
    return targetDate.isValid() && targetDate < QDateTime::currentDateTime() && status != MilestoneStatus::Completed;
}

int Milestone::getDaysRemaining() const {
    if (!targetDate.isValid()) return -1;
    return QDateTime::currentDateTime().daysTo(targetDate);
}

double Milestone::getBudgetVariance() const {
    if (budget <= 0.0) return 0.0;
    return ((actualCost - budget) / budget) * 100.0;
}

// MilestoneItem Implementation
MilestoneItem::MilestoneItem(const Milestone& milestone, QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , m_milestone(milestone)
{
    setupItem();
}

void MilestoneItem::setupItem() {
    updateFromMilestone(m_milestone);
}

void MilestoneItem::updateFromMilestone(const Milestone& milestone) {
    m_milestone = milestone;
    setText(0, milestone.title);
    setText(1, milestone.getPriorityString());
    setText(2, milestone.getStatusString());
    setText(3, milestone.targetDate.isValid() ? milestone.targetDate.toString("yyyy-MM-dd") : "");
    setText(4, milestone.owner);
    setText(5, QString::number(milestone.progress) + "%");
    
    QColor statusColor = milestone.getStatusColor();
    for (int i = 0; i < columnCount(); ++i) {
        setForeground(i, QBrush(statusColor));
    }
    
    if (milestone.isOverdue()) {
        setBackground(0, QBrush(QColor(255, 200, 200)));
    }
}

// MilestoneTrackerDialog Implementation
MilestoneTrackerDialog::MilestoneTrackerDialog(QWidget* parent)
    : QDialog(parent)
    , m_modified(false)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
{
    setWindowTitle("Milestone Tracker");
    setMinimumSize(1000, 700);
    resize(1400, 900);
    
    m_settings = new QSettings("LupineEngine", "MilestoneTracker", this);
    
    setupUI();
    loadSettings();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(60000);
    connect(m_updateTimer, &QTimer::timeout, this, &MilestoneTrackerDialog::updateProgress);
    m_updateTimer->start();

    // Initialize with a new project after UI is fully set up
    QTimer::singleShot(0, this, &MilestoneTrackerDialog::newProject);
}

MilestoneTrackerDialog::~MilestoneTrackerDialog() {
    saveSettings();
}

void MilestoneTrackerDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();

    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    // Right splitter for details and timeline
    m_rightSplitter = new QSplitter(Qt::Vertical, this);

    setupMilestoneList();
    setupMilestoneDetails();
    setupTimelineView();
    setupStatusBar();

    // Set up splitter layout
    m_mainSplitter->addWidget(m_leftWidget);
    m_rightSplitter->addWidget(m_detailsWidget);
    m_rightSplitter->addWidget(m_timelineWidget);
    m_mainSplitter->addWidget(m_rightSplitter);

    m_mainSplitter->setSizes({600, 400});
    m_rightSplitter->setSizes({300, 200});

    // Connect signals
    connect(m_milestoneTree, &QTreeWidget::itemSelectionChanged, this, &MilestoneTrackerDialog::onMilestoneSelectionChanged);
    connect(m_milestoneTree, &QTreeWidget::itemDoubleClicked, this, &MilestoneTrackerDialog::onMilestoneDoubleClicked);
    connect(m_milestoneTree, &QTreeWidget::itemChanged, this, &MilestoneTrackerDialog::onMilestoneItemChanged);
}

void MilestoneTrackerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setFixedHeight(24);  // Lock menu bar height to 24px
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Project", this, &MilestoneTrackerDialog::onNewProject, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &MilestoneTrackerDialog::onOpenProject, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &MilestoneTrackerDialog::onSaveProject, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &MilestoneTrackerDialog::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_generateReportAction = fileMenu->addAction("&Generate Report...", this, &MilestoneTrackerDialog::onGenerateReport);
    m_exportAction = fileMenu->addAction("&Export to CSV...", this, &MilestoneTrackerDialog::onExportCSV);
    m_importAction = fileMenu->addAction("&Import from CSV...", this, &MilestoneTrackerDialog::onImportCSV);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close);

    // Milestone menu
    QMenu* milestoneMenu = m_menuBar->addMenu("&Milestone");
    m_addMilestoneAction = milestoneMenu->addAction("&Add Milestone", this, &MilestoneTrackerDialog::onAddMilestone, QKeySequence("Ctrl+N"));
    m_editMilestoneAction = milestoneMenu->addAction("&Edit Milestone", this, &MilestoneTrackerDialog::onEditMilestone, QKeySequence("F2"));
    m_deleteMilestoneAction = milestoneMenu->addAction("&Delete Milestone", this, &MilestoneTrackerDialog::onDeleteMilestone, QKeySequence::Delete);
    milestoneMenu->addSeparator();
    m_markCompletedAction = milestoneMenu->addAction("Mark &Completed", this, &MilestoneTrackerDialog::onMarkCompleted, QKeySequence("Ctrl+D"));
    m_markInProgressAction = milestoneMenu->addAction("Mark &In Progress", this, &MilestoneTrackerDialog::onMarkInProgress, QKeySequence("Ctrl+P"));
    milestoneMenu->addSeparator();
    m_duplicateAction = milestoneMenu->addAction("D&uplicate", this, &MilestoneTrackerDialog::onDuplicateMilestone, QKeySequence("Ctrl+Shift+D"));

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_ganttAction = viewMenu->addAction("&Gantt Chart", this, &MilestoneTrackerDialog::onShowGantt);
}

void MilestoneTrackerDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(26);  // Reduce toolbar height to 26px
    m_toolBar->setIconSize(QSize(16, 16));  // Smaller icons for compact layout
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Milestone operations
    m_toolBar->addAction(m_addMilestoneAction);
    m_toolBar->addAction(m_editMilestoneAction);
    m_toolBar->addAction(m_deleteMilestoneAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_markCompletedAction);
    m_toolBar->addAction(m_markInProgressAction);
}

void MilestoneTrackerDialog::setupMilestoneList() {
    m_leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(m_leftWidget);

    setupFilters();
    leftLayout->addWidget(m_filterWidget);

    // Milestone tree
    m_milestoneTree = new QTreeWidget();
    m_milestoneTree->setHeaderLabels({"Milestone", "Priority", "Status", "Target Date", "Owner", "Progress"});
    m_milestoneTree->setRootIsDecorated(false);
    m_milestoneTree->setAlternatingRowColors(true);
    m_milestoneTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_milestoneTree->setSortingEnabled(true);

    // Set column widths
    m_milestoneTree->header()->resizeSection(0, 250);
    m_milestoneTree->header()->resizeSection(1, 80);
    m_milestoneTree->header()->resizeSection(2, 100);
    m_milestoneTree->header()->resizeSection(3, 100);
    m_milestoneTree->header()->resizeSection(4, 100);
    m_milestoneTree->header()->resizeSection(5, 80);

    leftLayout->addWidget(m_milestoneTree);
}

void MilestoneTrackerDialog::setupMilestoneDetails() {
    m_detailsWidget = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsWidget);

    detailsLayout->addWidget(new QLabel("Milestone Details"));

    // Title
    detailsLayout->addWidget(new QLabel("Title:"));
    m_titleEdit = new QLineEdit();
    detailsLayout->addWidget(m_titleEdit);

    // Description
    detailsLayout->addWidget(new QLabel("Description:"));
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    detailsLayout->addWidget(m_descriptionEdit);

    // Status and Priority
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel("Status:"));
    m_statusCombo = new QComboBox();
    m_statusCombo->addItems({"Not Started", "In Progress", "Completed", "Delayed", "Cancelled"});
    statusLayout->addWidget(m_statusCombo);

    statusLayout->addWidget(new QLabel("Priority:"));
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItems({"Low", "Normal", "High", "Critical"});
    statusLayout->addWidget(m_priorityCombo);
    detailsLayout->addLayout(statusLayout);

    // Dates
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Start:"));
    m_startDateEdit = new QDateEdit();
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDate(QDate::currentDate());
    dateLayout->addWidget(m_startDateEdit);

    dateLayout->addWidget(new QLabel("Target:"));
    m_targetDateEdit = new QDateEdit();
    m_targetDateEdit->setCalendarPopup(true);
    m_targetDateEdit->setDate(QDate::currentDate().addDays(30));
    dateLayout->addWidget(m_targetDateEdit);
    detailsLayout->addLayout(dateLayout);

    // Owner and budget
    QHBoxLayout* ownerLayout = new QHBoxLayout();
    ownerLayout->addWidget(new QLabel("Owner:"));
    m_ownerEdit = new QLineEdit();
    ownerLayout->addWidget(m_ownerEdit);
    detailsLayout->addLayout(ownerLayout);

    QHBoxLayout* budgetLayout = new QHBoxLayout();
    budgetLayout->addWidget(new QLabel("Budget:"));
    m_budgetEdit = new QLineEdit();
    m_budgetEdit->setPlaceholderText("0.00");
    budgetLayout->addWidget(m_budgetEdit);

    budgetLayout->addWidget(new QLabel("Actual:"));
    m_actualCostEdit = new QLineEdit();
    m_actualCostEdit->setPlaceholderText("0.00");
    budgetLayout->addWidget(m_actualCostEdit);
    detailsLayout->addLayout(budgetLayout);

    // Progress
    detailsLayout->addWidget(new QLabel("Progress:"));
    m_progressSlider = new QSlider(Qt::Horizontal);
    m_progressSlider->setRange(0, 100);
    m_progressLabel = new QLabel("0%");
    QHBoxLayout* progressLayout = new QHBoxLayout();
    progressLayout->addWidget(m_progressSlider);
    progressLayout->addWidget(m_progressLabel);
    detailsLayout->addLayout(progressLayout);

    // Info labels
    m_createdLabel = new QLabel();
    m_completedLabel = new QLabel();
    m_daysRemainingLabel = new QLabel();
    m_budgetVarianceLabel = new QLabel();
    detailsLayout->addWidget(m_createdLabel);
    detailsLayout->addWidget(m_completedLabel);
    detailsLayout->addWidget(m_daysRemainingLabel);
    detailsLayout->addWidget(m_budgetVarianceLabel);

    detailsLayout->addStretch();

    // Update button
    QPushButton* updateButton = new QPushButton("Update Milestone");
    connect(updateButton, &QPushButton::clicked, this, &MilestoneTrackerDialog::onEditMilestone);
    detailsLayout->addWidget(updateButton);

    // Connect progress slider
    connect(m_progressSlider, &QSlider::valueChanged, this, [this](int value) {
        m_progressLabel->setText(QString("%1%").arg(value));
    });
}

void MilestoneTrackerDialog::setupTimelineView() {
    m_timelineWidget = new QWidget();
    QVBoxLayout* timelineLayout = new QVBoxLayout(m_timelineWidget);

    timelineLayout->addWidget(new QLabel("Timeline View"));

    m_calendar = new QCalendarWidget();
    m_calendar->setMaximumHeight(200);
    timelineLayout->addWidget(m_calendar);

    m_timelineDetails = new QTextEdit();
    m_timelineDetails->setMaximumHeight(100);
    m_timelineDetails->setReadOnly(true);
    timelineLayout->addWidget(m_timelineDetails);
}

void MilestoneTrackerDialog::setupFilters() {
    m_filterWidget = new QWidget();
    m_filterWidget->setFixedHeight(50);  // Lock filter bar height to 50px
    QHBoxLayout* filterLayout = new QHBoxLayout(m_filterWidget);

    // Search filter
    filterLayout->addWidget(new QLabel("Search:"));
    m_searchFilter = new QLineEdit();
    m_searchFilter->setPlaceholderText("Search milestones...");
    filterLayout->addWidget(m_searchFilter);

    // Status filter
    filterLayout->addWidget(new QLabel("Status:"));
    m_statusFilter = new QComboBox();
    m_statusFilter->addItems({"All", "Not Started", "In Progress", "Completed", "Delayed", "Cancelled"});
    filterLayout->addWidget(m_statusFilter);

    // Priority filter
    filterLayout->addWidget(new QLabel("Priority:"));
    m_priorityFilter = new QComboBox();
    m_priorityFilter->addItems({"All", "Low", "Normal", "High", "Critical"});
    filterLayout->addWidget(m_priorityFilter);

    // Show completed checkbox
    m_showCompletedCheck = new QCheckBox("Show Completed");
    m_showCompletedCheck->setChecked(true);
    filterLayout->addWidget(m_showCompletedCheck);

    // Sort combo
    filterLayout->addWidget(new QLabel("Sort:"));
    m_sortCombo = new QComboBox();
    m_sortCombo->addItems({"Title", "Priority", "Status", "Target Date", "Owner", "Progress"});
    filterLayout->addWidget(m_sortCombo);

    // View mode
    filterLayout->addWidget(new QLabel("View:"));
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItems({"List", "Timeline", "Gantt"});
    filterLayout->addWidget(m_viewModeCombo);

    // Connect filter signals
    connect(m_searchFilter, &QLineEdit::textChanged, this, &MilestoneTrackerDialog::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MilestoneTrackerDialog::onFilterChanged);
    connect(m_priorityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MilestoneTrackerDialog::onFilterChanged);
    connect(m_showCompletedCheck, &QCheckBox::toggled, this, &MilestoneTrackerDialog::onShowCompletedToggled);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MilestoneTrackerDialog::onSortChanged);
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MilestoneTrackerDialog::onViewModeChanged);
}

void MilestoneTrackerDialog::setupStatusBar() {
    QHBoxLayout* statusLayout = new QHBoxLayout();

    m_statsLabel = new QLabel("0 milestones");
    statusLayout->addWidget(m_statsLabel);

    statusLayout->addStretch();

    statusLayout->addWidget(new QLabel("Overall Progress:"));
    m_overallProgressBar = new QProgressBar();
    m_overallProgressBar->setMaximumWidth(200);
    statusLayout->addWidget(m_overallProgressBar);

    m_budgetLabel = new QLabel("Budget: $0");
    statusLayout->addWidget(m_budgetLabel);

    QWidget* statusWidget = new QWidget();
    statusWidget->setLayout(statusLayout);
    statusWidget->setMaximumHeight(30);
    m_mainLayout->addWidget(statusWidget);
}

// Implementation methods
void MilestoneTrackerDialog::newProject() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_milestones.clear();
    m_currentFilePath.clear();
    m_projectName = "New Project";
    setModified(false);
    updateMilestoneList();
    updateWindowTitle();
    updateStatistics();
}

void MilestoneTrackerDialog::openProject() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Open Project", "", "Milestone Project Files (*.milestones);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            m_projectName = QFileInfo(filePath).baseName();
            setModified(false);
            updateMilestoneList();
            updateWindowTitle();
            updateStatistics();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load project file.");
        }
    }
}

void MilestoneTrackerDialog::saveProject() {
    if (m_currentFilePath.isEmpty()) {
        saveProjectAs();
    } else {
        if (saveToFile(m_currentFilePath)) {
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project file.");
        }
    }
}

void MilestoneTrackerDialog::saveProjectAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Project", "", "Milestone Project Files (*.milestones);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (saveToFile(filePath)) {
            m_currentFilePath = filePath;
            m_projectName = QFileInfo(filePath).baseName();
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project file.");
        }
    }
}

// Slot stubs
void MilestoneTrackerDialog::onNewProject() { newProject(); }
void MilestoneTrackerDialog::onOpenProject() { openProject(); }
void MilestoneTrackerDialog::onSaveProject() { saveProject(); }
void MilestoneTrackerDialog::onSaveProjectAs() { saveProjectAs(); }
void MilestoneTrackerDialog::onGenerateReport() { generateReport(); }
void MilestoneTrackerDialog::onExportCSV() { exportToCSV(); }
void MilestoneTrackerDialog::onImportCSV() { importFromCSV(); }
void MilestoneTrackerDialog::onShowGantt() { showGanttChart(); }
void MilestoneTrackerDialog::onAddMilestone() { addMilestone(); }
void MilestoneTrackerDialog::onEditMilestone() { editMilestone(); }
void MilestoneTrackerDialog::onDeleteMilestone() { deleteMilestone(); }
void MilestoneTrackerDialog::onMarkCompleted() { markMilestoneCompleted(); }
void MilestoneTrackerDialog::onMarkInProgress() { markMilestoneInProgress(); }
void MilestoneTrackerDialog::onDuplicateMilestone() { duplicateMilestone(); }
void MilestoneTrackerDialog::onMilestoneSelectionChanged() {
    updateMilestoneDetails();
}
void MilestoneTrackerDialog::onMilestoneDoubleClicked(QTreeWidgetItem* item, int column) {
    if (item && column == 0) { // Only allow renaming the title column
        m_milestoneTree->editItem(item, 0);
    }
}

void MilestoneTrackerDialog::onMilestoneItemChanged(QTreeWidgetItem* item, int column) {
    if (column == 0) { // Title changed
        QString milestoneId = item->data(0, Qt::UserRole).toString();
        Milestone* milestone = findMilestone(milestoneId);
        if (milestone) {
            milestone->title = item->text(0);
            setModified(true);
            updateMilestoneDetails(); // Refresh the details panel
        }
    }
}
void MilestoneTrackerDialog::onFilterChanged() {
    updateMilestoneList();
}

void MilestoneTrackerDialog::onSortChanged() {
    int sortColumn = m_sortCombo->currentIndex();
    m_milestoneTree->sortItems(sortColumn, Qt::AscendingOrder);
}

void MilestoneTrackerDialog::onShowCompletedToggled(bool) {
    updateMilestoneList();
}

void MilestoneTrackerDialog::onViewModeChanged() {
    // For now, just update the list - could implement different views later
    updateMilestoneList();
}
void MilestoneTrackerDialog::updateProgress() { /* TODO */ }
void MilestoneTrackerDialog::updateTimeline() { /* TODO */ }


void MilestoneTrackerDialog::updateMilestoneList() {
    m_milestoneTree->clear();

    for (const Milestone& milestone : m_milestones) {
        MilestoneItem* item = new MilestoneItem(milestone, m_milestoneTree);
        item->setData(0, Qt::UserRole, milestone.id);
    }

    // Apply sorting (only if UI is initialized)
    if (m_sortCombo) {
        int sortColumn = m_sortCombo->currentIndex();
        m_milestoneTree->sortItems(sortColumn, Qt::AscendingOrder);
    }
}
void MilestoneTrackerDialog::updateMilestoneDetails() {
    QTreeWidgetItem* currentItem = m_milestoneTree->currentItem();
    if (!currentItem) {
        // Clear details
        m_titleEdit->clear();
        m_descriptionEdit->clear();
        m_statusCombo->setCurrentIndex(0);
        m_priorityCombo->setCurrentIndex(0);
        m_startDateEdit->setDate(QDate::currentDate());
        m_targetDateEdit->setDate(QDate::currentDate().addDays(30));
        m_ownerEdit->clear();
        m_deliverablesEdit->clear();
        m_tagsEdit->clear();
        m_progressSlider->setValue(0);
        m_progressLabel->setText("0%");
        m_budgetEdit->clear();
        m_actualCostEdit->clear();
        m_budgetVarianceLabel->clear();
        m_createdLabel->clear();
        m_completedLabel->clear();
        m_daysRemainingLabel->clear();
        return;
    }

    QString milestoneId = currentItem->data(0, Qt::UserRole).toString();
    Milestone* milestone = findMilestone(milestoneId);
    if (!milestone) return;

    m_titleEdit->setText(milestone->title);
    m_descriptionEdit->setPlainText(milestone->description);
    m_statusCombo->setCurrentIndex(static_cast<int>(milestone->status));
    m_priorityCombo->setCurrentIndex(static_cast<int>(milestone->priority));
    m_startDateEdit->setDate(milestone->startDate.isValid() ? milestone->startDate.date() : QDate::currentDate());
    m_targetDateEdit->setDate(milestone->targetDate.isValid() ? milestone->targetDate.date() : QDate::currentDate().addDays(30));
    m_ownerEdit->setText(milestone->owner);
    m_deliverablesEdit->setPlainText(milestone->deliverables.join("\n"));
    m_tagsEdit->setText(milestone->tags.join(", "));
    m_progressSlider->setValue(milestone->progress);
    m_progressLabel->setText(QString("%1%").arg(milestone->progress));
    m_budgetEdit->setText(QString::number(milestone->budget, 'f', 2));
    m_actualCostEdit->setText(QString::number(milestone->actualCost, 'f', 2));

    // Calculate budget variance
    double variance = milestone->actualCost - milestone->budget;
    QString varianceText = QString("Variance: %1%2").arg(variance >= 0 ? "+" : "").arg(variance, 0, 'f', 2);
    m_budgetVarianceLabel->setText(varianceText);
    m_budgetVarianceLabel->setStyleSheet(variance > 0 ? "color: red;" : "color: green;");

    m_createdLabel->setText("Created: " + milestone->createdDate.toString("yyyy-MM-dd hh:mm"));
    if (milestone->completedDate.isValid()) {
        m_completedLabel->setText("Completed: " + milestone->completedDate.toString("yyyy-MM-dd hh:mm"));
    } else {
        m_completedLabel->setText("Not completed");
    }

    // Calculate days remaining
    if (milestone->targetDate.isValid()) {
        int daysRemaining = QDate::currentDate().daysTo(milestone->targetDate.date());
        if (daysRemaining >= 0) {
            m_daysRemainingLabel->setText(QString("Days remaining: %1").arg(daysRemaining));
        } else {
            m_daysRemainingLabel->setText(QString("Overdue by %1 days").arg(-daysRemaining));
            m_daysRemainingLabel->setStyleSheet("color: red;");
        }
    } else {
        m_daysRemainingLabel->setText("No target date set");
    }
}
void MilestoneTrackerDialog::updateWindowTitle() { setWindowTitle("Milestone Tracker"); }
void MilestoneTrackerDialog::updateStatistics() {
    int totalMilestones = m_milestones.size();
    int completedMilestones = 0;
    int inProgressMilestones = 0;
    int overdueMilestones = 0;

    for (const Milestone& milestone : m_milestones) {
        if (milestone.status == MilestoneStatus::Completed) {
            completedMilestones++;
        } else if (milestone.status == MilestoneStatus::InProgress) {
            inProgressMilestones++;
        }

        if (milestone.targetDate.isValid() &&
            milestone.targetDate.date() < QDate::currentDate() &&
            milestone.status != MilestoneStatus::Completed) {
            overdueMilestones++;
        }
    }

    // Update status bar or statistics display
    // This would need to be connected to actual UI elements
    // For now, just update the window title with basic stats
    QString statsText = QString("Milestone Tracker - %1 total, %2 completed, %3 in progress")
        .arg(totalMilestones).arg(completedMilestones).arg(inProgressMilestones);
    if (overdueMilestones > 0) {
        statsText += QString(", %1 overdue").arg(overdueMilestones);
    }
    setWindowTitle(statsText);
}
void MilestoneTrackerDialog::updateDependencyGraph() { /* TODO */ }
bool MilestoneTrackerDialog::hasUnsavedChanges() const { return m_modified; }
bool MilestoneTrackerDialog::promptSaveChanges() { return true; }
void MilestoneTrackerDialog::setModified(bool modified) { m_modified = modified; }
Milestone* MilestoneTrackerDialog::findMilestone(const QString& id) {
    for (Milestone& milestone : m_milestones) {
        if (milestone.id == id) {
            return &milestone;
        }
    }
    return nullptr;
}
MilestoneItem* MilestoneTrackerDialog::findMilestoneItem(const QString&) { return nullptr; }
void MilestoneTrackerDialog::addMilestoneToTree(const Milestone&) { /* TODO */ }
void MilestoneTrackerDialog::removeMilestoneFromTree(const QString&) { /* TODO */ }
void MilestoneTrackerDialog::loadSettings() { /* TODO */ }
void MilestoneTrackerDialog::saveSettings() { /* TODO */ }
bool MilestoneTrackerDialog::loadFromFile(const QString&) { return false; }
bool MilestoneTrackerDialog::saveToFile(const QString&) { return false; }

void MilestoneTrackerDialog::addMilestone() {
    Milestone newMilestone;
    newMilestone.title = "New Milestone";
    newMilestone.description = "";
    newMilestone.status = MilestoneStatus::NotStarted;
    newMilestone.priority = MilestonePriority::Normal;

    m_milestones.append(newMilestone);
    updateMilestoneList();
    setModified(true);
    updateStatistics();
}

void MilestoneTrackerDialog::editMilestone() {
    QTreeWidgetItem* currentItem = m_milestoneTree->currentItem();
    if (!currentItem) return;

    QString milestoneId = currentItem->data(0, Qt::UserRole).toString();
    Milestone* milestone = findMilestone(milestoneId);
    if (!milestone) return;

    // Update milestone from UI
    milestone->title = m_titleEdit->text();
    milestone->description = m_descriptionEdit->toPlainText();
    milestone->status = static_cast<MilestoneStatus>(m_statusCombo->currentIndex());
    milestone->priority = static_cast<MilestonePriority>(m_priorityCombo->currentIndex());
    milestone->startDate = QDateTime(m_startDateEdit->date(), QTime());
    milestone->targetDate = QDateTime(m_targetDateEdit->date(), QTime());
    milestone->owner = m_ownerEdit->text();
    milestone->deliverables = m_deliverablesEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    milestone->tags = m_tagsEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& tag : milestone->tags) {
        tag = tag.trimmed();
    }
    milestone->progress = m_progressSlider->value();
    milestone->budget = m_budgetEdit->text().toDouble();
    milestone->actualCost = m_actualCostEdit->text().toDouble();

    // Update completion date if status changed to completed
    if (milestone->status == MilestoneStatus::Completed && !milestone->completedDate.isValid()) {
        milestone->completedDate = QDateTime::currentDateTime();
        milestone->progress = 100;
    } else if (milestone->status != MilestoneStatus::Completed) {
        milestone->completedDate = QDateTime();
    }

    // Update the tree item
    MilestoneItem* milestoneItem = dynamic_cast<MilestoneItem*>(currentItem);
    if (milestoneItem) {
        milestoneItem->updateFromMilestone(*milestone);
    }

    updateMilestoneDetails();
    setModified(true);
    updateStatistics();
}

void MilestoneTrackerDialog::deleteMilestone() {
    QTreeWidgetItem* currentItem = m_milestoneTree->currentItem();
    if (!currentItem) return;

    QString milestoneId = currentItem->data(0, Qt::UserRole).toString();
    Milestone* milestone = findMilestone(milestoneId);
    if (!milestone) return;

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Milestone",
        QString("Are you sure you want to delete milestone '%1'?").arg(milestone->title),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Remove from list
        for (int i = 0; i < m_milestones.size(); ++i) {
            if (m_milestones[i].id == milestoneId) {
                m_milestones.removeAt(i);
                break;
            }
        }

        updateMilestoneList();
        updateMilestoneDetails();
        setModified(true);
        updateStatistics();
    }
}

void MilestoneTrackerDialog::markMilestoneCompleted() {
    QTreeWidgetItem* currentItem = m_milestoneTree->currentItem();
    if (!currentItem) return;

    QString milestoneId = currentItem->data(0, Qt::UserRole).toString();
    Milestone* milestone = findMilestone(milestoneId);
    if (!milestone) return;

    milestone->status = MilestoneStatus::Completed;
    milestone->completedDate = QDateTime::currentDateTime();
    milestone->progress = 100;

    MilestoneItem* milestoneItem = dynamic_cast<MilestoneItem*>(currentItem);
    if (milestoneItem) {
        milestoneItem->updateFromMilestone(*milestone);
    }

    updateMilestoneDetails();
    setModified(true);
    updateStatistics();
}

void MilestoneTrackerDialog::markMilestoneInProgress() {
    QTreeWidgetItem* currentItem = m_milestoneTree->currentItem();
    if (!currentItem) return;

    QString milestoneId = currentItem->data(0, Qt::UserRole).toString();
    Milestone* milestone = findMilestone(milestoneId);
    if (!milestone) return;

    milestone->status = MilestoneStatus::InProgress;
    milestone->completedDate = QDateTime();
    if (milestone->progress == 0) {
        milestone->progress = 25; // Set some initial progress
    }

    MilestoneItem* milestoneItem = dynamic_cast<MilestoneItem*>(currentItem);
    if (milestoneItem) {
        milestoneItem->updateFromMilestone(*milestone);
    }

    updateMilestoneDetails();
    setModified(true);
    updateStatistics();
}

void MilestoneTrackerDialog::duplicateMilestone() {
    // TODO: Implement milestone duplication
}

void MilestoneTrackerDialog::generateReport() {
    // TODO: Implement report generation
}

void MilestoneTrackerDialog::exportToCSV() {
    // TODO: Implement CSV export
}

void MilestoneTrackerDialog::importFromCSV() {
    // TODO: Implement CSV import
}

void MilestoneTrackerDialog::showGanttChart() {
    // TODO: Implement Gantt chart view
}

void MilestoneTrackerDialog::closeEvent(QCloseEvent* event) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}


