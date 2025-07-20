#include "lupine/editor/TileBuilderDialog.h"
#include "lupine/resources/Tileset3DResource.h"
#include "dialogs/ScribblerDialog.h"
#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QInputDialog>
#include <iostream>

namespace Lupine {

TileBuilderDialog::TileBuilderDialog(QWidget* parent)
    : QDialog(parent)
    , m_main_layout(nullptr)
    , m_menu_bar(nullptr)
    , m_tool_bar(nullptr)
    , m_main_splitter(nullptr)
    , m_status_bar(nullptr)
    , m_mesh_panel(nullptr)
    , m_texture_panel(nullptr)
    , m_preview(nullptr)
    , m_modified(false)
    , m_selected_face(MeshFace::Front)
    , m_texture_watcher(nullptr)
    , m_progress_bar(nullptr) {
    
    setWindowTitle("Tile Builder");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    setModal(false);
    
    // Initialize texture watcher
    m_texture_watcher = new TileTextureWatcher(this);
    connect(m_texture_watcher, &TileTextureWatcher::FileChanged,
            this, &TileBuilderDialog::OnTextureFileChanged);
    
    // Initialize with default cube
    m_tile_data.mesh_params.type = TilePrimitiveType::Cube;

    setupUI();

    // Generate mesh after UI is set up
    generateMesh();
    updateMeshPreview();
    updateWindowTitle();
    updateTextureList();
}

TileBuilderDialog::~TileBuilderDialog() {
    // Cleanup temporary files
    if (!m_tile_data.temp_model_path.empty()) {
        QFile::remove(QString::fromStdString(m_tile_data.temp_model_path));
    }
    if (!m_tile_data.temp_texture_template_path.empty()) {
        QFile::remove(QString::fromStdString(m_tile_data.temp_texture_template_path));
    }
    if (!m_tile_data.temp_material_path.empty()) {
        QFile::remove(QString::fromStdString(m_tile_data.temp_material_path));
    }
}

void TileBuilderDialog::SetTargetTileset(std::shared_ptr<Tileset3DResource> tileset) {
    m_target_tileset = tileset;
}

void TileBuilderDialog::NewTile() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    // Clear texture watcher
    if (m_texture_watcher) {
        m_texture_watcher->ClearFiles();
    }

    // Reset to default state
    m_tile_data = TileBuilderData();
    m_tile_data.mesh_params.type = TilePrimitiveType::Cube;
    m_current_file_path.clear();
    m_selected_face = MeshFace::Front;

    // Clear all face textures
    m_tile_data.face_textures.clear();

    // Reset UI controls to default values
    if (m_mesh_type_combo) {
        m_mesh_type_combo->setCurrentIndex(0); // Cube
    }

    generateMesh();
    updateMeshPreview();
    updateTextureList();
    updateTextureTransforms();

    // Clear preview textures
    if (m_preview) {
        auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
        for (MeshFace face : available_faces) {
            m_preview->UpdateFaceTexture(face, QString());
        }
    }

    setModified(false);
    updateWindowTitle();
}

void TileBuilderDialog::LoadTile() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }
    
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Tile",
        QDir::currentPath(),
        "Tile Builder Files (*.tilebuilder);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        // TODO: Implement tile loading from JSON
        QMessageBox::information(this, "Info", "Tile loading will be implemented in the next phase.");
    }
}

void TileBuilderDialog::SaveTile() {
    if (m_current_file_path.isEmpty()) {
        SaveTileAs();
    } else {
        // TODO: Implement tile saving to JSON
        setModified(false);
        updateWindowTitle();
    }
}

void TileBuilderDialog::SaveTileAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Tile",
        QDir::currentPath(),
        "Tile Builder Files (*.tilebuilder);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        m_current_file_path = filepath;
        SaveTile();
    }
}

void TileBuilderDialog::ExportToOBJ() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Export to OBJ",
        QDir::currentPath() + "/" + QString::fromStdString(m_tile_data.name) + ".obj",
        "Wavefront OBJ (*.obj);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        m_progress_bar->setVisible(true);
        m_progress_bar->setRange(0, 0); // Indeterminate progress
        m_status_bar->showMessage("Exporting to OBJ...");

        QApplication::processEvents();

        bool success = TileOBJExporter::ExportToOBJ(m_tile_data, filepath.toStdString(), true);

        m_progress_bar->setVisible(false);

        if (success) {
            m_status_bar->showMessage("OBJ export completed successfully", 3000);
            QMessageBox::information(this, "Export Complete",
                                   "OBJ file exported successfully!\n\nFile: " + filepath);
        } else {
            m_status_bar->showMessage("OBJ export failed", 3000);
            QMessageBox::warning(this, "Export Failed", "Failed to export OBJ file.");
        }
    }
}

