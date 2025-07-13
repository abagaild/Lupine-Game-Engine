#include "TilemapPainterDialog.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QHeaderView>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColorDialog>
#include <QInputDialog>
#include <fstream>
#include <cmath>
#include <nlohmann/json.hpp>

#include "lupine/tilemap/TilemapData.h"
#include "lupine/resources/TilesetResource.h"

using json = nlohmann::json;

// TilemapCanvas Implementation
TilemapCanvas::TilemapCanvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_project(nullptr)
    , m_current_tool(PaintTool::Brush)
    , m_current_tileset_id(0)
    , m_current_tile_id(-1)
    , m_show_grid(true)
    , m_snap_to_grid(true)
    , m_painting(false)
{
    setupScene();
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
}

TilemapCanvas::~TilemapCanvas() {
    if (m_scene) {
        delete m_scene;
    }
}

void TilemapCanvas::SetProject(Lupine::TilemapProject* project) {
    m_project = project;
    
    // Load all tilesets
    if (m_project) {
        for (const auto& tileset_ref : m_project->GetTilesets()) {
            loadTileset(tileset_ref.id);
        }
    }
    
    updateCanvas();
}

void TilemapCanvas::SetCurrentTool(PaintTool tool) {
    m_current_tool = tool;
}

void TilemapCanvas::SetCurrentTile(int tileset_id, int tile_id) {
    m_current_tileset_id = tileset_id;
    m_current_tile_id = tile_id;
}

void TilemapCanvas::SetShowGrid(bool show) {
    m_show_grid = show;
    updateGrid();
}

void TilemapCanvas::SetSnapToGrid(bool snap) {
    m_snap_to_grid = snap;
}

Lupine::Tileset2DResource* TilemapCanvas::GetLoadedTileset(int tileset_id) {
    auto it = m_loaded_tilesets.find(tileset_id);
    return (it != m_loaded_tilesets.end()) ? it->second.get() : nullptr;
}

void TilemapCanvas::drawTileHighlight(int x, int y) {
    if (!m_project) return;

    glm::ivec2 tile_size = m_project->GetTileSize();
    QRectF highlight_rect(x * tile_size.x, y * tile_size.y, tile_size.x, tile_size.y);

    QPen highlightPen(QColor(255, 255, 0, 128), 2);
    QBrush highlightBrush(QColor(255, 255, 0, 32));

    QGraphicsRectItem* highlight = m_scene->addRect(highlight_rect, highlightPen, highlightBrush);
    highlight->setZValue(999); // High Z value to render on top
}

void TilemapCanvas::ZoomIn() {
    scale(1.25, 1.25);
}

void TilemapCanvas::ZoomOut() {
    scale(0.8, 0.8);
}

void TilemapCanvas::ZoomToFit() {
    if (m_project) {
        QRectF bounds(0, 0, 
                     m_project->GetSize().x * m_project->GetTileSize().x,
                     m_project->GetSize().y * m_project->GetTileSize().y);
        fitInView(bounds, Qt::KeepAspectRatio);
    }
}

void TilemapCanvas::ZoomToActual() {
    resetTransform();
}

