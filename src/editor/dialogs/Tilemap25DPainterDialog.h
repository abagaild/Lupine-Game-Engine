#pragma once

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

#include "lupine/resources/TilesetResource.h"

// Forward declarations
namespace Lupine {
    class Camera;
    class Shader;
}

/**
 * @brief Painting tools for the tilemap 2.5D painter
 */
enum class Tilemap25DPaintTool {
    Paint,          // Paint faces with selected tile
    Erase,          // Erase faces
    Select,         // Select faces/vertices/edges for manipulation
    Eyedropper      // Pick tile from existing face
};

/**
 * @brief Selection modes for face manipulation
 */
enum class Tilemap25DSelectionMode {
    Face,           // Select entire faces
    Edge,           // Select edges for manipulation
    Vertex          // Select vertices for manipulation
};

/**
 * @brief Gizmo modes for 3D manipulation
 */
enum class Tilemap25DGizmoMode {
    None,
    Move,
    Rotate,
    Scale
};

/**
 * @brief Represents a painted face in 2.5D space
 */
struct PaintedFace {
    glm::vec3 vertices[4];      // Four vertices of the quad face
    glm::vec2 uvs[4];           // UV coordinates for texture mapping
    int tileset_id;             // ID of the tileset used
    int tile_id;                // ID of the tile within the tileset
    glm::vec3 normal;           // Face normal for lighting
    bool double_sided;          // Whether face is double-sided (default true)
    bool selected;              // Whether face is currently selected
    
    PaintedFace() : tileset_id(-1), tile_id(-1), normal(0, 0, 1), double_sided(true), selected(false) {
        // Initialize as unit quad in XY plane
        vertices[0] = glm::vec3(-0.5f, -0.5f, 0.0f);
        vertices[1] = glm::vec3( 0.5f, -0.5f, 0.0f);
        vertices[2] = glm::vec3( 0.5f,  0.5f, 0.0f);
        vertices[3] = glm::vec3(-0.5f,  0.5f, 0.0f);
        
        uvs[0] = glm::vec2(0.0f, 0.0f);
        uvs[1] = glm::vec2(1.0f, 0.0f);
        uvs[2] = glm::vec2(1.0f, 1.0f);
        uvs[3] = glm::vec2(0.0f, 1.0f);
    }
};

/**
 * @brief Represents a selected vertex for manipulation
 */
struct SelectedVertex {
    int face_index;
    int vertex_index;
    glm::vec3 original_position;
    
    SelectedVertex(int face_idx, int vert_idx, const glm::vec3& pos)
        : face_index(face_idx), vertex_index(vert_idx), original_position(pos) {}
};

/**
 * @brief Represents a selected edge for manipulation
 */
struct SelectedEdge {
    int face_index;
    int edge_index;  // 0-3 for the four edges of a quad
    glm::vec3 start_pos;
    glm::vec3 end_pos;
    
    SelectedEdge(int face_idx, int edge_idx, const glm::vec3& start, const glm::vec3& end)
        : face_index(face_idx), edge_index(edge_idx), start_pos(start), end_pos(end) {}
};

/**
 * @brief Custom OpenGL widget for 2.5D tilemap painting
 */