void TileBuilderDialog::AddToTileset() {
    if (!m_target_tileset) {
        QMessageBox::warning(this, "Warning", "No target tileset selected.");
        return;
    }

    if (m_tile_data.mesh_data.vertices.empty()) {
        QMessageBox::warning(this, "Warning", "No mesh data to add to tileset.");
        return;
    }

    // Get tile name from user
    bool ok;
    QString tile_name = QInputDialog::getText(this, "Add to Tileset",
                                            "Enter tile name:", QLineEdit::Normal,
                                            QString::fromStdString(m_tile_data.name), &ok);
    if (!ok || tile_name.isEmpty()) {
        return;
    }

    m_progress_bar->setVisible(true);
    m_progress_bar->setRange(0, 100);
    m_status_bar->showMessage("Adding tile to tileset...");

    try {
        // Create a permanent directory for the tile assets
        QString tileset_dir = QDir::currentPath(); // Use current directory as fallback
        QString tile_assets_dir = tileset_dir + "/tile_assets/" + tile_name;
        QDir().mkpath(tile_assets_dir);

        m_progress_bar->setValue(20);
        QApplication::processEvents();

        // Export OBJ file
        QString obj_path = tile_assets_dir + "/" + tile_name + ".obj";
        if (!TileOBJExporter::ExportToOBJ(m_tile_data, obj_path.toStdString(), true)) {
            throw std::runtime_error("Failed to export OBJ file");
        }

        m_progress_bar->setValue(50);
        QApplication::processEvents();

        // Copy textures to tile assets directory
        std::map<MeshFace, QString> copied_textures;
        for (const auto& face_pair : m_tile_data.face_textures) {
            if (!face_pair.second.texture_path.empty()) {
                QString source_path = QString::fromStdString(face_pair.second.texture_path);
                QFileInfo source_info(source_path);
                QString dest_path = tile_assets_dir + "/" + source_info.fileName();

                if (QFile::copy(source_path, dest_path)) {
                    copied_textures[face_pair.first] = dest_path;
                }
            }
        }

        m_progress_bar->setValue(70);
        QApplication::processEvents();

        // Create tile data for the tileset
        Lupine::Tile3DData tile_data;
        tile_data.id = m_target_tileset->GetNextTileId();
        tile_data.name = tile_name.toStdString();
        tile_data.mesh_path = QDir(tileset_dir).relativeFilePath(obj_path).toStdString();

        // Generate a preview image (simplified - just use the first texture if available)
        if (!copied_textures.empty()) {
            QString preview_path = tile_assets_dir + "/" + tile_name + "_preview.png";
            QString first_texture = copied_textures.begin()->second;

            // Create a simple preview by copying and scaling the first texture
            QImage preview_image(first_texture);
            if (!preview_image.isNull()) {
                QImage scaled = preview_image.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (scaled.save(preview_path)) {
                    tile_data.preview_image_path = QDir(tileset_dir).relativeFilePath(preview_path).toStdString();
                }
            }
        }

        m_progress_bar->setValue(90);
        QApplication::processEvents();

        // Add tile to tileset
        m_target_tileset->AddTile(tile_data);
        int tile_id = tile_data.id; // Use the tile's ID

        m_progress_bar->setValue(100);
        QApplication::processEvents();

        m_progress_bar->setVisible(false);
        m_status_bar->showMessage("Tile added to tileset successfully", 3000);

        // Emit signal to notify that a tile was added
        emit tileAddedToTileset(tile_id);

        QMessageBox::information(this, "Success",
                               QString("Tile '%1' added to tileset successfully!\n\nTile ID: %2\nAssets saved to: %3")
                               .arg(tile_name)
                               .arg(tile_id)
                               .arg(tile_assets_dir));

        // Mark tile as ready for export
        m_tile_data.ready_for_export = true;
        setModified(false);

    } catch (const std::exception& e) {
        m_progress_bar->setVisible(false);
        m_status_bar->showMessage("Failed to add tile to tileset", 3000);
        QMessageBox::critical(this, "Error",
                            QString("Failed to add tile to tileset:\n%1").arg(e.what()));
    }
}

void TileBuilderDialog::OnMeshTypeChanged() {
    if (!m_mesh_type_combo) return;
    
    int index = m_mesh_type_combo->currentIndex();
    TilePrimitiveType new_type = static_cast<TilePrimitiveType>(index);
    
    if (new_type != m_tile_data.mesh_params.type) {
        m_tile_data.mesh_params.type = new_type;
        generateMesh();
        updateMeshPreview();
        updateTextureList();
        setModified(true);
    }
}