void TilemapCanvas::mousePressEvent(QMouseEvent* event) {
    if (!m_project || event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    glm::ivec2 tile_pos = screenToTile(event->pos());
    
    if (tile_pos.x >= 0 && tile_pos.x < m_project->GetSize().x &&
        tile_pos.y >= 0 && tile_pos.y < m_project->GetSize().y) {
        
        emit tileClicked(tile_pos.x, tile_pos.y);
        
        switch (m_current_tool) {
            case PaintTool::Brush:
                m_painting = true;
                paintTile(tile_pos.x, tile_pos.y);
                break;
            case PaintTool::Bucket:
                floodFill(tile_pos.x, tile_pos.y);
                break;
            case PaintTool::Eraser:
                m_painting = true;
                eraseTile(tile_pos.x, tile_pos.y);
                break;
            case PaintTool::Eyedropper:
                // TODO: Implement eyedropper tool
                break;
            default:
                break;
        }
        
        m_last_paint_pos = event->pos();
    }
    
    QGraphicsView::mousePressEvent(event);
}

void TilemapCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (!m_project || !m_painting) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    glm::ivec2 tile_pos = screenToTile(event->pos());
    
    if (tile_pos.x >= 0 && tile_pos.x < m_project->GetSize().x &&
        tile_pos.y >= 0 && tile_pos.y < m_project->GetSize().y) {
        
        switch (m_current_tool) {
            case PaintTool::Brush:
                paintTile(tile_pos.x, tile_pos.y);
                break;
            case PaintTool::Eraser:
                eraseTile(tile_pos.x, tile_pos.y);
                break;
            default:
                break;
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void TilemapCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_painting = false;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void TilemapCanvas::wheelEvent(QWheelEvent* event) {
    // Zoom with mouse wheel
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void TilemapCanvas::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);
    
    // Draw additional overlays here if needed
}

void TilemapCanvas::onSceneChanged() {
    // Handle scene changes if needed
}

void TilemapCanvas::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    connect(m_scene, &QGraphicsScene::changed, this, &TilemapCanvas::onSceneChanged);
}

void TilemapCanvas::updateCanvas() {
    if (!m_project) {
        m_scene->clear();
        return;
    }
    
    m_scene->clear();
    
    // Set scene rect
    glm::ivec2 map_size = m_project->GetSize();
    glm::ivec2 tile_size = m_project->GetTileSize();
    m_scene->setSceneRect(0, 0, map_size.x * tile_size.x, map_size.y * tile_size.y);
    
    // Render all layers
    for (int layer_idx = 0; layer_idx < m_project->GetLayerCount(); ++layer_idx) {
        const Lupine::TilemapLayer* layer = m_project->GetLayer(layer_idx);
        if (!layer || !layer->IsVisible()) continue;
        
        for (int y = 0; y < map_size.y; ++y) {
            for (int x = 0; x < map_size.x; ++x) {
                const Lupine::TileInstance& tile = layer->GetTile(x, y);
                if (!tile.IsEmpty()) {
                    renderTile(x, y, tile, layer->GetOpacity());
                }
            }
        }
    }
    
    updateGrid();
}

void TilemapCanvas::updateGrid() {
    if (!m_project || !m_show_grid) return;
    
    glm::ivec2 map_size = m_project->GetSize();
    glm::ivec2 tile_size = m_project->GetTileSize();
    
    QPen gridPen(QColor(255, 255, 255, 64), 1);
    
    // Draw vertical lines
    for (int x = 0; x <= map_size.x; ++x) {
        int lineX = x * tile_size.x;
        QGraphicsLineItem* line = m_scene->addLine(
            lineX, 0,
            lineX, map_size.y * tile_size.y,
            gridPen
        );
        line->setZValue(1000); // High Z value to render on top
    }
    
    // Draw horizontal lines
    for (int y = 0; y <= map_size.y; ++y) {
        int lineY = y * tile_size.y;
        QGraphicsLineItem* line = m_scene->addLine(
            0, lineY,
            map_size.x * tile_size.x, lineY,
            gridPen
        );
        line->setZValue(1000); // High Z value to render on top
    }
}

void TilemapCanvas::paintTile(int x, int y) {
    if (!m_project || m_current_tile_id < 0) return;
    
    Lupine::TilemapLayer* layer = m_project->GetActiveLayer();
    if (!layer || layer->IsLocked()) return;
    
    Lupine::TileInstance tile(m_current_tile_id, m_current_tileset_id);
    layer->SetTile(x, y, tile);
    
    emit tilePainted(x, y, m_current_tileset_id, m_current_tile_id);
    emit projectModified();
    
    // Update just this tile in the view
    updateCanvas(); // For now, update entire canvas - could be optimized
}

void TilemapCanvas::floodFill(int x, int y) {
    if (!m_project || m_current_tile_id < 0) return;
    
    Lupine::TilemapLayer* layer = m_project->GetActiveLayer();
    if (!layer || layer->IsLocked()) return;
    
    Lupine::TileInstance tile(m_current_tile_id, m_current_tileset_id);
    layer->FloodFill(x, y, tile);
    
    emit projectModified();
    updateCanvas();
}

void TilemapCanvas::eraseTile(int x, int y) {
    if (!m_project) return;
    
    Lupine::TilemapLayer* layer = m_project->GetActiveLayer();
    if (!layer || layer->IsLocked()) return;
    
    layer->ClearTile(x, y);
    
    emit projectModified();
    updateCanvas();
}

glm::ivec2 TilemapCanvas::screenToTile(const QPoint& screen_pos) const {
    if (!m_project) return glm::ivec2(-1, -1);
    
    QPointF scene_pos = mapToScene(screen_pos);
    glm::ivec2 tile_size = m_project->GetTileSize();
    
    int tile_x = static_cast<int>(scene_pos.x() / tile_size.x);
    int tile_y = static_cast<int>(scene_pos.y() / tile_size.y);
    
    return glm::ivec2(tile_x, tile_y);
}

QPoint TilemapCanvas::tileToScreen(const glm::ivec2& tile_pos) const {
    if (!m_project) return QPoint(0, 0);
    
    glm::ivec2 tile_size = m_project->GetTileSize();
    QPointF scene_pos(tile_pos.x * tile_size.x, tile_pos.y * tile_size.y);
    QPoint screen_pos = mapFromScene(scene_pos);
    
    return screen_pos;
}

void TilemapCanvas::renderTile(int x, int y, const Lupine::TileInstance& tile, float layer_opacity) {
    auto tileset_it = m_loaded_tilesets.find(tile.tileset_id);
    if (tileset_it == m_loaded_tilesets.end()) return;

    auto pixmap_it = m_tileset_pixmaps.find(tile.tileset_id);
    if (pixmap_it == m_tileset_pixmaps.end()) return;

    const Lupine::Tileset2DResource* tileset = tileset_it->second.get();
    const QPixmap& tileset_pixmap = pixmap_it->second;

    const Lupine::TileData* tile_data = tileset->GetTile(tile.tile_id);
    if (!tile_data) return;

    // Calculate tile position
    glm::ivec2 tile_size = m_project->GetTileSize();
    QRectF dest_rect(x * tile_size.x, y * tile_size.y, tile_size.x, tile_size.y);

    // Get source rect from tileset
    glm::vec4 tex_region = tile_data->texture_region;
    QRectF source_rect(tex_region.x, tex_region.y, tex_region.z, tex_region.w);

    // Create pixmap item
    QPixmap tile_pixmap = tileset_pixmap.copy(source_rect.toRect());
    tile_pixmap = tile_pixmap.scaled(tile_size.x, tile_size.y, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    QGraphicsPixmapItem* item = m_scene->addPixmap(tile_pixmap);
    item->setPos(dest_rect.topLeft());
    item->setOpacity(layer_opacity * tile.metadata.opacity);

    // Apply tint if needed
    if (tile.metadata.tint != glm::vec4(1.0f)) {
        // TODO: Apply color tint to the pixmap
    }
}

void TilemapCanvas::loadTileset(int tileset_id) {
    const Lupine::TilesetReference* tileset_ref = m_project->GetTileset(tileset_id);
    if (!tileset_ref) return;

    auto tileset = std::make_unique<Lupine::Tileset2DResource>();
    if (tileset->LoadFromFile(tileset_ref->path)) {
        // Load the tileset image
        QPixmap pixmap(QString::fromStdString(tileset->GetTexturePath()));
        if (!pixmap.isNull()) {
            m_tileset_pixmaps[tileset_id] = pixmap;
        }

        m_loaded_tilesets[tileset_id] = std::move(tileset);
    }
}

// TilesetPalette Implementation
TilesetPalette::TilesetPalette(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_tileset_item(nullptr)
    , m_selection_rect(nullptr)
    , m_tileset_id(-1)
    , m_tileset(nullptr)
    , m_selected_tile_id(-1)
{
    setupScene();
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
}

TilesetPalette::~TilesetPalette() {
    if (m_scene) {
        delete m_scene;
    }
}

void TilesetPalette::SetTileset(int tileset_id, Lupine::Tileset2DResource* tileset) {
    m_tileset_id = tileset_id;
    m_tileset = tileset;
    m_selected_tile_id = -1;
    updatePalette();
}

void TilesetPalette::ClearTileset() {
    m_tileset_id = -1;
    m_tileset = nullptr;
    m_selected_tile_id = -1;
    m_scene->clear();
    m_tileset_item = nullptr;
    m_selection_rect = nullptr;
}

void TilesetPalette::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_tileset) {
        QPointF scene_pos = mapToScene(event->pos());
        int tile_id = getTileIdAt(scene_pos);
        if (tile_id >= 0) {
            m_selected_tile_id = tile_id;
            updatePalette(); // Update selection rectangle
            emit tileSelected(m_tileset_id, tile_id);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void TilesetPalette::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
}

void TilesetPalette::updatePalette() {
    m_scene->clear();
    m_tileset_item = nullptr;
    m_selection_rect = nullptr;

    if (!m_tileset || m_tileset->GetTexturePath().empty()) {
        return;
    }

    // Load tileset image
    QString imagePath = QString::fromStdString(m_tileset->GetTexturePath());
    m_tileset_pixmap = QPixmap(imagePath);

    if (m_tileset_pixmap.isNull()) {
        return;
    }

    // Add tileset image to scene
    m_tileset_item = m_scene->addPixmap(m_tileset_pixmap);
    m_tileset_item->setPos(0, 0);

    // Draw grid lines
    QPen gridPen(QColor(255, 255, 255, 128), 1);
    glm::ivec2 tileSize = m_tileset->GetTileSize();
    glm::ivec2 gridSize = m_tileset->GetGridSize();
    int spacing = m_tileset->GetSpacing();
    int margin = m_tileset->GetMargin();

    // Draw vertical lines
    for (int x = 0; x <= gridSize.x; ++x) {
        int lineX = margin + x * (tileSize.x + spacing);
        if (x == gridSize.x) lineX -= spacing;

        QGraphicsLineItem* line = m_scene->addLine(
            lineX, margin,
            lineX, margin + gridSize.y * (tileSize.y + spacing) - spacing,
            gridPen
        );
        line->setZValue(1);
    }

    // Draw horizontal lines
    for (int y = 0; y <= gridSize.y; ++y) {
        int lineY = margin + y * (tileSize.y + spacing);
        if (y == gridSize.y) lineY -= spacing;

        QGraphicsLineItem* line = m_scene->addLine(
            margin, lineY,
            margin + gridSize.x * (tileSize.x + spacing) - spacing, lineY,
            gridPen
        );
        line->setZValue(1);
    }

    // Draw selection rectangle if a tile is selected
    if (m_selected_tile_id >= 0) {
        const Lupine::TileData* tile = m_tileset->GetTile(m_selected_tile_id);
        if (tile) {
            QPen selectionPen(QColor(255, 0, 255), 2);
            QRectF tileRect(tile->texture_region.x, tile->texture_region.y,
                           tile->texture_region.z, tile->texture_region.w);
            m_selection_rect = m_scene->addRect(tileRect, selectionPen);
            m_selection_rect->setZValue(2);
        }
    }

    // Fit view to contents
    fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

int TilesetPalette::getTileIdAt(const QPointF& scene_pos) const {
    if (!m_tileset) return -1;

    glm::ivec2 tileSize = m_tileset->GetTileSize();
    glm::ivec2 gridSize = m_tileset->GetGridSize();
    int spacing = m_tileset->GetSpacing();
    int margin = m_tileset->GetMargin();

    // Calculate grid position
    int x = static_cast<int>((scene_pos.x() - margin) / (tileSize.x + spacing));
    int y = static_cast<int>((scene_pos.y() - margin) / (tileSize.y + spacing));

    // Check bounds
    if (x < 0 || x >= gridSize.x || y < 0 || y >= gridSize.y) {
        return -1;
    }

    return m_tileset->GetTileIdFromGridPosition(glm::ivec2(x, y));
}

// TilemapPainterDialog Implementation
TilemapPainterDialog::TilemapPainterDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_project(std::make_unique<Lupine::TilemapProject>())
    , m_modified(false)
    , m_currentTool(PaintTool::Brush)
    , m_currentTilesetId(-1)
    , m_currentTileId(-1)
{
    setWindowTitle("Tilemap Painter");
    setMinimumSize(1400, 900);
    resize(1600, 1000);

    setupUI();
    updateWindowTitle();
    updateCanvas();
}

TilemapPainterDialog::~TilemapPainterDialog() {
}

void TilemapPainterDialog::NewProject() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_project = std::make_unique<Lupine::TilemapProject>();
    m_currentFilePath.clear();
    setModified(false);

    updateProjectProperties();
    updateTilesetList();
    updateLayerList();
    updateCanvas();
    updatePalette();
    updateWindowTitle();
}

void TilemapPainterDialog::LoadProject(const QString& filepath) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    auto newProject = std::make_unique<Lupine::TilemapProject>();
    if (newProject->LoadFromFile(filepath.toStdString())) {
        m_project = std::move(newProject);
        m_currentFilePath = filepath;
        setModified(false);

        updateProjectProperties();
        updateTilesetList();
        updateLayerList();
        updateCanvas();
        updatePalette();
        updateWindowTitle();

        QMessageBox::information(this, "Success", "Tilemap project loaded successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to load tilemap project!");
    }
}

void TilemapPainterDialog::SaveProject() {
    if (m_currentFilePath.isEmpty()) {
        SaveProjectAs();
        return;
    }

    if (m_project->SaveToFile(m_currentFilePath.toStdString())) {
        setModified(false);
        updateWindowTitle();
        QMessageBox::information(this, "Success", "Tilemap project saved successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to save tilemap project!");
    }
}

void TilemapPainterDialog::SaveProjectAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Tilemap Project",
        QDir::currentPath(),
        "Tilemap Project Files (*.tilemap);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".tilemap")) {
            filepath += ".tilemap";
        }

        m_currentFilePath = filepath;
        SaveProject();
    }
}

