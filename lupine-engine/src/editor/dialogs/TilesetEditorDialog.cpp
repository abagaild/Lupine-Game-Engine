#include "TilesetEditorDialog.h"
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QStandardPaths>
#include <QDir>
#include <QHeaderView>
#include <QGraphicsEffect>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QInputDialog>
#include <cmath>

// TilesetView Implementation
TilesetView::TilesetView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_tileset_item(nullptr)
    , m_selection_rect(nullptr)
    , m_tileset(nullptr)
    , m_selected_tile_id(-1)
{
    setupScene();
    setDragMode(QGraphicsView::RubberBandDrag);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
}

TilesetView::~TilesetView() {
    if (m_scene) {
        delete m_scene;
    }
}

void TilesetView::SetTileset(Lupine::Tileset2DResource* tileset) {
    m_tileset = tileset;
    m_selected_tile_id = -1;
    RefreshView();
}

void TilesetView::RefreshView() {
    if (!m_tileset || m_tileset->GetTexturePath().empty()) {
        m_scene->clear();
        m_tileset_item = nullptr;
        m_selection_rect = nullptr;
        return;
    }

    // Load tileset image
    QString imagePath = QString::fromStdString(m_tileset->GetTexturePath());
    m_tileset_pixmap = QPixmap(imagePath);
    
    if (m_tileset_pixmap.isNull()) {
        m_scene->clear();
        m_tileset_item = nullptr;
        m_selection_rect = nullptr;
        return;
    }

    // Clear scene and add tileset image
    m_scene->clear();
    m_tileset_item = m_scene->addPixmap(m_tileset_pixmap);
    m_tileset_item->setPos(0, 0);

    // Update tile grid
    updateTileGrid();
    
    // Update selection
    updateSelection();

    // Fit view to contents
    fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void TilesetView::SetSelectedTile(int tile_id) {
    if (m_selected_tile_id != tile_id) {
        m_selected_tile_id = tile_id;
        updateSelection();
        emit tileSelected(tile_id);
    }
}

void TilesetView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_tileset) {
        QPointF scene_pos = mapToScene(event->pos());
        int tile_id = getTileIdAt(scene_pos);
        if (tile_id >= 0) {
            SetSelectedTile(tile_id);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void TilesetView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_tileset) {
        QPointF scene_pos = mapToScene(event->pos());
        int tile_id = getTileIdAt(scene_pos);
        if (tile_id >= 0) {
            emit tileDoubleClicked(tile_id);
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void TilesetView::wheelEvent(QWheelEvent* event) {
    // Zoom with mouse wheel
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void TilesetView::onSceneChanged() {
    // Handle scene changes if needed
}

void TilesetView::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    connect(m_scene, &QGraphicsScene::changed, this, &TilesetView::onSceneChanged);
}

void TilesetView::updateTileGrid() {
    if (!m_tileset || !m_tileset_item) return;

    // Draw grid lines over the tileset
    QPen gridPen(QColor(255, 255, 255, 128), 1);
    
    glm::ivec2 tileSize = m_tileset->GetTileSize();
    glm::ivec2 gridSize = m_tileset->GetGridSize();
    int spacing = m_tileset->GetSpacing();
    int margin = m_tileset->GetMargin();

    // Draw vertical lines
    for (int x = 0; x <= gridSize.x; ++x) {
        int lineX = margin + x * (tileSize.x + spacing);
        if (x == gridSize.x) lineX -= spacing; // Last line
        
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
        if (y == gridSize.y) lineY -= spacing; // Last line
        
        QGraphicsLineItem* line = m_scene->addLine(
            margin, lineY,
            margin + gridSize.x * (tileSize.x + spacing) - spacing, lineY,
            gridPen
        );
        line->setZValue(1);
    }
}

void TilesetView::updateSelection() {
    // Remove existing selection rectangle
    if (m_selection_rect) {
        m_scene->removeItem(m_selection_rect);
        delete m_selection_rect;
        m_selection_rect = nullptr;
    }

    if (m_selected_tile_id >= 0 && m_tileset) {
        QRectF tileRect = getTileRect(m_selected_tile_id);
        if (!tileRect.isEmpty()) {
            QPen selectionPen(QColor(255, 0, 255), 2);
            m_selection_rect = m_scene->addRect(tileRect, selectionPen);
            m_selection_rect->setZValue(2);
        }
    }
}

int TilesetView::getTileIdAt(const QPointF& scene_pos) const {
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

QRectF TilesetView::getTileRect(int tile_id) const {
    if (!m_tileset) return QRectF();

    const Lupine::TileData* tile = m_tileset->GetTile(tile_id);
    if (!tile) return QRectF();

    glm::vec4 region = tile->texture_region;
    return QRectF(region.x, region.y, region.z, region.w);
}

// TilesetEditorDialog Implementation
TilesetEditorDialog::TilesetEditorDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_tileset(std::make_unique<Lupine::Tileset2DResource>())
    , m_modified(false)
    , m_currentTileId(-1)
{
    setWindowTitle("Tileset Editor");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUI();
    updateWindowTitle();
}

TilesetEditorDialog::~TilesetEditorDialog() {
}

void TilesetEditorDialog::NewTileset() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_tileset = std::make_unique<Lupine::Tileset2DResource>();
    m_currentFilePath.clear();
    m_currentTileId = -1;
    setModified(false);
    
    updateTilesetView();
    updateTileProperties();
    updateWindowTitle();
}

void TilesetEditorDialog::LoadTileset(const QString& filepath) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    auto newTileset = std::make_unique<Lupine::Tileset2DResource>();
    if (newTileset->LoadFromFile(filepath.toStdString())) {
        m_tileset = std::move(newTileset);
        m_currentFilePath = filepath;
        m_currentTileId = -1;
        setModified(false);
        
        // Update UI with loaded data
        m_imagePathEdit->setText(QString::fromStdString(m_tileset->GetTexturePath()));
        glm::ivec2 tileSize = m_tileset->GetTileSize();
        m_tileSizeXSpin->setValue(tileSize.x);
        m_tileSizeYSpin->setValue(tileSize.y);
        glm::ivec2 gridSize = m_tileset->GetGridSize();
        m_gridSizeXSpin->setValue(gridSize.x);
        m_gridSizeYSpin->setValue(gridSize.y);
        m_spacingSpin->setValue(m_tileset->GetSpacing());
        m_marginSpin->setValue(m_tileset->GetMargin());
        
        updateTilesetView();
        updateTileProperties();
        updateWindowTitle();
        calculateAutoGridSize();

        QMessageBox::information(this, "Success", "Tileset loaded successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to load tileset file!");
    }
}

void TilesetEditorDialog::SaveTileset() {
    if (m_currentFilePath.isEmpty()) {
        SaveTilesetAs();
        return;
    }

    if (m_tileset->SaveToFile(m_currentFilePath.toStdString())) {
        setModified(false);
        updateWindowTitle();
        QMessageBox::information(this, "Success", "Tileset saved successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to save tileset file!");
    }
}

void TilesetEditorDialog::SaveTilesetAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Tileset",
        QDir::currentPath(),
        "Tileset Files (*.tileset);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".tileset")) {
            filepath += ".tileset";
        }
        
        m_currentFilePath = filepath;
        SaveTileset();
    }
}