void TileBuilderDialog::OnDimensionsChanged() {
    if (!m_width_spin || !m_height_spin || !m_depth_spin) return;
    
    glm::vec3 new_dimensions(
        static_cast<float>(m_width_spin->value()),
        static_cast<float>(m_height_spin->value()),
        static_cast<float>(m_depth_spin->value())
    );
    
    if (new_dimensions != m_tile_data.mesh_params.dimensions) {
        m_tile_data.mesh_params.dimensions = new_dimensions;
        generateMesh();
        updateMeshPreview();
        setModified(true);
    }
}

void TileBuilderDialog::OnSubdivisionsChanged() {
    if (!m_subdivisions_spin) return;
    
    int new_subdivisions = m_subdivisions_spin->value();
    if (new_subdivisions != m_tile_data.mesh_params.subdivisions) {
        m_tile_data.mesh_params.subdivisions = new_subdivisions;
        generateMesh();
        updateMeshPreview();
        setModified(true);
    }
}

void TileBuilderDialog::OnRadiusChanged() {
    if (!m_radius_spin) return;
    
    float new_radius = static_cast<float>(m_radius_spin->value());
    if (new_radius != m_tile_data.mesh_params.radius) {
        m_tile_data.mesh_params.radius = new_radius;
        generateMesh();
        updateMeshPreview();
        setModified(true);
    }
}

void TileBuilderDialog::OnHeightChanged() {
    if (!m_mesh_height_spin) return;
    
    float new_height = static_cast<float>(m_mesh_height_spin->value());
    if (new_height != m_tile_data.mesh_params.height) {
        m_tile_data.mesh_params.height = new_height;
        generateMesh();
        updateMeshPreview();
        setModified(true);
    }
}

void TileBuilderDialog::OnClosedChanged() {
    if (!m_closed_check) return;
    
    bool new_closed = m_closed_check->isChecked();
    TilePrimitiveType new_type = new_closed ? TilePrimitiveType::CylinderClosed : TilePrimitiveType::CylinderOpen;
    
    if (m_tile_data.mesh_params.type == TilePrimitiveType::CylinderOpen || 
        m_tile_data.mesh_params.type == TilePrimitiveType::CylinderClosed) {
        if (new_type != m_tile_data.mesh_params.type) {
            m_tile_data.mesh_params.type = new_type;
            generateMesh();
            updateMeshPreview();
            updateTextureList();
            setModified(true);
        }
    }
}

void TileBuilderDialog::generateMesh() {
    m_tile_data.mesh_data = TilePrimitiveMeshGenerator::GenerateMesh(m_tile_data.mesh_params);
    
    // Clear existing face textures for faces that no longer exist
    auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
    auto it = m_tile_data.face_textures.begin();
    while (it != m_tile_data.face_textures.end()) {
        if (std::find(available_faces.begin(), available_faces.end(), it->first) == available_faces.end()) {
            it = m_tile_data.face_textures.erase(it);
        } else {
            ++it;
        }
    }
    
    // Generate texture template
    generateTextureTemplate();
}

void TileBuilderDialog::generateTextureTemplate() {
    // Create temporary directory for templates
    QString temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/lupine_tile_builder";
    QDir().mkpath(temp_dir);
    
    QString template_path = temp_dir + "/template.png";
    m_tile_data.temp_texture_template_path = template_path.toStdString();
    
    // Generate the template
    TileTextureTemplateGenerator::GenerateTemplate(
        m_tile_data.mesh_data,
        glm::ivec2(512, 512),
        m_tile_data.temp_texture_template_path
    );
}

void TileBuilderDialog::setupUI() {
    m_main_layout = new QVBoxLayout(this);
    m_main_layout->setContentsMargins(0, 0, 0, 0);
    m_main_layout->setSpacing(0);

    setupMenuBar();
    setupToolBar();
    setupMainPanels();
    setupStatusBar();
}

void TileBuilderDialog::setupMenuBar() {
    m_menu_bar = new QMenuBar(this);
    m_main_layout->addWidget(m_menu_bar);

    // File menu
    QMenu* file_menu = m_menu_bar->addMenu("&File");

    QAction* new_action = file_menu->addAction("&New Tile");
    new_action->setShortcut(QKeySequence::New);
    connect(new_action, &QAction::triggered, this, &TileBuilderDialog::NewTile);

    QAction* load_action = file_menu->addAction("&Load Tile...");
    load_action->setShortcut(QKeySequence::Open);
    connect(load_action, &QAction::triggered, this, &TileBuilderDialog::LoadTile);

    QAction* save_action = file_menu->addAction("&Save Tile");
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &TileBuilderDialog::SaveTile);

    QAction* save_as_action = file_menu->addAction("Save Tile &As...");
    save_as_action->setShortcut(QKeySequence::SaveAs);
    connect(save_as_action, &QAction::triggered, this, &TileBuilderDialog::SaveTileAs);

    file_menu->addSeparator();

    QAction* export_obj_action = file_menu->addAction("&Export to OBJ...");
    connect(export_obj_action, &QAction::triggered, this, &TileBuilderDialog::ExportToOBJ);

    file_menu->addSeparator();

    QAction* add_tileset_action = file_menu->addAction("&Add to Tileset");
    connect(add_tileset_action, &QAction::triggered, this, &TileBuilderDialog::AddToTileset);

    file_menu->addSeparator();

    QAction* close_action = file_menu->addAction("&Close");
    close_action->setShortcut(QKeySequence::Close);
    connect(close_action, &QAction::triggered, this, &QDialog::close);
}

