#include "FeatureBugTrackerDialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QHeaderView>
#include <QUuid>

// Issue Implementation
Issue::Issue()
    : id(QUuid::createUuid().toString())
    , type(IssueType::Feature)
    , title("New Issue")
    , status(IssueStatus::Open)
    , priority(IssuePriority::Major)
    , severity(IssueSeverity::Medium)
    , implementationLevel(ImplementationLevel::NotStarted)
    , createdDate(QDateTime::currentDateTime())
    , updatedDate(QDateTime::currentDateTime())
    , estimatedHours(0)
    , actualHours(0)
{
}

Issue::Issue(IssueType type, const QString& title, const QString& description)
    : id(QUuid::createUuid().toString())
    , type(type)
    , title(title)
    , description(description)
    , status(IssueStatus::Open)
    , priority(IssuePriority::Major)
    , severity(IssueSeverity::Medium)
    , implementationLevel(ImplementationLevel::NotStarted)
    , createdDate(QDateTime::currentDateTime())
    , updatedDate(QDateTime::currentDateTime())
    , estimatedHours(0)
    , actualHours(0)
{
}

QJsonObject Issue::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = static_cast<int>(type);
    obj["title"] = title;
    obj["description"] = description;
    obj["status"] = static_cast<int>(status);
    obj["priority"] = static_cast<int>(priority);
    obj["severity"] = static_cast<int>(severity);
    obj["implementationLevel"] = static_cast<int>(implementationLevel);
    obj["createdDate"] = createdDate.toString(Qt::ISODate);
    obj["updatedDate"] = updatedDate.toString(Qt::ISODate);
    obj["resolvedDate"] = resolvedDate.toString(Qt::ISODate);
    obj["reporter"] = reporter;
    obj["assignee"] = assignee;
    obj["component"] = component;
    obj["version"] = version;
    obj["targetVersion"] = targetVersion;
    obj["reproductionSteps"] = reproductionSteps;
    obj["expectedBehavior"] = expectedBehavior;
    obj["actualBehavior"] = actualBehavior;
    obj["environment"] = environment;
    obj["estimatedHours"] = estimatedHours;
    obj["actualHours"] = actualHours;
    
    QJsonArray tagsArray;
    for (const QString& tag : tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;
    
    QJsonArray commentsArray;
    for (const QString& comment : comments) {
        commentsArray.append(comment);
    }
    obj["comments"] = commentsArray;
    
    return obj;
}

void Issue::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    type = static_cast<IssueType>(json["type"].toInt());
    title = json["title"].toString();
    description = json["description"].toString();
    status = static_cast<IssueStatus>(json["status"].toInt());
    priority = static_cast<IssuePriority>(json["priority"].toInt());
    severity = static_cast<IssueSeverity>(json["severity"].toInt());
    implementationLevel = static_cast<ImplementationLevel>(json["implementationLevel"].toInt());
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    updatedDate = QDateTime::fromString(json["updatedDate"].toString(), Qt::ISODate);
    resolvedDate = QDateTime::fromString(json["resolvedDate"].toString(), Qt::ISODate);
    reporter = json["reporter"].toString();
    assignee = json["assignee"].toString();
    component = json["component"].toString();
    version = json["version"].toString();
    targetVersion = json["targetVersion"].toString();
    reproductionSteps = json["reproductionSteps"].toString();
    expectedBehavior = json["expectedBehavior"].toString();
    actualBehavior = json["actualBehavior"].toString();
    environment = json["environment"].toString();
    estimatedHours = json["estimatedHours"].toInt();
    actualHours = json["actualHours"].toInt();
    
    tags.clear();
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& value : tagsArray) {
        tags.append(value.toString());
    }
    
    comments.clear();
    QJsonArray commentsArray = json["comments"].toArray();
    for (const QJsonValue& value : commentsArray) {
        comments.append(value.toString());
    }
}

QString Issue::getTypeString() const {
    switch (type) {
        case IssueType::Feature: return "Feature";
        case IssueType::Bug: return "Bug";
        case IssueType::Enhancement: return "Enhancement";
        case IssueType::Task: return "Task";
        default: return "Feature";
    }
}

QString Issue::getStatusString() const {
    switch (status) {
        case IssueStatus::Open: return "Open";
        case IssueStatus::InProgress: return "In Progress";
        case IssueStatus::Testing: return "Testing";
        case IssueStatus::Resolved: return "Resolved";
        case IssueStatus::Closed: return "Closed";
        case IssueStatus::Reopened: return "Reopened";
        default: return "Open";
    }
}

QString Issue::getPriorityString() const {
    switch (priority) {
        case IssuePriority::Trivial: return "Trivial";
        case IssuePriority::Minor: return "Minor";
        case IssuePriority::Major: return "Major";
        case IssuePriority::Critical: return "Critical";
        case IssuePriority::Blocker: return "Blocker";
        default: return "Major";
    }
}

QString Issue::getSeverityString() const {
    switch (severity) {
        case IssueSeverity::Low: return "Low";
        case IssueSeverity::Medium: return "Medium";
        case IssueSeverity::High: return "High";
        case IssueSeverity::Critical: return "Critical";
        default: return "Medium";
    }
}

QString Issue::getImplementationLevelString() const {
    switch (implementationLevel) {
        case ImplementationLevel::NotStarted: return "Not Started";
        case ImplementationLevel::Planning: return "Planning";
        case ImplementationLevel::InDevelopment: return "In Development";
        case ImplementationLevel::CodeReview: return "Code Review";
        case ImplementationLevel::Testing: return "Testing";
        case ImplementationLevel::Documentation: return "Documentation";
        case ImplementationLevel::Complete: return "Complete";
        default: return "Not Started";
    }
}

QColor Issue::getTypeColor() const {
    switch (type) {
        case IssueType::Feature: return QColor(0, 128, 255);
        case IssueType::Bug: return QColor(255, 0, 0);
        case IssueType::Enhancement: return QColor(0, 255, 0);
        case IssueType::Task: return QColor(255, 165, 0);
        default: return QColor(128, 128, 128);
    }
}

QColor Issue::getStatusColor() const {
    switch (status) {
        case IssueStatus::Open: return QColor(128, 128, 128);
        case IssueStatus::InProgress: return QColor(255, 165, 0);
        case IssueStatus::Testing: return QColor(255, 255, 0);
        case IssueStatus::Resolved: return QColor(0, 255, 0);
        case IssueStatus::Closed: return QColor(0, 128, 0);
        case IssueStatus::Reopened: return QColor(255, 0, 255);
        default: return QColor(128, 128, 128);
    }
}

QColor Issue::getPriorityColor() const {
    switch (priority) {
        case IssuePriority::Trivial: return QColor(200, 200, 200);
        case IssuePriority::Minor: return QColor(255, 255, 0);
        case IssuePriority::Major: return QColor(255, 165, 0);
        case IssuePriority::Critical: return QColor(255, 0, 0);
        case IssuePriority::Blocker: return QColor(128, 0, 0);
        default: return QColor(255, 165, 0);
    }
}

