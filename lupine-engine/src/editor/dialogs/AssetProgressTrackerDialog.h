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
#include <QListWidget>
#include <QGroupBox>
#include <QColorDialog>
#include <QPainter>

/**
 * @brief Represents a stage in the asset production pipeline
 */
class AssetStage
{
public:
    AssetStage();
    AssetStage(const QString& name, const QColor& color = Qt::blue);
    
    QString id;
    QString name;
    QString description;
    QColor color;
    int order;
    bool isRequired;
    int estimatedDays;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

/**
 * @brief Represents an asset type with its production pipeline
 */
class AssetType
{
public:
    AssetType();
    AssetType(const QString& name);
    
    QString id;
    QString name;
    QString description;
    QList<AssetStage> stages;
    QString defaultAssignee;
    QStringList tags;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    AssetStage* findStage(const QString& stageId);
    int getStageIndex(const QString& stageId) const;
};

/**
 * @brief Represents an individual asset in production
 */
class Asset
{
public:
    Asset();
    Asset(const QString& name, const QString& assetTypeId);
    
    QString id;
    QString name;
    QString description;
    QString assetTypeId;
    QString currentStageId;
    QString assignee;
    QDateTime createdDate;
    QDateTime updatedDate;
    QDateTime targetDate;
    QStringList tags;
    QString filePath;
    QString notes;
    int priority; // 1-5
    QMap<QString, QDateTime> stageCompletionDates;
    QMap<QString, QString> stageNotes;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    bool isStageCompleted(const QString& stageId) const;
    int getCurrentStageIndex(const AssetType& assetType) const;
    int getCompletionPercentage(const AssetType& assetType) const;
    bool isOverdue() const;
    int getDaysInCurrentStage() const;
};

/**
 * @brief Custom tree widget item for assets
 */
class AssetItem : public QTreeWidgetItem
{
public:
    explicit AssetItem(const Asset& asset, const AssetType& assetType, QTreeWidget* parent = nullptr);
    
    void updateFromAsset(const Asset& asset, const AssetType& assetType);
    Asset getAsset() const { return m_asset; }
    void setAsset(const Asset& asset) { m_asset = asset; }
    
private:
    void setupItem(const AssetType& assetType);
    
    Asset m_asset;
};

/**
 * @brief Asset progress tracking and pipeline management dialog
 */
class AssetProgressTrackerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssetProgressTrackerDialog(QWidget* parent = nullptr);
    ~AssetProgressTrackerDialog();

    // File operations
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    
    // Asset type operations
    void addAssetType();
    void editAssetType();
    void deleteAssetType();
    void duplicateAssetType();
    
    // Asset operations
    void addAsset();
    void editAsset();
    void deleteAsset();
    void duplicateAsset();
    void moveAssetToNextStage();
    void moveAssetToPreviousStage();
    void setAssetStage();
    
    // Pipeline management
    void generateReport();
    void exportToCSV();
    void importFromCSV();
    void showPipelineView();

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
    void onShowPipeline();
    
    void onAddAssetType();
    void onEditAssetType();
    void onDeleteAssetType();
    void onDuplicateAssetType();
    
    void onAddAsset();
    void onEditAsset();
    void onDeleteAsset();
    void onDuplicateAsset();
    void onMoveToNextStage();
    void onMoveToPreviousStage();
    void onSetStage();
    
    void onAssetSelectionChanged();
    void onAssetDoubleClicked(QTreeWidgetItem* item, int column);
    void onAssetItemChanged(QTreeWidgetItem* item, int column);
    void onAssetTypeChanged();

    // Stage management slots
    void onAddStage();
    void onEditStage();
    void onDeleteStage();
    void onMoveStageUp();
    void onMoveStageDown();
    void onStageSelectionChanged();

    void onFilterChanged();
    void onSortChanged();
    void onShowCompletedToggled(bool show);
    void onTabChanged(int index);