void TileBuilderDialog::setupToolBar() {
    m_tool_bar = new QToolBar(this);
    m_main_layout->addWidget(m_tool_bar);

    m_tool_bar->addAction("New", this, &TileBuilderDialog::NewTile);
    m_tool_bar->addAction("Load", this, &TileBuilderDialog::LoadTile);
    m_tool_bar->addAction("Save", this, &TileBuilderDialog::SaveTile);
    m_tool_bar->addSeparator();
    m_tool_bar->addAction("Export OBJ", this, &TileBuilderDialog::ExportToOBJ);
    m_tool_bar->addAction("Add to Tileset", this, &TileBuilderDialog::AddToTileset);
}

void TileBuilderDialog::setupMainPanels() {
    m_main_splitter = new QSplitter(Qt::Horizontal, this);
    m_main_layout->addWidget(m_main_splitter);

    setupMeshParametersPanel();
    setupPreviewPanel();
    setupTexturePanel();

    // Set splitter proportions
    m_main_splitter->setSizes({300, 600, 300});
}

void TileBuilderDialog::setupMeshParametersPanel() {
    m_mesh_panel = new QWidget();
    m_main_splitter->addWidget(m_mesh_panel);

    QVBoxLayout* layout = new QVBoxLayout(m_mesh_panel);

    // Mesh type selection
    QGroupBox* type_group = new QGroupBox("Mesh Type");
    QVBoxLayout* type_layout = new QVBoxLayout(type_group);

    m_mesh_type_combo = new QComboBox();
    m_mesh_type_combo->addItems({
        "Cube", "Rectangle", "Triangular Pyramid", "Pyramid",
        "Cone", "Sphere", "Cylinder (Open)", "Cylinder (Closed)"
    });
    connect(m_mesh_type_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TileBuilderDialog::OnMeshTypeChanged);
    type_layout->addWidget(m_mesh_type_combo);
    layout->addWidget(type_group);

    // Dimensions
    QGroupBox* dim_group = new QGroupBox("Dimensions");
    QGridLayout* dim_layout = new QGridLayout(dim_group);

    dim_layout->addWidget(new QLabel("Width:"), 0, 0);
    m_width_spin = new QDoubleSpinBox();
    m_width_spin->setRange(0.1, 10.0);
    m_width_spin->setValue(1.0);
    m_width_spin->setSingleStep(0.1);
    connect(m_width_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnDimensionsChanged);
    dim_layout->addWidget(m_width_spin, 0, 1);

    dim_layout->addWidget(new QLabel("Height:"), 1, 0);
    m_height_spin = new QDoubleSpinBox();
    m_height_spin->setRange(0.1, 10.0);
    m_height_spin->setValue(1.0);
    m_height_spin->setSingleStep(0.1);
    connect(m_height_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnDimensionsChanged);
    dim_layout->addWidget(m_height_spin, 1, 1);

    dim_layout->addWidget(new QLabel("Depth:"), 2, 0);
    m_depth_spin = new QDoubleSpinBox();
    m_depth_spin->setRange(0.1, 10.0);
    m_depth_spin->setValue(1.0);
    m_depth_spin->setSingleStep(0.1);
    connect(m_depth_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnDimensionsChanged);
    dim_layout->addWidget(m_depth_spin, 2, 1);
    layout->addWidget(dim_group);

    // Advanced parameters
    QGroupBox* advanced_group = new QGroupBox("Advanced");
    QGridLayout* advanced_layout = new QGridLayout(advanced_group);

    advanced_layout->addWidget(new QLabel("Subdivisions:"), 0, 0);
    m_subdivisions_spin = new QSpinBox();
    m_subdivisions_spin->setRange(3, 64);
    m_subdivisions_spin->setValue(16);
    connect(m_subdivisions_spin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TileBuilderDialog::OnSubdivisionsChanged);
    advanced_layout->addWidget(m_subdivisions_spin, 0, 1);

    advanced_layout->addWidget(new QLabel("Radius:"), 1, 0);
    m_radius_spin = new QDoubleSpinBox();
    m_radius_spin->setRange(0.1, 5.0);
    m_radius_spin->setValue(0.5);
    m_radius_spin->setSingleStep(0.1);
    connect(m_radius_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnRadiusChanged);
    advanced_layout->addWidget(m_radius_spin, 1, 1);

    advanced_layout->addWidget(new QLabel("Mesh Height:"), 2, 0);
    m_mesh_height_spin = new QDoubleSpinBox();
    m_mesh_height_spin->setRange(0.1, 5.0);
    m_mesh_height_spin->setValue(1.0);
    m_mesh_height_spin->setSingleStep(0.1);
    connect(m_mesh_height_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnHeightChanged);
    advanced_layout->addWidget(m_mesh_height_spin, 2, 1);

    m_closed_check = new QCheckBox("Closed (for cylinders)");
    m_closed_check->setChecked(true);
    connect(m_closed_check, &QCheckBox::toggled,
            this, &TileBuilderDialog::OnClosedChanged);
    advanced_layout->addWidget(m_closed_check, 3, 0, 1, 2);
    layout->addWidget(advanced_group);

    layout->addStretch();
}