bool Issue::isOverdue() const {
    // For now, consider issues overdue if they've been open for more than 30 days
    return createdDate.daysTo(QDateTime::currentDateTime()) > 30 && status != IssueStatus::Closed && status != IssueStatus::Resolved;
}

int Issue::getDaysOpen() const {
    if (status == IssueStatus::Closed || status == IssueStatus::Resolved) {
        return createdDate.daysTo(resolvedDate);
    }
    return createdDate.daysTo(QDateTime::currentDateTime());
}

// IssueItem Implementation
IssueItem::IssueItem(const Issue& issue, QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , m_issue(issue)
{
    setupItem();
}

void IssueItem::setupItem() {
    updateFromIssue(m_issue);
}

void IssueItem::updateFromIssue(const Issue& issue) {
    m_issue = issue;
    
    setText(0, issue.title);
    setText(1, issue.getTypeString());
    setText(2, issue.getStatusString());
    setText(3, issue.getPriorityString());
    setText(4, issue.getSeverityString());
    setText(5, issue.assignee);
    setText(6, issue.component);
    setText(7, issue.createdDate.toString("yyyy-MM-dd"));
    
    // Set colors based on type and priority
    QColor typeColor = issue.getTypeColor();
    QColor priorityColor = issue.getPriorityColor();
    
    setForeground(1, QBrush(typeColor));
    setForeground(3, QBrush(priorityColor));
    
    // Mark overdue issues
    if (issue.isOverdue()) {
        setBackground(0, QBrush(QColor(255, 200, 200)));
    }
}

// FeatureBugTrackerDialog Implementation
FeatureBugTrackerDialog::FeatureBugTrackerDialog(QWidget* parent)
    : QDialog(parent)
    , m_modified(false)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
{
    setWindowTitle("Feature/Bug Tracker");
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    m_settings = new QSettings("LupineEngine", "FeatureBugTracker", this);
    
    setupUI();
    loadSettings();
    
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(60000);
    connect(m_updateTimer, &QTimer::timeout, this, &FeatureBugTrackerDialog::updateProgress);
    m_updateTimer->start();
    
    newProject();
}

FeatureBugTrackerDialog::~FeatureBugTrackerDialog() {
    saveSettings();
}

void FeatureBugTrackerDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    setupMenuBar();
    setupToolBar();
    
    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
    
    setupIssueList();
    setupIssueDetails();
    setupStatusBar();
    
    // Connect signals
    connect(m_issueTree, &QTreeWidget::itemSelectionChanged, this, &FeatureBugTrackerDialog::onIssueSelectionChanged);
    connect(m_issueTree, &QTreeWidget::itemDoubleClicked, this, &FeatureBugTrackerDialog::onIssueDoubleClicked);
    connect(m_issueTree, &QTreeWidget::itemChanged, this, &FeatureBugTrackerDialog::onIssueItemChanged);
}

// Stub implementations for now - will complete in next chunk
void FeatureBugTrackerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setFixedHeight(24);  // Lock menu bar height to 24px
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Project", this, &FeatureBugTrackerDialog::onNewProject, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &FeatureBugTrackerDialog::onOpenProject, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &FeatureBugTrackerDialog::onSaveProject, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &FeatureBugTrackerDialog::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_generateReportAction = fileMenu->addAction("&Generate Report...", this, &FeatureBugTrackerDialog::onGenerateReport);
    m_exportAction = fileMenu->addAction("&Export to CSV...", this, &FeatureBugTrackerDialog::onExportCSV);
    m_importAction = fileMenu->addAction("&Import from CSV...", this, &FeatureBugTrackerDialog::onImportCSV);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close);

    // Issue menu
    QMenu* issueMenu = m_menuBar->addMenu("&Issue");
    m_addFeatureAction = issueMenu->addAction("Add &Feature", this, &FeatureBugTrackerDialog::onAddFeature, QKeySequence("Ctrl+F"));
    m_addBugAction = issueMenu->addAction("Add &Bug", this, &FeatureBugTrackerDialog::onAddBug, QKeySequence("Ctrl+B"));
    m_addEnhancementAction = issueMenu->addAction("Add &Enhancement", this, &FeatureBugTrackerDialog::onAddEnhancement, QKeySequence("Ctrl+E"));
    issueMenu->addSeparator();
    m_editIssueAction = issueMenu->addAction("&Edit Issue", this, &FeatureBugTrackerDialog::onEditIssue, QKeySequence("F2"));
    m_deleteIssueAction = issueMenu->addAction("&Delete Issue", this, &FeatureBugTrackerDialog::onDeleteIssue, QKeySequence::Delete);
    m_duplicateAction = issueMenu->addAction("D&uplicate", this, &FeatureBugTrackerDialog::onDuplicateIssue, QKeySequence("Ctrl+D"));
    issueMenu->addSeparator();
    m_resolveAction = issueMenu->addAction("Mark &Resolved", this, &FeatureBugTrackerDialog::onResolveIssue, QKeySequence("Ctrl+R"));
    m_closeAction = issueMenu->addAction("Mark &Closed", this, &FeatureBugTrackerDialog::onCloseIssue, QKeySequence("Ctrl+Shift+C"));
    m_reopenAction = issueMenu->addAction("Re&open", this, &FeatureBugTrackerDialog::onReopenIssue, QKeySequence("Ctrl+O"));

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_statisticsAction = viewMenu->addAction("&Statistics", this, &FeatureBugTrackerDialog::onShowStatistics);
}

void FeatureBugTrackerDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(26);  // Reduce toolbar height to 26px
    m_toolBar->setIconSize(QSize(16, 16));  // Smaller icons for compact layout
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Issue operations
    m_toolBar->addAction(m_addFeatureAction);
    m_toolBar->addAction(m_addBugAction);
    m_toolBar->addAction(m_addEnhancementAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_editIssueAction);
    m_toolBar->addAction(m_deleteIssueAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_resolveAction);
    m_toolBar->addAction(m_closeAction);
}

void FeatureBugTrackerDialog::setupIssueList() {
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);

    setupFilters();
    leftLayout->addWidget(m_filterWidget);

    m_issueTree = new QTreeWidget();
    m_issueTree->setHeaderLabels({"Title", "Type", "Status", "Priority", "Severity", "Assignee", "Component", "Created"});
    m_issueTree->setRootIsDecorated(false);
    m_issueTree->setAlternatingRowColors(true);
    m_issueTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_issueTree->setSortingEnabled(true);

    // Set column widths
    m_issueTree->header()->resizeSection(0, 300);
    m_issueTree->header()->resizeSection(1, 80);
    m_issueTree->header()->resizeSection(2, 100);
    m_issueTree->header()->resizeSection(3, 80);
    m_issueTree->header()->resizeSection(4, 80);
    m_issueTree->header()->resizeSection(5, 100);
    m_issueTree->header()->resizeSection(6, 100);
    m_issueTree->header()->resizeSection(7, 100);

    leftLayout->addWidget(m_issueTree);

    m_mainSplitter->addWidget(leftWidget);
    m_mainSplitter->setSizes({700, 500});
}

