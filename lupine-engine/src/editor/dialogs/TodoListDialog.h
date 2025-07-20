#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDateEdit>
#include <QProgressBar>
#include <QSpinBox>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDateTime>
#include <QTimer>

enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

enum class TaskStatus {
    NotStarted = 0,
    InProgress = 1,
    Completed = 2,
    Cancelled = 3,
    OnHold = 4
};

/**
 * @brief Represents a single task in the todo list
 */
class TodoTask
{
public:
    TodoTask();
    TodoTask(const QString& title, const QString& description = QString());
    
    // Properties
    QString id;
    QString title;
    QString description;
    TaskPriority priority;
    TaskStatus status;
    QDateTime createdDate;
    QDateTime dueDate;
    QDateTime completedDate;
    QString assignee;
    QStringList tags;
    int progress; // 0-100
    
    // Hierarchy
    QString parentId;
    QStringList childIds;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Utility
    QString getPriorityString() const;
    QString getStatusString() const;
    QColor getStatusColor() const;
    bool isOverdue() const;
};

/**
 * @brief Custom tree widget item for todo tasks
 */
class TodoTaskItem : public QTreeWidgetItem
{
public:
    explicit TodoTaskItem(const TodoTask& task, QTreeWidget* parent = nullptr);
    explicit TodoTaskItem(const TodoTask& task, QTreeWidgetItem* parent = nullptr);
    
    void updateFromTask(const TodoTask& task);
    TodoTask getTask() const { return m_task; }
    void setTask(const TodoTask& task) { m_task = task; updateFromTask(task); }
    
private:
    void setupItem();
    
    TodoTask m_task;
};

/**
 * @brief Todo list management dialog
 */
class TodoListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TodoListDialog(QWidget* parent = nullptr);
    ~TodoListDialog();

    // File operations
    void newTodoList();
    void openTodoList();
    void saveTodoList();
    void saveTodoListAs();
    
    // Task operations
    void addTask();
    void addSubTask();
    void editTask();
    void deleteTask();
    void markTaskCompleted();
    void markTaskInProgress();
    void moveTaskUp();
    void moveTaskDown();
    
    // List management
    void clearCompletedTasks();
    void exportToText();
    void importFromText();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewTodoList();
    void onOpenTodoList();
    void onSaveTodoList();
    void onSaveTodoListAs();
    void onExportText();
    void onImportText();
    
    void onAddTask();
    void onAddSubTask();
    void onEditTask();
    void onDeleteTask();
    void onMarkCompleted();
    void onMarkInProgress();
    void onMoveUp();
    void onMoveDown();
    void onClearCompleted();
    
    void onTaskSelectionChanged();
    void onTaskDoubleClicked(QTreeWidgetItem* item, int column);
    void onTaskChanged(QTreeWidgetItem* item, int column);
    void onProgressChanged(int value);

    void onFilterChanged();
    void onSortChanged();
    void onShowCompletedToggled(bool show);

    void updateProgress();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupTaskList();
    void setupTaskDetails();
    void setupFilters();
    void setupStatusBar();
    
    void updateTaskList();
    void updateTaskDetails();
    void updateWindowTitle();
    void updateStatistics();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    TodoTask* findTask(const QString& id);
    TodoTaskItem* findTaskItem(const QString& id);
    TodoTaskItem* findTaskItemRecursive(QTreeWidgetItem* parent, const QString& id);
    void addTaskToTree(const TodoTask& task, QTreeWidgetItem* parent = nullptr);
    void removeTaskFromTree(const QString& id);
    bool shouldShowTask(const TodoTask& task);
    void writeTaskToText(QTextStream& out, const TodoTask& task, int indent);
    
    void loadSettings();
    void saveSettings();
    
    // File management
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // Task list
    QTreeWidget* m_taskTree;
    QWidget* m_filterWidget;
    QComboBox* m_statusFilter;
    QComboBox* m_priorityFilter;
    QLineEdit* m_searchFilter;
    QCheckBox* m_showCompletedCheck;
    QComboBox* m_sortCombo;
    
    // Task details
    QWidget* m_detailsWidget;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QComboBox* m_priorityCombo;
    QComboBox* m_statusCombo;
    QDateEdit* m_dueDateEdit;
    QLineEdit* m_assigneeEdit;
    QLineEdit* m_tagsEdit;
    QSpinBox* m_progressSpin;
    QProgressBar* m_progressBar;
    QLabel* m_createdLabel;
    QLabel* m_completedLabel;
    
    // Status bar
    QLabel* m_statsLabel;
    QProgressBar* m_overallProgressBar;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_importAction;
    QAction* m_exitAction;
    
    QAction* m_addTaskAction;
    QAction* m_addSubTaskAction;
    QAction* m_editTaskAction;
    QAction* m_deleteTaskAction;
    QAction* m_markCompletedAction;
    QAction* m_markInProgressAction;
    QAction* m_moveUpAction;
    QAction* m_moveDownAction;
    QAction* m_clearCompletedAction;
    
    // Data
    QList<TodoTask> m_tasks;
    QString m_currentFilePath;
    bool m_modified;
    
    // Settings
    QSettings* m_settings;
    
    // Update timer
    QTimer* m_updateTimer;
};