// Slot implementations
void TilesetEditorDialog::onNewTileset() {
    NewTileset();
}

void TilesetEditorDialog::onLoadTileset() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Tileset",
        QDir::currentPath(),
        "Tileset Files (*.tileset);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadTileset(filepath);
    }
}

void TilesetEditorDialog::onSaveTileset() {
    SaveTileset();
}

void TilesetEditorDialog::onSaveAs() {
    SaveTilesetAs();
}

void TilesetEditorDialog::onImportImage() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Import Tileset Image",
        QDir::currentPath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tga);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        m_imagePathEdit->setText(filepath);
        onImagePathChanged();
    }
}

void TilesetEditorDialog::onImagePathChanged() {
    QString imagePath = m_imagePathEdit->text();
    m_tileset->SetTexturePath(imagePath.toStdString());
    refreshImagePreview();
    calculateAutoGridSize();
    setModified(true);
}

void TilesetEditorDialog::onTileSizeChanged() {
    glm::ivec2 tileSize(m_tileSizeXSpin->value(), m_tileSizeYSpin->value());
    m_tileset->SetTileSize(tileSize);
    calculateAutoGridSize();
    setModified(true);
}

void TilesetEditorDialog::onGridSizeChanged() {
    glm::ivec2 gridSize(m_gridSizeXSpin->value(), m_gridSizeYSpin->value());
    m_tileset->SetGridSize(gridSize);
    setModified(true);
}