void FeatureBugTrackerDialog::setupIssueDetails() {
    m_detailsTabWidget = new QTabWidget();
    m_mainSplitter->addWidget(m_detailsTabWidget);

    // Basic details tab
    m_basicDetailsTab = new QWidget();
    QVBoxLayout* basicLayout = new QVBoxLayout(m_basicDetailsTab);

    // Title
    basicLayout->addWidget(new QLabel("Title:"));
    m_titleEdit = new QLineEdit();
    basicLayout->addWidget(m_titleEdit);

    // Description
    basicLayout->addWidget(new QLabel("Description:"));
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(100);
    basicLayout->addWidget(m_descriptionEdit);

    // Type, Status, Priority, Severity
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:"));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"Feature", "Bug", "Enhancement", "Task"});
    typeLayout->addWidget(m_typeCombo);

    typeLayout->addWidget(new QLabel("Status:"));
    m_statusCombo = new QComboBox();
    m_statusCombo->addItems({"Open", "In Progress", "Testing", "Resolved", "Closed", "Reopened"});
    typeLayout->addWidget(m_statusCombo);
    basicLayout->addLayout(typeLayout);

    QHBoxLayout* priorityLayout = new QHBoxLayout();
    priorityLayout->addWidget(new QLabel("Priority:"));
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItems({"Trivial", "Minor", "Major", "Critical", "Blocker"});
    priorityLayout->addWidget(m_priorityCombo);

    priorityLayout->addWidget(new QLabel("Severity:"));
    m_severityCombo = new QComboBox();
    m_severityCombo->addItems({"Low", "Medium", "High", "Critical"});
    priorityLayout->addWidget(m_severityCombo);
    basicLayout->addLayout(priorityLayout);

    // Implementation Level
    QHBoxLayout* implLayout = new QHBoxLayout();
    implLayout->addWidget(new QLabel("Implementation:"));
    m_implementationCombo = new QComboBox();
    m_implementationCombo->addItems({"Not Started", "Planning", "In Development", "Code Review", "Testing", "Documentation", "Complete"});
    implLayout->addWidget(m_implementationCombo);
    basicLayout->addLayout(implLayout);

    // Reporter, Assignee, Component
    QHBoxLayout* peopleLayout = new QHBoxLayout();
    peopleLayout->addWidget(new QLabel("Reporter:"));
    m_reporterEdit = new QLineEdit();
    peopleLayout->addWidget(m_reporterEdit);

    peopleLayout->addWidget(new QLabel("Assignee:"));
    m_assigneeEdit = new QLineEdit();
    peopleLayout->addWidget(m_assigneeEdit);
    basicLayout->addLayout(peopleLayout);

    QHBoxLayout* componentLayout = new QHBoxLayout();
    componentLayout->addWidget(new QLabel("Component:"));
    m_componentEdit = new QLineEdit();
    componentLayout->addWidget(m_componentEdit);

    componentLayout->addWidget(new QLabel("Version:"));
    m_versionEdit = new QLineEdit();
    componentLayout->addWidget(m_versionEdit);

    componentLayout->addWidget(new QLabel("Target:"));
    m_targetVersionEdit = new QLineEdit();
    componentLayout->addWidget(m_targetVersionEdit);
    basicLayout->addLayout(componentLayout);

    // Tags
    basicLayout->addWidget(new QLabel("Tags (comma separated):"));
    m_tagsEdit = new QLineEdit();
    basicLayout->addWidget(m_tagsEdit);

    // Status labels
    m_createdLabel = new QLabel();
    m_updatedLabel = new QLabel();
    m_resolvedLabel = new QLabel();
    m_daysOpenLabel = new QLabel();
    basicLayout->addWidget(m_createdLabel);
    basicLayout->addWidget(m_updatedLabel);
    basicLayout->addWidget(m_resolvedLabel);
    basicLayout->addWidget(m_daysOpenLabel);

    basicLayout->addStretch();

    // Update button
    QPushButton* updateButton = new QPushButton("Update Issue");
    connect(updateButton, &QPushButton::clicked, this, &FeatureBugTrackerDialog::onEditIssue);
    basicLayout->addWidget(updateButton);

    m_detailsTabWidget->addTab(m_basicDetailsTab, "Details");

    // Bug details tab
    m_bugDetailsTab = new QWidget();
    QVBoxLayout* bugLayout = new QVBoxLayout(m_bugDetailsTab);

    bugLayout->addWidget(new QLabel("Reproduction Steps:"));
    m_reproductionStepsEdit = new QTextEdit();
    m_reproductionStepsEdit->setMaximumHeight(80);
    bugLayout->addWidget(m_reproductionStepsEdit);

    bugLayout->addWidget(new QLabel("Expected Behavior:"));
    m_expectedBehaviorEdit = new QTextEdit();
    m_expectedBehaviorEdit->setMaximumHeight(80);
    bugLayout->addWidget(m_expectedBehaviorEdit);

    bugLayout->addWidget(new QLabel("Actual Behavior:"));
    m_actualBehaviorEdit = new QTextEdit();
    m_actualBehaviorEdit->setMaximumHeight(80);
    bugLayout->addWidget(m_actualBehaviorEdit);

    bugLayout->addWidget(new QLabel("Environment:"));
    m_environmentEdit = new QLineEdit();
    bugLayout->addWidget(m_environmentEdit);

    bugLayout->addStretch();

    m_detailsTabWidget->addTab(m_bugDetailsTab, "Bug Details");

    // Time tracking tab
    m_timeTrackingTab = new QWidget();
    QVBoxLayout* timeLayout = new QVBoxLayout(m_timeTrackingTab);

    QHBoxLayout* hoursLayout = new QHBoxLayout();
    hoursLayout->addWidget(new QLabel("Estimated Hours:"));
    m_estimatedHoursSpinBox = new QSpinBox();
    m_estimatedHoursSpinBox->setRange(0, 1000);
    hoursLayout->addWidget(m_estimatedHoursSpinBox);

    hoursLayout->addWidget(new QLabel("Actual Hours:"));
    m_actualHoursSpinBox = new QSpinBox();
    m_actualHoursSpinBox->setRange(0, 1000);
    hoursLayout->addWidget(m_actualHoursSpinBox);
    timeLayout->addLayout(hoursLayout);

    m_timeVarianceLabel = new QLabel();
    timeLayout->addWidget(m_timeVarianceLabel);

    m_timeProgressBar = new QProgressBar();
    timeLayout->addWidget(m_timeProgressBar);

    timeLayout->addStretch();

    m_detailsTabWidget->addTab(m_timeTrackingTab, "Time Tracking");

    // Comments tab
    m_commentsTab = new QWidget();
    QVBoxLayout* commentsLayout = new QVBoxLayout(m_commentsTab);

    commentsLayout->addWidget(new QLabel("Comments:"));
    m_commentsEdit = new QTextEdit();
    m_commentsEdit->setReadOnly(true);
    commentsLayout->addWidget(m_commentsEdit);

    QHBoxLayout* newCommentLayout = new QHBoxLayout();
    m_newCommentEdit = new QLineEdit();
    m_newCommentEdit->setPlaceholderText("Add a comment...");
    newCommentLayout->addWidget(m_newCommentEdit);

    m_addCommentButton = new QPushButton("Add");
    connect(m_addCommentButton, &QPushButton::clicked, this, [this]() {
        QString comment = m_newCommentEdit->text().trimmed();
        if (!comment.isEmpty()) {
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
            QString fullComment = QString("[%1] %2").arg(timestamp, comment);

            IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
            if (currentItem) {
                Issue* issue = findIssue(currentItem->getIssue().id);
                if (issue) {
                    issue->comments.append(fullComment);
                    updateIssueDetails();
                    setModified(true);
                }
            }
            m_newCommentEdit->clear();
        }
    });
    newCommentLayout->addWidget(m_addCommentButton);
    commentsLayout->addLayout(newCommentLayout);

    m_detailsTabWidget->addTab(m_commentsTab, "Comments");
}

