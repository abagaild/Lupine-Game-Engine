#pragma once

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QListWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <memory>

#include "lupine/resources/Tileset3DResource.h"

namespace Lupine {
    class Tileset3DResource;
    class TileBuilderDialog;
}

/**
 * @brief Custom OpenGL widget for displaying 3D tile previews
 */
class Tile3DPreviewWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT

public:
    explicit Tile3DPreviewWidget(QWidget* parent = nullptr);
    ~Tile3DPreviewWidget();

    void SetTile(Lupine::Tile3DData* tile);
    void ClearTile();

signals:
    void tileClicked();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupCamera();
    void renderTile();
    void setupShaders();
    void setupBuffers();
    void drawGrid();
    void drawPlaceholderCube();

    Lupine::Tile3DData* m_tile;
    float m_camera_distance;
    float m_camera_rotation_x;
    float m_camera_rotation_y;
    QPoint m_last_mouse_pos;
    bool m_mouse_pressed;

    // OpenGL resources
    unsigned int m_shader_program;
    unsigned int m_grid_shader_program;
    unsigned int m_vao, m_vbo, m_ebo;
    unsigned int m_grid_vao, m_grid_vbo;
};

/**
 * @brief Dialog for creating and editing 3D tileset resources (.tileset3d files)
 * 
 * This dialog provides tools for:
 * - Importing 3D model files (.obj, .fbx, etc.)
 * - Managing collections of 3D tiles
 * - Organizing tiles into categories
 * - Editing tile properties (transform, collision, custom data)
 * - Previewing 3D tiles
 * - Saving/loading .tileset3d resource files
 */
class Tileset3DEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit Tileset3DEditorDialog(QWidget* parent = nullptr);
    ~Tileset3DEditorDialog();

    // Resource management
    void NewTileset();
    void LoadTileset(const QString& filepath);
    void SaveTileset();
    void SaveTilesetAs();

private slots:
    // File operations
    void onNewTileset();
    void onLoadTileset();
    void onSaveTileset();
    void onSaveAs();

    // Tileset properties
    void onTilesetNameChanged();
    void onTilesetDescriptionChanged();

    // Tile management
    void onImportTile();
    void onRemoveTile();
    void onDuplicateTile();
    void onTileSelectionChanged();
    void onTileDoubleClicked(QListWidgetItem* item);
    void onOpenTileBuilder();

    // Category management
    void onAddCategory();
    void onRemoveCategory();
    void onCategorySelectionChanged();
    void onAssignTileToCategory();
    void onRemoveTileFromCategory();

    // Tile properties
    void onTileNameChanged();
    void onTileMeshPathChanged();
    void onBrowseMeshPath();
    void onTilePreviewImageChanged();
    void onBrowsePreviewImage();

    // Transform editing
    void onTransformPositionChanged();
    void onTransformRotationChanged();
    void onTransformScaleChanged();
    void onResetTransform();

    // Collision editing
    void onCollisionTypeChanged();
    void onCollisionDataChanged();
    void onBrowseCollisionMesh();

    // Custom data editing
    void onAddCustomProperty();
    void onRemoveCustomProperty();
    void onCustomPropertyChanged();

    // Preview
    void onTilePreviewClicked();
    void onLoadTileModel();