// Slot implementations
void TilemapPainterDialog::onNewProject() {
    NewProject();
}

void TilemapPainterDialog::onLoadProject() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Tilemap Project",
        QDir::currentPath(),
        "Tilemap Project Files (*.tilemap);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadProject(filepath);
    }
}

void TilemapPainterDialog::onSaveProject() {
    SaveProject();
}

void TilemapPainterDialog::onSaveAs() {
    SaveProjectAs();
}

void TilemapPainterDialog::onProjectNameChanged() {
    m_project->SetName(m_projectNameEdit->text().toStdString());
    setModified(true);
    updateWindowTitle();
}

void TilemapPainterDialog::onMapSizeChanged() {
    glm::ivec2 newSize(m_mapWidthSpin->value(), m_mapHeightSpin->value());
    m_project->SetSize(newSize);
    setModified(true);
    updateCanvas();
}

void TilemapPainterDialog::onTileSizeChanged() {
    glm::ivec2 newTileSize(m_tileWidthSpin->value(), m_tileHeightSpin->value());
    m_project->SetTileSize(newTileSize);
    setModified(true);
    updateCanvas();
}

void TilemapPainterDialog::onBackgroundColorChanged() {
    QColor color = QColorDialog::getColor(
        QColor::fromRgbF(m_project->GetBackgroundColor().r,
                        m_project->GetBackgroundColor().g,
                        m_project->GetBackgroundColor().b,
                        m_project->GetBackgroundColor().a),
        this,
        "Select Background Color"
    );

    if (color.isValid()) {
        glm::vec4 bgColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        m_project->SetBackgroundColor(bgColor);
        setModified(true);

        // Update button color
        QString styleSheet = QString("background-color: %1").arg(color.name());
        m_backgroundColorButton->setStyleSheet(styleSheet);
    }
}