void TilesetEditorDialog::onSpacingChanged() {
    m_tileset->SetSpacing(m_spacingSpin->value());
    setModified(true);
}

void TilesetEditorDialog::onMarginChanged() {
    m_tileset->SetMargin(m_marginSpin->value());
    setModified(true);
}

void TilesetEditorDialog::onGenerateTiles() {
    m_tileset->GenerateTilesFromGrid();
    updateTilesetView();
    setModified(true);
}

void TilesetEditorDialog::onTileSelected(int tile_id) {
    m_currentTileId = tile_id;
    updateTileProperties();

    // Update tile info label
    if (tile_id >= 0) {
        const Lupine::TileData* tile = m_tileset->GetTile(tile_id);
        if (tile) {
            m_tileInfoLabel->setText(QString("Selected Tile: %1 (Grid: %2, %3)")
                .arg(tile_id)
                .arg(tile->grid_position.x)
                .arg(tile->grid_position.y));
        }
    } else {
        m_tileInfoLabel->setText("No tile selected");
    }
}

void TilesetEditorDialog::onTileDoubleClicked(int tile_id) {
    // Could open a detailed tile editor here
    onTileSelected(tile_id);
}

void TilesetEditorDialog::onCollisionTypeChanged() {
    if (m_currentTileId < 0) return;

    Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) return;

    tile->collision.type = static_cast<Lupine::TileCollisionType>(m_collisionTypeCombo->currentIndex());
    updateCollisionEditor();
    setModified(true);
}

void TilesetEditorDialog::onCollisionDataChanged() {
    if (m_currentTileId < 0) return;

    Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) return;

    tile->collision.offset.x = m_collisionOffsetXSpin->value();
    tile->collision.offset.y = m_collisionOffsetYSpin->value();
    tile->collision.size.x = m_collisionSizeXSpin->value();
    tile->collision.size.y = m_collisionSizeYSpin->value();
    setModified(true);
}

void TilesetEditorDialog::onAddCustomProperty() {
    if (m_currentTileId < 0) return;

    // Simple dialog to add a new property
    bool ok;
    QString name = QInputDialog::getText(this, "Add Custom Property", "Property Name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
        if (tile) {
            Lupine::TileDataValue value;
            value.type = Lupine::TileDataType::String;
            value.string_value = "Default Value";
            tile->custom_data[name.toStdString()] = value;
            updateCustomDataEditor();
            setModified(true);
        }
    }
}

void TilesetEditorDialog::onRemoveCustomProperty() {
    if (m_currentTileId < 0) return;

    QTreeWidgetItem* item = m_customDataTree->currentItem();
    if (!item) return;

    QString propertyName = item->text(0);
    Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->custom_data.erase(propertyName.toStdString());
        updateCustomDataEditor();
        setModified(true);
    }
}

void TilesetEditorDialog::onCustomPropertyChanged() {
    // Handle custom property value changes
    setModified(true);
}

void TilesetEditorDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupMainPanels();

    // Add menu bar
    setupMenuBar();
}

void TilesetEditorDialog::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    menuBar->setMaximumHeight(50);  // Limit menu bar height to 50 pixels
    m_mainLayout->insertWidget(0, menuBar);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Tileset", this, &TilesetEditorDialog::onNewTileset, QKeySequence::New);
    fileMenu->addAction("&Open Tileset...", this, &TilesetEditorDialog::onLoadTileset, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Tileset", this, &TilesetEditorDialog::onSaveTileset, QKeySequence::Save);
    fileMenu->addAction("Save Tileset &As...", this, &TilesetEditorDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);
}