private:
    void setupUI();
    void setupMenuBar();
    void setupMainPanels();
    void setupTilesetPropertiesPanel();
    void setupTileManagementPanel();
    void setupCategoryPanel();
    void setupTilePropertiesPanel();
    void setupTransformEditor();
    void setupCollisionEditor();
    void setupCustomDataEditor();

    void updateWindowTitle();
    void updateTileList();
    void updateCategoryList();
    void updateTileProperties();
    void updateTransformEditor();
    void updateCollisionEditor();
    void updateCustomDataEditor();
    void updateTilePreview();

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    int getSelectedTileId() const;
    QString getSelectedCategoryName() const;

    // UI Components
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;

    // Left panel - Tileset properties and tile management
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;
    
    // Tileset properties group
    QGroupBox* m_tilesetPropertiesGroup;
    QVBoxLayout* m_tilesetPropertiesLayout;
    QLineEdit* m_tilesetNameEdit;
    QTextEdit* m_tilesetDescriptionEdit;

    // Tile management group
    QGroupBox* m_tileManagementGroup;
    QVBoxLayout* m_tileManagementLayout;
    QListWidget* m_tileList;
    QHBoxLayout* m_tileButtonLayout;
    QPushButton* m_importTileButton;
    QPushButton* m_removeTileButton;
    QPushButton* m_duplicateTileButton;

    // Category management group
    QGroupBox* m_categoryGroup;
    QVBoxLayout* m_categoryLayout;
    QListWidget* m_categoryList;
    QHBoxLayout* m_categoryButtonLayout;
    QPushButton* m_addCategoryButton;
    QPushButton* m_removeCategoryButton;
    QPushButton* m_assignToCategoryButton;
    QPushButton* m_removeFromCategoryButton;

    // Center panel - Tile preview
    QWidget* m_centerPanel;
    QVBoxLayout* m_centerLayout;
    Tile3DPreviewWidget* m_tilePreview;
    QLabel* m_tileInfoLabel;
    QPushButton* m_loadModelButton;

    // Right panel - Tile properties
    QWidget* m_rightPanel;
    QVBoxLayout* m_rightLayout;
    QTabWidget* m_propertiesTab;

    // General tab
    QWidget* m_generalTab;
    QVBoxLayout* m_generalLayout;
    QLineEdit* m_tileNameEdit;
    QLineEdit* m_tileMeshPathEdit;
    QPushButton* m_browseMeshButton;
    QLineEdit* m_tilePreviewImageEdit;
    QPushButton* m_browsePreviewButton;

    // Transform tab
    QWidget* m_transformTab;
    QVBoxLayout* m_transformLayout;
    QGroupBox* m_positionGroup;
    QDoubleSpinBox* m_positionXSpin;
    QDoubleSpinBox* m_positionYSpin;
    QDoubleSpinBox* m_positionZSpin;
    QGroupBox* m_rotationGroup;
    QDoubleSpinBox* m_rotationXSpin;
    QDoubleSpinBox* m_rotationYSpin;
    QDoubleSpinBox* m_rotationZSpin;
    QGroupBox* m_scaleGroup;
    QDoubleSpinBox* m_scaleXSpin;
    QDoubleSpinBox* m_scaleYSpin;
    QDoubleSpinBox* m_scaleZSpin;
    QPushButton* m_resetTransformButton;

    // Collision tab
    QWidget* m_collisionTab;
    QVBoxLayout* m_collisionLayout;
    QComboBox* m_collisionTypeCombo;
    QGroupBox* m_collisionDataGroup;
    QGridLayout* m_collisionDataLayout;
    QDoubleSpinBox* m_collisionOffsetXSpin;
    QDoubleSpinBox* m_collisionOffsetYSpin;
    QDoubleSpinBox* m_collisionOffsetZSpin;
    QDoubleSpinBox* m_collisionSizeXSpin;
    QDoubleSpinBox* m_collisionSizeYSpin;
    QDoubleSpinBox* m_collisionSizeZSpin;
    QLineEdit* m_collisionMeshEdit;
    QPushButton* m_browseCollisionMeshButton;
    QDoubleSpinBox* m_collisionMarginSpin;

    // Custom data tab
    QWidget* m_customDataTab;
    QVBoxLayout* m_customDataLayout;
    QTreeWidget* m_customDataTree;
    QHBoxLayout* m_customDataButtonLayout;
    QPushButton* m_addPropertyButton;
    QPushButton* m_removePropertyButton;

    // Data
    std::unique_ptr<Lupine::Tileset3DResource> m_tileset;
    QString m_currentFilePath;
    bool m_modified;
    int m_currentTileId;
};
