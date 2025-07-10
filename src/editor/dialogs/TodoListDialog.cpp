#include "TodoListDialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QHeaderView>
#include <QUuid>
#include <QTextStream>

// TodoTask Implementation
TodoTask::TodoTask()
    : id(QUuid::createUuid().toString())
    , title("New Task")
    , priority(TaskPriority::Normal)
    , status(TaskStatus::NotStarted)
    , createdDate(QDateTime::currentDateTime())
    , progress(0)
{
}

TodoTask::TodoTask(const QString& title, const QString& description)
    : id(QUuid::createUuid().toString())
    , title(title)
    , description(description)
    , priority(TaskPriority::Normal)
    , status(TaskStatus::NotStarted)
    , createdDate(QDateTime::currentDateTime())
    , progress(0)
{
}

QJsonObject TodoTask::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["description"] = description;
    obj["priority"] = static_cast<int>(priority);
    obj["status"] = static_cast<int>(status);
    obj["createdDate"] = createdDate.toString(Qt::ISODate);
    obj["dueDate"] = dueDate.toString(Qt::ISODate);
    obj["completedDate"] = completedDate.toString(Qt::ISODate);
    obj["assignee"] = assignee;
    obj["progress"] = progress;
    obj["parentId"] = parentId;
    
    QJsonArray tagsArray;
    for (const QString& tag : tags) {
        tagsArray.append(tag);
    }
    obj["tags"] = tagsArray;
    
    QJsonArray childArray;
    for (const QString& childId : childIds) {
        childArray.append(childId);
    }
    obj["childIds"] = childArray;
    
    return obj;
}

void TodoTask::fromJson(const QJsonObject& json) {
    id = json["id"].toString();
    title = json["title"].toString();
    description = json["description"].toString();
    priority = static_cast<TaskPriority>(json["priority"].toInt());
    status = static_cast<TaskStatus>(json["status"].toInt());
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    dueDate = QDateTime::fromString(json["dueDate"].toString(), Qt::ISODate);
    completedDate = QDateTime::fromString(json["completedDate"].toString(), Qt::ISODate);
    assignee = json["assignee"].toString();
    progress = json["progress"].toInt();
    parentId = json["parentId"].toString();
    
    tags.clear();
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& value : tagsArray) {
        tags.append(value.toString());
    }
    
    childIds.clear();
    QJsonArray childArray = json["childIds"].toArray();
    for (const QJsonValue& value : childArray) {
        childIds.append(value.toString());
    }
}

QString TodoTask::getPriorityString() const {
    switch (priority) {
        case TaskPriority::Low: return "Low";
        case TaskPriority::Normal: return "Normal";
        case TaskPriority::High: return "High";
        case TaskPriority::Critical: return "Critical";
        default: return "Normal";
    }
}

QString TodoTask::getStatusString() const {
    switch (status) {
        case TaskStatus::NotStarted: return "Not Started";
        case TaskStatus::InProgress: return "In Progress";
        case TaskStatus::Completed: return "Completed";
        case TaskStatus::Cancelled: return "Cancelled";
        case TaskStatus::OnHold: return "On Hold";
        default: return "Not Started";
    }
}

QColor TodoTask::getStatusColor() const {
    switch (status) {
        case TaskStatus::NotStarted: return QColor(128, 128, 128);
        case TaskStatus::InProgress: return QColor(255, 165, 0);
        case TaskStatus::Completed: return QColor(0, 128, 0);
        case TaskStatus::Cancelled: return QColor(255, 0, 0);
        case TaskStatus::OnHold: return QColor(128, 0, 128);
        default: return QColor(128, 128, 128);
    }
}

bool TodoTask::isOverdue() const {
    return dueDate.isValid() && dueDate < QDateTime::currentDateTime() && status != TaskStatus::Completed;
}