void TilesetEditorDialog::setupMainPanels() {
    // Left panel - Image import and configuration
    m_leftPanel = new QWidget();
    m_leftPanel->setMinimumWidth(300);
    m_leftPanel->setMaximumWidth(350);
    setupImageImportPanel();
    m_mainSplitter->addWidget(m_leftPanel);

    // Center panel - Tileset view
    m_centerPanel = new QWidget();
    setupTilesetViewPanel();
    m_mainSplitter->addWidget(m_centerPanel);

    // Right panel - Tile properties
    m_rightPanel = new QWidget();
    m_rightPanel->setMinimumWidth(300);
    m_rightPanel->setMaximumWidth(350);
    setupTilePropertiesPanel();
    m_mainSplitter->addWidget(m_rightPanel);

    // Set splitter proportions
    m_mainSplitter->setStretchFactor(0, 0); // Left panel - fixed
    m_mainSplitter->setStretchFactor(1, 1); // Center panel - expandable
    m_mainSplitter->setStretchFactor(2, 0); // Right panel - fixed
}

void TilesetEditorDialog::setupImageImportPanel() {
    m_leftLayout = new QVBoxLayout(m_leftPanel);

    // Image import group
    m_imageGroup = new QGroupBox("Image Import");
    m_imageLayout = new QVBoxLayout(m_imageGroup);

    // Image path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Image Path:"));
    m_imagePathEdit = new QLineEdit();
    m_browseImageButton = new QPushButton("Browse...");
    pathLayout->addWidget(m_imagePathEdit);
    pathLayout->addWidget(m_browseImageButton);
    m_imageLayout->addLayout(pathLayout);

    // Import button
    m_importImageButton = new QPushButton("Import Image");
    m_imageLayout->addWidget(m_importImageButton);

    // Image preview
    m_imagePreviewLabel = new QLabel();
    m_imagePreviewLabel->setMinimumHeight(150);
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->setStyleSheet("border: 1px solid gray;");
    m_imagePreviewLabel->setText("No image loaded");
    m_imageLayout->addWidget(m_imagePreviewLabel);

    m_leftLayout->addWidget(m_imageGroup);

    // Slicing configuration group
    m_slicingGroup = new QGroupBox("Slicing Configuration");
    m_slicingLayout = new QGridLayout(m_slicingGroup);

    // Tile size
    m_slicingLayout->addWidget(new QLabel("Tile Size:"), 0, 0);
    m_tileSizeXSpin = new QSpinBox();
    m_tileSizeXSpin->setRange(1, 1024);
    m_tileSizeXSpin->setValue(32);
    m_tileSizeYSpin = new QSpinBox();
    m_tileSizeYSpin->setRange(1, 1024);
    m_tileSizeYSpin->setValue(32);
    QHBoxLayout* tileSizeLayout = new QHBoxLayout();
    tileSizeLayout->addWidget(m_tileSizeXSpin);
    tileSizeLayout->addWidget(new QLabel("x"));
    tileSizeLayout->addWidget(m_tileSizeYSpin);
    m_slicingLayout->addLayout(tileSizeLayout, 0, 1);

    // Grid size
    m_slicingLayout->addWidget(new QLabel("Grid Size:"), 1, 0);
    m_gridSizeXSpin = new QSpinBox();
    m_gridSizeXSpin->setRange(1, 100);
    m_gridSizeXSpin->setValue(1);
    m_gridSizeYSpin = new QSpinBox();
    m_gridSizeYSpin->setRange(1, 100);
    m_gridSizeYSpin->setValue(1);
    QHBoxLayout* gridSizeLayout = new QHBoxLayout();
    gridSizeLayout->addWidget(m_gridSizeXSpin);
    gridSizeLayout->addWidget(new QLabel("x"));
    gridSizeLayout->addWidget(m_gridSizeYSpin);
    m_slicingLayout->addLayout(gridSizeLayout, 1, 1);

    // Spacing and margin
    m_slicingLayout->addWidget(new QLabel("Spacing:"), 2, 0);
    m_spacingSpin = new QSpinBox();
    m_spacingSpin->setRange(0, 100);
    m_spacingSpin->setValue(0);
    m_slicingLayout->addWidget(m_spacingSpin, 2, 1);

    m_slicingLayout->addWidget(new QLabel("Margin:"), 3, 0);
    m_marginSpin = new QSpinBox();
    m_marginSpin->setRange(0, 100);
    m_marginSpin->setValue(0);
    m_slicingLayout->addWidget(m_marginSpin, 3, 1);

    // Generate tiles button
    m_generateTilesButton = new QPushButton("Generate Tiles");
    m_slicingLayout->addWidget(m_generateTilesButton, 4, 0, 1, 2);

    m_leftLayout->addWidget(m_slicingGroup);
    m_leftLayout->addStretch();

    // Connect signals
    connect(m_browseImageButton, &QPushButton::clicked, this, &TilesetEditorDialog::onImportImage);
    connect(m_importImageButton, &QPushButton::clicked, this, &TilesetEditorDialog::onImportImage);
    connect(m_imagePathEdit, &QLineEdit::textChanged, this, &TilesetEditorDialog::onImagePathChanged);
    connect(m_tileSizeXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onTileSizeChanged);
    connect(m_tileSizeYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onTileSizeChanged);
    connect(m_gridSizeXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onGridSizeChanged);
    connect(m_gridSizeYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onGridSizeChanged);
    connect(m_spacingSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onSpacingChanged);
    connect(m_marginSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onMarginChanged);
    connect(m_generateTilesButton, &QPushButton::clicked, this, &TilesetEditorDialog::onGenerateTiles);
}

