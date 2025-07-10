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
#include <QCalendarWidget>
#include <QGroupBox>
#include <QSlider>

enum class MilestoneStatus {
    NotStarted = 0,
    InProgress = 1,
    Completed = 2,
    Delayed = 3,
    Cancelled = 4
};

enum class MilestonePriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Represents a project milestone
 */
class Milestone
{
public:
    Milestone();
    Milestone(const QString& title, const QString& description = QString());
    
    // Properties
    QString id;
    QString title;
    QString description;
    MilestoneStatus status;
    MilestonePriority priority;
    QDateTime createdDate;
    QDateTime startDate;
    QDateTime targetDate;
    QDateTime completedDate;
    QString owner;
    QStringList dependencies; // IDs of other milestones
    QStringList deliverables;
    QStringList tags;
    int progress; // 0-100
    double budget;
    double actualCost;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Utility
    QString getStatusString() const;
    QString getPriorityString() const;
    QColor getStatusColor() const;
    bool isOverdue() const;
    int getDaysRemaining() const;
    double getBudgetVariance() const;
};

/**
 * @brief Custom tree widget item for milestones
 */
class MilestoneItem : public QTreeWidgetItem
{
public:
    explicit MilestoneItem(const Milestone& milestone, QTreeWidget* parent = nullptr);
    
    void updateFromMilestone(const Milestone& milestone);
    Milestone getMilestone() const { return m_milestone; }
    void setMilestone(const Milestone& milestone) { m_milestone = milestone; updateFromMilestone(milestone); }
    
private:
    void setupItem();
    
    Milestone m_milestone;
};

/**
 * @brief Milestone tracking and project management dialog
 */
class MilestoneTrackerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MilestoneTrackerDialog(QWidget* parent = nullptr);
    ~MilestoneTrackerDialog();

    // File operations
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    
    // Milestone operations
    void addMilestone();
    void editMilestone();
    void deleteMilestone();
    void markMilestoneCompleted();
    void markMilestoneInProgress();
    void duplicateMilestone();
    
    // Project management
    void generateReport();
    void exportToCSV();
    void importFromCSV();
    void showGanttChart();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onGenerateReport();
    void onExportCSV();
    void onImportCSV();
    void onShowGantt();
    
    void onAddMilestone();
    void onEditMilestone();
    void onDeleteMilestone();
    void onMarkCompleted();
    void onMarkInProgress();
    void onDuplicateMilestone();
    
    void onMilestoneSelectionChanged();
    void onMilestoneDoubleClicked(QTreeWidgetItem* item, int column);
    void onMilestoneItemChanged(QTreeWidgetItem* item, int column);
    
    void onFilterChanged();
    void onSortChanged();
    void onShowCompletedToggled(bool show);
    void onViewModeChanged();
    
    void updateProgress();
    void updateTimeline();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMilestoneList();
    void setupMilestoneDetails();
    void setupTimelineView();
    void setupFilters();
    void setupStatusBar();
    
    void updateMilestoneList();
    void updateMilestoneDetails();
    void updateWindowTitle();
    void updateStatistics();
    void updateDependencyGraph();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    Milestone* findMilestone(const QString& id);
    MilestoneItem* findMilestoneItem(const QString& id);
    void addMilestoneToTree(const Milestone& milestone);
    void removeMilestoneFromTree(const QString& id);
    
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
    QSplitter* m_rightSplitter;
    
    // Milestone list
    QWidget* m_leftWidget;
    QTreeWidget* m_milestoneTree;
    QWidget* m_filterWidget;
    QComboBox* m_statusFilter;
    QComboBox* m_priorityFilter;
    QLineEdit* m_searchFilter;
    QCheckBox* m_showCompletedCheck;
    QComboBox* m_sortCombo;
    QComboBox* m_viewModeCombo;
    
    // Milestone details
    QWidget* m_detailsWidget;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QComboBox* m_statusCombo;
    QComboBox* m_priorityCombo;
    QDateEdit* m_startDateEdit;
    QDateEdit* m_targetDateEdit;
    QLineEdit* m_ownerEdit;
    QTextEdit* m_deliverablesEdit;
    QLineEdit* m_tagsEdit;
    QSlider* m_progressSlider;
    QLabel* m_progressLabel;
    QLineEdit* m_budgetEdit;
    QLineEdit* m_actualCostEdit;
    QLabel* m_budgetVarianceLabel;
    QLabel* m_createdLabel;
    QLabel* m_completedLabel;
    QLabel* m_daysRemainingLabel;
    
    // Timeline view
    QWidget* m_timelineWidget;
    QCalendarWidget* m_calendar;
    QTextEdit* m_timelineDetails;
    
    // Status bar
    QLabel* m_statsLabel;
    QProgressBar* m_overallProgressBar;
    QLabel* m_budgetLabel;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_generateReportAction;
    QAction* m_exportAction;
    QAction* m_importAction;
    QAction* m_ganttAction;
    QAction* m_exitAction;
    
    QAction* m_addMilestoneAction;
    QAction* m_editMilestoneAction;
    QAction* m_deleteMilestoneAction;
    QAction* m_markCompletedAction;
    QAction* m_markInProgressAction;
    QAction* m_duplicateAction;
    
    // Data
    QList<Milestone> m_milestones;
    QString m_currentFilePath;
    QString m_projectName;
    bool m_modified;
    
    // Settings
    QSettings* m_settings;
    
    // Update timer
    QTimer* m_updateTimer;
};