void TileBuilderDialog::setupPreviewPanel() {
    m_preview = new TileBuilderPreview();
    m_main_splitter->addWidget(m_preview);

    connect(m_preview, &TileBuilderPreview::FaceClicked,
            this, &TileBuilderDialog::OnPreviewFaceClicked);
}

void TileBuilderDialog::setupStatusBar() {
    m_status_bar = new QStatusBar(this);
    m_main_layout->addWidget(m_status_bar);

    m_progress_bar = new QProgressBar();
    m_progress_bar->setVisible(false);
    m_status_bar->addPermanentWidget(m_progress_bar);

    m_status_bar->showMessage("Ready");
}

bool TileBuilderDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool TileBuilderDialog::promptSaveChanges() {
    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<TileBuilderDialog*>(this),
        "Unsaved Changes",
        "The tile has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (result == QMessageBox::Save) {
        SaveTile();
        return !hasUnsavedChanges(); // Return true if save was successful
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void TileBuilderDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

void TileBuilderDialog::updateWindowTitle() {
    QString title = "Tile Builder";
    if (!m_current_file_path.isEmpty()) {
        title += " - " + QFileInfo(m_current_file_path).baseName();
    } else {
        title += " - " + QString::fromStdString(m_tile_data.name);
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void TileBuilderDialog::updateMeshPreview() {
    if (m_preview) {
        m_preview->SetTileData(m_tile_data);
        m_preview->RefreshPreview();
    }
}

void TileBuilderDialog::setupTexturePanel() {
    m_texture_panel = new QWidget();
    m_main_splitter->addWidget(m_texture_panel);

    QVBoxLayout* layout = new QVBoxLayout(m_texture_panel);

    // Face selection
    QGroupBox* face_group = new QGroupBox("Faces");
    QVBoxLayout* face_layout = new QVBoxLayout(face_group);

    m_face_list = new QListWidget();
    connect(m_face_list, &QListWidget::currentRowChanged,
            this, [this](int row) {
                if (row >= 0) {
                    auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
                    if (row < static_cast<int>(available_faces.size())) {
                        m_selected_face = available_faces[row];
                        OnFaceSelected(m_selected_face);
                    }
                }
            });
    face_layout->addWidget(m_face_list);
    layout->addWidget(face_group);

    // Texture operations
    QGroupBox* texture_group = new QGroupBox("Texture Operations");
    QVBoxLayout* texture_layout = new QVBoxLayout(texture_group);

    m_load_texture_btn = new QPushButton("Load Face Texture...");
    connect(m_load_texture_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnLoadTexture);
    texture_layout->addWidget(m_load_texture_btn);

    m_load_full_texture_btn = new QPushButton("Load Full Texture...");
    connect(m_load_full_texture_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnLoadFullTexture);
    texture_layout->addWidget(m_load_full_texture_btn);

    m_download_template_btn = new QPushButton("Download Template");
    connect(m_download_template_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnDownloadTemplate);
    texture_layout->addWidget(m_download_template_btn);

    texture_layout->addWidget(new QLabel("Edit Texture:"));

    m_open_scribbler_btn = new QPushButton("Open in Scribbler");
    connect(m_open_scribbler_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnOpenInScribbler);
    texture_layout->addWidget(m_open_scribbler_btn);

    m_open_external_btn = new QPushButton("Open in External Editor");
    connect(m_open_external_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnOpenInExternalEditor);
    texture_layout->addWidget(m_open_external_btn);

    m_select_external_btn = new QPushButton("Select External Editor...");
    connect(m_select_external_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnSelectExternalEditor);
    texture_layout->addWidget(m_select_external_btn);

    m_reload_texture_btn = new QPushButton("Reload Texture");
    connect(m_reload_texture_btn, &QPushButton::clicked, this, &TileBuilderDialog::OnReloadTexture);
    texture_layout->addWidget(m_reload_texture_btn);

    layout->addWidget(texture_group);

    // Texture transforms
    QGroupBox* transform_group = new QGroupBox("Texture Transform");
    QGridLayout* transform_layout = new QGridLayout(transform_group);

    transform_layout->addWidget(new QLabel("Offset X:"), 0, 0);
    m_offset_x_spin = new QDoubleSpinBox();
    m_offset_x_spin->setRange(-10.0, 10.0);
    m_offset_x_spin->setSingleStep(0.1);
    connect(m_offset_x_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnTextureOffsetChanged);
    transform_layout->addWidget(m_offset_x_spin, 0, 1);

    transform_layout->addWidget(new QLabel("Offset Y:"), 1, 0);
    m_offset_y_spin = new QDoubleSpinBox();
    m_offset_y_spin->setRange(-10.0, 10.0);
    m_offset_y_spin->setSingleStep(0.1);
    connect(m_offset_y_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnTextureOffsetChanged);
    transform_layout->addWidget(m_offset_y_spin, 1, 1);

    transform_layout->addWidget(new QLabel("Scale X:"), 2, 0);
    m_scale_x_spin = new QDoubleSpinBox();
    m_scale_x_spin->setRange(0.1, 10.0);
    m_scale_x_spin->setValue(1.0);
    m_scale_x_spin->setSingleStep(0.1);
    connect(m_scale_x_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnTextureScaleChanged);
    transform_layout->addWidget(m_scale_x_spin, 2, 1);

    transform_layout->addWidget(new QLabel("Scale Y:"), 3, 0);
    m_scale_y_spin = new QDoubleSpinBox();
    m_scale_y_spin->setRange(0.1, 10.0);
    m_scale_y_spin->setValue(1.0);
    m_scale_y_spin->setSingleStep(0.1);
    connect(m_scale_y_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnTextureScaleChanged);
    transform_layout->addWidget(m_scale_y_spin, 3, 1);

    transform_layout->addWidget(new QLabel("Rotation:"), 4, 0);
    m_rotation_spin = new QDoubleSpinBox();
    m_rotation_spin->setRange(-360.0, 360.0);
    m_rotation_spin->setSingleStep(1.0);
    m_rotation_spin->setSuffix("°");
    connect(m_rotation_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TileBuilderDialog::OnTextureRotationChanged);
    transform_layout->addWidget(m_rotation_spin, 4, 1);

    layout->addWidget(transform_group);

    // Texture preview
    m_texture_preview = new QLabel();
    m_texture_preview->setMinimumSize(200, 200);
    m_texture_preview->setStyleSheet("border: 1px solid gray;");
    m_texture_preview->setAlignment(Qt::AlignCenter);
    m_texture_preview->setText("No texture");
    layout->addWidget(m_texture_preview);

    layout->addStretch();
}

void TileBuilderDialog::updateTextureList() {
    if (!m_face_list) return;

    m_face_list->clear();

    auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
    for (MeshFace face : available_faces) {
        QString face_name = QString::fromStdString(TilePrimitiveMeshGenerator::GetFaceName(face));

        // Check if face has texture
        if (m_tile_data.face_textures.find(face) != m_tile_data.face_textures.end() &&
            !m_tile_data.face_textures[face].texture_path.empty()) {
            face_name += " ✓";
        }

        m_face_list->addItem(face_name);
    }

    // Select first face if available
    if (!available_faces.empty()) {
        m_face_list->setCurrentRow(0);
        m_selected_face = available_faces[0];
    }
}

void TileBuilderDialog::OnFaceSelected(MeshFace face) {
    m_selected_face = face;
    updateTextureTransforms();

    // Update texture preview
    if (m_tile_data.face_textures.find(face) != m_tile_data.face_textures.end()) {
        const auto& face_texture = m_tile_data.face_textures[face];
        if (!face_texture.texture_path.empty()) {
            QPixmap pixmap(QString::fromStdString(face_texture.texture_path));
            if (!pixmap.isNull()) {
                m_texture_preview->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    } else {
        m_texture_preview->setText("No texture");
        m_texture_preview->setPixmap(QPixmap());
    }
}

void TileBuilderDialog::updateTextureTransforms() {
    if (m_tile_data.face_textures.find(m_selected_face) != m_tile_data.face_textures.end()) {
        const auto& transform = m_tile_data.face_textures[m_selected_face];

        m_offset_x_spin->setValue(transform.offset.x);
        m_offset_y_spin->setValue(transform.offset.y);
        m_scale_x_spin->setValue(transform.scale.x);
        m_scale_y_spin->setValue(transform.scale.y);
        m_rotation_spin->setValue(transform.rotation);
    } else {
        m_offset_x_spin->setValue(0.0);
        m_offset_y_spin->setValue(0.0);
        m_scale_x_spin->setValue(1.0);
        m_scale_y_spin->setValue(1.0);
        m_rotation_spin->setValue(0.0);
    }
}

// Texture operation implementations
void TileBuilderDialog::OnLoadTexture() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Face Texture",
        QDir::currentPath(),
        getTextureFilter()
    );

    if (!filepath.isEmpty()) {
        m_tile_data.face_textures[m_selected_face].texture_path = filepath.toStdString();
        m_tile_data.face_textures[m_selected_face].use_full_texture = false;

        // Add to file watcher
        m_texture_watcher->AddFile(filepath);

        // Update preview
        if (m_preview) {
            m_preview->UpdateFaceTexture(m_selected_face, filepath);
        }

        OnFaceSelected(m_selected_face);
        updateTextureList();
        setModified(true);
    }
}

void TileBuilderDialog::OnLoadFullTexture() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Full Texture",
        QDir::currentPath(),
        getTextureFilter()
    );

    if (!filepath.isEmpty()) {
        // Apply to all faces
        auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
        for (MeshFace face : available_faces) {
            m_tile_data.face_textures[face].texture_path = filepath.toStdString();
            m_tile_data.face_textures[face].use_full_texture = true;

            // Update preview for each face
            if (m_preview) {
                m_preview->UpdateFaceTexture(face, filepath);
            }
        }

        // Add to file watcher
        m_texture_watcher->AddFile(filepath);

        OnFaceSelected(m_selected_face);
        updateTextureList();
        setModified(true);
    }
}

void TileBuilderDialog::OnDownloadTemplate() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Texture Template",
        QDir::currentPath() + "/texture_template.png",
        "PNG Files (*.png);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        if (!m_tile_data.temp_texture_template_path.empty()) {
            QFile::copy(QString::fromStdString(m_tile_data.temp_texture_template_path), filepath);
            QMessageBox::information(this, "Success", "Texture template saved successfully!");
        } else {
            QMessageBox::warning(this, "Error", "No texture template available.");
        }
    }
}