    void updateProgress();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupAssetTypeManager();
    void setupAssetList();
    void setupAssetDetails();
    void setupPipelineView();
    void setupFilters();
    void setupStatusBar();
    
    void updateAssetTypeList();
    void updateAssetList();
    void updateAssetDetails();
    void updateStagesList();
    void updateCurrentAssetFromUI();
    void updateAssetDetailsFromAsset(const Asset& asset);
    void clearAssetDetails();
    void updateFilterComboBoxes();
    void updateAssetTypeCombo();
    void updateStageCombo(const QString& assetTypeId);
    void updatePipelineView();
    void exportPipelineView(const QString& fileName);
    QWidget* createAssetCard(const Asset& asset);
    void updateWindowTitle();
    void updateStatistics();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    AssetType* findAssetType(const QString& id);
    Asset* findAsset(const QString& id);
    AssetItem* findAssetItem(const QString& id);
    void addAssetToTree(const Asset& asset);
    void removeAssetFromTree(const QString& id);
    
    void loadSettings();
    void saveSettings();
    
    // File management
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QTabWidget* m_mainTabWidget;
    
    // Asset Types tab
    QWidget* m_assetTypesTab;
    QSplitter* m_assetTypesSplitter;
    QListWidget* m_assetTypesList;
    QWidget* m_assetTypeDetailsWidget;
    QLineEdit* m_assetTypeNameEdit;
    QTextEdit* m_assetTypeDescriptionEdit;
    QListWidget* m_stagesList;
    QPushButton* m_addStageButton;
    QPushButton* m_editStageButton;
    QPushButton* m_deleteStageButton;
    QPushButton* m_moveStageUpButton;
    QPushButton* m_moveStageDownButton;
    
    // Assets tab
    QWidget* m_assetsTab;
    QSplitter* m_assetsSplitter;
    QTreeWidget* m_assetTree;
    QWidget* m_filterWidget;
    QComboBox* m_assetTypeFilter;
    QComboBox* m_stageFilter;
    QComboBox* m_assigneeFilter;
    QLineEdit* m_searchFilter;
    QCheckBox* m_showCompletedCheck;
    QComboBox* m_sortCombo;
    
    // Asset details
    QWidget* m_assetDetailsWidget;
    QLineEdit* m_assetNameEdit;
    QTextEdit* m_assetDescriptionEdit;
    QComboBox* m_assetTypeCombo;
    QComboBox* m_currentStageCombo;
    QLineEdit* m_assigneeEdit;
    QDateEdit* m_targetDateEdit;
    QSpinBox* m_prioritySpinBox;
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseFileButton;
    QTextEdit* m_notesEdit;
    QProgressBar* m_assetProgressBar;
    QLabel* m_createdLabel;
    QLabel* m_updatedLabel;
    QLabel* m_daysInStageLabel;
    
    // Pipeline view tab
    QWidget* m_pipelineTab;
    QWidget* m_pipelineViewWidget;
    
    // Status bar
    QLabel* m_statsLabel;
    QLabel* m_assetCountLabel;
    QProgressBar* m_overallProgressBar;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_generateReportAction;
    QAction* m_exportAction;
    QAction* m_importAction;
    QAction* m_pipelineViewAction;
    QAction* m_exitAction;
    
    QAction* m_addAssetTypeAction;
    QAction* m_editAssetTypeAction;
    QAction* m_deleteAssetTypeAction;
    QAction* m_duplicateAssetTypeAction;
    
    QAction* m_addAssetAction;
    QAction* m_editAssetAction;
    QAction* m_deleteAssetAction;
    QAction* m_duplicateAssetAction;
    QAction* m_nextStageAction;
    QAction* m_previousStageAction;
    QAction* m_setStageAction;
    
    // Data
    QList<AssetType> m_assetTypes;
    QList<Asset> m_assets;
    QString m_currentFilePath;
    QString m_projectName;
    bool m_modified;
    
    // Settings
    QSettings* m_settings;
    
    // Update timer
    QTimer* m_updateTimer;
};