void TilemapPainterDialog::onAddTileset() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Add Tileset",
        QDir::currentPath(),
        "Tileset Files (*.tileset);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        QFileInfo fileInfo(filepath);
        Lupine::TilesetReference tileset(0, fileInfo.baseName().toStdString(), filepath.toStdString());

        int tileset_id = m_project->AddTileset(tileset);
        setModified(true);
        updateTilesetList();

        // Select the new tileset
        for (int i = 0; i < m_tilesetList->count(); ++i) {
            QListWidgetItem* item = m_tilesetList->item(i);
            if (item->data(Qt::UserRole).toInt() == tileset_id) {
                m_tilesetList->setCurrentItem(item);
                break;
            }
        }
    }
}

void TilemapPainterDialog::onRemoveTileset() {
    QListWidgetItem* item = m_tilesetList->currentItem();
    if (!item) return;

    int tileset_id = item->data(Qt::UserRole).toInt();
    m_project->RemoveTileset(tileset_id);
    setModified(true);
    updateTilesetList();
    updatePalette();
}

void TilemapPainterDialog::onTilesetSelectionChanged() {
    QListWidgetItem* item = m_tilesetList->currentItem();
    if (!item) {
        updatePalette();
        return;
    }

    int tileset_id = item->data(Qt::UserRole).toInt();
    m_currentTilesetId = tileset_id;
    loadTileset(tileset_id);
    updatePalette();
}

void TilemapPainterDialog::onAddLayer() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Layer", "Layer Name:", QLineEdit::Normal, "New Layer", &ok);
    if (ok && !name.isEmpty()) {
        int layer_index = m_project->AddLayer(name.toStdString());
        setModified(true);
        updateLayerList();

        // Select the new layer
        m_layerList->setCurrentRow(layer_index);
    }
}

void TilemapPainterDialog::onRemoveLayer() {
    int current_row = m_layerList->currentRow();
    if (current_row >= 0) {
        m_project->RemoveLayer(current_row);
        setModified(true);
        updateLayerList();
        updateCanvas();
    }
}

void TilemapPainterDialog::onDuplicateLayer() {
    int current_row = m_layerList->currentRow();
    if (current_row >= 0) {
        const Lupine::TilemapLayer* layer = m_project->GetLayer(current_row);
        if (layer) {
            QString newName = QString::fromStdString(layer->GetName()) + " Copy";
            int new_layer_index = m_project->AddLayer(newName.toStdString());

            // Copy layer data
            Lupine::TilemapLayer* newLayer = m_project->GetLayer(new_layer_index);
            if (newLayer) {
                newLayer->SetOpacity(layer->GetOpacity());
                newLayer->SetVisible(layer->IsVisible());

                // Copy tiles
                glm::ivec2 size = m_project->GetSize();
                for (int y = 0; y < size.y; ++y) {
                    for (int x = 0; x < size.x; ++x) {
                        newLayer->SetTile(x, y, layer->GetTile(x, y));
                    }
                }
            }

            setModified(true);
            updateLayerList();
            updateCanvas();

            // Select the new layer
            m_layerList->setCurrentRow(new_layer_index);
        }
    }
}

void TilemapPainterDialog::onMoveLayerUp() {
    int current_row = m_layerList->currentRow();
    if (current_row > 0) {
        m_project->MoveLayer(current_row, current_row - 1);
        setModified(true);
        updateLayerList();
        updateCanvas();
        m_layerList->setCurrentRow(current_row - 1);
    }
}

void TilemapPainterDialog::onMoveLayerDown() {
    int current_row = m_layerList->currentRow();
    if (current_row >= 0 && current_row < m_project->GetLayerCount() - 1) {
        m_project->MoveLayer(current_row, current_row + 1);
        setModified(true);
        updateLayerList();
        updateCanvas();
        m_layerList->setCurrentRow(current_row + 1);
    }
}

