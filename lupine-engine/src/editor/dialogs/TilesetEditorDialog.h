#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QListWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimer>
#include <memory>

#include "lupine/resources/TilesetResource.h"

namespace Lupine {
    class Tileset2DResource;
}

/**
 * @brief Custom graphics view for displaying and editing tilesets
 */
class TilesetView : public QGraphicsView {
    Q_OBJECT

public:
    explicit TilesetView(QWidget* parent = nullptr);
    ~TilesetView();

    void SetTileset(Lupine::Tileset2DResource* tileset);
    void RefreshView();
    void SetSelectedTile(int tile_id);
    int GetSelectedTile() const { return m_selected_tile_id; }

signals:
    void tileSelected(int tile_id);
    void tileDoubleClicked(int tile_id);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneChanged();

private:
    void setupScene();
    void updateTileGrid();
    void updateSelection();
    int getTileIdAt(const QPointF& scene_pos) const;
    QRectF getTileRect(int tile_id) const;

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_tileset_item;
    QGraphicsRectItem* m_selection_rect;
    Lupine::Tileset2DResource* m_tileset;
    int m_selected_tile_id;
    QPixmap m_tileset_pixmap;
};

/**
 * @brief Dialog for creating and editing tileset resources (.tileset files)
 * 
 * This dialog provides tools for:
 * - Importing tileset images
 * - Configuring tile slicing parameters
 * - Editing individual tile properties (collision, custom data)
 * - Previewing the tileset
 * - Saving/loading .tileset resource files
 */
class TilesetEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit TilesetEditorDialog(QWidget* parent = nullptr);
    ~TilesetEditorDialog();

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

    // Image import
    void onImportImage();
    void onImagePathChanged();
    void onTileSizeChanged();
    void onGridSizeChanged();
    void onSpacingChanged();
    void onMarginChanged();
    void onGenerateTiles();

    // Tile selection and editing
    void onTileSelected(int tile_id);
    void onTileDoubleClicked(int tile_id);
    void onCollisionTypeChanged();
    void onCollisionDataChanged();
    void onAddCustomProperty();
    void onRemoveCustomProperty();
    void onCustomPropertyChanged();

private:
    void setupUI();
    void setupMenuBar();
    void setupMainPanels();
    void setupImageImportPanel();
    void setupTilesetViewPanel();
    void setupTilePropertiesPanel();
    void setupCollisionEditor();
    void setupCustomDataEditor();

    void updateWindowTitle();
    void updateTilesetView();
    void updateTileProperties();
    void updateCollisionEditor();
    void updateCustomDataEditor();
    void refreshImagePreview();
    void calculateAutoGridSize();

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;

    // Left panel - Image import and configuration
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;
    
    // Image import group
    QGroupBox* m_imageGroup;
    QVBoxLayout* m_imageLayout;
    QLineEdit* m_imagePathEdit;
    QPushButton* m_browseImageButton;
    QPushButton* m_importImageButton;
    QLabel* m_imagePreviewLabel;

    // Slicing configuration group
    QGroupBox* m_slicingGroup;
    QGridLayout* m_slicingLayout;
    QSpinBox* m_tileSizeXSpin;
    QSpinBox* m_tileSizeYSpin;
    QSpinBox* m_gridSizeXSpin;
    QSpinBox* m_gridSizeYSpin;
    QSpinBox* m_spacingSpin;
    QSpinBox* m_marginSpin;
    QPushButton* m_generateTilesButton;

    // Center panel - Tileset view
    QWidget* m_centerPanel;
    QVBoxLayout* m_centerLayout;
    TilesetView* m_tilesetView;
    QLabel* m_tileInfoLabel;

    // Right panel - Tile properties
    QWidget* m_rightPanel;
    QVBoxLayout* m_rightLayout;
    QTabWidget* m_propertiesTab;

    // Collision tab
    QWidget* m_collisionTab;
    QVBoxLayout* m_collisionLayout;
    QComboBox* m_collisionTypeCombo;
    QGroupBox* m_collisionDataGroup;
    QGridLayout* m_collisionDataLayout;
    QSpinBox* m_collisionOffsetXSpin;
    QSpinBox* m_collisionOffsetYSpin;
    QSpinBox* m_collisionSizeXSpin;
    QSpinBox* m_collisionSizeYSpin;

    // Custom data tab
    QWidget* m_customDataTab;
    QVBoxLayout* m_customDataLayout;
    QTreeWidget* m_customDataTree;
    QHBoxLayout* m_customDataButtonLayout;
    QPushButton* m_addPropertyButton;
    QPushButton* m_removePropertyButton;

    // Data
    std::unique_ptr<Lupine::Tileset2DResource> m_tileset;
    QString m_currentFilePath;
    bool m_modified;
    int m_currentTileId;
};