// TodoTaskItem Implementation
TodoTaskItem::TodoTaskItem(const TodoTask& task, QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , m_task(task)
{
    setupItem();
}

TodoTaskItem::TodoTaskItem(const TodoTask& task, QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent)
    , m_task(task)
{
    setupItem();
}

void TodoTaskItem::setupItem() {
    updateFromTask(m_task);
}

void TodoTaskItem::updateFromTask(const TodoTask& task) {
    m_task = task;
    
    setText(0, task.title);
    setText(1, task.getPriorityString());
    setText(2, task.getStatusString());
    setText(3, task.dueDate.isValid() ? task.dueDate.toString("yyyy-MM-dd") : "");
    setText(4, task.assignee);
    setText(5, QString::number(task.progress) + "%");
    
    // Set colors based on status
    QColor statusColor = task.getStatusColor();
    for (int i = 0; i < columnCount(); ++i) {
        setForeground(i, QBrush(statusColor));
    }
    
    // Mark overdue tasks
    if (task.isOverdue()) {
        setBackground(0, QBrush(QColor(255, 200, 200)));
    }
    
    // Set checkbox for completion
    setCheckState(0, task.status == TaskStatus::Completed ? Qt::Checked : Qt::Unchecked);
}

// TodoListDialog Implementation
TodoListDialog::TodoListDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_taskTree(nullptr)
    , m_detailsWidget(nullptr)
    , m_modified(false)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
{
    setWindowTitle("Todo List Manager");
    setMinimumSize(1000, 700);
    resize(1400, 900);
    
    m_settings = new QSettings("LupineEngine", "TodoList", this);
    
    setupUI();
    loadSettings();
    
    // Setup update timer
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(60000); // Update every minute
    connect(m_updateTimer, &QTimer::timeout, this, &TodoListDialog::updateProgress);
    m_updateTimer->start();
    
    newTodoList();
}

TodoListDialog::~TodoListDialog() {
    saveSettings();
}

void TodoListDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    setupMenuBar();
    setupToolBar();
    
    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
    
    setupTaskList();
    setupTaskDetails();
    setupStatusBar();
    
    // Connect signals
    connect(m_taskTree, &QTreeWidget::itemSelectionChanged, this, &TodoListDialog::onTaskSelectionChanged);
    connect(m_taskTree, &QTreeWidget::itemDoubleClicked, this, &TodoListDialog::onTaskDoubleClicked);
    connect(m_taskTree, &QTreeWidget::itemChanged, this, &TodoListDialog::onTaskChanged);
}

void TodoListDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_menuBar->setFixedHeight(24);  // Lock menu bar height to 24px
    m_mainLayout->addWidget(m_menuBar);
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Todo List", this, &TodoListDialog::onNewTodoList, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &TodoListDialog::onOpenTodoList, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &TodoListDialog::onSaveTodoList, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &TodoListDialog::onSaveTodoListAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_exportAction = fileMenu->addAction("&Export to Text...", this, &TodoListDialog::onExportText);
    m_importAction = fileMenu->addAction("&Import from Text...", this, &TodoListDialog::onImportText);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close);
    
    // Task menu
    QMenu* taskMenu = m_menuBar->addMenu("&Task");
    m_addTaskAction = taskMenu->addAction("&Add Task", this, &TodoListDialog::onAddTask, QKeySequence("Ctrl+N"));
    m_addSubTaskAction = taskMenu->addAction("Add &Subtask", this, &TodoListDialog::onAddSubTask, QKeySequence("Ctrl+Shift+N"));
    taskMenu->addSeparator();
    m_editTaskAction = taskMenu->addAction("&Edit Task", this, &TodoListDialog::onEditTask, QKeySequence("F2"));
    m_deleteTaskAction = taskMenu->addAction("&Delete Task", this, &TodoListDialog::onDeleteTask, QKeySequence::Delete);
    taskMenu->addSeparator();
    m_markCompletedAction = taskMenu->addAction("Mark &Completed", this, &TodoListDialog::onMarkCompleted, QKeySequence("Ctrl+D"));
    m_markInProgressAction = taskMenu->addAction("Mark &In Progress", this, &TodoListDialog::onMarkInProgress, QKeySequence("Ctrl+P"));
    taskMenu->addSeparator();
    m_moveUpAction = taskMenu->addAction("Move &Up", this, &TodoListDialog::onMoveUp, QKeySequence("Ctrl+Up"));
    m_moveDownAction = taskMenu->addAction("Move &Down", this, &TodoListDialog::onMoveDown, QKeySequence("Ctrl+Down"));
    taskMenu->addSeparator();
    m_clearCompletedAction = taskMenu->addAction("&Clear Completed", this, &TodoListDialog::onClearCompleted);
}

void TodoListDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(26);  // Reduce toolbar height to 26px
    m_toolBar->setIconSize(QSize(16, 16));  // Smaller icons for compact layout
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Task operations
    m_toolBar->addAction(m_addTaskAction);
    m_toolBar->addAction(m_addSubTaskAction);
    m_toolBar->addAction(m_editTaskAction);
    m_toolBar->addAction(m_deleteTaskAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_markCompletedAction);
    m_toolBar->addAction(m_markInProgressAction);
}

void TodoListDialog::setupTaskList() {
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);

    setupFilters();
    leftLayout->addWidget(m_filterWidget);

    // Task tree
    m_taskTree = new QTreeWidget();
    m_taskTree->setHeaderLabels({"Task", "Priority", "Status", "Due Date", "Assignee", "Progress"});
    m_taskTree->setRootIsDecorated(true);
    m_taskTree->setAlternatingRowColors(true);
    m_taskTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskTree->setSortingEnabled(true);

    // Set column widths
    m_taskTree->header()->resizeSection(0, 300);
    m_taskTree->header()->resizeSection(1, 80);
    m_taskTree->header()->resizeSection(2, 100);
    m_taskTree->header()->resizeSection(3, 100);
    m_taskTree->header()->resizeSection(4, 100);
    m_taskTree->header()->resizeSection(5, 80);

    leftLayout->addWidget(m_taskTree);

    m_mainSplitter->addWidget(leftWidget);
    m_mainSplitter->setSizes({600, 400});
}

void TodoListDialog::setupTaskDetails() {
    m_detailsWidget = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsWidget);

    detailsLayout->addWidget(new QLabel("Task Details"));

    // Title
    detailsLayout->addWidget(new QLabel("Title:"));
    m_titleEdit = new QLineEdit();
    detailsLayout->addWidget(m_titleEdit);

    // Description
    detailsLayout->addWidget(new QLabel("Description:"));
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(100);
    detailsLayout->addWidget(m_descriptionEdit);

    // Priority and Status
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel("Priority:"));
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItems({"Low", "Normal", "High", "Critical"});
    statusLayout->addWidget(m_priorityCombo);

    statusLayout->addWidget(new QLabel("Status:"));
    m_statusCombo = new QComboBox();
    m_statusCombo->addItems({"Not Started", "In Progress", "Completed", "Cancelled", "On Hold"});
    statusLayout->addWidget(m_statusCombo);
    detailsLayout->addLayout(statusLayout);

    // Due date and assignee
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Due Date:"));
    m_dueDateEdit = new QDateEdit();
    m_dueDateEdit->setCalendarPopup(true);
    m_dueDateEdit->setDate(QDate::currentDate().addDays(7));
    dateLayout->addWidget(m_dueDateEdit);

    dateLayout->addWidget(new QLabel("Assignee:"));
    m_assigneeEdit = new QLineEdit();
    dateLayout->addWidget(m_assigneeEdit);
    detailsLayout->addLayout(dateLayout);

    // Tags and progress
    detailsLayout->addWidget(new QLabel("Tags (comma separated):"));
    m_tagsEdit = new QLineEdit();
    detailsLayout->addWidget(m_tagsEdit);

    // Progress input and display
    QHBoxLayout* progressLayout = new QHBoxLayout();
    progressLayout->addWidget(new QLabel("Progress:"));
    m_progressSpin = new QSpinBox();
    m_progressSpin->setRange(0, 100);
    m_progressSpin->setSuffix("%");
    m_progressSpin->setMaximumWidth(80);
    progressLayout->addWidget(m_progressSpin);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    progressLayout->addWidget(m_progressBar);
    detailsLayout->addLayout(progressLayout);

    // Timestamps
    m_createdLabel = new QLabel();
    m_completedLabel = new QLabel();
    detailsLayout->addWidget(m_createdLabel);
    detailsLayout->addWidget(m_completedLabel);

    detailsLayout->addStretch();

    // Connect progress spin box to update task progress
    connect(m_progressSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TodoListDialog::onProgressChanged);

    // Update button
    QPushButton* updateButton = new QPushButton("Update Task");
    connect(updateButton, &QPushButton::clicked, this, &TodoListDialog::onEditTask);
    detailsLayout->addWidget(updateButton);

    m_mainSplitter->addWidget(m_detailsWidget);
}