void TilemapPainterDialog::onLayerSelectionChanged() {
    int current_row = m_layerList->currentRow();
    m_project->SetActiveLayerIndex(current_row);

    // Update layer opacity slider
    const Lupine::TilemapLayer* layer = m_project->GetActiveLayer();
    if (layer) {
        m_layerOpacitySlider->setValue(static_cast<int>(layer->GetOpacity() * 100));
    }
}

void TilemapPainterDialog::onLayerVisibilityChanged() {
    // This would be connected to checkboxes in the layer list
    updateCanvas();
    setModified(true);
}

void TilemapPainterDialog::onLayerOpacityChanged() {
    Lupine::TilemapLayer* layer = m_project->GetActiveLayer();
    if (layer) {
        float opacity = m_layerOpacitySlider->value() / 100.0f;
        layer->SetOpacity(opacity);
        updateCanvas();
        setModified(true);
    }
}

void TilemapPainterDialog::onToolChanged() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    if (action == m_brushAction) {
        m_currentTool = PaintTool::Brush;
    } else if (action == m_bucketAction) {
        m_currentTool = PaintTool::Bucket;
    } else if (action == m_eraserAction) {
        m_currentTool = PaintTool::Eraser;
    } else if (action == m_eyedropperAction) {
        m_currentTool = PaintTool::Eyedropper;
    } else if (action == m_rectangleAction) {
        m_currentTool = PaintTool::Rectangle;
    } else if (action == m_lineAction) {
        m_currentTool = PaintTool::Line;
    }

    m_canvas->SetCurrentTool(m_currentTool);
}

void TilemapPainterDialog::onTileSelected(int tileset_id, int tile_id) {
    m_currentTilesetId = tileset_id;
    m_currentTileId = tile_id;
    m_canvas->SetCurrentTile(tileset_id, tile_id);
}

void TilemapPainterDialog::onTileClicked(int x, int y) {
    // Handle tile clicks if needed
    (void)x;
    (void)y;
}

void TilemapPainterDialog::onTilePainted(int x, int y, int tileset_id, int tile_id) {
    // Handle tile painting events if needed
    (void)x;
    (void)y;
    (void)tileset_id;
    (void)tile_id;
}

void TilemapPainterDialog::onProjectModified() {
    setModified(true);
}

// UI Setup Methods
void TilemapPainterDialog::setupUI() {
    setupMenuBar();
    setupToolBar();
    setupMainPanels();

    // Connect signals after widgets are created
    if (m_canvas) {
        connect(m_canvas, &TilemapCanvas::tileClicked, this, &TilemapPainterDialog::onTileClicked);
        connect(m_canvas, &TilemapCanvas::tilePainted, this, &TilemapPainterDialog::onTilePainted);
        connect(m_canvas, &TilemapCanvas::projectModified, this, &TilemapPainterDialog::onProjectModified);
    }

    if (m_palette) {
        connect(m_palette, &TilesetPalette::tileSelected, this, &TilemapPainterDialog::onTileSelected);
    }
}