void FeatureBugTrackerDialog::setupFilters() {
    m_filterWidget = new QWidget();
    m_filterWidget->setFixedHeight(50);  // Lock filter bar height to 50px
    QHBoxLayout* filterLayout = new QHBoxLayout(m_filterWidget);

    // Search filter
    filterLayout->addWidget(new QLabel("Search:"));
    m_searchFilter = new QLineEdit();
    m_searchFilter->setPlaceholderText("Search issues...");
    filterLayout->addWidget(m_searchFilter);

    // Type filter
    filterLayout->addWidget(new QLabel("Type:"));
    m_typeFilter = new QComboBox();
    m_typeFilter->addItems({"All", "Feature", "Bug", "Enhancement", "Task"});
    filterLayout->addWidget(m_typeFilter);

    // Status filter
    filterLayout->addWidget(new QLabel("Status:"));
    m_statusFilter = new QComboBox();
    m_statusFilter->addItems({"All", "Open", "In Progress", "Testing", "Resolved", "Closed", "Reopened"});
    filterLayout->addWidget(m_statusFilter);

    // Priority filter
    filterLayout->addWidget(new QLabel("Priority:"));
    m_priorityFilter = new QComboBox();
    m_priorityFilter->addItems({"All", "Trivial", "Minor", "Major", "Critical", "Blocker"});
    filterLayout->addWidget(m_priorityFilter);

    // Severity filter
    filterLayout->addWidget(new QLabel("Severity:"));
    m_severityFilter = new QComboBox();
    m_severityFilter->addItems({"All", "Low", "Medium", "High", "Critical"});
    filterLayout->addWidget(m_severityFilter);

    // Show closed checkbox
    m_showClosedCheck = new QCheckBox("Show Closed");
    m_showClosedCheck->setChecked(false);
    filterLayout->addWidget(m_showClosedCheck);

    // Sort combo
    filterLayout->addWidget(new QLabel("Sort by:"));
    m_sortCombo = new QComboBox();
    m_sortCombo->addItems({"Title", "Type", "Status", "Priority", "Severity", "Created Date"});
    filterLayout->addWidget(m_sortCombo);

    // Connect filter signals
    connect(m_searchFilter, &QLineEdit::textChanged, this, &FeatureBugTrackerDialog::onFilterChanged);
    connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureBugTrackerDialog::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureBugTrackerDialog::onFilterChanged);
    connect(m_priorityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureBugTrackerDialog::onFilterChanged);
    connect(m_severityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureBugTrackerDialog::onFilterChanged);
    connect(m_showClosedCheck, &QCheckBox::toggled, this, &FeatureBugTrackerDialog::onShowClosedToggled);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureBugTrackerDialog::onSortChanged);
}

void FeatureBugTrackerDialog::setupStatusBar() {
    QHBoxLayout* statusLayout = new QHBoxLayout();

    m_statsLabel = new QLabel("0 issues");
    statusLayout->addWidget(m_statsLabel);

    statusLayout->addStretch();

    m_featureCountLabel = new QLabel("Features: 0");
    statusLayout->addWidget(m_featureCountLabel);

    m_bugCountLabel = new QLabel("Bugs: 0");
    statusLayout->addWidget(m_bugCountLabel);

    statusLayout->addWidget(new QLabel("Completion:"));
    m_completionProgressBar = new QProgressBar();
    m_completionProgressBar->setMaximumWidth(200);
    statusLayout->addWidget(m_completionProgressBar);

    QWidget* statusWidget = new QWidget();
    statusWidget->setLayout(statusLayout);
    statusWidget->setMaximumHeight(30);
    m_mainLayout->addWidget(statusWidget);
}

// Implementation methods
void FeatureBugTrackerDialog::newProject() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_issues.clear();
    m_currentFilePath.clear();
    m_projectName = "New Project";
    setModified(false);
    updateIssueList();
    updateWindowTitle();
    updateStatistics();
}

void FeatureBugTrackerDialog::openProject() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Open Project", "", "Issue Tracker Files (*.issues);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            m_projectName = QFileInfo(filePath).baseName();
            setModified(false);
            updateIssueList();
            updateWindowTitle();
            updateStatistics();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load project file.");
        }
    }
}

