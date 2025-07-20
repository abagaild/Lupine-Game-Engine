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
#include <QTabWidget>
#include <QSpinBox>
#include <QSlider>

enum class IssueType {
    Feature = 0,
    Bug = 1,
    Enhancement = 2,
    Task = 3
};

enum class IssueStatus {
    Open = 0,
    InProgress = 1,
    Testing = 2,
    Resolved = 3,
    Closed = 4,
    Reopened = 5
};

enum class IssuePriority {
    Trivial = 0,
    Minor = 1,
    Major = 2,
    Critical = 3,
    Blocker = 4
};

enum class IssueSeverity {
    Low = 0,
    Medium = 1,
    High = 2,
    Critical = 3
};

enum class ImplementationLevel {
    NotStarted = 0,
    Planning = 1,
    InDevelopment = 2,
    CodeReview = 3,
    Testing = 4,
    Documentation = 5,
    Complete = 6
};

/**
 * @brief Represents a feature request or bug report
 */
class Issue
{
public:
    Issue();
    Issue(IssueType type, const QString& title, const QString& description = QString());
    
    // Properties
    QString id;
    IssueType type;
    QString title;
    QString description;
    IssueStatus status;
    IssuePriority priority;
    IssueSeverity severity;
    ImplementationLevel implementationLevel;
    QDateTime createdDate;
    QDateTime updatedDate;
    QDateTime resolvedDate;
    QString reporter;
    QString assignee;
    QString component;
    QString version;
    QString targetVersion;
    QStringList tags;
    QStringList attachments;
    QString reproductionSteps;
    QString expectedBehavior;
    QString actualBehavior;
    QString environment;
    int estimatedHours;
    int actualHours;
    QStringList comments;
    QStringList relatedIssues;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Utility
    QString getTypeString() const;
    QString getStatusString() const;
    QString getPriorityString() const;
    QString getSeverityString() const;
    QString getImplementationLevelString() const;
    QColor getTypeColor() const;
    QColor getStatusColor() const;
    QColor getPriorityColor() const;
    bool isOverdue() const;
    int getDaysOpen() const;
};

/**
 * @brief Custom tree widget item for issues
 */
class IssueItem : public QTreeWidgetItem
{
public:
    explicit IssueItem(const Issue& issue, QTreeWidget* parent = nullptr);
    
    void updateFromIssue(const Issue& issue);
    Issue getIssue() const { return m_issue; }
    void setIssue(const Issue& issue) { m_issue = issue; updateFromIssue(issue); }
    
private:
    void setupItem();
    
    Issue m_issue;
};

/**
 * @brief Feature and bug tracking dialog
 */
class FeatureBugTrackerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FeatureBugTrackerDialog(QWidget* parent = nullptr);
    ~FeatureBugTrackerDialog();

    // File operations
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    
    // Issue operations
    void addFeature();
    void addBug();
    void addEnhancement();
    void editIssue();
    void deleteIssue();
    void duplicateIssue();
    void resolveIssue();
    void closeIssue();
    void reopenIssue();
    
    // Project management
    void generateReport();
    void exportToCSV();
    void importFromCSV();
    void showStatistics();

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
    void onShowStatistics();
    
    void onAddFeature();
    void onAddBug();
    void onAddEnhancement();
    void onEditIssue();
    void onDeleteIssue();
    void onDuplicateIssue();
    void onResolveIssue();
    void onCloseIssue();
    void onReopenIssue();
    
    void onIssueSelectionChanged();
    void onIssueDoubleClicked(QTreeWidgetItem* item, int column);
    void onIssueItemChanged(QTreeWidgetItem* item, int column);
    
    void onFilterChanged();
    void onSortChanged();
    void onShowClosedToggled(bool show);
    void onTabChanged(int index);
    
    void updateProgress();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupIssueList();
    void setupIssueDetails();
    void setupFilters();
    void setupStatusBar();
    
    void updateIssueList();
    void updateIssueDetails();
    void updateWindowTitle();
    void updateStatistics();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    Issue* findIssue(const QString& id);
    IssueItem* findIssueItem(const QString& id);
    void addIssueToTree(const Issue& issue);
    void removeIssueFromTree(const QString& id);
    
    void loadSettings();
    void saveSettings();

    // File management
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);

    // Helper methods
    bool shouldShowIssue(const Issue& issue);
    QStringList parseCSVLine(const QString& line);
    IssueType stringToIssueType(const QString& str);
    IssueStatus stringToIssueStatus(const QString& str);
    IssuePriority stringToIssuePriority(const QString& str);
    IssueSeverity stringToIssueSeverity(const QString& str);
    void generateHTMLReport(QTextStream& out);
    void generateTextReport(QTextStream& out);
    QString generateStatisticsReport();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // Issue list
    QTreeWidget* m_issueTree;
    QWidget* m_filterWidget;
    QComboBox* m_typeFilter;
    QComboBox* m_statusFilter;
    QComboBox* m_priorityFilter;
    QComboBox* m_severityFilter;
    QLineEdit* m_searchFilter;
    QCheckBox* m_showClosedCheck;
    QComboBox* m_sortCombo;
    
    // Issue details (tabbed)
    QTabWidget* m_detailsTabWidget;
    
    // Basic details tab
    QWidget* m_basicDetailsTab;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QComboBox* m_typeCombo;
    QComboBox* m_statusCombo;
    QComboBox* m_priorityCombo;
    QComboBox* m_severityCombo;
    QComboBox* m_implementationCombo;
    QLineEdit* m_reporterEdit;
    QLineEdit* m_assigneeEdit;
    QLineEdit* m_componentEdit;
    QLineEdit* m_versionEdit;
    QLineEdit* m_targetVersionEdit;
    QLineEdit* m_tagsEdit;
    
    // Bug details tab
    QWidget* m_bugDetailsTab;
    QTextEdit* m_reproductionStepsEdit;
    QTextEdit* m_expectedBehaviorEdit;
    QTextEdit* m_actualBehaviorEdit;
    QLineEdit* m_environmentEdit;
    
    // Time tracking tab
    QWidget* m_timeTrackingTab;
    QSpinBox* m_estimatedHoursSpinBox;
    QSpinBox* m_actualHoursSpinBox;
    QLabel* m_timeVarianceLabel;
    QProgressBar* m_timeProgressBar;
    
    // Comments tab
    QWidget* m_commentsTab;
    QTextEdit* m_commentsEdit;
    QLineEdit* m_newCommentEdit;
    QPushButton* m_addCommentButton;
    
    // Status labels
    QLabel* m_createdLabel;
    QLabel* m_updatedLabel;
    QLabel* m_resolvedLabel;
    QLabel* m_daysOpenLabel;
    
    // Status bar
    QLabel* m_statsLabel;
    QLabel* m_featureCountLabel;
    QLabel* m_bugCountLabel;
    QProgressBar* m_completionProgressBar;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_generateReportAction;
    QAction* m_exportAction;
    QAction* m_importAction;
    QAction* m_statisticsAction;
    QAction* m_exitAction;
    
    QAction* m_addFeatureAction;
    QAction* m_addBugAction;
    QAction* m_addEnhancementAction;
    QAction* m_editIssueAction;
    QAction* m_deleteIssueAction;
    QAction* m_duplicateAction;
    QAction* m_resolveAction;
    QAction* m_closeAction;
    QAction* m_reopenAction;
    
    // Data
    QList<Issue> m_issues;
    QString m_currentFilePath;
    QString m_projectName;
    bool m_modified;
    
    // Settings
    QSettings* m_settings;
    
    // Update timer
    QTimer* m_updateTimer;
};
