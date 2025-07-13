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
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QListWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimer>
#include <memory>

#include "lupine/tilemap/TilemapData.h"
#include "lupine/resources/TilesetResource.h"

/**
 * @brief Painting tools for the tilemap painter
 */
enum class PaintTool {
    Brush,          // Paint individual tiles
    Bucket,         // Flood fill
    Eraser,         // Erase tiles
    Eyedropper,     // Pick tile from map
    Rectangle,      // Draw rectangles
    Line            // Draw lines
};

/**
 * @brief Custom graphics view for tilemap editing
 */
class TilemapCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit TilemapCanvas(QWidget* parent = nullptr);
    ~TilemapCanvas();

    void SetProject(Lupine::TilemapProject* project);
    void SetCurrentTool(PaintTool tool);
    void SetCurrentTile(int tileset_id, int tile_id);
    void SetShowGrid(bool show);
    void SetSnapToGrid(bool snap);

    // Tileset access
    Lupine::Tileset2DResource* GetLoadedTileset(int tileset_id);

    // Zoom controls
    void ZoomIn();
    void ZoomOut();
    void ZoomToFit();
    void ZoomToActual();

signals:
    void tileClicked(int x, int y);
    void tilePainted(int x, int y, int tileset_id, int tile_id);
    void projectModified();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSceneChanged();

private:
    void setupScene();
    void updateCanvas();
    void updateGrid();
    void paintTile(int x, int y);
    void floodFill(int x, int y);
    void eraseTile(int x, int y);
    glm::ivec2 screenToTile(const QPoint& screen_pos) const;
    QPoint tileToScreen(const glm::ivec2& tile_pos) const;
    void drawTileHighlight(int x, int y);
    void renderTile(int x, int y, const Lupine::TileInstance& tile, float layer_opacity);

public:
    void loadTileset(int tileset_id);

    QGraphicsScene* m_scene;
    Lupine::TilemapProject* m_project;
    PaintTool m_current_tool;
    int m_current_tileset_id;
    int m_current_tile_id;
    bool m_show_grid;
    bool m_snap_to_grid;
    bool m_painting;
    QPoint m_last_paint_pos;
    
    // Loaded tileset resources
    std::map<int, std::unique_ptr<Lupine::Tileset2DResource>> m_loaded_tilesets;
    std::map<int, QPixmap> m_tileset_pixmaps;
};

/**
 * @brief Tileset palette widget for selecting tiles
 */
class TilesetPalette : public QGraphicsView {
    Q_OBJECT

public:
    explicit TilesetPalette(QWidget* parent = nullptr);
    ~TilesetPalette();

    void SetTileset(int tileset_id, Lupine::Tileset2DResource* tileset);
    void ClearTileset();

signals:
    void tileSelected(int tileset_id, int tile_id);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupScene();
    void updatePalette();
    int getTileIdAt(const QPointF& scene_pos) const;

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_tileset_item;
    QGraphicsRectItem* m_selection_rect;
    int m_tileset_id;
    Lupine::Tileset2DResource* m_tileset;
    int m_selected_tile_id;
    QPixmap m_tileset_pixmap;
};

/**
 * @brief Dialog for painting and editing tilemaps
 * 
 * This dialog provides a complete tilemap painting environment with:
 * - Layer management with transparency and visibility
 * - Multiple tileset support
 * - Various painting tools (brush, bucket, eraser, etc.)
 * - Grid and snap-to-grid functionality
 * - Save/load tilemap projects
 */
class TilemapPainterDialog : public QDialog {
    Q_OBJECT

public:
    explicit TilemapPainterDialog(QWidget* parent = nullptr);
    ~TilemapPainterDialog();

    // Project management
    void NewProject();
    void LoadProject(const QString& filepath);
    void SaveProject();
    void SaveProjectAs();

    // Export to components
    void ExportToTilemap2D(const QString& filepath);
    void ExportToTilemap25D(const QString& filepath);

private slots:
    // File operations
    void onNewProject();
    void onLoadProject();
    void onSaveProject();
    void onSaveAs();

    // Project properties
    void onProjectNameChanged();
    void onMapSizeChanged();
    void onTileSizeChanged();
    void onBackgroundColorChanged();

    // Tileset management
    void onAddTileset();
    void onRemoveTileset();
    void onTilesetSelectionChanged();

    // Layer management
    void onAddLayer();
    void onRemoveLayer();
    void onDuplicateLayer();
    void onMoveLayerUp();
    void onMoveLayerDown();
    void onLayerSelectionChanged();
    void onLayerVisibilityChanged();
    void onLayerOpacityChanged();

    // Tool selection
    void onToolChanged();
    void onTileSelected(int tileset_id, int tile_id);

    // Canvas events
    void onTileClicked(int x, int y);
    void onTilePainted(int x, int y, int tileset_id, int tile_id);
    void onProjectModified();

    // View controls
    void onZoomIn();
    void onZoomOut();
    void onZoomToFit();
    void onZoomToActual();
    void onToggleGrid();
    void onToggleSnap();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupProjectPropertiesPanel();
    void setupTilesetPanel();
    void setupLayerPanel();
    void setupCanvasPanel();
    void setupPalettePanel();

    void updateWindowTitle();
    void updateProjectProperties();
    void updateTilesetList();
    void updateLayerList();
    void updateCanvas();
    void updatePalette();
    void loadTileset(int tileset_id);

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;

    // Left panel - Project properties and tilesets
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;
    
    // Project properties group
    QGroupBox* m_projectGroup;
    QLineEdit* m_projectNameEdit;
    QSpinBox* m_mapWidthSpin;
    QSpinBox* m_mapHeightSpin;
    QSpinBox* m_tileWidthSpin;
    QSpinBox* m_tileHeightSpin;
    QPushButton* m_backgroundColorButton;

    // Tileset management group
    QGroupBox* m_tilesetGroup;
    QListWidget* m_tilesetList;
    QPushButton* m_addTilesetButton;
    QPushButton* m_removeTilesetButton;

    // Center panel - Canvas
    QWidget* m_centerPanel;
    QVBoxLayout* m_centerLayout;
    TilemapCanvas* m_canvas;

    // Right panel - Layers and palette
    QWidget* m_rightPanel;
    QVBoxLayout* m_rightLayout;
    
    // Layer management group
    QGroupBox* m_layerGroup;
    QListWidget* m_layerList;
    QHBoxLayout* m_layerButtonLayout;
    QPushButton* m_addLayerButton;
    QPushButton* m_removeLayerButton;
    QPushButton* m_duplicateLayerButton;
    QPushButton* m_moveLayerUpButton;
    QPushButton* m_moveLayerDownButton;
    QSlider* m_layerOpacitySlider;

    // Tileset palette group
    QGroupBox* m_paletteGroup;
    TilesetPalette* m_palette;

    // Tool actions
    QActionGroup* m_toolGroup;
    QAction* m_brushAction;
    QAction* m_bucketAction;
    QAction* m_eraserAction;
    QAction* m_eyedropperAction;
    QAction* m_rectangleAction;
    QAction* m_lineAction;

    // View actions
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomToFitAction;
    QAction* m_zoomToActualAction;
    QAction* m_toggleGridAction;
    QAction* m_toggleSnapAction;

    // Data
    std::unique_ptr<Lupine::TilemapProject> m_project;
    QString m_currentFilePath;
    bool m_modified;
    PaintTool m_currentTool;
    int m_currentTilesetId;
    int m_currentTileId;
};