void TodoListDialog::setupFilters() {
    m_filterWidget = new QWidget();
    m_filterWidget->setFixedHeight(50);  // Lock filter bar height to 50px
    QHBoxLayout* filterLayout = new QHBoxLayout(m_filterWidget);

    // Search filter
    filterLayout->addWidget(new QLabel("Search:"));
    m_searchFilter = new QLineEdit();
    m_searchFilter->setPlaceholderText("Search tasks...");
    filterLayout->addWidget(m_searchFilter);

    // Status filter
    filterLayout->addWidget(new QLabel("Status:"));
    m_statusFilter = new QComboBox();
    m_statusFilter->addItems({"All", "Not Started", "In Progress", "Completed", "Cancelled", "On Hold"});
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
    filterLayout->addWidget(new QLabel("Sort by:"));
    m_sortCombo = new QComboBox();
    m_sortCombo->addItems({"Title", "Priority", "Status", "Due Date", "Created Date"});
    filterLayout->addWidget(m_sortCombo);

    // Connect filter signals
    connect(m_searchFilter, &QLineEdit::textChanged, this, &TodoListDialog::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TodoListDialog::onFilterChanged);
    connect(m_priorityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TodoListDialog::onFilterChanged);
    connect(m_showCompletedCheck, &QCheckBox::toggled, this, &TodoListDialog::onShowCompletedToggled);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TodoListDialog::onSortChanged);
}

void TodoListDialog::setupStatusBar() {
    QHBoxLayout* statusLayout = new QHBoxLayout();

    m_statsLabel = new QLabel("0 tasks");
    statusLayout->addWidget(m_statsLabel);

    statusLayout->addStretch();

    statusLayout->addWidget(new QLabel("Overall Progress:"));
    m_overallProgressBar = new QProgressBar();
    m_overallProgressBar->setMaximumWidth(200);
    statusLayout->addWidget(m_overallProgressBar);

    QWidget* statusWidget = new QWidget();
    statusWidget->setLayout(statusLayout);
    statusWidget->setMaximumHeight(30);
    m_mainLayout->addWidget(statusWidget);
}

// Full implementations
void TodoListDialog::newTodoList() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_tasks.clear();
    m_currentFilePath.clear();
    setModified(false);
    updateTaskList();
    updateWindowTitle();
    updateStatistics();
}

void TodoListDialog::openTodoList() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Open Todo List", "", "Todo List Files (*.todo);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            setModified(false);
            updateTaskList();
            updateWindowTitle();
            updateStatistics();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load todo list file.");
        }
    }
}

void TodoListDialog::saveTodoList() {
    if (m_currentFilePath.isEmpty()) {
        saveTodoListAs();
    } else {
        if (saveToFile(m_currentFilePath)) {
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save todo list file.");
        }
    }
}

void TodoListDialog::saveTodoListAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Todo List", "", "Todo List Files (*.todo);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (saveToFile(filePath)) {
            m_currentFilePath = filePath;
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save todo list file.");
        }
    }
}

void TodoListDialog::addTask() {
    TodoTask newTask;
    newTask.title = "New Task";
    newTask.description = "";
    newTask.priority = TaskPriority::Normal;
    newTask.status = TaskStatus::NotStarted;

    m_tasks.append(newTask);
    addTaskToTree(newTask);
    setModified(true);
    updateStatistics();

    // Select the new task
    TodoTaskItem* item = findTaskItem(newTask.id);
    if (item) {
        m_taskTree->setCurrentItem(item);
        m_taskTree->editItem(item, 0); // Start editing the title
    }
}

void TodoListDialog::addSubTask() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) {
        addTask(); // If no parent selected, add as root task
        return;
    }

    TodoTask newTask;
    newTask.title = "New Subtask";
    newTask.description = "";
    newTask.priority = TaskPriority::Normal;
    newTask.status = TaskStatus::NotStarted;
    newTask.parentId = currentItem->getTask().id;

    // Add to parent's children list
    TodoTask* parentTask = findTask(newTask.parentId);
    if (parentTask) {
        parentTask->childIds.append(newTask.id);
    }

    m_tasks.append(newTask);
    addTaskToTree(newTask, currentItem);
    setModified(true);
    updateStatistics();

    // Expand parent and select new task
    currentItem->setExpanded(true);
    TodoTaskItem* item = findTaskItem(newTask.id);
    if (item) {
        m_taskTree->setCurrentItem(item);
        m_taskTree->editItem(item, 0);
    }
}