void FeatureBugTrackerDialog::saveProject() {
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

void FeatureBugTrackerDialog::saveProjectAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Project", "", "Issue Tracker Files (*.issues);;JSON Files (*.json)");
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

void FeatureBugTrackerDialog::addFeature() {
    Issue newIssue(IssueType::Feature, "New Feature", "");
    newIssue.reporter = "User";
    m_issues.append(newIssue);
    addIssueToTree(newIssue);
    setModified(true);
    updateStatistics();

    // Select the new issue
    IssueItem* item = findIssueItem(newIssue.id);
    if (item) {
        m_issueTree->setCurrentItem(item);
        m_issueTree->editItem(item, 0);
    }
}

void FeatureBugTrackerDialog::addBug() {
    Issue newIssue(IssueType::Bug, "New Bug", "");
    newIssue.reporter = "User";
    newIssue.priority = IssuePriority::Critical;
    newIssue.severity = IssueSeverity::High;
    m_issues.append(newIssue);
    addIssueToTree(newIssue);
    setModified(true);
    updateStatistics();

    // Select the new issue and switch to bug details tab
    IssueItem* item = findIssueItem(newIssue.id);
    if (item) {
        m_issueTree->setCurrentItem(item);
        m_detailsTabWidget->setCurrentIndex(1); // Bug details tab
        m_issueTree->editItem(item, 0);
    }
}

void FeatureBugTrackerDialog::addEnhancement() {
    Issue newIssue(IssueType::Enhancement, "New Enhancement", "");
    newIssue.reporter = "User";
    newIssue.priority = IssuePriority::Minor;
    m_issues.append(newIssue);
    addIssueToTree(newIssue);
    setModified(true);
    updateStatistics();

    // Select the new issue
    IssueItem* item = findIssueItem(newIssue.id);
    if (item) {
        m_issueTree->setCurrentItem(item);
        m_issueTree->editItem(item, 0);
    }
}

void FeatureBugTrackerDialog::editIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue issue = currentItem->getIssue();

    // Update issue from details panel
    issue.title = m_titleEdit->text();
    issue.description = m_descriptionEdit->toPlainText();
    issue.type = static_cast<IssueType>(m_typeCombo->currentIndex());
    issue.status = static_cast<IssueStatus>(m_statusCombo->currentIndex());
    issue.priority = static_cast<IssuePriority>(m_priorityCombo->currentIndex());
    issue.severity = static_cast<IssueSeverity>(m_severityCombo->currentIndex());
    issue.implementationLevel = static_cast<ImplementationLevel>(m_implementationCombo->currentIndex());
    issue.reporter = m_reporterEdit->text();
    issue.assignee = m_assigneeEdit->text();
    issue.component = m_componentEdit->text();
    issue.version = m_versionEdit->text();
    issue.targetVersion = m_targetVersionEdit->text();
    issue.tags = m_tagsEdit->text().split(',', Qt::SkipEmptyParts);
    issue.reproductionSteps = m_reproductionStepsEdit->toPlainText();
    issue.expectedBehavior = m_expectedBehaviorEdit->toPlainText();
    issue.actualBehavior = m_actualBehaviorEdit->toPlainText();
    issue.environment = m_environmentEdit->text();
    issue.estimatedHours = m_estimatedHoursSpinBox->value();
    issue.actualHours = m_actualHoursSpinBox->value();
    issue.updatedDate = QDateTime::currentDateTime();

    // Clean up tags
    for (QString& tag : issue.tags) {
        tag = tag.trimmed();
    }

    // Update resolution date if status changed to resolved/closed
    if ((issue.status == IssueStatus::Resolved || issue.status == IssueStatus::Closed) && !issue.resolvedDate.isValid()) {
        issue.resolvedDate = QDateTime::currentDateTime();
    } else if (issue.status != IssueStatus::Resolved && issue.status != IssueStatus::Closed) {
        issue.resolvedDate = QDateTime();
    }

    // Update the issue in the list
    Issue* originalIssue = findIssue(issue.id);
    if (originalIssue) {
        *originalIssue = issue;
        currentItem->updateFromIssue(issue);
        setModified(true);
        updateStatistics();
    }
}

void FeatureBugTrackerDialog::deleteIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue issue = currentItem->getIssue();

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Issue",
        QString("Are you sure you want to delete '%1'?").arg(issue.title),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        removeIssueFromTree(issue.id);
        setModified(true);
        updateStatistics();
    }
}

void FeatureBugTrackerDialog::duplicateIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue originalIssue = currentItem->getIssue();
    Issue newIssue = originalIssue;
    newIssue.id = QUuid::createUuid().toString();
    newIssue.title = originalIssue.title + " (Copy)";
    newIssue.status = IssueStatus::Open;
    newIssue.createdDate = QDateTime::currentDateTime();
    newIssue.updatedDate = QDateTime::currentDateTime();
    newIssue.resolvedDate = QDateTime();
    newIssue.comments.clear();

    m_issues.append(newIssue);
    addIssueToTree(newIssue);
    setModified(true);
    updateStatistics();
}

void FeatureBugTrackerDialog::resolveIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue* issue = findIssue(currentItem->getIssue().id);
    if (issue) {
        issue->status = IssueStatus::Resolved;
        issue->resolvedDate = QDateTime::currentDateTime();
        issue->updatedDate = QDateTime::currentDateTime();
        currentItem->updateFromIssue(*issue);
        updateIssueDetails();
        setModified(true);
        updateStatistics();
    }
}

void FeatureBugTrackerDialog::closeIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue* issue = findIssue(currentItem->getIssue().id);
    if (issue) {
        issue->status = IssueStatus::Closed;
        if (!issue->resolvedDate.isValid()) {
            issue->resolvedDate = QDateTime::currentDateTime();
        }
        issue->updatedDate = QDateTime::currentDateTime();
        currentItem->updateFromIssue(*issue);
        updateIssueDetails();
        setModified(true);
        updateStatistics();
    }
}

void FeatureBugTrackerDialog::reopenIssue() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) return;

    Issue* issue = findIssue(currentItem->getIssue().id);
    if (issue) {
        issue->status = IssueStatus::Reopened;
        issue->resolvedDate = QDateTime();
        issue->updatedDate = QDateTime::currentDateTime();
        currentItem->updateFromIssue(*issue);
        updateIssueDetails();
        setModified(true);
        updateStatistics();
    }
}

void FeatureBugTrackerDialog::generateReport() {
    QString filePath = QFileDialog::getSaveFileName(this, "Generate Report", "", "HTML Files (*.html);;Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to create report file.");
        return;
    }

    QTextStream out(&file);

    if (filePath.endsWith(".html")) {
        generateHTMLReport(out);
    } else {
        generateTextReport(out);
    }
}

void FeatureBugTrackerDialog::exportToCSV() {
    QString filePath = QFileDialog::getSaveFileName(this, "Export to CSV", "", "CSV Files (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to create CSV file.");
        return;
    }

    QTextStream out(&file);

    // CSV Header
    out << "ID,Title,Type,Status,Priority,Severity,Reporter,Assignee,Component,Version,Target Version,Created,Updated,Resolved,Estimated Hours,Actual Hours,Description\n";

    // CSV Data
    for (const Issue& issue : m_issues) {
        out << issue.id << ","
            << "\"" << issue.title << "\","
            << issue.getTypeString() << ","
            << issue.getStatusString() << ","
            << issue.getPriorityString() << ","
            << issue.getSeverityString() << ","
            << issue.reporter << ","
            << issue.assignee << ","
            << issue.component << ","
            << issue.version << ","
            << issue.targetVersion << ","
            << issue.createdDate.toString("yyyy-MM-dd") << ","
            << issue.updatedDate.toString("yyyy-MM-dd") << ","
            << (issue.resolvedDate.isValid() ? issue.resolvedDate.toString("yyyy-MM-dd") : "") << ","
            << issue.estimatedHours << ","
            << issue.actualHours << ","
            << "\"" << QString(issue.description).replace("\"", "\"\"") << "\"\n";
    }
}

void FeatureBugTrackerDialog::importFromCSV() {
    QString filePath = QFileDialog::getOpenFileName(this, "Import from CSV", "", "CSV Files (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to read CSV file.");
        return;
    }

    QTextStream in(&file);
    QString header = in.readLine(); // Skip header

    int importedCount = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = parseCSVLine(line);

        if (fields.size() >= 16) {
            Issue issue;
            issue.title = fields[1];
            issue.type = stringToIssueType(fields[2]);
            issue.status = stringToIssueStatus(fields[3]);
            issue.priority = stringToIssuePriority(fields[4]);
            issue.severity = stringToIssueSeverity(fields[5]);
            issue.reporter = fields[6];
            issue.assignee = fields[7];
            issue.component = fields[8];
            issue.version = fields[9];
            issue.targetVersion = fields[10];
            issue.createdDate = QDateTime::fromString(fields[11], "yyyy-MM-dd");
            issue.updatedDate = QDateTime::fromString(fields[12], "yyyy-MM-dd");
            if (!fields[13].isEmpty()) {
                issue.resolvedDate = QDateTime::fromString(fields[13], "yyyy-MM-dd");
            }
            issue.estimatedHours = fields[14].toInt();
            issue.actualHours = fields[15].toInt();
            issue.description = fields[16];

            m_issues.append(issue);
            importedCount++;
        }
    }

    updateIssueList();
    setModified(true);
    updateStatistics();

    QMessageBox::information(this, "Import Complete", QString("Imported %1 issues.").arg(importedCount));
}