void TileBuilderDialog::OnOpenInScribbler() {
    QString texture_path;
    bool create_new = false;

    // Check if face has texture
    if (m_tile_data.face_textures.find(m_selected_face) != m_tile_data.face_textures.end() &&
        !m_tile_data.face_textures[m_selected_face].texture_path.empty()) {
        texture_path = QString::fromStdString(m_tile_data.face_textures[m_selected_face].texture_path);
    } else {
        // Create a new texture file based on template
        QString face_name = QString::fromStdString(TilePrimitiveMeshGenerator::GetFaceName(m_selected_face));
        QString temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/lupine_tile_builder";
        QDir().mkpath(temp_dir);

        texture_path = temp_dir + "/" + QString::fromStdString(m_tile_data.name) + "_" + face_name + ".png";

        // Generate face template
        if (m_tile_data.mesh_data.face_uv_bounds.find(m_selected_face) != m_tile_data.mesh_data.face_uv_bounds.end()) {
            TileTextureTemplateGenerator::GenerateFaceTemplate(
                m_selected_face,
                m_tile_data.mesh_data.face_uv_bounds.at(m_selected_face),
                glm::ivec2(512, 512),
                texture_path.toStdString()
            );
            create_new = true;
        } else {
            QMessageBox::warning(this, "Error", "No texture template available for this face.");
            return;
        }
    }

    // Launch Scribbler dialog
    try {
        ScribblerDialog* scribbler = new ScribblerDialog(this);

        if (create_new) {
            // Load the template we just created
            if (scribbler->LoadImage(texture_path)) {
                // Set the texture path for this face
                m_tile_data.face_textures[m_selected_face].texture_path = texture_path.toStdString();

                // Add to file watcher
                m_texture_watcher->AddFile(texture_path);

                updateTextureList();
                setModified(true);
            }
        } else {
            // Load existing texture
            scribbler->LoadImage(texture_path);
        }

        // Show Scribbler
        scribbler->show();
        scribbler->raise();
        scribbler->activateWindow();

        // Connect to save signal to update our preview
        connect(scribbler, &ScribblerDialog::imageSaved, this, [this, texture_path]() {
            // Reload texture in preview
            if (m_preview) {
                m_preview->UpdateFaceTexture(m_selected_face, texture_path);
            }
            OnFaceSelected(m_selected_face);
        });

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error",
                            QString("Failed to launch Scribbler: %1").arg(e.what()));
    }
}