class Tilemap25DCanvas : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit Tilemap25DCanvas(QWidget* parent = nullptr);
    ~Tilemap25DCanvas();

    // Tool and mode settings
    void SetCurrentTool(Tilemap25DPaintTool tool);
    void SetSelectionMode(Tilemap25DSelectionMode mode);
    void SetGizmoMode(Tilemap25DGizmoMode mode);
    void SetCurrentTile(int tileset_id, int tile_id);
    
    // Grid settings
    void SetGridSize(float size);
    void SetShowGrid(bool show);
    void SetSnapToGrid(bool snap);
    void ShiftGridHorizontal(float offset);
    void ShiftGridVertical(float offset);

    // Per-axis grid snapping
    void SetSnapXAxis(bool snap);
    void SetSnapYAxis(bool snap);
    void SetSnapZAxis(bool snap);
    void SetGridSizePerAxis(const glm::vec3& grid_size);
    bool GetSnapXAxis() const { return m_snap_x_axis; }
    bool GetSnapYAxis() const { return m_snap_y_axis; }
    bool GetSnapZAxis() const { return m_snap_z_axis; }
    glm::vec3 GetGridSizePerAxis() const { return m_grid_size_per_axis; }

    // Edge snapping
    void SetSnapToEdges(bool snap);
    void SetEdgeSnapDistance(float distance);
    bool GetSnapToEdges() const { return m_snap_to_edges; }
    float GetEdgeSnapDistance() const { return m_edge_snap_distance; }
    
    // Tileset management
    void LoadTileset(int tileset_id, const QString& tileset_path);
    Lupine::Tileset2DResource* GetTileset(int tileset_id);
    
    // Face management
    void ClearFaces();
    const std::vector<PaintedFace>& GetFaces() const { return m_faces; }
    void SetFaces(const std::vector<PaintedFace>& faces);
    
    // Selection
    void ClearSelection();
    const std::vector<int>& GetSelectedFaces() const { return m_selected_faces; }
    const std::vector<SelectedVertex>& GetSelectedVertices() const { return m_selected_vertices; }
    const std::vector<SelectedEdge>& GetSelectedEdges() const { return m_selected_edges; }
    
    // Export
    bool ExportToOBJ(const QString& filepath, bool generate_texture_atlas = true);

    // Public key event handling for grid manipulation
    void HandleKeyPress(QKeyEvent* event);

signals:
    void facePainted(int face_index);
    void faceErased(int face_index);
    void selectionChanged();
    void sceneModified();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // Rendering
    void setupShaders();
    void setupBuffers();
    void renderGrid();
    void renderFaces();
    void renderFacesImmediate();
    void renderGizmos();
    void renderSelection();
    void renderPreview();
    
    // Camera and view
    void updateCamera();
    void updateProjectionMatrix();
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    // Interaction
    glm::vec3 screenToWorld(const QPoint& screen_pos, float depth = 0.0f);
    glm::vec3 snapToGrid(const glm::vec3& position);
    glm::vec3 snapToEdges(const glm::vec3& position);
    int pickFace(const QPoint& screen_pos);
    int pickVertex(const QPoint& screen_pos, int& face_index);
    int pickEdge(const QPoint& screen_pos, int& face_index);
    
    // Face operations
    void paintFace(const glm::vec3& position, const glm::vec3& normal);
    void eraseFace(int face_index);
    void selectFace(int face_index, bool add_to_selection = false);
    void selectVertex(int face_index, int vertex_index, bool add_to_selection = false);
    void selectEdge(int face_index, int edge_index, bool add_to_selection = false);
    
    // Gizmo operations
    void updateGizmoTransform();
    bool testGizmoHit(const QPoint& screen_pos);
    void manipulateSelection(const glm::vec3& delta);
    float distancePointToLine(const glm::vec3& point, const glm::vec3& line_dir,
                             const glm::vec3& line_start, const glm::vec3& line_end);
    bool rayIntersectQuad(const glm::vec3& ray_start, const glm::vec3& ray_dir,
                         const glm::vec3 vertices[4], float& distance);
    bool rayIntersectTriangle(const glm::vec3& ray_start, const glm::vec3& ray_dir,
                             const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                             float& distance);
    
    // UV and texture management
    void calculateUVs(PaintedFace& face);
    void updateFaceTexture(PaintedFace& face);

    // Preview management
    void updatePreview(const QPoint& mouse_pos);
    void clearPreview();
    
    // Grid manipulation
    void processGridShift(QKeyEvent* event);

    // Gizmo rendering methods
    void renderMoveGizmo();
    void renderRotateGizmo();
    void renderScaleGizmo();

    // Export helpers
    bool generateTextureAtlas(const QString& base_filepath,
                             std::unordered_map<int, glm::vec2>& tileset_uv_offsets);
    bool generateMaterialFile(const QString& obj_filepath, const QString& texture_filename);