void FeatureBugTrackerDialog::showStatistics() {
    QString stats = generateStatisticsReport();
    QMessageBox::information(this, "Project Statistics", stats);
}

// Slot implementations
void FeatureBugTrackerDialog::onNewProject() { newProject(); }
void FeatureBugTrackerDialog::onOpenProject() { openProject(); }
void FeatureBugTrackerDialog::onSaveProject() { saveProject(); }
void FeatureBugTrackerDialog::onSaveProjectAs() { saveProjectAs(); }
void FeatureBugTrackerDialog::onGenerateReport() { generateReport(); }
void FeatureBugTrackerDialog::onExportCSV() { exportToCSV(); }
void FeatureBugTrackerDialog::onImportCSV() { importFromCSV(); }
void FeatureBugTrackerDialog::onShowStatistics() { showStatistics(); }
void FeatureBugTrackerDialog::onAddFeature() { addFeature(); }
void FeatureBugTrackerDialog::onAddBug() { addBug(); }
void FeatureBugTrackerDialog::onAddEnhancement() { addEnhancement(); }
void FeatureBugTrackerDialog::onEditIssue() { editIssue(); }
void FeatureBugTrackerDialog::onDeleteIssue() { deleteIssue(); }
void FeatureBugTrackerDialog::onDuplicateIssue() { duplicateIssue(); }
void FeatureBugTrackerDialog::onResolveIssue() { resolveIssue(); }
void FeatureBugTrackerDialog::onCloseIssue() { closeIssue(); }
void FeatureBugTrackerDialog::onReopenIssue() { reopenIssue(); }

void FeatureBugTrackerDialog::onIssueSelectionChanged() {
    updateIssueDetails();
}

void FeatureBugTrackerDialog::onIssueDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        m_issueTree->editItem(item, 0);
    }
}

void FeatureBugTrackerDialog::onIssueItemChanged(QTreeWidgetItem* item, int column) {
    if (column == 0) { // Title changed
        IssueItem* issueItem = dynamic_cast<IssueItem*>(item);
        if (issueItem) {
            Issue* issue = findIssue(issueItem->getIssue().id);
            if (issue) {
                issue->title = item->text(0);
                setModified(true);
                updateIssueDetails(); // Refresh the details panel
            }
        }
    }
}

void FeatureBugTrackerDialog::onFilterChanged() {
    updateIssueList();
}

void FeatureBugTrackerDialog::onSortChanged() {
    int sortColumn = m_sortCombo->currentIndex();
    m_issueTree->sortItems(sortColumn, Qt::AscendingOrder);
}

void FeatureBugTrackerDialog::onShowClosedToggled(bool show) {
    Q_UNUSED(show);
    updateIssueList();
}

void FeatureBugTrackerDialog::onTabChanged(int index) {
    Q_UNUSED(index);
    // Could be used to update specific tab content
}

void FeatureBugTrackerDialog::updateProgress() {
    updateStatistics();
}

void FeatureBugTrackerDialog::updateIssueList() {
    m_issueTree->clear();

    for (const Issue& issue : m_issues) {
        if (shouldShowIssue(issue)) {
            addIssueToTree(issue);
        }
    }
}

void FeatureBugTrackerDialog::updateIssueDetails() {
    IssueItem* currentItem = dynamic_cast<IssueItem*>(m_issueTree->currentItem());
    if (!currentItem) {
        // Clear details
        m_titleEdit->clear();
        m_descriptionEdit->clear();
        m_typeCombo->setCurrentIndex(0);
        m_statusCombo->setCurrentIndex(0);
        m_priorityCombo->setCurrentIndex(2);
        m_severityCombo->setCurrentIndex(1);
        m_implementationCombo->setCurrentIndex(0);
        m_reporterEdit->clear();
        m_assigneeEdit->clear();
        m_componentEdit->clear();
        m_versionEdit->clear();
        m_targetVersionEdit->clear();
        m_tagsEdit->clear();
        m_reproductionStepsEdit->clear();
        m_expectedBehaviorEdit->clear();
        m_actualBehaviorEdit->clear();
        m_environmentEdit->clear();
        m_estimatedHoursSpinBox->setValue(0);
        m_actualHoursSpinBox->setValue(0);
        m_timeVarianceLabel->clear();
        m_timeProgressBar->setValue(0);
        m_commentsEdit->clear();
        m_createdLabel->clear();
        m_updatedLabel->clear();
        m_resolvedLabel->clear();
        m_daysOpenLabel->clear();
        return;
    }

    Issue issue = currentItem->getIssue();

    m_titleEdit->setText(issue.title);
    m_descriptionEdit->setPlainText(issue.description);
    m_typeCombo->setCurrentIndex(static_cast<int>(issue.type));
    m_statusCombo->setCurrentIndex(static_cast<int>(issue.status));
    m_priorityCombo->setCurrentIndex(static_cast<int>(issue.priority));
    m_severityCombo->setCurrentIndex(static_cast<int>(issue.severity));
    m_implementationCombo->setCurrentIndex(static_cast<int>(issue.implementationLevel));
    m_reporterEdit->setText(issue.reporter);
    m_assigneeEdit->setText(issue.assignee);
    m_componentEdit->setText(issue.component);
    m_versionEdit->setText(issue.version);
    m_targetVersionEdit->setText(issue.targetVersion);
    m_tagsEdit->setText(issue.tags.join(", "));
    m_reproductionStepsEdit->setPlainText(issue.reproductionSteps);
    m_expectedBehaviorEdit->setPlainText(issue.expectedBehavior);
    m_actualBehaviorEdit->setPlainText(issue.actualBehavior);
    m_environmentEdit->setText(issue.environment);
    m_estimatedHoursSpinBox->setValue(issue.estimatedHours);
    m_actualHoursSpinBox->setValue(issue.actualHours);

    // Time tracking
    if (issue.estimatedHours > 0) {
        int progress = (issue.actualHours * 100) / issue.estimatedHours;
        m_timeProgressBar->setValue(qMin(progress, 100));

        int variance = issue.actualHours - issue.estimatedHours;
        if (variance > 0) {
            m_timeVarianceLabel->setText(QString("Over by %1 hours").arg(variance));
            m_timeVarianceLabel->setStyleSheet("color: red;");
        } else if (variance < 0) {
            m_timeVarianceLabel->setText(QString("Under by %1 hours").arg(-variance));
            m_timeVarianceLabel->setStyleSheet("color: green;");
        } else {
            m_timeVarianceLabel->setText("On target");
            m_timeVarianceLabel->setStyleSheet("color: blue;");
        }
    } else {
        m_timeProgressBar->setValue(0);
        m_timeVarianceLabel->clear();
    }

    // Comments
    m_commentsEdit->setPlainText(issue.comments.join("\n"));

    // Status labels
    m_createdLabel->setText("Created: " + issue.createdDate.toString("yyyy-MM-dd hh:mm"));
    m_updatedLabel->setText("Updated: " + issue.updatedDate.toString("yyyy-MM-dd hh:mm"));
    if (issue.resolvedDate.isValid()) {
        m_resolvedLabel->setText("Resolved: " + issue.resolvedDate.toString("yyyy-MM-dd hh:mm"));
    } else {
        m_resolvedLabel->setText("Not resolved");
    }
    m_daysOpenLabel->setText(QString("Days open: %1").arg(issue.getDaysOpen()));
}