void TodoListDialog::editTask() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    TodoTask task = currentItem->getTask();

    // Update task from details panel
    task.title = m_titleEdit->text();
    task.description = m_descriptionEdit->toPlainText();
    task.priority = static_cast<TaskPriority>(m_priorityCombo->currentIndex());
    task.status = static_cast<TaskStatus>(m_statusCombo->currentIndex());
    task.dueDate = QDateTime(m_dueDateEdit->date(), QTime());
    task.assignee = m_assigneeEdit->text();
    task.tags = m_tagsEdit->text().split(',', Qt::SkipEmptyParts);

    // Clean up tags
    for (QString& tag : task.tags) {
        tag = tag.trimmed();
    }

    // Update completion date if status changed to completed
    if (task.status == TaskStatus::Completed && !task.completedDate.isValid()) {
        task.completedDate = QDateTime::currentDateTime();
    } else if (task.status != TaskStatus::Completed) {
        task.completedDate = QDateTime();
    }

    // Update the task in the list
    TodoTask* originalTask = findTask(task.id);
    if (originalTask) {
        *originalTask = task;
        currentItem->updateFromTask(task);
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::deleteTask() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    TodoTask task = currentItem->getTask();

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Task",
        QString("Are you sure you want to delete '%1' and all its subtasks?").arg(task.title),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        removeTaskFromTree(task.id);
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::markTaskCompleted() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    TodoTask* task = findTask(currentItem->getTask().id);
    if (task) {
        task->status = TaskStatus::Completed;
        task->completedDate = QDateTime::currentDateTime();
        task->progress = 100;
        currentItem->updateFromTask(*task);
        updateTaskDetails();
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::markTaskInProgress() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    TodoTask* task = findTask(currentItem->getTask().id);
    if (task) {
        task->status = TaskStatus::InProgress;
        task->completedDate = QDateTime();
        if (task->progress == 0) {
            task->progress = 25; // Set some initial progress
        }
        currentItem->updateFromTask(*task);
        updateTaskDetails();
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::moveTaskUp() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    QTreeWidgetItem* parent = currentItem->parent();
    QTreeWidget* tree = currentItem->treeWidget();

    int index;
    if (parent) {
        index = parent->indexOfChild(currentItem);
        if (index > 0) {
            parent->removeChild(currentItem);
            parent->insertChild(index - 1, currentItem);
            tree->setCurrentItem(currentItem);
            setModified(true);
        }
    } else {
        index = tree->indexOfTopLevelItem(currentItem);
        if (index > 0) {
            tree->takeTopLevelItem(index);
            tree->insertTopLevelItem(index - 1, currentItem);
            tree->setCurrentItem(currentItem);
            setModified(true);
        }
    }
}

void TodoListDialog::moveTaskDown() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    QTreeWidgetItem* parent = currentItem->parent();
    QTreeWidget* tree = currentItem->treeWidget();

    int index;
    if (parent) {
        index = parent->indexOfChild(currentItem);
        if (index < parent->childCount() - 1) {
            parent->removeChild(currentItem);
            parent->insertChild(index + 1, currentItem);
            tree->setCurrentItem(currentItem);
            setModified(true);
        }
    } else {
        index = tree->indexOfTopLevelItem(currentItem);
        if (index < tree->topLevelItemCount() - 1) {
            tree->takeTopLevelItem(index);
            tree->insertTopLevelItem(index + 1, currentItem);
            tree->setCurrentItem(currentItem);
            setModified(true);
        }
    }
}

void TodoListDialog::clearCompletedTasks() {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Clear Completed Tasks",
        "Are you sure you want to remove all completed tasks?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Remove completed tasks from the list
        m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
            [](const TodoTask& task) { return task.status == TaskStatus::Completed; }),
            m_tasks.end());

        updateTaskList();
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::exportToText() {
    QString filePath = QFileDialog::getSaveFileName(this, "Export to Text", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to create text file.");
        return;
    }

    QTextStream out(&file);
    out << "Todo List Export\n";
    out << "================\n\n";

    for (const TodoTask& task : m_tasks) {
        if (task.parentId.isEmpty()) { // Only root tasks
            writeTaskToText(out, task, 0);
        }
    }
}