void TilemapPainterDialog::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Project", this, &TilemapPainterDialog::onNewProject, QKeySequence::New);
    fileMenu->addAction("&Open Project...", this, &TilemapPainterDialog::onLoadProject, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Project", this, &TilemapPainterDialog::onSaveProject, QKeySequence::Save);
    fileMenu->addAction("Save Project &As...", this, &TilemapPainterDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();

    // Export submenu
    QMenu* exportMenu = fileMenu->addMenu("&Export");
    exportMenu->addAction("Export to Tilemap2D...", [this]() {
        QString filepath = QFileDialog::getSaveFileName(this, "Export Tilemap2D Data",
                                                       QDir::currentPath(), "JSON Files (*.json)");
        if (!filepath.isEmpty()) {
            ExportToTilemap2D(filepath);
        }
    });
    exportMenu->addAction("Export to Tilemap2.5D...", [this]() {
        QString filepath = QFileDialog::getSaveFileName(this, "Export Tilemap2.5D Data",
                                                       QDir::currentPath(), "JSON Files (*.json)");
        if (!filepath.isEmpty()) {
            ExportToTilemap25D(filepath);
        }
    });

    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);

    // Edit menu
    QMenu* editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Add Layer", this, &TilemapPainterDialog::onAddLayer);
    editMenu->addAction("&Remove Layer", this, &TilemapPainterDialog::onRemoveLayer);
    editMenu->addAction("&Duplicate Layer", this, &TilemapPainterDialog::onDuplicateLayer);
    editMenu->addSeparator();
    editMenu->addAction("Add &Tileset...", this, &TilemapPainterDialog::onAddTileset);
    editMenu->addAction("Remove Tileset", this, &TilemapPainterDialog::onRemoveTileset);

    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    m_toggleGridAction = viewMenu->addAction("Show &Grid", this, &TilemapPainterDialog::onToggleGrid);
    m_toggleGridAction->setCheckable(true);
    m_toggleGridAction->setChecked(true);

    m_toggleSnapAction = viewMenu->addAction("&Snap to Grid", this, &TilemapPainterDialog::onToggleSnap);
    m_toggleSnapAction->setCheckable(true);
    m_toggleSnapAction->setChecked(true);

    viewMenu->addSeparator();
    viewMenu->addAction("Zoom &In", this, &TilemapPainterDialog::onZoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out", this, &TilemapPainterDialog::onZoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction("Zoom to &Fit", this, &TilemapPainterDialog::onZoomToFit);
    viewMenu->addAction("&Actual Size", this, &TilemapPainterDialog::onZoomToActual);

    // Store menu bar to be added to layout later
    m_menuBar = menuBar;
}

void TilemapPainterDialog::setupToolBar() {
    m_toolBar = new QToolBar("Tools", this);
    m_toolBar->setMaximumHeight(50);

    // Tool group for exclusive selection
    m_toolGroup = new QActionGroup(this);

    // Paint tools
    m_brushAction = m_toolBar->addAction(QIcon(":/icons/brush.png"), "Brush");
    m_brushAction->setCheckable(true);
    m_brushAction->setChecked(true);
    m_brushAction->setToolTip("Brush Tool (B)");
    m_brushAction->setShortcut(QKeySequence("B"));
    m_toolGroup->addAction(m_brushAction);

    m_bucketAction = m_toolBar->addAction(QIcon(":/icons/bucket.png"), "Bucket Fill");
    m_bucketAction->setCheckable(true);
    m_bucketAction->setToolTip("Bucket Fill Tool (G)");
    m_bucketAction->setShortcut(QKeySequence("G"));
    m_toolGroup->addAction(m_bucketAction);

    m_eraserAction = m_toolBar->addAction(QIcon(":/icons/eraser.png"), "Eraser");
    m_eraserAction->setCheckable(true);
    m_eraserAction->setToolTip("Eraser Tool (E)");
    m_eraserAction->setShortcut(QKeySequence("E"));
    m_toolGroup->addAction(m_eraserAction);

    m_eyedropperAction = m_toolBar->addAction(QIcon(":/icons/eyedropper.png"), "Eyedropper");
    m_eyedropperAction->setCheckable(true);
    m_eyedropperAction->setToolTip("Eyedropper Tool (I)");
    m_eyedropperAction->setShortcut(QKeySequence("I"));
    m_toolGroup->addAction(m_eyedropperAction);

    m_rectangleAction = m_toolBar->addAction(QIcon(":/icons/rectangle.png"), "Rectangle");
    m_rectangleAction->setCheckable(true);
    m_rectangleAction->setToolTip("Rectangle Tool (R)");
    m_rectangleAction->setShortcut(QKeySequence("R"));
    m_toolGroup->addAction(m_rectangleAction);

    m_lineAction = m_toolBar->addAction(QIcon(":/icons/line.png"), "Line");
    m_lineAction->setCheckable(true);
    m_lineAction->setToolTip("Line Tool (L)");
    m_lineAction->setShortcut(QKeySequence("L"));
    m_toolGroup->addAction(m_lineAction);

    m_toolBar->addSeparator();

    // View controls
    m_zoomInAction = m_toolBar->addAction(QIcon(":/icons/zoom_in.png"), "Zoom In");
    m_zoomInAction->setToolTip("Zoom In (+)");
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);

    m_zoomOutAction = m_toolBar->addAction(QIcon(":/icons/zoom_out.png"), "Zoom Out");
    m_zoomOutAction->setToolTip("Zoom Out (-)");
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);

    m_zoomToFitAction = m_toolBar->addAction(QIcon(":/icons/zoom_fit.png"), "Zoom to Fit");
    m_zoomToFitAction->setToolTip("Zoom to Fit (F)");
    m_zoomToFitAction->setShortcut(QKeySequence("F"));

    m_zoomToActualAction = m_toolBar->addAction(QIcon(":/icons/zoom_actual.png"), "Actual Size");
    m_zoomToActualAction->setToolTip("Actual Size (1)");
    m_zoomToActualAction->setShortcut(QKeySequence("1"));

    // Connect tool actions
    connect(m_toolGroup, &QActionGroup::triggered, this, &TilemapPainterDialog::onToolChanged);
    connect(m_zoomInAction, &QAction::triggered, this, &TilemapPainterDialog::onZoomIn);
    connect(m_zoomOutAction, &QAction::triggered, this, &TilemapPainterDialog::onZoomOut);
    connect(m_zoomToFitAction, &QAction::triggered, this, &TilemapPainterDialog::onZoomToFit);
    connect(m_zoomToActualAction, &QAction::triggered, this, &TilemapPainterDialog::onZoomToActual);
}

void TilemapPainterDialog::setupMainPanels() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Add menu bar
    if (m_menuBar) {
        m_mainLayout->setMenuBar(m_menuBar);
    }

    // Add toolbar
    m_mainLayout->addWidget(m_toolBar);

    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupProjectPropertiesPanel();
    setupCanvasPanel();
    setupLayerPanel();

    // Set splitter proportions
    m_mainSplitter->setSizes({300, 800, 300});
    m_mainSplitter->setStretchFactor(0, 0);  // Left panel fixed
    m_mainSplitter->setStretchFactor(1, 1);  // Canvas stretches
    m_mainSplitter->setStretchFactor(2, 0);  // Right panel fixed
}

