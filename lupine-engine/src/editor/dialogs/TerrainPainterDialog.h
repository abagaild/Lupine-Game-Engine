#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QListWidget>
#include <QTreeWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QTimer>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declarations
namespace Lupine {
    class Scene;
    class Node3D;
    class TerrainRenderer;
    class TerrainData;
}

class AssetBrowserPanel;
class SceneViewPanel;

/**
 * @brief Terrain painting tools enumeration
 */
enum class TerrainTool {
    None,
    HeightRaise,
    HeightLower,
    HeightFlatten,
    HeightSmooth,
    TexturePaint,
    AssetScatter,
    AssetErase
};

/**
 * @brief Brush shape types for terrain painting
 */
enum class TerrainBrushShape {
    Circle,
    Square,
    Custom
};

/**
 * @brief Terrain painter viewport widget
 * 
 * Custom OpenGL widget for rendering and interacting with terrain
 */
class TerrainViewport : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT

public:
    explicit TerrainViewport(QWidget* parent = nullptr);
    ~TerrainViewport();

    void setScene(Lupine::Scene* scene);
    void setTerrain(Lupine::TerrainRenderer* terrain);
    
    // Tool settings
    void setCurrentTool(TerrainTool tool);
    void setBrushSize(float size);
    void setBrushStrength(float strength);
    void setBrushShape(TerrainBrushShape shape);
    void setBrushFalloff(float falloff);

    // Camera controls
    void resetCamera();

signals:
    void terrainModified();
    void statusMessage(const QString& message);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateCamera();
    void handleTerrainPainting(const QPoint& position);
    glm::vec3 screenToWorld(const QPoint& screenPos);

    // Rendering methods
    void setupShaders();
    void setupBuffers();
    void updateCameraMatrices();
    void renderGrid();
    void renderTerrain();
    void renderBrushPreview();

    Lupine::Scene* m_scene;
    Lupine::TerrainRenderer* m_terrain;

    // Camera controls
    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    float m_aspectRatio;

    // Camera matrices
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    // Tool state
    TerrainTool m_currentTool;
    float m_brushSize;
    float m_brushStrength;
    TerrainBrushShape m_brushShape;
    float m_brushFalloff;
    bool m_isPainting;
    bool m_isPanning;
    QPoint m_lastMousePos;

    // Rendering
    QTimer* m_updateTimer;

    // OpenGL resources
    unsigned int m_shaderProgram;
    unsigned int m_gridVAO, m_gridVBO;
    unsigned int m_terrainVAO, m_terrainVBO, m_terrainEBO;
    unsigned int m_brushVAO, m_brushVBO;
};

/**
 * @brief Main terrain painter dialog
 * 
 * Comprehensive terrain editing tool with height painting, texture painting,
 * and asset scattering capabilities
 */
class TerrainPainterDialog : public QDialog {
    Q_OBJECT

public:
    explicit TerrainPainterDialog(QWidget* parent = nullptr);
    ~TerrainPainterDialog();

    // File operations
    void newTerrain();
    void openTerrain();
    void saveTerrain();
    void saveTerrainAs();
    void exportToOBJ();

    // Terrain operations
    void createTerrainChunk();
    void deleteTerrainChunk();
    void resizeTerrain();
    void clearTerrain();
    void flattenTerrain();
    void generateNoise();

    // Texture operations
    int getSelectedTextureLayer() const;

    // Asset operations
    QString getSelectedAssetPath() const;
    float getScatterDensity() const;
    float getScatterScaleVariance() const;
    float getScatterRotationVariance() const;
    glm::vec2 getScatterHeightOffset() const;

public slots:
    void onToolChanged();
    void onBrushSettingsChanged();
    void onTerrainSettingsChanged();
    void onTextureSelected();
    void onAssetSelected(const QString& assetPath);
    void onTerrainModified();

private slots:
    void onNewTerrain();
    void onOpenTerrain();
    void onSaveTerrain();
    void onSaveTerrainAs();
    void onExportOBJ();
    void onResetView();
    void onShowGrid(bool show);
    void onShowWireframe(bool show);
    void onEnableCollision(bool enable);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainLayout();
    void setupToolPanel();
    void setupBrushPanel();
    void setupTexturePanel();
    void setupAssetPanel();
    void setupTerrainPanel();
    void setupStatusBar();
    void setupConnections();

    // Texture layer management
    void addTextureLayer(const QString& texturePath);
    void removeTextureLayer(int layerIndex);
    
    void updateWindowTitle();
    void updateToolUI();
    void updateBrushPreview();
    void updateTerrainStats();

    // File operations
    void loadTerrainFile(const QString& filePath);
    void saveTerrainFile(const QString& filePath);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;
    QStatusBar* m_statusBar;
    
    // Viewport
    TerrainViewport* m_viewport;
    
    // Tool panels
    QTabWidget* m_toolTabs;
    QWidget* m_toolPanel;
    QWidget* m_brushPanel;
    QWidget* m_texturePanel;
    QWidget* m_assetPanel;
    QWidget* m_terrainPanel;
    
    // Tool controls
    QComboBox* m_toolCombo;
    QSlider* m_brushSizeSlider;
    QSlider* m_brushStrengthSlider;
    QSlider* m_brushFalloffSlider;
    QComboBox* m_brushShapeCombo;
    QDoubleSpinBox* m_brushSizeSpin;
    QDoubleSpinBox* m_brushStrengthSpin;
    QDoubleSpinBox* m_brushFalloffSpin;
    
    // Texture controls
    QListWidget* m_textureList;
    QPushButton* m_addTextureButton;
    QPushButton* m_removeTextureButton;
    QSlider* m_textureOpacitySlider;
    QSlider* m_textureScaleSlider;
    
    // Asset controls
    AssetBrowserPanel* m_assetBrowser;
    QCheckBox* m_scatterModeCheck;
    QSlider* m_scatterDensitySlider;
    QSlider* m_scatterScaleVarianceSlider;
    QSlider* m_scatterRotationVarianceSlider;
    QSlider* m_scatterHeightOffsetSlider;
    
    // Terrain controls
    QSpinBox* m_terrainWidthSpin;
    QSpinBox* m_terrainHeightSpin;
    QDoubleSpinBox* m_terrainScaleSpin;
    QSpinBox* m_chunkSizeSpin;
    QCheckBox* m_enableCollisionCheck;
    QComboBox* m_collisionTypeCombo;
    
    // Status widgets
    QLabel* m_statusLabel;
    QLabel* m_terrainStatsLabel;
    QLabel* m_terrainStatsDisplay;
    QProgressBar* m_operationProgress;
    
    // Actions
    QAction* m_newTerrainAction;
    QAction* m_openTerrainAction;
    QAction* m_saveTerrainAction;
    QAction* m_saveTerrainAsAction;
    QAction* m_exportOBJAction;
    QAction* m_resetViewAction;
    QAction* m_showGridAction;
    QAction* m_showWireframeAction;
    
    // Data
    std::unique_ptr<Lupine::Scene> m_terrainScene;
    Lupine::Node3D* m_terrainNode;
    Lupine::TerrainRenderer* m_terrainRenderer;
    std::shared_ptr<Lupine::TerrainData> m_terrainData;
    
    QString m_currentFilePath;
    QString m_selectedAssetPath;
    bool m_isModified;
    TerrainTool m_currentTool;
};