void TodoListDialog::importFromText() {
    QString filePath = QFileDialog::getOpenFileName(this, "Import from Text", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to read text file.");
        return;
    }

    // Simple text import - each line becomes a task
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith("=") && line != "Todo List Export") {
            TodoTask task;
            task.title = line;
            task.status = TaskStatus::NotStarted;
            task.priority = TaskPriority::Normal;
            m_tasks.append(task);
        }
    }

    updateTaskList();
    setModified(true);
    updateStatistics();
}

// Slot implementations
void TodoListDialog::onNewTodoList() { newTodoList(); }
void TodoListDialog::onOpenTodoList() { openTodoList(); }
void TodoListDialog::onSaveTodoList() { saveTodoList(); }
void TodoListDialog::onSaveTodoListAs() { saveTodoListAs(); }
void TodoListDialog::onExportText() { exportToText(); }
void TodoListDialog::onImportText() { importFromText(); }
void TodoListDialog::onAddTask() { addTask(); }
void TodoListDialog::onAddSubTask() { addSubTask(); }
void TodoListDialog::onEditTask() { editTask(); }
void TodoListDialog::onDeleteTask() { deleteTask(); }
void TodoListDialog::onMarkCompleted() { markTaskCompleted(); }
void TodoListDialog::onMarkInProgress() { markTaskInProgress(); }
void TodoListDialog::onMoveUp() { moveTaskUp(); }
void TodoListDialog::onMoveDown() { moveTaskDown(); }
void TodoListDialog::onClearCompleted() { clearCompletedTasks(); }

void TodoListDialog::onTaskSelectionChanged() {
    updateTaskDetails();
}

void TodoListDialog::onTaskDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        m_taskTree->editItem(item, 0);
    }
}

void TodoListDialog::onTaskChanged(QTreeWidgetItem* item, int column) {
    if (column == 0) { // Title changed
        TodoTaskItem* taskItem = dynamic_cast<TodoTaskItem*>(item);
        if (taskItem) {
            TodoTask* task = findTask(taskItem->getTask().id);
            if (task) {
                task->title = item->text(0);
                setModified(true);
            }
        }
    }
}

void TodoListDialog::onProgressChanged(int value) {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) return;

    TodoTask* task = findTask(currentItem->getTask().id);
    if (task) {
        task->progress = value;
        m_progressBar->setValue(value);

        // Update status based on progress
        if (value == 100 && task->status != TaskStatus::Completed) {
            task->status = TaskStatus::Completed;
            task->completedDate = QDateTime::currentDateTime();
        } else if (value > 0 && value < 100 && task->status == TaskStatus::NotStarted) {
            task->status = TaskStatus::InProgress;
        } else if (value == 0 && task->status == TaskStatus::InProgress) {
            task->status = TaskStatus::NotStarted;
            task->completedDate = QDateTime();
        }

        currentItem->updateFromTask(*task);
        updateTaskDetails();
        setModified(true);
        updateStatistics();
    }
}

void TodoListDialog::onFilterChanged() {
    updateTaskList();
}

void TodoListDialog::onSortChanged() {
    int sortColumn = m_sortCombo->currentIndex();
    m_taskTree->sortItems(sortColumn, Qt::AscendingOrder);
}

void TodoListDialog::onShowCompletedToggled(bool show) {
    Q_UNUSED(show);
    updateTaskList();
}

void TodoListDialog::updateProgress() {
    updateStatistics();
}

void TodoListDialog::updateTaskList() {
    m_taskTree->clear();

    // Add root tasks first
    for (const TodoTask& task : m_tasks) {
        if (task.parentId.isEmpty()) {
            if (shouldShowTask(task)) {
                addTaskToTree(task);
            }
        }
    }

    // Expand all items
    m_taskTree->expandAll();
}

