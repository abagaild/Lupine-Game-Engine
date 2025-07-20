#pragma once

#ifndef ASSETBROWSERPANEL_H
#define ASSETBROWSERPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QListView>
#include <QLabel>
#include <QScrollArea>
#include <QTextEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QButtonGroup>
#include <QToolButton>
#include <QFrame>
#include <QStatusBar>
#include <QSet>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QScrollArea>
#include <QListWidget>

// Forward declarations
class AssetFilterProxyModel;
class PlacementMode;
class AssetPreviewModel;
class AssetTagManager;
class AssetListView;



class AssetBrowserPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AssetBrowserPanel(QWidget *parent = nullptr);
    ~AssetBrowserPanel();

    void setProjectPath(const QString& projectPath);

    // Preview controls
    void enableModelPreviews(bool enabled);

    // Getter for asset model
    AssetPreviewModel* getAssetModel() const { return m_assetModel; }

    // Placement mode
    PlacementMode* getPlacementMode() const { return m_placementMode; }

    // Get currently selected asset path
    QString getSelectedAssetPath() const;

signals:
    void assetSelected(const QString& assetPath);
    void assetDoubleClicked(const QString& assetPath);

private slots:
    void onAssetClicked(const QModelIndex& index);
    void onAssetDoubleClicked(const QModelIndex& index);
    void onSearchTextChanged(const QString& text);
    void onFilterChanged();
    void onClearFilters();
    void onViewModeChanged();
    void onNavigateUp();
    void onNavigateHome();
    void onPlacementModeToggled(bool enabled);
    void onPlacementModeTypeChanged(int index);
    void onSurfaceDetectionTypeChanged(int index);
    void onGridSnapToggled(bool enabled);
    void onGridSnapYToggled(bool enabled);
    void onSurfaceSnapToggled(bool enabled);
    void onAlignToSurfaceNormalToggled(bool enabled);
    void onGridSizeChanged(double value);
    void onPlacementGridYChanged(double value);
    void onSurfaceSnapToleranceChanged(double value);
    void onGhostOpacityChanged(int value);
    void onDefaultPlacementTypeChanged();
    void onTagFilterChanged();
    void onCreateTag();
    void onManageTags();
    void onTagAsset();
    void onPreviewsToggled(bool enabled);
    void onLazyLoadingToggled(bool enabled);
    void onMaxConcurrentChanged(int value);
    void onPreviewDelayChanged(int value);