void TileBuilderDialog::OnOpenInExternalEditor() {
    if (m_external_editor_path.isEmpty()) {
        OnSelectExternalEditor();
        if (m_external_editor_path.isEmpty()) {
            return;
        }
    }

    QString texture_path;

    // Check if face has texture
    if (m_tile_data.face_textures.find(m_selected_face) != m_tile_data.face_textures.end() &&
        !m_tile_data.face_textures[m_selected_face].texture_path.empty()) {
        texture_path = QString::fromStdString(m_tile_data.face_textures[m_selected_face].texture_path);
    } else {
        QMessageBox::warning(this, "Error", "No texture selected for this face.");
        return;
    }

    // Launch external editor
    QProcess::startDetached(m_external_editor_path, {texture_path});
}

void TileBuilderDialog::OnSelectExternalEditor() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Select External Image Editor",
        QDir::currentPath(),
        "Executable Files (*.exe);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        m_external_editor_path = filepath;
        // TODO: Save to settings
    }
}

void TileBuilderDialog::OnReloadTexture() {
    if (m_tile_data.face_textures.find(m_selected_face) != m_tile_data.face_textures.end()) {
        QString texture_path = QString::fromStdString(m_tile_data.face_textures[m_selected_face].texture_path);
        if (!texture_path.isEmpty()) {
            // Force reload texture in preview
            if (m_preview) {
                m_preview->UpdateFaceTexture(m_selected_face, texture_path);
            }
            OnFaceSelected(m_selected_face);
        }
    }
}