void TodoListDialog::updateTaskDetails() {
    TodoTaskItem* currentItem = dynamic_cast<TodoTaskItem*>(m_taskTree->currentItem());
    if (!currentItem) {
        // Clear details
        m_titleEdit->clear();
        m_descriptionEdit->clear();
        m_priorityCombo->setCurrentIndex(0);
        m_statusCombo->setCurrentIndex(0);
        m_dueDateEdit->setDate(QDate::currentDate().addDays(7));
        m_assigneeEdit->clear();
        m_tagsEdit->clear();
        m_progressSpin->setValue(0);
        m_progressBar->setValue(0);
        m_createdLabel->clear();
        m_completedLabel->clear();
        return;
    }

    TodoTask task = currentItem->getTask();

    m_titleEdit->setText(task.title);
    m_descriptionEdit->setPlainText(task.description);
    m_priorityCombo->setCurrentIndex(static_cast<int>(task.priority));
    m_statusCombo->setCurrentIndex(static_cast<int>(task.status));
    m_dueDateEdit->setDate(task.dueDate.isValid() ? task.dueDate.date() : QDate::currentDate().addDays(7));
    m_assigneeEdit->setText(task.assignee);
    m_tagsEdit->setText(task.tags.join(", "));
    m_progressSpin->setValue(task.progress);
    m_progressBar->setValue(task.progress);

    m_createdLabel->setText("Created: " + task.createdDate.toString("yyyy-MM-dd hh:mm"));
    if (task.completedDate.isValid()) {
        m_completedLabel->setText("Completed: " + task.completedDate.toString("yyyy-MM-dd hh:mm"));
    } else {
        m_completedLabel->setText("Not completed");
    }
}

void TodoListDialog::updateWindowTitle() {
    QString title = "Todo List Manager";
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).baseName();
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void TodoListDialog::updateStatistics() {
    int totalTasks = m_tasks.size();
    int completedTasks = 0;
    int inProgressTasks = 0;

    for (const TodoTask& task : m_tasks) {
        if (task.status == TaskStatus::Completed) {
            completedTasks++;
        } else if (task.status == TaskStatus::InProgress) {
            inProgressTasks++;
        }
    }

    m_statsLabel->setText(QString("%1 tasks (%2 completed, %3 in progress)")
        .arg(totalTasks).arg(completedTasks).arg(inProgressTasks));

    int overallProgress = totalTasks > 0 ? (completedTasks * 100) / totalTasks : 0;
    m_overallProgressBar->setValue(overallProgress);
}

bool TodoListDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool TodoListDialog::promptSaveChanges() {
    QMessageBox::StandardButton result = QMessageBox::question(this,
        "Unsaved Changes",
        "The todo list has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (result == QMessageBox::Save) {
        saveTodoList();
        return !m_modified; // Return true if save was successful
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void TodoListDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

TodoTask* TodoListDialog::findTask(const QString& id) {
    for (TodoTask& task : m_tasks) {
        if (task.id == id) {
            return &task;
        }
    }
    return nullptr;
}

TodoTaskItem* TodoListDialog::findTaskItem(const QString& id) {
    for (int i = 0; i < m_taskTree->topLevelItemCount(); ++i) {
        TodoTaskItem* item = findTaskItemRecursive(m_taskTree->topLevelItem(i), id);
        if (item) return item;
    }
    return nullptr;
}

TodoTaskItem* TodoListDialog::findTaskItemRecursive(QTreeWidgetItem* parent, const QString& id) {
    TodoTaskItem* taskItem = dynamic_cast<TodoTaskItem*>(parent);
    if (taskItem && taskItem->getTask().id == id) {
        return taskItem;
    }

    for (int i = 0; i < parent->childCount(); ++i) {
        TodoTaskItem* item = findTaskItemRecursive(parent->child(i), id);
        if (item) return item;
    }

    return nullptr;
}

void TodoListDialog::addTaskToTree(const TodoTask& task, QTreeWidgetItem* parent) {
    TodoTaskItem* item = new TodoTaskItem(task, parent ? nullptr : m_taskTree);
    if (parent) {
        parent->addChild(item);
    }

    // Add child tasks
    for (const QString& childId : task.childIds) {
        TodoTask* childTask = findTask(childId);
        if (childTask) {
            addTaskToTree(*childTask, item);
        }
    }
}

void TodoListDialog::removeTaskFromTree(const QString& id) {
    TodoTask* task = findTask(id);
    if (!task) return;

    // Remove all child tasks first
    for (const QString& childId : task->childIds) {
        removeTaskFromTree(childId);
    }

    // Remove from parent's child list if it has a parent
    if (!task->parentId.isEmpty()) {
        TodoTask* parentTask = findTask(task->parentId);
        if (parentTask) {
            parentTask->childIds.removeAll(id);
        }
    }

    // Remove from tasks list
    m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
        [id](const TodoTask& t) { return t.id == id; }), m_tasks.end());

    // Remove from tree widget
    TodoTaskItem* item = findTaskItem(id);
    if (item) {
        delete item;
    }
}