void TilemapPainterDialog::setupProjectPropertiesPanel() {
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);

    setupTilesetPanel();

    // Project properties group
    m_projectGroup = new QGroupBox("Project Properties");
    QGridLayout* projectLayout = new QGridLayout(m_projectGroup);

    // Project name
    projectLayout->addWidget(new QLabel("Name:"), 0, 0);
    m_projectNameEdit = new QLineEdit();
    projectLayout->addWidget(m_projectNameEdit, 0, 1);
    connect(m_projectNameEdit, &QLineEdit::textChanged, this, &TilemapPainterDialog::onProjectNameChanged);

    // Map size
    projectLayout->addWidget(new QLabel("Map Size:"), 1, 0);
    QHBoxLayout* mapSizeLayout = new QHBoxLayout();
    m_mapWidthSpin = new QSpinBox();
    m_mapWidthSpin->setRange(1, 1000);
    m_mapWidthSpin->setValue(20);
    m_mapHeightSpin = new QSpinBox();
    m_mapHeightSpin->setRange(1, 1000);
    m_mapHeightSpin->setValue(15);
    mapSizeLayout->addWidget(m_mapWidthSpin);
    mapSizeLayout->addWidget(new QLabel("x"));
    mapSizeLayout->addWidget(m_mapHeightSpin);
    mapSizeLayout->addStretch();
    projectLayout->addLayout(mapSizeLayout, 1, 1);
    connect(m_mapWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilemapPainterDialog::onMapSizeChanged);
    connect(m_mapHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilemapPainterDialog::onMapSizeChanged);

    // Tile size
    projectLayout->addWidget(new QLabel("Tile Size:"), 2, 0);
    QHBoxLayout* tileSizeLayout = new QHBoxLayout();
    m_tileWidthSpin = new QSpinBox();
    m_tileWidthSpin->setRange(1, 256);
    m_tileWidthSpin->setValue(32);
    m_tileHeightSpin = new QSpinBox();
    m_tileHeightSpin->setRange(1, 256);
    m_tileHeightSpin->setValue(32);
    tileSizeLayout->addWidget(m_tileWidthSpin);
    tileSizeLayout->addWidget(new QLabel("x"));
    tileSizeLayout->addWidget(m_tileHeightSpin);
    tileSizeLayout->addStretch();
    projectLayout->addLayout(tileSizeLayout, 2, 1);
    connect(m_tileWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilemapPainterDialog::onTileSizeChanged);
    connect(m_tileHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilemapPainterDialog::onTileSizeChanged);

    // Background color
    projectLayout->addWidget(new QLabel("Background:"), 3, 0);
    m_backgroundColorButton = new QPushButton();
    m_backgroundColorButton->setMaximumSize(60, 30);
    m_backgroundColorButton->setStyleSheet("background-color: #333333");
    projectLayout->addWidget(m_backgroundColorButton, 3, 1, Qt::AlignLeft);
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &TilemapPainterDialog::onBackgroundColorChanged);

    m_leftLayout->addWidget(m_projectGroup);
    m_leftLayout->addStretch();

    m_mainSplitter->addWidget(m_leftPanel);
}

void TilemapPainterDialog::setupTilesetPanel() {
    m_tilesetGroup = new QGroupBox("Tilesets");
    QVBoxLayout* tilesetLayout = new QVBoxLayout(m_tilesetGroup);

    // Tileset list
    m_tilesetList = new QListWidget();
    m_tilesetList->setMaximumHeight(150);
    tilesetLayout->addWidget(m_tilesetList);
    connect(m_tilesetList, &QListWidget::currentItemChanged, this, &TilemapPainterDialog::onTilesetSelectionChanged);

    // Tileset buttons
    QHBoxLayout* tilesetButtonLayout = new QHBoxLayout();
    m_addTilesetButton = new QPushButton("Add");
    m_removeTilesetButton = new QPushButton("Remove");
    tilesetButtonLayout->addWidget(m_addTilesetButton);
    tilesetButtonLayout->addWidget(m_removeTilesetButton);
    tilesetLayout->addLayout(tilesetButtonLayout);

    connect(m_addTilesetButton, &QPushButton::clicked, this, &TilemapPainterDialog::onAddTileset);
    connect(m_removeTilesetButton, &QPushButton::clicked, this, &TilemapPainterDialog::onRemoveTileset);

    m_leftLayout->addWidget(m_tilesetGroup);
}

void TilemapPainterDialog::setupCanvasPanel() {
    m_centerPanel = new QWidget();
    m_centerLayout = new QVBoxLayout(m_centerPanel);
    m_centerLayout->setContentsMargins(0, 0, 0, 0);

    // Canvas
    m_canvas = new TilemapCanvas();
    m_centerLayout->addWidget(m_canvas);

    m_mainSplitter->addWidget(m_centerPanel);
}

void TilemapPainterDialog::setupLayerPanel() {
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);

    // Layer management group
    m_layerGroup = new QGroupBox("Layers");
    QVBoxLayout* layerLayout = new QVBoxLayout(m_layerGroup);

    // Layer list
    m_layerList = new QListWidget();
    m_layerList->setMaximumHeight(200);
    layerLayout->addWidget(m_layerList);
    connect(m_layerList, &QListWidget::currentRowChanged, this, &TilemapPainterDialog::onLayerSelectionChanged);

    // Layer buttons
    m_layerButtonLayout = new QHBoxLayout();
    m_addLayerButton = new QPushButton("Add");
    m_removeLayerButton = new QPushButton("Remove");
    m_duplicateLayerButton = new QPushButton("Duplicate");
    m_layerButtonLayout->addWidget(m_addLayerButton);
    m_layerButtonLayout->addWidget(m_removeLayerButton);
    m_layerButtonLayout->addWidget(m_duplicateLayerButton);
    layerLayout->addLayout(m_layerButtonLayout);

    // Layer move buttons
    QHBoxLayout* layerMoveLayout = new QHBoxLayout();
    m_moveLayerUpButton = new QPushButton("Move Up");
    m_moveLayerDownButton = new QPushButton("Move Down");
    layerMoveLayout->addWidget(m_moveLayerUpButton);
    layerMoveLayout->addWidget(m_moveLayerDownButton);
    layerLayout->addLayout(layerMoveLayout);

    // Layer opacity
    layerLayout->addWidget(new QLabel("Opacity:"));
    m_layerOpacitySlider = new QSlider(Qt::Horizontal);
    m_layerOpacitySlider->setRange(0, 100);
    m_layerOpacitySlider->setValue(100);
    layerLayout->addWidget(m_layerOpacitySlider);

    // Connect layer signals
    connect(m_addLayerButton, &QPushButton::clicked, this, &TilemapPainterDialog::onAddLayer);
    connect(m_removeLayerButton, &QPushButton::clicked, this, &TilemapPainterDialog::onRemoveLayer);
    connect(m_duplicateLayerButton, &QPushButton::clicked, this, &TilemapPainterDialog::onDuplicateLayer);
    connect(m_moveLayerUpButton, &QPushButton::clicked, this, &TilemapPainterDialog::onMoveLayerUp);
    connect(m_moveLayerDownButton, &QPushButton::clicked, this, &TilemapPainterDialog::onMoveLayerDown);
    connect(m_layerOpacitySlider, &QSlider::valueChanged, this, &TilemapPainterDialog::onLayerOpacityChanged);

    m_rightLayout->addWidget(m_layerGroup);

    setupPalettePanel();

    m_rightLayout->addStretch();
    m_mainSplitter->addWidget(m_rightPanel);
}

