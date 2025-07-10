#pragma once

#include "Tilemap25DPainterDialog.h"

/**
 * @brief Tileset palette widget for selecting tiles
 */
class Tilemap25DPalette : public QGraphicsView {
    Q_OBJECT

public:
    explicit Tilemap25DPalette(QWidget* parent = nullptr);
    ~Tilemap25DPalette();

    void SetTileset(int tileset_id, Lupine::Tileset2DResource* tileset);
    void ClearTileset();
    void SetSelectedTile(int tile_id);
    int GetSelectedTile() const { return m_selected_tile_id; }

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
 * @brief Dialog for painting and editing 2.5D tilemaps
 * 
 * This dialog provides a complete 2.5D tilemap painting environment with:
 * - 3D face painting with tileset textures
 * - Vertex and edge manipulation with gizmos
 * - Grid snapping with keyboard controls
 * - Double-sided face rendering
 * - OBJ export with texture atlasing
 */
class Tilemap25DPainterDialog : public QDialog {
    Q_OBJECT

public:
    explicit Tilemap25DPainterDialog(QWidget* parent = nullptr);
    ~Tilemap25DPainterDialog();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // File menu actions
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    bool onSaveProjectAs();
    void onExportOBJ();
    void onExit();

    // Edit menu actions
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onSelectAll();
    void onDeselectAll();

    // View menu actions
    void onResetView();
    void onFitToView();
    void onZoomIn();
    void onZoomOut();
    void onToggleGrid();
    void onToggleSnap();

    // Tools menu actions
    void onPaintTool();
    void onEraseTool();
    void onSelectTool();
    void onEyedropperTool();

    // Selection mode actions
    void onFaceSelectionMode();
    void onEdgeSelectionMode();
    void onVertexSelectionMode();

    // Gizmo mode actions
    void onMoveGizmo();
    void onRotateGizmo();
    void onScaleGizmo();

    // Tileset management
    void onLoadTileset();
    void onRemoveTileset();
    void onTilesetSelectionChanged();

    // Canvas interactions
    void onFacePainted(int face_index);
    void onFaceErased(int face_index);
    void onSelectionChanged();
    void onSceneModified();
    void onTileSelected(int tileset_id, int tile_id);

    // Grid controls
    void onGridSizeChanged();
    void onShowGridChanged();
    void onSnapToGridChanged();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupToolPanel();
    void setupViewportPanel();
    void setupStatusBar();
    
    void updateWindowTitle();
    void updateToolStates();
    void updateSelectionInfo();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    // Project management
    bool saveProject(const QString& filepath);
    bool loadProject(const QString& filepath);
    void newProject();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // 3D Viewport
    Tilemap25DCanvas* m_canvas;
    
    // Tool panels
    QWidget* m_toolPanel;
    Tilemap25DPalette* m_paletteWidget;
    QListWidget* m_tilesetList;
    
    // Tool settings
    QComboBox* m_toolCombo;
    QComboBox* m_selectionModeCombo;
    QComboBox* m_gizmoModeCombo;
    QSlider* m_gridSizeSlider;
    QDoubleSpinBox* m_gridSizeSpinBox;
    QCheckBox* m_showGridCheck;
    QCheckBox* m_snapToGridCheck;
    
    // Status bar
    QLabel* m_faceCountLabel;
    QLabel* m_selectionInfoLabel;
    QLabel* m_statusLabel;
    
    // Menu actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportOBJAction;
    QAction* m_exitAction;
    
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_deleteAction;
    QAction* m_selectAllAction;
    QAction* m_deselectAllAction;
    
    QAction* m_resetViewAction;
    QAction* m_fitToViewAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_toggleGridAction;
    QAction* m_toggleSnapAction;
    
    QAction* m_paintToolAction;
    QAction* m_eraseToolAction;
    QAction* m_selectToolAction;
    QAction* m_eyedropperToolAction;
    
    QAction* m_faceSelectionAction;
    QAction* m_edgeSelectionAction;
    QAction* m_vertexSelectionAction;
    
    QAction* m_moveGizmoAction;
    QAction* m_rotateGizmoAction;
    QAction* m_scaleGizmoAction;
    
    // Tool action groups
    QActionGroup* m_toolGroup;
    QActionGroup* m_selectionModeGroup;
    QActionGroup* m_gizmoModeGroup;
    
    // State
    QString m_currentFilePath;
    bool m_modified;
    int m_nextTilesetId;
    
    // Loaded tilesets
    std::unordered_map<int, QString> m_tilesetPaths;
    std::unordered_map<int, std::unique_ptr<Lupine::Tileset2DResource>> m_tilesets;
};