bool TodoListDialog::shouldShowTask(const TodoTask& task) {
    // Apply filters
    if (!m_showCompletedCheck->isChecked() && task.status == TaskStatus::Completed) {
        return false;
    }

    if (m_statusFilter->currentIndex() > 0) {
        TaskStatus filterStatus = static_cast<TaskStatus>(m_statusFilter->currentIndex() - 1);
        if (task.status != filterStatus) {
            return false;
        }
    }

    if (m_priorityFilter->currentIndex() > 0) {
        TaskPriority filterPriority = static_cast<TaskPriority>(m_priorityFilter->currentIndex() - 1);
        if (task.priority != filterPriority) {
            return false;
        }
    }

    if (!m_searchFilter->text().isEmpty()) {
        QString searchText = m_searchFilter->text().toLower();
        if (!task.title.toLower().contains(searchText) &&
            !task.description.toLower().contains(searchText)) {
            return false;
        }
    }

    return true;
}

void TodoListDialog::writeTaskToText(QTextStream& out, const TodoTask& task, int indent) {
    QString indentStr = QString("  ").repeated(indent);
    QString statusChar = task.status == TaskStatus::Completed ? "✓" : "○";

    out << indentStr << statusChar << " " << task.title;
    if (task.dueDate.isValid()) {
        out << " (Due: " << task.dueDate.toString("yyyy-MM-dd") << ")";
    }
    out << "\n";

    if (!task.description.isEmpty()) {
        out << indentStr << "  " << task.description << "\n";
    }

    // Write child tasks
    for (const QString& childId : task.childIds) {
        TodoTask* childTask = findTask(childId);
        if (childTask) {
            writeTaskToText(out, *childTask, indent + 1);
        }
    }
}

void TodoListDialog::loadSettings() {
    restoreGeometry(m_settings->value("geometry").toByteArray());

    if (m_mainSplitter) {
        m_mainSplitter->restoreState(m_settings->value("splitterState").toByteArray());
    }

    m_showCompletedCheck->setChecked(m_settings->value("showCompleted", true).toBool());
    m_statusFilter->setCurrentIndex(m_settings->value("statusFilter", 0).toInt());
    m_priorityFilter->setCurrentIndex(m_settings->value("priorityFilter", 0).toInt());
    m_sortCombo->setCurrentIndex(m_settings->value("sortBy", 0).toInt());
}

void TodoListDialog::saveSettings() {
    m_settings->setValue("geometry", saveGeometry());

    if (m_mainSplitter) {
        m_settings->setValue("splitterState", m_mainSplitter->saveState());
    }

    m_settings->setValue("showCompleted", m_showCompletedCheck->isChecked());
    m_settings->setValue("statusFilter", m_statusFilter->currentIndex());
    m_settings->setValue("priorityFilter", m_priorityFilter->currentIndex());
    m_settings->setValue("sortBy", m_sortCombo->currentIndex());
}

bool TodoListDialog::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray tasksArray = root["tasks"].toArray();

    m_tasks.clear();
    for (const QJsonValue& value : tasksArray) {
        TodoTask task;
        task.fromJson(value.toObject());
        m_tasks.append(task);
    }

    return true;
}

bool TodoListDialog::saveToFile(const QString& filePath) {
    QJsonObject root;
    QJsonArray tasksArray;

    for (const TodoTask& task : m_tasks) {
        tasksArray.append(task.toJson());
    }

    root["tasks"] = tasksArray;
    root["version"] = "1.0";
    root["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

void TodoListDialog::closeEvent(QCloseEvent* event) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

#include "TodoListDialog.moc"