private:
    void setupUI();
    void setupFilterPanel();
    // Preview panel removed for cleaner interface
    void setupPlacementModePanel();
    void setupTaggingPanel();
    void updateTagFilterList();
    void updateStatusBar();
    void applyFilters();
    void navigateToFolder(const QString& folderPath);
    void updateNavigationButtons();

    // File type detection methods
    bool isImageFile(const QString& filePath) const;
    bool is3DModelFile(const QString& filePath) const;
    bool isTextFile(const QString& filePath) const;
    bool isSceneFile(const QString& filePath) const;
    bool isScriptFile(const QString& filePath) const;
    bool isAudioFile(const QString& filePath) const;
    bool isAnimationFile(const QString& filePath) const;
    bool isTilemapFile(const QString& filePath) const;
    bool isVideoFile(const QString& filePath) const;

    // Category detection methods
    QString getFileCategory(const QString& filePath) const;
    QString getFileType(const QString& filePath) const;

    // Main layout components
    QVBoxLayout* m_layout;
    QSplitter* m_splitter;
    AssetListView* m_assetList;

    // Filter panel components
    QScrollArea* m_filterScrollArea;
    QFrame* m_filterPanel;
    QVBoxLayout* m_filterLayout;
    QLineEdit* m_searchEdit;
    QPushButton* m_clearFiltersButton;

    // Placement mode components
    QGroupBox* m_placementModeGroup;
    QCheckBox* m_placementModeCheck;
    QComboBox* m_placementModeTypeCombo;
    QComboBox* m_surfaceDetectionTypeCombo;
    QCheckBox* m_gridSnapCheck;
    QCheckBox* m_gridSnapYCheck;
    QCheckBox* m_surfaceSnapCheck;
    QCheckBox* m_alignToSurfaceNormalCheck;
    QDoubleSpinBox* m_gridSizeSpinBox;
    QDoubleSpinBox* m_placementGridYSpinBox;
    QDoubleSpinBox* m_surfaceSnapToleranceSpinBox;
    QSlider* m_ghostOpacitySlider;
    QLabel* m_ghostOpacityLabel;
    QComboBox* m_default2DSpriteCombo;
    QComboBox* m_default3DSpriteCombo;
    QComboBox* m_default3DModelCombo;

    // Tagging components
    QGroupBox* m_taggingGroup;
    QLineEdit* m_tagSearchEdit;
    QListWidget* m_tagFilterList;
    QPushButton* m_createTagButton;
    QPushButton* m_manageTagsButton;
    QPushButton* m_tagAssetButton;
    QCheckBox* m_previewsEnabledCheck;
    QCheckBox* m_lazyLoadingCheck;
    QSpinBox* m_maxConcurrentSpinBox;
    QSpinBox* m_previewDelaySpinBox;

    // Navigation components
    QPushButton* m_upButton;
    QPushButton* m_homeButton;
    QLabel* m_currentPathLabel;

    // File type filter checkboxes
    QGroupBox* m_fileTypeGroup;
    QVBoxLayout* m_fileTypeLayout;
    QCheckBox* m_showImagesCheck;
    QCheckBox* m_show3DModelsCheck;
    QCheckBox* m_showScriptsCheck;
    QCheckBox* m_showScenesCheck;
    QCheckBox* m_showAudioCheck;
    QCheckBox* m_showAnimationsCheck;
    QCheckBox* m_showTilemapsCheck;
    QCheckBox* m_showVideosCheck;
    QCheckBox* m_showTextCheck;
    QCheckBox* m_showOthersCheck;

    // Category filter
    QGroupBox* m_categoryGroup;
    QVBoxLayout* m_categoryLayout;
    QCheckBox* m_show2DCheck;
    QCheckBox* m_show3DCheck;
    QCheckBox* m_showUICheck;

    // View mode controls
    QComboBox* m_viewModeCombo;

    // Preview panel components removed for cleaner interface

    // Status bar
    QStatusBar* m_statusBar;

    // Models and filtering
    AssetPreviewModel* m_assetModel;
    AssetFilterProxyModel* m_filterModel;

    // State
    QString m_projectPath;
    QString m_currentFolderPath;
    bool m_filtersVisible;

    // Placement mode
    PlacementMode* m_placementMode;

    // Tagging system
    AssetTagManager* m_tagManager;
};

/**
 * @brief Custom proxy model for filtering assets by type and category
 */
class AssetFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit AssetFilterProxyModel(QObject* parent = nullptr);

    void setSearchFilter(const QString& searchText);
    void setFileTypeFilters(const QSet<QString>& enabledTypes);
    void setCategoryFilters(const QSet<QString>& enabledCategories);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_searchText;
    QSet<QString> m_enabledFileTypes;
    QSet<QString> m_enabledCategories;

    QString getFileType(const QString& filePath) const;
    QString getFileCategory(const QString& filePath) const;
    bool isImageFile(const QString& filePath) const;
    bool is3DModelFile(const QString& filePath) const;
    bool isScriptFile(const QString& filePath) const;
    bool isSceneFile(const QString& filePath) const;
    bool isAudioFile(const QString& filePath) const;
    bool isAnimationFile(const QString& filePath) const;
    bool isTilemapFile(const QString& filePath) const;
    bool isVideoFile(const QString& filePath) const;
    bool isTextFile(const QString& filePath) const;
};

#endif // ASSETBROWSERPANEL_H