void TilesetEditorDialog::setupTilesetViewPanel() {
    m_centerLayout = new QVBoxLayout(m_centerPanel);

    // Title
    QLabel* titleLabel = new QLabel("Tileset Preview");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_centerLayout->addWidget(titleLabel);

    // Tileset view
    m_tilesetView = new TilesetView();
    m_tilesetView->SetTileset(m_tileset.get());
    m_centerLayout->addWidget(m_tilesetView);

    // Tile info label
    m_tileInfoLabel = new QLabel("No tile selected");
    m_tileInfoLabel->setStyleSheet("padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc;");
    m_centerLayout->addWidget(m_tileInfoLabel);

    // Connect signals
    connect(m_tilesetView, &TilesetView::tileSelected, this, &TilesetEditorDialog::onTileSelected);
    connect(m_tilesetView, &TilesetView::tileDoubleClicked, this, &TilesetEditorDialog::onTileDoubleClicked);
}

void TilesetEditorDialog::setupTilePropertiesPanel() {
    m_rightLayout = new QVBoxLayout(m_rightPanel);

    // Title
    QLabel* titleLabel = new QLabel("Tile Properties");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_rightLayout->addWidget(titleLabel);

    // Properties tab widget
    m_propertiesTab = new QTabWidget();
    m_rightLayout->addWidget(m_propertiesTab);

    // Setup tabs
    setupCollisionEditor();
    setupCustomDataEditor();
}