void TilemapPainterDialog::setupPalettePanel() {
    // Tileset palette group
    m_paletteGroup = new QGroupBox("Tileset Palette");
    QVBoxLayout* paletteLayout = new QVBoxLayout(m_paletteGroup);

    // Palette view
    m_palette = new TilesetPalette();
    m_palette->setMinimumHeight(300);
    paletteLayout->addWidget(m_palette);

    m_rightLayout->addWidget(m_paletteGroup);
}

// Update Methods
void TilemapPainterDialog::updateWindowTitle() {
    QString title = "Tilemap Painter";
    if (!m_project->GetName().empty()) {
        title += " - " + QString::fromStdString(m_project->GetName());
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void TilemapPainterDialog::updateProjectProperties() {
    if (!m_project) return;

    m_projectNameEdit->setText(QString::fromStdString(m_project->GetName()));

    glm::ivec2 size = m_project->GetSize();
    m_mapWidthSpin->setValue(size.x);
    m_mapHeightSpin->setValue(size.y);

    glm::ivec2 tileSize = m_project->GetTileSize();
    m_tileWidthSpin->setValue(tileSize.x);
    m_tileHeightSpin->setValue(tileSize.y);

    glm::vec4 bgColor = m_project->GetBackgroundColor();
    QColor color = QColor::fromRgbF(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    QString styleSheet = QString("background-color: %1").arg(color.name());
    m_backgroundColorButton->setStyleSheet(styleSheet);
}

void TilemapPainterDialog::updateTilesetList() {
    m_tilesetList->clear();

    if (!m_project) return;

    for (const auto& tileset_ref : m_project->GetTilesets()) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(tileset_ref.name));
        item->setData(Qt::UserRole, tileset_ref.id);
        m_tilesetList->addItem(item);
    }
}

void TilemapPainterDialog::updateLayerList() {
    m_layerList->clear();

    if (!m_project) return;

    for (int i = 0; i < m_project->GetLayerCount(); ++i) {
        const Lupine::TilemapLayer* layer = m_project->GetLayer(i);
        if (layer) {
            QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(layer->GetName()));

            // Add visibility checkbox
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(layer->IsVisible() ? Qt::Checked : Qt::Unchecked);

            m_layerList->addItem(item);
        }
    }

    // Select active layer
    if (m_project->GetActiveLayerIndex() < m_layerList->count()) {
        m_layerList->setCurrentRow(m_project->GetActiveLayerIndex());
    }
}

void TilemapPainterDialog::updateCanvas() {
    if (m_canvas) {
        m_canvas->SetProject(m_project.get());
    }
}

void TilemapPainterDialog::updatePalette() {
    if (!m_palette) return;

    if (m_currentTilesetId < 0) {
        m_palette->ClearTileset();
        return;
    }

    // Find the loaded tileset
    Lupine::Tileset2DResource* tileset = m_canvas->GetLoadedTileset(m_currentTilesetId);
    if (tileset) {
        m_palette->SetTileset(m_currentTilesetId, tileset);
    } else {
        m_palette->ClearTileset();
    }
}

void TilemapPainterDialog::loadTileset(int tileset_id) {
    // This is handled by the canvas
    if (m_canvas) {
        m_canvas->loadTileset(tileset_id);
    }
}

// Utility Methods
bool TilemapPainterDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool TilemapPainterDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<TilemapPainterDialog*>(this),
        "Unsaved Changes",
        "The tilemap project has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    switch (result) {
        case QMessageBox::Save:
            SaveProject();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void TilemapPainterDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

// View Control Slots
void TilemapPainterDialog::onZoomIn() {
    if (m_canvas) {
        m_canvas->ZoomIn();
    }
}

void TilemapPainterDialog::onZoomOut() {
    if (m_canvas) {
        m_canvas->ZoomOut();
    }
}

void TilemapPainterDialog::onZoomToFit() {
    if (m_canvas) {
        m_canvas->ZoomToFit();
    }
}

void TilemapPainterDialog::onZoomToActual() {
    if (m_canvas) {
        m_canvas->ZoomToActual();
    }
}

void TilemapPainterDialog::onToggleGrid() {
    if (m_canvas) {
        bool showGrid = m_toggleGridAction->isChecked();
        m_canvas->SetShowGrid(showGrid);
    }
}

void TilemapPainterDialog::onToggleSnap() {
    if (m_canvas) {
        bool snapToGrid = m_toggleSnapAction->isChecked();
        m_canvas->SetSnapToGrid(snapToGrid);
    }
}

// Export Methods
void TilemapPainterDialog::ExportToTilemap2D(const QString& filepath) {
    if (!m_project) return;

    // Convert TilemapProject to simple TilemapData format
    // For now, we'll export the active layer only
    const Lupine::TilemapLayer* activeLayer = m_project->GetActiveLayer();
    if (!activeLayer) return;

    json j;
    j["size"] = {m_project->GetSize().x, m_project->GetSize().y};

    // Convert tiles from TileInstance to simple tile IDs
    std::vector<int> tiles;
    glm::ivec2 size = m_project->GetSize();
    tiles.resize(size.x * size.y, -1);

    for (int y = 0; y < size.y; ++y) {
        for (int x = 0; x < size.x; ++x) {
            const Lupine::TileInstance& tile = activeLayer->GetTile(x, y);
            if (!tile.IsEmpty()) {
                tiles[y * size.x + x] = tile.tile_id;
            }
        }
    }

    j["tiles"] = tiles;

    // Save to file
    std::ofstream file(filepath.toStdString());
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        QMessageBox::information(this, "Export Successful", "Tilemap exported for Tilemap2D component!");
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to save tilemap data!");
    }
}

void TilemapPainterDialog::ExportToTilemap25D(const QString& filepath) {
    // Same as 2D for now - could be extended for different behavior
    ExportToTilemap2D(filepath);
}