private:
    // Tool state
    Tilemap25DPaintTool m_current_tool;
    Tilemap25DSelectionMode m_selection_mode;
    Tilemap25DGizmoMode m_gizmo_mode;
    int m_current_tileset_id;
    int m_current_tile_id;
    
    // Grid settings
    float m_grid_size;
    bool m_show_grid;
    bool m_snap_to_grid;
    glm::vec2 m_grid_offset;

    // Per-axis grid snapping
    bool m_snap_x_axis;
    bool m_snap_y_axis;
    bool m_snap_z_axis;
    glm::vec3 m_grid_size_per_axis;

    // Edge snapping
    bool m_snap_to_edges;
    float m_edge_snap_distance;

    // Preview state
    bool m_show_preview;
    glm::vec3 m_preview_position;
    PaintedFace m_preview_face;

    // Tile rotation state
    float m_tile_rotation_y; // Q/E keys - rotate around Y axis
    float m_tile_rotation_x; // W key - rotate around X axis
    float m_tile_rotation_z; // S key - rotate around Z axis
    
    // Camera and view
    std::unique_ptr<Lupine::Camera> m_camera;
    float m_camera_distance;
    float m_camera_rotation_x;
    float m_camera_rotation_y;
    glm::vec3 m_camera_target;
    
    // Mouse interaction
    QPoint m_last_mouse_pos;
    bool m_mouse_pressed;
    bool m_dragging_gizmo;
    bool m_is_panning;
    bool m_is_orbiting;
    Qt::MouseButton m_pressed_button;
    glm::vec3 m_drag_start_pos;
    
    // Scene data
    std::vector<PaintedFace> m_faces;
    std::unordered_map<int, std::unique_ptr<Lupine::Tileset2DResource>> m_tilesets;
    std::unordered_map<int, unsigned int> m_tileset_textures;
    
    // Selection
    std::vector<int> m_selected_faces;
    std::vector<SelectedVertex> m_selected_vertices;
    std::vector<SelectedEdge> m_selected_edges;
    
    // Rendering resources
    std::unique_ptr<Lupine::Shader> m_face_shader;
    std::unique_ptr<Lupine::Shader> m_grid_shader;
    std::unique_ptr<Lupine::Shader> m_gizmo_shader;
    
    unsigned int m_grid_vao, m_grid_vbo;
    unsigned int m_face_vao, m_face_vbo, m_face_ebo;
    unsigned int m_gizmo_vao, m_gizmo_vbo;
    
    // Gizmo state
    glm::vec3 m_gizmo_position;
    glm::mat4 m_gizmo_transform;
    int m_gizmo_axis; // 0=none, 1=X, 2=Y, 3=Z

    // Texture atlas state
    int m_atlas_size;
    float m_atlas_texture_scale;
};

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
    void onSnapXAxisChanged();
    void onSnapYAxisChanged();
    void onSnapZAxisChanged();
    void onGridSizePerAxisChanged();
    void onSnapToEdgesChanged();
    void onEdgeSnapDistanceChanged();

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

    // Per-axis grid controls
    QCheckBox* m_snapXAxisCheck;
    QCheckBox* m_snapYAxisCheck;
    QCheckBox* m_snapZAxisCheck;
    QDoubleSpinBox* m_gridSizeXSpinBox;
    QDoubleSpinBox* m_gridSizeYSpinBox;
    QDoubleSpinBox* m_gridSizeZSpinBox;

    // Edge snapping controls
    QCheckBox* m_snapToEdgesCheck;
    QDoubleSpinBox* m_edgeSnapDistanceSpinBox;

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
