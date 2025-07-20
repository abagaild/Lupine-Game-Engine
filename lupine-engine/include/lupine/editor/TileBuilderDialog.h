#pragma once

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include "TileBuilder.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QTimer>
#include <memory>

class QMenuBar;
class QToolBar;
class QStatusBar;

namespace Lupine {

class Tileset3DResource;

/**
 * @brief 3D preview widget for tile builder
 */
class TileBuilderPreview : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT

public:
    explicit TileBuilderPreview(QWidget* parent = nullptr);
    ~TileBuilderPreview();

    /**
     * @brief Set the tile data to preview
     * @param tile_data Tile data to display
     */
    void SetTileData(const TileBuilderData& tile_data);
    
    /**
     * @brief Update texture for a specific face
     * @param face Face to update
     * @param texture_path Path to texture file
     */
    void UpdateFaceTexture(MeshFace face, const QString& texture_path);
    
    /**
     * @brief Refresh the preview
     */
    void RefreshPreview();

signals:
    void FaceClicked(MeshFace face);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupCamera();
    void renderMesh();
    void setupShaders();
    void loadTextures();
    void updateUVCoordinates();
    void createTextureAtlas();
    MeshFace pickFace(const QPoint& mouse_pos);

    TileBuilderData m_tile_data;
    
    // Camera controls
    float m_camera_distance;
    float m_camera_rotation_x;
    float m_camera_rotation_y;
    QPoint m_last_mouse_pos;
    bool m_mouse_pressed;
    
    // OpenGL resources
    unsigned int m_shader_program;
    unsigned int m_vao, m_vbo, m_ebo;
    std::map<MeshFace, unsigned int> m_face_textures;
    unsigned int m_atlas_texture_id;

    // Rendering state
    bool m_gl_initialized;
    bool m_mesh_loaded;
};

/**
 * @brief Main Tile Builder dialog
 */
class TileBuilderDialog : public QDialog {
    Q_OBJECT

public:
    explicit TileBuilderDialog(QWidget* parent = nullptr);
    ~TileBuilderDialog();
    
    /**
     * @brief Set the target tileset for adding tiles
     * @param tileset Tileset resource
     */
    void SetTargetTileset(std::shared_ptr<Tileset3DResource> tileset);

signals:
    void tileAddedToTileset(int tile_id);

public slots:
    void NewTile();
    void LoadTile();
    void SaveTile();
    void SaveTileAs();
    void ExportToOBJ();
    void AddToTileset();

private slots:
    // Mesh parameter changes
    void OnMeshTypeChanged();
    void OnDimensionsChanged();
    void OnSubdivisionsChanged();
    void OnRadiusChanged();
    void OnHeightChanged();
    void OnClosedChanged();
    
    // Texture operations
    void OnFaceSelected(MeshFace face);
    void OnLoadTexture();
    void OnLoadFullTexture();
    void OnDownloadTemplate();
    void OnOpenInScribbler();
    void OnOpenInExternalEditor();
    void OnSelectExternalEditor();
    void OnReloadTexture();
    
    // Texture transforms
    void OnTextureOffsetChanged();
    void OnTextureScaleChanged();
    void OnTextureRotationChanged();
    
    // File watching
    void OnTextureFileChanged(const QString& file_path);
    
    // Preview
    void OnPreviewFaceClicked(MeshFace face);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupMeshParametersPanel();
    void setupTexturePanel();
    void setupPreviewPanel();
    void setupStatusBar();
    
    void updateMeshPreview();
    void updateTextureList();
    void updateTextureTransforms();
    void updateWindowTitle();
    void generateMesh();
    void generateTextureTemplate();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    QString getTextureFilter() const;
    QString getModelFilter() const;
    
    // UI Components
    QVBoxLayout* m_main_layout;
    QMenuBar* m_menu_bar;
    QToolBar* m_tool_bar;
    QSplitter* m_main_splitter;
    QStatusBar* m_status_bar;
    
    // Mesh parameters panel
    QWidget* m_mesh_panel;
    QComboBox* m_mesh_type_combo;
    QDoubleSpinBox* m_width_spin;
    QDoubleSpinBox* m_height_spin;
    QDoubleSpinBox* m_depth_spin;
    QSpinBox* m_subdivisions_spin;
    QDoubleSpinBox* m_radius_spin;
    QDoubleSpinBox* m_mesh_height_spin;
    QCheckBox* m_closed_check;
    
    // Texture panel
    QWidget* m_texture_panel;
    QListWidget* m_face_list;
    QLabel* m_texture_preview;
    QPushButton* m_load_texture_btn;
    QPushButton* m_load_full_texture_btn;
    QPushButton* m_download_template_btn;
    QPushButton* m_open_scribbler_btn;
    QPushButton* m_open_external_btn;
    QPushButton* m_select_external_btn;
    QPushButton* m_reload_texture_btn;
    
    // Texture transform controls
    QDoubleSpinBox* m_offset_x_spin;
    QDoubleSpinBox* m_offset_y_spin;
    QDoubleSpinBox* m_scale_x_spin;
    QDoubleSpinBox* m_scale_y_spin;
    QDoubleSpinBox* m_rotation_spin;
    
    // Preview panel
    TileBuilderPreview* m_preview;
    
    // Data
    TileBuilderData m_tile_data;
    std::shared_ptr<Tileset3DResource> m_target_tileset;
    QString m_current_file_path;
    bool m_modified;
    MeshFace m_selected_face;
    QString m_external_editor_path;
    
    // File watching
    TileTextureWatcher* m_texture_watcher;
    
    // Progress
    QProgressBar* m_progress_bar;
};

} // namespace Lupine
