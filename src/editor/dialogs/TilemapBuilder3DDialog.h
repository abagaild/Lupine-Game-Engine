#pragma once

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSlider>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QButtonGroup>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtCore/QTimer>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace Lupine {
    class Tileset3DResource;
    struct Tile3DData;
}

/**
 * @brief Tool modes for 3D tilemap editing
 */
enum class TileTool {
    Place,       ///< Place tiles
    Erase,       ///< Erase tiles
    Select       ///< Select tiles
};

/**
 * @brief Placement modes for 3D tiles
 */
enum class TilePlacementMode {
    GridSnap,    ///< Snap to grid positions
    FaceSnap,    ///< Snap to faces of existing tiles
    FreePlace    ///< Free placement
};

/**
 * @brief Represents a placed 3D tile in the tilemap
 */
struct PlacedTile3D {
    int tile_id;                    ///< ID of the tile from tileset
    glm::vec3 position;             ///< World position
    glm::vec3 rotation;             ///< Rotation in degrees (euler angles)
    glm::vec3 scale;                ///< Scale factors
    bool selected;                  ///< Whether this tile is selected
    
    PlacedTile3D(int id, const glm::vec3& pos)
        : tile_id(id), position(pos), rotation(0.0f), scale(1.0f), selected(false) {}
};

/**
 * @brief 3D viewport widget for tilemap building
 */
class TilemapCanvas3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit TilemapCanvas3D(QWidget* parent = nullptr);
    ~TilemapCanvas3D();

    // Tile operations
    void AddTile(int tile_id, const glm::vec3& position);
    void RemoveTile(const glm::vec3& position);
    void ClearTiles();
    void SetSelectedTileId(int tile_id) { m_selectedTileId = tile_id; }
    
    // Selection
    void SelectTile(const glm::vec3& position);
    void ClearSelection();
    PlacedTile3D* GetSelectedTile();
    
    // Settings
    void SetTileset(std::shared_ptr<Lupine::Tileset3DResource> tileset);
    void SetCurrentTool(TileTool tool) { m_currentTool = tool; }
    TileTool GetCurrentTool() const { return m_currentTool; }
    void SetPlacementMode(TilePlacementMode mode) { m_placementMode = mode; }
    TilePlacementMode GetPlacementMode() const { return m_placementMode; }

    void SetGridSize(float size) { m_gridSize = size; }
    float GetGridSize() const { return m_gridSize; }

    void SetGridBaseY(float baseY) { m_gridBaseY = baseY; update(); }
    float GetGridBaseY() const { return m_gridBaseY; }

    void SetShowGrid(bool show) { m_showGrid = show; update(); }
    bool GetShowGrid() const { return m_showGrid; }

    // Preview/Ghost functionality
    void SetShowPreview(bool show) { m_showPreview = show; }
    bool GetShowPreview() const { return m_showPreview; }
    void UpdatePreview(const QPoint& mousePos);

    // Camera controls
    void ResetCamera();
    void FocusOnTiles();
    
    // Export
    bool ExportToOBJ(const QString& filepath, bool optimizeMesh = true);
    bool ExportToFBX(const QString& filepath, bool optimizeMesh = true);
    
    // Tilemap data
    bool SaveTilemap(const QString& filepath);
    bool LoadTilemap(const QString& filepath);

    // Getters
    size_t GetTileCount() const { return m_placedTiles.size(); }