void TilesetEditorDialog::setupCollisionEditor() {
    m_collisionTab = new QWidget();
    m_collisionLayout = new QVBoxLayout(m_collisionTab);

    // Collision type
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Collision Type:"));
    m_collisionTypeCombo = new QComboBox();
    m_collisionTypeCombo->addItems({"None", "Rectangle", "Circle", "Polygon", "Convex"});
    typeLayout->addWidget(m_collisionTypeCombo);
    m_collisionLayout->addLayout(typeLayout);

    // Collision data group
    m_collisionDataGroup = new QGroupBox("Collision Data");
    m_collisionDataLayout = new QGridLayout(m_collisionDataGroup);

    // Offset
    m_collisionDataLayout->addWidget(new QLabel("Offset:"), 0, 0);
    m_collisionOffsetXSpin = new QSpinBox();
    m_collisionOffsetXSpin->setRange(-1000, 1000);
    m_collisionOffsetYSpin = new QSpinBox();
    m_collisionOffsetYSpin->setRange(-1000, 1000);
    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->addWidget(m_collisionOffsetXSpin);
    offsetLayout->addWidget(new QLabel(","));
    offsetLayout->addWidget(m_collisionOffsetYSpin);
    m_collisionDataLayout->addLayout(offsetLayout, 0, 1);

    // Size
    m_collisionDataLayout->addWidget(new QLabel("Size:"), 1, 0);
    m_collisionSizeXSpin = new QSpinBox();
    m_collisionSizeXSpin->setRange(1, 1000);
    m_collisionSizeXSpin->setValue(32);
    m_collisionSizeYSpin = new QSpinBox();
    m_collisionSizeYSpin->setRange(1, 1000);
    m_collisionSizeYSpin->setValue(32);
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(m_collisionSizeXSpin);
    sizeLayout->addWidget(new QLabel("x"));
    sizeLayout->addWidget(m_collisionSizeYSpin);
    m_collisionDataLayout->addLayout(sizeLayout, 1, 1);

    m_collisionLayout->addWidget(m_collisionDataGroup);
    m_collisionLayout->addStretch();

    m_propertiesTab->addTab(m_collisionTab, "Collision");

    // Connect signals
    connect(m_collisionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TilesetEditorDialog::onCollisionTypeChanged);
    connect(m_collisionOffsetXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onCollisionDataChanged);
    connect(m_collisionOffsetYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onCollisionDataChanged);
    connect(m_collisionSizeXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onCollisionDataChanged);
    connect(m_collisionSizeYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditorDialog::onCollisionDataChanged);
}

void TilesetEditorDialog::setupCustomDataEditor() {
    m_customDataTab = new QWidget();
    m_customDataLayout = new QVBoxLayout(m_customDataTab);

    // Custom data tree
    m_customDataTree = new QTreeWidget();
    m_customDataTree->setHeaderLabels({"Property", "Type", "Value"});
    m_customDataTree->header()->setStretchLastSection(true);
    m_customDataLayout->addWidget(m_customDataTree);

    // Buttons
    m_customDataButtonLayout = new QHBoxLayout();
    m_addPropertyButton = new QPushButton("Add Property");
    m_removePropertyButton = new QPushButton("Remove Property");
    m_customDataButtonLayout->addWidget(m_addPropertyButton);
    m_customDataButtonLayout->addWidget(m_removePropertyButton);
    m_customDataButtonLayout->addStretch();
    m_customDataLayout->addLayout(m_customDataButtonLayout);

    m_propertiesTab->addTab(m_customDataTab, "Custom Data");

    // Connect signals
    connect(m_addPropertyButton, &QPushButton::clicked, this, &TilesetEditorDialog::onAddCustomProperty);
    connect(m_removePropertyButton, &QPushButton::clicked, this, &TilesetEditorDialog::onRemoveCustomProperty);
}

void TilesetEditorDialog::updateWindowTitle() {
    QString title = "Tileset Editor";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.fileName();
    } else {
        title += " - Untitled";
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void TilesetEditorDialog::updateTilesetView() {
    m_tilesetView->SetTileset(m_tileset.get());
    m_tilesetView->RefreshView();
}

void TilesetEditorDialog::updateTileProperties() {
    updateCollisionEditor();
    updateCustomDataEditor();
}

void TilesetEditorDialog::updateCollisionEditor() {
    if (m_currentTileId < 0) {
        m_collisionTypeCombo->setEnabled(false);
        m_collisionDataGroup->setEnabled(false);
        return;
    }

    const Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) {
        m_collisionTypeCombo->setEnabled(false);
        m_collisionDataGroup->setEnabled(false);
        return;
    }

    m_collisionTypeCombo->setEnabled(true);
    m_collisionDataGroup->setEnabled(true);

    // Update collision type
    m_collisionTypeCombo->setCurrentIndex(static_cast<int>(tile->collision.type));

    // Update collision data
    m_collisionOffsetXSpin->setValue(static_cast<int>(tile->collision.offset.x));
    m_collisionOffsetYSpin->setValue(static_cast<int>(tile->collision.offset.y));
    m_collisionSizeXSpin->setValue(static_cast<int>(tile->collision.size.x));
    m_collisionSizeYSpin->setValue(static_cast<int>(tile->collision.size.y));

    // Enable/disable collision data based on type
    bool hasCollisionData = (tile->collision.type != Lupine::TileCollisionType::None);
    m_collisionDataGroup->setEnabled(hasCollisionData);
}