void FeatureBugTrackerDialog::updateWindowTitle() {
    QString title = "Feature/Bug Tracker";
    if (!m_projectName.isEmpty()) {
        title += " - " + m_projectName;
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void FeatureBugTrackerDialog::updateStatistics() {
    int totalIssues = m_issues.size();
    int featureCount = 0;
    int bugCount = 0;
    int enhancementCount = 0;
    int taskCount = 0;
    int openCount = 0;
    int resolvedCount = 0;
    int closedCount = 0;

    for (const Issue& issue : m_issues) {
        switch (issue.type) {
            case IssueType::Feature: featureCount++; break;
            case IssueType::Bug: bugCount++; break;
            case IssueType::Enhancement: enhancementCount++; break;
            case IssueType::Task: taskCount++; break;
        }

        switch (issue.status) {
            case IssueStatus::Open:
            case IssueStatus::InProgress:
            case IssueStatus::Testing:
            case IssueStatus::Reopened:
                openCount++;
                break;
            case IssueStatus::Resolved:
                resolvedCount++;
                break;
            case IssueStatus::Closed:
                closedCount++;
                break;
        }
    }

    m_statsLabel->setText(QString("%1 issues (%2 open, %3 resolved, %4 closed)")
        .arg(totalIssues).arg(openCount).arg(resolvedCount).arg(closedCount));

    m_featureCountLabel->setText(QString("Features: %1").arg(featureCount));
    m_bugCountLabel->setText(QString("Bugs: %1").arg(bugCount));

    int completionProgress = totalIssues > 0 ? ((resolvedCount + closedCount) * 100) / totalIssues : 0;
    m_completionProgressBar->setValue(completionProgress);
}

bool FeatureBugTrackerDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool FeatureBugTrackerDialog::promptSaveChanges() {
    QMessageBox::StandardButton result = QMessageBox::question(this,
        "Unsaved Changes",
        "The project has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (result == QMessageBox::Save) {
        saveProject();
        return !m_modified; // Return true if save was successful
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void FeatureBugTrackerDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

Issue* FeatureBugTrackerDialog::findIssue(const QString& id) {
    for (Issue& issue : m_issues) {
        if (issue.id == id) {
            return &issue;
        }
    }
    return nullptr;
}

IssueItem* FeatureBugTrackerDialog::findIssueItem(const QString& id) {
    for (int i = 0; i < m_issueTree->topLevelItemCount(); ++i) {
        IssueItem* item = dynamic_cast<IssueItem*>(m_issueTree->topLevelItem(i));
        if (item && item->getIssue().id == id) {
            return item;
        }
    }
    return nullptr;
}

void FeatureBugTrackerDialog::addIssueToTree(const Issue& issue) {
    IssueItem* item = new IssueItem(issue, m_issueTree);
    Q_UNUSED(item);
}

void FeatureBugTrackerDialog::removeIssueFromTree(const QString& id) {
    // Remove from issues list
    m_issues.erase(std::remove_if(m_issues.begin(), m_issues.end(),
        [id](const Issue& issue) { return issue.id == id; }), m_issues.end());

    // Remove from tree widget
    IssueItem* item = findIssueItem(id);
    if (item) {
        delete item;
    }
}

bool FeatureBugTrackerDialog::shouldShowIssue(const Issue& issue) {
    // Apply filters
    if (!m_showClosedCheck->isChecked() && (issue.status == IssueStatus::Closed || issue.status == IssueStatus::Resolved)) {
        return false;
    }

    if (m_typeFilter->currentIndex() > 0) {
        IssueType filterType = static_cast<IssueType>(m_typeFilter->currentIndex() - 1);
        if (issue.type != filterType) {
            return false;
        }
    }

    if (m_statusFilter->currentIndex() > 0) {
        IssueStatus filterStatus = static_cast<IssueStatus>(m_statusFilter->currentIndex() - 1);
        if (issue.status != filterStatus) {
            return false;
        }
    }

    if (m_priorityFilter->currentIndex() > 0) {
        IssuePriority filterPriority = static_cast<IssuePriority>(m_priorityFilter->currentIndex() - 1);
        if (issue.priority != filterPriority) {
            return false;
        }
    }

    if (m_severityFilter->currentIndex() > 0) {
        IssueSeverity filterSeverity = static_cast<IssueSeverity>(m_severityFilter->currentIndex() - 1);
        if (issue.severity != filterSeverity) {
            return false;
        }
    }

    if (!m_searchFilter->text().isEmpty()) {
        QString searchText = m_searchFilter->text().toLower();
        if (!issue.title.toLower().contains(searchText) &&
            !issue.description.toLower().contains(searchText) &&
            !issue.assignee.toLower().contains(searchText) &&
            !issue.component.toLower().contains(searchText)) {
            return false;
        }
    }

    return true;
}

void FeatureBugTrackerDialog::loadSettings() {
    restoreGeometry(m_settings->value("geometry").toByteArray());

    if (m_mainSplitter) {
        m_mainSplitter->restoreState(m_settings->value("splitterState").toByteArray());
    }

    m_showClosedCheck->setChecked(m_settings->value("showClosed", false).toBool());
    m_typeFilter->setCurrentIndex(m_settings->value("typeFilter", 0).toInt());
    m_statusFilter->setCurrentIndex(m_settings->value("statusFilter", 0).toInt());
    m_priorityFilter->setCurrentIndex(m_settings->value("priorityFilter", 0).toInt());
    m_severityFilter->setCurrentIndex(m_settings->value("severityFilter", 0).toInt());
    m_sortCombo->setCurrentIndex(m_settings->value("sortBy", 0).toInt());
}

void FeatureBugTrackerDialog::saveSettings() {
    m_settings->setValue("geometry", saveGeometry());

    if (m_mainSplitter) {
        m_settings->setValue("splitterState", m_mainSplitter->saveState());
    }

    m_settings->setValue("showClosed", m_showClosedCheck->isChecked());
    m_settings->setValue("typeFilter", m_typeFilter->currentIndex());
    m_settings->setValue("statusFilter", m_statusFilter->currentIndex());
    m_settings->setValue("priorityFilter", m_priorityFilter->currentIndex());
    m_settings->setValue("severityFilter", m_severityFilter->currentIndex());
    m_settings->setValue("sortBy", m_sortCombo->currentIndex());
}

bool FeatureBugTrackerDialog::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray issuesArray = root["issues"].toArray();

    m_issues.clear();
    for (const QJsonValue& value : issuesArray) {
        Issue issue;
        issue.fromJson(value.toObject());
        m_issues.append(issue);
    }

    return true;
}

bool FeatureBugTrackerDialog::saveToFile(const QString& filePath) {
    QJsonObject root;
    QJsonArray issuesArray;

    for (const Issue& issue : m_issues) {
        issuesArray.append(issue.toJson());
    }

    root["issues"] = issuesArray;
    root["version"] = "1.0";
    root["projectName"] = m_projectName;
    root["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

void FeatureBugTrackerDialog::closeEvent(QCloseEvent* event) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

// Helper methods for CSV import
QStringList FeatureBugTrackerDialog::parseCSVLine(const QString& line) {
    QStringList result;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                current += '"';
                ++i; // Skip next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            result.append(current);
            current.clear();
        } else {
            current += c;
        }
    }

    result.append(current);
    return result;
}

IssueType FeatureBugTrackerDialog::stringToIssueType(const QString& str) {
    if (str == "Bug") return IssueType::Bug;
    if (str == "Enhancement") return IssueType::Enhancement;
    if (str == "Task") return IssueType::Task;
    return IssueType::Feature;
}

IssueStatus FeatureBugTrackerDialog::stringToIssueStatus(const QString& str) {
    if (str == "In Progress") return IssueStatus::InProgress;
    if (str == "Testing") return IssueStatus::Testing;
    if (str == "Resolved") return IssueStatus::Resolved;
    if (str == "Closed") return IssueStatus::Closed;
    if (str == "Reopened") return IssueStatus::Reopened;
    return IssueStatus::Open;
}

IssuePriority FeatureBugTrackerDialog::stringToIssuePriority(const QString& str) {
    if (str == "Trivial") return IssuePriority::Trivial;
    if (str == "Minor") return IssuePriority::Minor;
    if (str == "Critical") return IssuePriority::Critical;
    if (str == "Blocker") return IssuePriority::Blocker;
    return IssuePriority::Major;
}

IssueSeverity FeatureBugTrackerDialog::stringToIssueSeverity(const QString& str) {
    if (str == "Low") return IssueSeverity::Low;
    if (str == "High") return IssueSeverity::High;
    if (str == "Critical") return IssueSeverity::Critical;
    return IssueSeverity::Medium;
}

void FeatureBugTrackerDialog::generateHTMLReport(QTextStream& out) {
    out << "<!DOCTYPE html>\n<html>\n<head>\n<title>Issue Tracker Report</title>\n";
    out << "<style>\nbody { font-family: Arial, sans-serif; }\n";
    out << "table { border-collapse: collapse; width: 100%; }\n";
    out << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    out << "th { background-color: #f2f2f2; }\n";
    out << ".feature { color: blue; }\n.bug { color: red; }\n.enhancement { color: green; }\n.task { color: orange; }\n";
    out << "</style>\n</head>\n<body>\n";
    out << "<h1>Issue Tracker Report</h1>\n";
    out << "<p>Generated on: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm") << "</p>\n";

    // Statistics
    out << "<h2>Statistics</h2>\n";
    out << "<p>Total Issues: " << m_issues.size() << "</p>\n";

    // Issues table
    out << "<h2>Issues</h2>\n";
    out << "<table>\n<tr><th>Title</th><th>Type</th><th>Status</th><th>Priority</th><th>Assignee</th><th>Created</th></tr>\n";

    for (const Issue& issue : m_issues) {
        QString typeClass = issue.getTypeString().toLower();
        out << "<tr><td>" << issue.title << "</td>";
        out << "<td class=\"" << typeClass << "\">" << issue.getTypeString() << "</td>";
        out << "<td>" << issue.getStatusString() << "</td>";
        out << "<td>" << issue.getPriorityString() << "</td>";
        out << "<td>" << issue.assignee << "</td>";
        out << "<td>" << issue.createdDate.toString("yyyy-MM-dd") << "</td></tr>\n";
    }

    out << "</table>\n</body>\n</html>\n";
}

void FeatureBugTrackerDialog::generateTextReport(QTextStream& out) {
    out << "Issue Tracker Report\n";
    out << "===================\n\n";
    out << "Generated on: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm") << "\n\n";

    out << "Statistics:\n";
    out << "-----------\n";
    out << "Total Issues: " << m_issues.size() << "\n\n";

    out << "Issues:\n";
    out << "-------\n";

    for (const Issue& issue : m_issues) {
        out << "Title: " << issue.title << "\n";
        out << "Type: " << issue.getTypeString() << "\n";
        out << "Status: " << issue.getStatusString() << "\n";
        out << "Priority: " << issue.getPriorityString() << "\n";
        out << "Assignee: " << issue.assignee << "\n";
        out << "Created: " << issue.createdDate.toString("yyyy-MM-dd") << "\n";
        if (!issue.description.isEmpty()) {
            out << "Description: " << issue.description << "\n";
        }
        out << "\n";
    }
}

QString FeatureBugTrackerDialog::generateStatisticsReport() {
    int totalIssues = m_issues.size();
    int featureCount = 0, bugCount = 0, enhancementCount = 0, taskCount = 0;
    int openCount = 0, resolvedCount = 0, closedCount = 0;

    for (const Issue& issue : m_issues) {
        switch (issue.type) {
            case IssueType::Feature: featureCount++; break;
            case IssueType::Bug: bugCount++; break;
            case IssueType::Enhancement: enhancementCount++; break;
            case IssueType::Task: taskCount++; break;
        }

        switch (issue.status) {
            case IssueStatus::Open:
            case IssueStatus::InProgress:
            case IssueStatus::Testing:
            case IssueStatus::Reopened:
                openCount++;
                break;
            case IssueStatus::Resolved:
                resolvedCount++;
                break;
            case IssueStatus::Closed:
                closedCount++;
                break;
        }
    }

    QString stats;
    stats += QString("Total Issues: %1\n\n").arg(totalIssues);
    stats += QString("By Type:\n");
    stats += QString("  Features: %1\n").arg(featureCount);
    stats += QString("  Bugs: %1\n").arg(bugCount);
    stats += QString("  Enhancements: %1\n").arg(enhancementCount);
    stats += QString("  Tasks: %1\n\n").arg(taskCount);
    stats += QString("By Status:\n");
    stats += QString("  Open: %1\n").arg(openCount);
    stats += QString("  Resolved: %1\n").arg(resolvedCount);
    stats += QString("  Closed: %1\n").arg(closedCount);

    if (totalIssues > 0) {
        int completionRate = ((resolvedCount + closedCount) * 100) / totalIssues;
        stats += QString("\nCompletion Rate: %1%").arg(completionRate);
    }

    return stats;
}