signals:
    void tileAdded(int tile_id, const glm::vec3& position);
    void tileRemoved(const glm::vec3& position);
    void tileSelected(PlacedTile3D* tile);
    void sceneModified();
    void gridBaseYChanged(float baseY);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupShaders();
    void setupBuffers();
    void updateCamera();
    
    void drawGrid();
    void drawTiles();
    void drawGizmos();
    void drawPreviewTile();
    void drawCube(float size);
    
    glm::vec3 screenToWorld(const QPoint& screenPos);
    glm::vec3 snapToGrid(const glm::vec3& position);
    glm::vec3 snapToFace(const glm::vec3& position);
    PlacedTile3D* getTileAt(const glm::vec3& position);
    glm::vec3 getSnapPosition(const glm::vec3& worldPos);
    
    // Camera
    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    glm::vec3 m_cameraUp;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projMatrix;
    
    // Mouse interaction
    bool m_mousePressed;
    Qt::MouseButton m_pressedButton;
    QPoint m_lastMousePos;
    
    // Tilemap data
    std::vector<PlacedTile3D> m_placedTiles;
    std::shared_ptr<Lupine::Tileset3DResource> m_tileset;
    PlacedTile3D* m_selectedTile;
    
    // Settings
    TileTool m_currentTool;
    TilePlacementMode m_placementMode;
    float m_gridSize;
    float m_gridBaseY;
    bool m_showGrid;
    int m_selectedTileId;

    // Preview/Ghost
    bool m_showPreview;
    glm::vec3 m_previewPosition;
    
    // OpenGL resources
    unsigned int m_shaderProgram;
    unsigned int m_gridShaderProgram;
    unsigned int m_gridVAO, m_gridVBO;
    unsigned int m_cubeVAO, m_cubeVBO, m_cubeEBO;
};

/**
 * @brief Widget for displaying and selecting tiles from a 3D tileset
 */
class TilePaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit TilePaletteWidget(QWidget* parent = nullptr);
    
    void SetTileset(std::shared_ptr<Lupine::Tileset3DResource> tileset);
    int GetSelectedTileId() const { return m_selectedTileId; }

signals:
    void tileSelected(int tile_id);

private slots:
    void onTileClicked(QListWidgetItem* item);

private:
    void setupUI();
    void updateTileList();
    void updateTileGrid();

    QVBoxLayout* m_layout;
    QTabWidget* m_tabWidget;
    QListWidget* m_tileList;
    QWidget* m_gridWidget;
    QGridLayout* m_gridLayout;
    std::shared_ptr<Lupine::Tileset3DResource> m_tileset;
    int m_selectedTileId;
};

/**
 * @brief Main dialog for 3D tilemap building
 */
class TilemapBuilder3DDialog : public QDialog {
    Q_OBJECT

public:
    explicit TilemapBuilder3DDialog(QWidget* parent = nullptr);
    ~TilemapBuilder3DDialog();

private slots:
    // File operations
    void onNewTilemap();
    void onOpenTilemap();
    void onSaveTilemap();
    void onSaveAs();
    void onLoadTileset();
    void onExportOBJ();
    void onExportFBX();
    
    // Tool settings
    void onToolChanged(int toolId);
    void onPlaceToolSelected();
    void onEraseToolSelected();
    void onSelectToolSelected();
    void onPlacementModeChanged();
    void onGridSizeChanged();
    void onShowGridChanged();
    
    // Camera controls
    void onResetCamera();
    void onFocusOnTiles();
    
    // Tile operations
    void onTileSelected(int tile_id);
    void onTileAdded(int tile_id, const glm::vec3& position);
    void onTileRemoved(const glm::vec3& position);
    void onSceneModified();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupToolPanel();
    void setupViewportPanel();
    
    void updateWindowTitle();
    void updateTileCount();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // 3D Viewport
    TilemapCanvas3D* m_canvas;
    
    // Tool panels
    QWidget* m_toolPanel;
    TilePaletteWidget* m_paletteWidget;
    
    // Tool settings
    QButtonGroup* m_toolButtonGroup;
    QPushButton* m_placeToolButton;
    QPushButton* m_eraseToolButton;
    QPushButton* m_selectToolButton;
    QComboBox* m_placementModeCombo;
    QSlider* m_gridSizeSlider;
    QDoubleSpinBox* m_gridSizeSpinBox;
    QCheckBox* m_showGridCheck;
    
    // Status
    QLabel* m_tileCountLabel;
    QLabel* m_statusLabel;
    
    // Data
    QString m_currentFilePath;
    QString m_currentTilesetPath;
    bool m_modified;
    std::shared_ptr<Lupine::Tileset3DResource> m_tileset;
};