void TilesetEditorDialog::updateCustomDataEditor() {
    m_customDataTree->clear();

    if (m_currentTileId < 0) {
        m_addPropertyButton->setEnabled(false);
        m_removePropertyButton->setEnabled(false);
        return;
    }

    const Lupine::TileData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) {
        m_addPropertyButton->setEnabled(false);
        m_removePropertyButton->setEnabled(false);
        return;
    }

    m_addPropertyButton->setEnabled(true);
    m_removePropertyButton->setEnabled(true);

    // Populate custom data tree
    for (const auto& pair : tile->custom_data) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_customDataTree);
        item->setText(0, QString::fromStdString(pair.first));

        const Lupine::TileDataValue& value = pair.second;
        QString typeStr;
        QString valueStr;

        switch (value.type) {
            case Lupine::TileDataType::String:
                typeStr = "String";
                valueStr = QString::fromStdString(value.string_value);
                break;
            case Lupine::TileDataType::Integer:
                typeStr = "Integer";
                valueStr = QString::number(value.int_value);
                break;
            case Lupine::TileDataType::Float:
                typeStr = "Float";
                valueStr = QString::number(value.float_value);
                break;
            case Lupine::TileDataType::Boolean:
                typeStr = "Boolean";
                valueStr = value.bool_value ? "true" : "false";
                break;
            case Lupine::TileDataType::Color:
                typeStr = "Color";
                valueStr = QString("(%1, %2, %3, %4)")
                    .arg(value.color_value.x)
                    .arg(value.color_value.y)
                    .arg(value.color_value.z)
                    .arg(value.color_value.w);
                break;
        }

        item->setText(1, typeStr);
        item->setText(2, valueStr);
    }
}

void TilesetEditorDialog::refreshImagePreview() {
    QString imagePath = m_imagePathEdit->text();
    if (imagePath.isEmpty()) {
        m_imagePreviewLabel->setText("No image loaded");
        m_imagePreviewLabel->setPixmap(QPixmap());
        return;
    }

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        m_imagePreviewLabel->setText("Invalid image");
        m_imagePreviewLabel->setPixmap(QPixmap());
        return;
    }

    // Scale pixmap to fit preview label
    QPixmap scaledPixmap = pixmap.scaled(
        m_imagePreviewLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_imagePreviewLabel->setPixmap(scaledPixmap);
}

bool TilesetEditorDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool TilesetEditorDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<TilesetEditorDialog*>(this),
        "Unsaved Changes",
        "The tileset has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (result) {
        case QMessageBox::Save:
            SaveTileset();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void TilesetEditorDialog::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

void TilesetEditorDialog::calculateAutoGridSize() {
    QString imagePath = m_imagePathEdit->text();
    if (imagePath.isEmpty()) {
        return;
    }

    // Load image to get dimensions
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        return;
    }

    int imageWidth = pixmap.width();
    int imageHeight = pixmap.height();
    int tileWidth = m_tileSizeXSpin->value();
    int tileHeight = m_tileSizeYSpin->value();

    if (tileWidth <= 0 || tileHeight <= 0) {
        return;
    }

    // Calculate grid size based on image dimensions and tile size
    int gridWidth = imageWidth / tileWidth;
    int gridHeight = imageHeight / tileHeight;

    // Update grid size spin boxes
    m_gridSizeXSpin->setValue(gridWidth);
    m_gridSizeYSpin->setValue(gridHeight);

    // Update tileset
    glm::ivec2 gridSize(gridWidth, gridHeight);
    m_tileset->SetGridSize(gridSize);
}