void TileBuilderDialog::OnTextureOffsetChanged() {
    if (m_offset_x_spin && m_offset_y_spin) {
        m_tile_data.face_textures[m_selected_face].offset = glm::vec2(
            static_cast<float>(m_offset_x_spin->value()),
            static_cast<float>(m_offset_y_spin->value())
        );
        setModified(true);

        // Update preview with new transform
        if (m_preview) {
            m_preview->SetTileData(m_tile_data);
        }
        updateMeshPreview();
    }
}

void TileBuilderDialog::OnTextureScaleChanged() {
    if (m_scale_x_spin && m_scale_y_spin) {
        m_tile_data.face_textures[m_selected_face].scale = glm::vec2(
            static_cast<float>(m_scale_x_spin->value()),
            static_cast<float>(m_scale_y_spin->value())
        );
        setModified(true);

        // Update preview with new transform
        if (m_preview) {
            m_preview->SetTileData(m_tile_data);
        }
        updateMeshPreview();
    }
}

void TileBuilderDialog::OnTextureRotationChanged() {
    if (m_rotation_spin) {
        m_tile_data.face_textures[m_selected_face].rotation = static_cast<float>(m_rotation_spin->value());
        setModified(true);

        // Update preview with new transform
        if (m_preview) {
            m_preview->SetTileData(m_tile_data);
        }
        updateMeshPreview();
    }
}

void TileBuilderDialog::OnTextureFileChanged(const QString& file_path) {
    // Find which face(s) use this texture and reload
    for (auto& pair : m_tile_data.face_textures) {
        if (QString::fromStdString(pair.second.texture_path) == file_path) {
            if (m_preview) {
                m_preview->UpdateFaceTexture(pair.first, file_path);
            }
        }
    }

    // Update preview if it's the currently selected face
    if (m_tile_data.face_textures.find(m_selected_face) != m_tile_data.face_textures.end() &&
        QString::fromStdString(m_tile_data.face_textures[m_selected_face].texture_path) == file_path) {
        OnFaceSelected(m_selected_face);
    }

    m_status_bar->showMessage("Texture reloaded: " + QFileInfo(file_path).fileName(), 2000);
}

void TileBuilderDialog::OnPreviewFaceClicked(MeshFace face) {
    // Find the face in the list and select it
    auto available_faces = TilePrimitiveMeshGenerator::GetAvailableFaces(m_tile_data.mesh_params.type);
    auto it = std::find(available_faces.begin(), available_faces.end(), face);
    if (it != available_faces.end()) {
        int index = static_cast<int>(std::distance(available_faces.begin(), it));
        if (m_face_list) {
            m_face_list->setCurrentRow(index);
        }
    }
}

QString TileBuilderDialog::getTextureFilter() const {
    return "Image Files (*.png *.jpg *.jpeg *.bmp *.tga *.tiff);;All Files (*)";
}

QString TileBuilderDialog::getModelFilter() const {
    return "Model Files (*.obj *.fbx *.dae *.gltf *.glb);;All Files (*)";
}

} // namespace Lupine
