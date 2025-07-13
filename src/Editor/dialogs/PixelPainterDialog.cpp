#include "PixelPainterDialog.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QDialog>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QBuffer>
#include <cmath>

// PixelLayer Implementation
PixelLayer::PixelLayer(const QString& name, int width, int height)
    : m_name(name)
    , m_image(width, height, QImage::Format_ARGB32)
    , m_visible(true)
    , m_opacity(1.0f)
    , m_blendMode(BlendMode::Normal)
    , m_alphaLocked(false)
    , m_clippingMask(false)
{
    m_image.fill(Qt::transparent);
}

PixelLayer::~PixelLayer() {
}

void PixelLayer::SetPixel(int x, int y, const QColor& color) {
    if (x >= 0 && x < m_image.width() && y >= 0 && y < m_image.height()) {
        if (m_alphaLocked && m_image.pixelColor(x, y).alpha() == 0) {
            return; // Don't paint on transparent pixels when alpha locked
        }
        m_image.setPixelColor(x, y, color);
    }
}

QColor PixelLayer::GetPixel(int x, int y) const {
    if (x >= 0 && x < m_image.width() && y >= 0 && y < m_image.height()) {
        return m_image.pixelColor(x, y);
    }
    return QColor();
}

void PixelLayer::Clear() {
    m_image.fill(Qt::transparent);
}

void PixelLayer::Resize(int width, int height) {
    QImage newImage(width, height, QImage::Format_ARGB32);
    newImage.fill(Qt::transparent);
    
    QPainter painter(&newImage);
    painter.drawImage(0, 0, m_image);
    painter.end();
    
    m_image = newImage;
}

// PixelCanvas Implementation
PixelCanvas::PixelCanvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_canvasItem(nullptr)
    , m_gridItem(nullptr)
    , m_canvasSize(32, 32)
    , m_currentTool(PixelTool::Brush)
    , m_brushSize(1)
    , m_primaryColor(Qt::black)
    , m_secondaryColor(Qt::white)
    , m_showGrid(true)
    , m_painting(false)
    , m_activeLayerIndex(0)
{
    setupScene();
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    
    // Create default layer
    AddLayer("Background");
}

PixelCanvas::~PixelCanvas() {
    if (m_scene) {
        delete m_scene;
    }
}

void PixelCanvas::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Create canvas item
    m_canvasItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_canvasItem);
    
    // Create grid item
    m_gridItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_gridItem);
    
    connect(m_scene, &QGraphicsScene::changed, this, &PixelCanvas::onSceneChanged);
}

void PixelCanvas::SetCanvasSize(int width, int height) {
    m_canvasSize = QSize(width, height);
    
    // Resize all layers
    for (auto& layer : m_layers) {
        layer->Resize(width, height);
    }
    
    updateCanvas();
    updateGrid();
}

void PixelCanvas::AddLayer(const QString& name) {
    auto layer = std::make_unique<PixelLayer>(name, m_canvasSize.width(), m_canvasSize.height());
    m_layers.push_back(std::move(layer));
    
    if (m_layers.size() == 1) {
        m_activeLayerIndex = 0;
    }
    
    updateCanvas();
    emit layerChanged();
}

void PixelCanvas::RemoveLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size()) && m_layers.size() > 1) {
        m_layers.erase(m_layers.begin() + index);
        
        if (m_activeLayerIndex >= static_cast<int>(m_layers.size())) {
            m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }
        
        updateCanvas();
        emit layerChanged();
    }
}

void PixelCanvas::SetActiveLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        m_activeLayerIndex = index;
        emit layerChanged();
    }
}

PixelLayer* PixelCanvas::GetActiveLayer() {
    if (m_activeLayerIndex >= 0 && m_activeLayerIndex < static_cast<int>(m_layers.size())) {
        return m_layers[m_activeLayerIndex].get();
    }
    return nullptr;
}

PixelLayer* PixelCanvas::GetLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        return m_layers[index].get();
    }
    return nullptr;
}

void PixelCanvas::SetShowGrid(bool show) {
    m_showGrid = show;
    updateGrid();
}

void PixelCanvas::ZoomIn() {
    scale(2.0, 2.0);
}

void PixelCanvas::ZoomOut() {
    scale(0.5, 0.5);
}

void PixelCanvas::ZoomToFit() {
    fitInView(m_canvasItem, Qt::KeepAspectRatio);
}

void PixelCanvas::ZoomToActual() {
    resetTransform();
}

void PixelCanvas::NewCanvas(int width, int height) {
    m_layers.clear();
    m_canvasSize = QSize(width, height);
    m_activeLayerIndex = 0;
    
    AddLayer("Background");
    updateCanvas();
    updateGrid();
    
    emit canvasModified();
}

void PixelCanvas::updateCanvas() {
    compositeImage();
    
    if (m_canvasItem) {
        QPixmap pixmap = QPixmap::fromImage(m_compositeImage);
        m_canvasItem->setPixmap(pixmap);
        
        // Update scene rect
        m_scene->setSceneRect(0, 0, m_canvasSize.width(), m_canvasSize.height());
    }
}

void PixelCanvas::updateGrid() {
    if (!m_gridItem) return;
    
    if (!m_showGrid) {
        m_gridItem->setVisible(false);
        return;
    }
    
    m_gridItem->setVisible(true);
    
    // Create grid pixmap
    QPixmap gridPixmap(m_canvasSize.width(), m_canvasSize.height());
    gridPixmap.fill(Qt::transparent);
    
    QPainter painter(&gridPixmap);
    painter.setPen(QPen(QColor(128, 128, 128, 128), 0));
    
    // Draw vertical lines
    for (int x = 0; x <= m_canvasSize.width(); ++x) {
        painter.drawLine(x, 0, x, m_canvasSize.height());
    }
    
    // Draw horizontal lines
    for (int y = 0; y <= m_canvasSize.height(); ++y) {
        painter.drawLine(0, y, m_canvasSize.width(), y);
    }
    
    painter.end();
    m_gridItem->setPixmap(gridPixmap);
}

void PixelCanvas::compositeImage() {
    m_compositeImage = QImage(m_canvasSize, QImage::Format_ARGB32);
    m_compositeImage.fill(Qt::transparent);

    QPainter painter(&m_compositeImage);
    painter.setRenderHint(QPainter::Antialiasing, false); // Disable antialiasing for pixel art
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Composite layers from bottom to top
    for (const auto& layer : m_layers) {
        if (!layer->IsVisible()) continue;

        QImage layerImage = layer->GetImage();

        // Reset painter state for each layer
        painter.setOpacity(layer->GetOpacity());

        // Apply blend mode
        switch (layer->GetBlendMode()) {
            case BlendMode::Normal:
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                break;
            case BlendMode::Multiply:
                painter.setCompositionMode(QPainter::CompositionMode_Multiply);
                break;
            case BlendMode::Overlay:
                painter.setCompositionMode(QPainter::CompositionMode_Overlay);
                break;
            case BlendMode::SoftLight:
                painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
                break;
        }

        painter.drawImage(0, 0, layerImage);

        // Reset opacity for next layer
        painter.setOpacity(1.0f);
    }

    painter.end();
}

QPoint PixelCanvas::screenToPixel(const QPoint& screenPos) const {
    QPointF scenePos = mapToScene(screenPos);
    return QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
}

QPoint PixelCanvas::pixelToScreen(const QPoint& pixelPos) const {
    QPointF scenePos(pixelPos.x(), pixelPos.y());
    return mapFromScene(scenePos);
}

void PixelCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pixelPos = screenToPixel(event->pos());
        
        if (pixelPos.x() >= 0 && pixelPos.x() < m_canvasSize.width() &&
            pixelPos.y() >= 0 && pixelPos.y() < m_canvasSize.height()) {
            
            switch (m_currentTool) {
                case PixelTool::Brush:
                    m_painting = true;
                    paintPixel(pixelPos.x(), pixelPos.y(), m_primaryColor);
                    break;
                case PixelTool::Eraser:
                    m_painting = true;
                    erasePixel(pixelPos.x(), pixelPos.y());
                    break;
                case PixelTool::Bucket:
                    floodFill(pixelPos.x(), pixelPos.y(), m_primaryColor);
                    break;
                case PixelTool::Eyedropper: {
                    QColor color = pickColor(pixelPos.x(), pixelPos.y());
                    emit colorPicked(color);
                    break;
                }
                case PixelTool::Wand:
                    // TODO: Implement magic wand selection
                    break;
            }
            
            m_lastPaintPos = event->pos();
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void PixelCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (m_painting && (event->buttons() & Qt::LeftButton)) {
        QPoint pixelPos = screenToPixel(event->pos());
        
        if (pixelPos.x() >= 0 && pixelPos.x() < m_canvasSize.width() &&
            pixelPos.y() >= 0 && pixelPos.y() < m_canvasSize.height()) {
            
            switch (m_currentTool) {
                case PixelTool::Brush:
                    paintPixel(pixelPos.x(), pixelPos.y(), m_primaryColor);
                    break;
                case PixelTool::Eraser:
                    erasePixel(pixelPos.x(), pixelPos.y());
                    break;
                default:
                    break;
            }
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_painting = false;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void PixelCanvas::wheelEvent(QWheelEvent* event) {
    // Zoom with mouse wheel
    if (event->modifiers() & Qt::ControlModifier) {
        const double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void PixelCanvas::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);
}

void PixelCanvas::onSceneChanged() {
    // Handle scene changes if needed
}

void PixelCanvas::paintPixel(int x, int y, const QColor& color) {
    PixelLayer* layer = GetActiveLayer();
    if (!layer) return;
    
    // Apply brush size
    int halfSize = m_brushSize / 2;
    for (int dx = -halfSize; dx <= halfSize; ++dx) {
        for (int dy = -halfSize; dy <= halfSize; ++dy) {
            int px = x + dx;
            int py = y + dy;
            
            if (px >= 0 && px < m_canvasSize.width() && py >= 0 && py < m_canvasSize.height()) {
                layer->SetPixel(px, py, color);
            }
        }
    }
    
    updateCanvas();
    emit canvasModified();
}

void PixelCanvas::floodFill(int x, int y, const QColor& fillColor) {
    PixelLayer* layer = GetActiveLayer();
    if (!layer) return;
    
    QColor targetColor = layer->GetPixel(x, y);
    if (targetColor == fillColor) return;
    
    // Simple flood fill implementation
    std::vector<QPoint> stack;
    stack.push_back(QPoint(x, y));
    
    while (!stack.empty()) {
        QPoint p = stack.back();
        stack.pop_back();
        
        if (p.x() < 0 || p.x() >= m_canvasSize.width() || 
            p.y() < 0 || p.y() >= m_canvasSize.height()) {
            continue;
        }
        
        if (layer->GetPixel(p.x(), p.y()) != targetColor) {
            continue;
        }
        
        layer->SetPixel(p.x(), p.y(), fillColor);
        
        stack.push_back(QPoint(p.x() + 1, p.y()));
        stack.push_back(QPoint(p.x() - 1, p.y()));
        stack.push_back(QPoint(p.x(), p.y() + 1));
        stack.push_back(QPoint(p.x(), p.y() - 1));
    }
    
    updateCanvas();
    emit canvasModified();
}

void PixelCanvas::erasePixel(int x, int y) {
    paintPixel(x, y, Qt::transparent);
}

QColor PixelCanvas::pickColor(int x, int y) {
    if (x >= 0 && x < m_compositeImage.width() && y >= 0 && y < m_compositeImage.height()) {
        return m_compositeImage.pixelColor(x, y);
    }
    return QColor();
}

// ColorWheelWidget Implementation
ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent)
    , m_selectedColor(Qt::red)
    , m_selectedPos(100, 100)
    , m_dragging(false)
{
    setFixedSize(200, 200);
    setMouseTracking(true);
}

ColorWheelWidget::~ColorWheelWidget() {
}

void ColorWheelWidget::SetSelectedColor(const QColor& color) {
    m_selectedColor = color;
    update();
    emit colorChanged(color);
}

void ColorWheelWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int size = qMin(width(), height()) - 20;
    QRect rect((width() - size) / 2, (height() - size) / 2, size, size);

    // Draw color wheel
    QConicalGradient gradient(rect.center(), 0);
    for (int i = 0; i < 360; i += 5) {
        gradient.setColorAt(i / 360.0, QColor::fromHsv(i, 255, 255));
    }

    painter.setBrush(QBrush(gradient));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect);

    // Draw saturation/brightness overlay
    QRadialGradient saturationGradient(rect.center(), size / 2);
    saturationGradient.setColorAt(0, QColor(255, 255, 255, 0));   // Transparent white at center
    saturationGradient.setColorAt(1, QColor(0, 0, 0, 255));       // Opaque black at edge

    painter.setBrush(QBrush(saturationGradient));
    painter.drawEllipse(rect);

    // Draw selection indicator
    painter.setPen(QPen(Qt::white, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(m_selectedPos.x() - 6, m_selectedPos.y() - 6, 12, 12);

    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(m_selectedPos.x() - 5, m_selectedPos.y() - 5, 10, 10);
}

void ColorWheelWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        updateColor(event->pos());
    }
}

void ColorWheelWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        updateColor(event->pos());
    }
}

void ColorWheelWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void ColorWheelWidget::updateColor(const QPoint& pos) {
    int size = qMin(width(), height()) - 20;
    QPoint center(width() / 2, height() / 2);
    QPoint offset = pos - center;

    double distance = sqrt(offset.x() * offset.x() + offset.y() * offset.y());
    double maxDistance = size / 2.0;

    // Clamp to circle bounds
    if (distance > maxDistance) {
        double scale = maxDistance / distance;
        offset.setX(static_cast<int>(offset.x() * scale));
        offset.setY(static_cast<int>(offset.y() * scale));
        distance = maxDistance;
    }

    m_selectedPos = center + offset;
    QColor color = colorAtPosition(m_selectedPos);
    if (color.isValid()) {
        m_selectedColor = color;
        update();
        emit colorChanged(color);
    }
}

QColor ColorWheelWidget::colorAtPosition(const QPoint& pos) const {
    int size = qMin(width(), height()) - 20;
    QPoint center(width() / 2, height() / 2);
    QPoint offset = pos - center;

    double distance = sqrt(offset.x() * offset.x() + offset.y() * offset.y());
    double maxDistance = size / 2.0;

    if (distance > maxDistance) {
        return QColor(); // Outside the wheel
    }

    double angle = atan2(-offset.y(), offset.x()) * 180.0 / M_PI; // Negative Y for correct orientation
    if (angle < 0) angle += 360;

    int hue = static_cast<int>(angle);
    int saturation = static_cast<int>((distance / maxDistance) * 255);
    int value = 255;

    return QColor::fromHsv(hue, saturation, value);
}

// ColorPaletteWidget Implementation
ColorPaletteWidget::ColorPaletteWidget(QWidget* parent)
    : QWidget(parent)
    , m_selectedIndex(-1)
    , m_swatchSize(20, 20)
{
    setMinimumHeight(100);

    // Add some default colors
    m_colors.push_back(Qt::black);
    m_colors.push_back(Qt::white);
    m_colors.push_back(Qt::red);
    m_colors.push_back(Qt::green);
    m_colors.push_back(Qt::blue);
    m_colors.push_back(Qt::yellow);
    m_colors.push_back(Qt::cyan);
    m_colors.push_back(Qt::magenta);
}

ColorPaletteWidget::~ColorPaletteWidget() {
}

void ColorPaletteWidget::AddColor(const QColor& color) {
    m_colors.push_back(color);
    update();
}

void ColorPaletteWidget::RemoveColor(int index) {
    if (index >= 0 && index < static_cast<int>(m_colors.size())) {
        m_colors.erase(m_colors.begin() + index);
        if (m_selectedIndex == index) {
            m_selectedIndex = -1;
        } else if (m_selectedIndex > index) {
            m_selectedIndex--;
        }
        update();
    }
}

void ColorPaletteWidget::ClearPalette() {
    m_colors.clear();
    m_selectedIndex = -1;
    update();
}

void ColorPaletteWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    int cols = width() / m_swatchSize.width();
    if (cols == 0) cols = 1;

    for (int i = 0; i < static_cast<int>(m_colors.size()); ++i) {
        int row = i / cols;
        int col = i % cols;

        QRect rect(col * m_swatchSize.width(), row * m_swatchSize.height(),
                   m_swatchSize.width(), m_swatchSize.height());

        painter.fillRect(rect, m_colors[i]);

        if (i == m_selectedIndex) {
            painter.setPen(QPen(Qt::white, 2));
            painter.drawRect(rect.adjusted(1, 1, -1, -1));
            painter.setPen(QPen(Qt::black, 1));
            painter.drawRect(rect.adjusted(2, 2, -2, -2));
        } else {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawRect(rect);
        }
    }
}

void ColorPaletteWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int index = getColorIndexAt(event->pos());
        if (index >= 0 && index < static_cast<int>(m_colors.size())) {
            m_selectedIndex = index;
            update();
            emit colorSelected(m_colors[index]);
        }
    }
}

int ColorPaletteWidget::getColorIndexAt(const QPoint& pos) const {
    int cols = width() / m_swatchSize.width();
    if (cols == 0) return -1;

    int col = pos.x() / m_swatchSize.width();
    int row = pos.y() / m_swatchSize.height();

    return row * cols + col;
}

// PixelPainterDialog Implementation
PixelPainterDialog::PixelPainterDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_canvas(nullptr)
    , m_toolPanel(nullptr)
    , m_layerPanel(nullptr)
    , m_colorPanel(nullptr)
    , m_modified(false)
    , m_currentTool(PixelTool::Brush)
{
    setWindowTitle("Pixel Painter");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUI();
    updateWindowTitle();
    updateLayerList();
    updateToolStates();
}

PixelPainterDialog::~PixelPainterDialog() {
}

void PixelPainterDialog::setupUI() {
    setupMenuBar();
    setupToolBar();
    setupMainPanels();

    // Connect canvas signals
    if (m_canvas) {
        connect(m_canvas, &PixelCanvas::canvasModified, this, &PixelPainterDialog::onCanvasModified);
        connect(m_canvas, &PixelCanvas::layerChanged, this, &PixelPainterDialog::onLayerChanged);
        connect(m_canvas, &PixelCanvas::colorPicked, this, &PixelPainterDialog::onColorPicked);
    }
}

void PixelPainterDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New Canvas...", this, &PixelPainterDialog::onNewCanvas, QKeySequence::New);
    fileMenu->addAction("&Open...", this, &PixelPainterDialog::onOpenFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", this, &PixelPainterDialog::onSaveFile, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &PixelPainterDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Export Image...", this, &PixelPainterDialog::onExportImage);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo", [](){ /* TODO */ }, QKeySequence::Undo);
    editMenu->addAction("&Redo", [](){ /* TODO */ }, QKeySequence::Redo);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    QAction* gridAction = viewMenu->addAction("Show &Grid");
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    connect(gridAction, &QAction::toggled, [this](bool checked) {
        if (m_canvas) m_canvas->SetShowGrid(checked);
    });

    viewMenu->addSeparator();
    viewMenu->addAction("Zoom &In", [this](){ if (m_canvas) m_canvas->ZoomIn(); }, QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out", [this](){ if (m_canvas) m_canvas->ZoomOut(); }, QKeySequence::ZoomOut);
    viewMenu->addAction("Zoom to &Fit", [this](){ if (m_canvas) m_canvas->ZoomToFit(); });
    viewMenu->addAction("&Actual Size", [this](){ if (m_canvas) m_canvas->ZoomToActual(); });
}

void PixelPainterDialog::setupToolBar() {
    m_toolBar = new QToolBar("Tools", this);
    m_toolBar->setMaximumHeight(50);

    // Tool group for exclusive selection
    m_toolGroup = new QActionGroup(this);

    // Drawing tools
    m_brushAction = m_toolBar->addAction("Brush");
    m_brushAction->setCheckable(true);
    m_brushAction->setChecked(true);
    m_brushAction->setToolTip("Brush Tool (B)");
    m_brushAction->setShortcut(QKeySequence("B"));
    m_toolGroup->addAction(m_brushAction);

    m_eraserAction = m_toolBar->addAction("Eraser");
    m_eraserAction->setCheckable(true);
    m_eraserAction->setToolTip("Eraser Tool (E)");
    m_eraserAction->setShortcut(QKeySequence("E"));
    m_toolGroup->addAction(m_eraserAction);

    m_bucketAction = m_toolBar->addAction("Bucket");
    m_bucketAction->setCheckable(true);
    m_bucketAction->setToolTip("Bucket Fill Tool (G)");
    m_bucketAction->setShortcut(QKeySequence("G"));
    m_toolGroup->addAction(m_bucketAction);

    m_eyedropperAction = m_toolBar->addAction("Eyedropper");
    m_eyedropperAction->setCheckable(true);
    m_eyedropperAction->setToolTip("Eyedropper Tool (I)");
    m_eyedropperAction->setShortcut(QKeySequence("I"));
    m_toolGroup->addAction(m_eyedropperAction);

    m_wandAction = m_toolBar->addAction("Wand");
    m_wandAction->setCheckable(true);
    m_wandAction->setToolTip("Magic Wand Tool (W)");
    m_wandAction->setShortcut(QKeySequence("W"));
    m_toolGroup->addAction(m_wandAction);

    m_toolBar->addSeparator();

    // Brush size controls
    m_toolBar->addWidget(new QLabel("Size:"));
    m_brushSizeSlider = new QSlider(Qt::Horizontal);
    m_brushSizeSlider->setRange(1, 20);
    m_brushSizeSlider->setValue(1);
    m_brushSizeSlider->setMaximumWidth(100);
    m_toolBar->addWidget(m_brushSizeSlider);

    m_brushSizeSpinBox = new QSpinBox();
    m_brushSizeSpinBox->setRange(1, 20);
    m_brushSizeSpinBox->setValue(1);
    m_brushSizeSpinBox->setMaximumWidth(60);
    m_toolBar->addWidget(m_brushSizeSpinBox);

    // Connect tool signals
    connect(m_toolGroup, &QActionGroup::triggered, this, &PixelPainterDialog::onToolChanged);
    connect(m_brushSizeSlider, &QSlider::valueChanged, this, &PixelPainterDialog::onBrushSizeChanged);
    connect(m_brushSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PixelPainterDialog::onBrushSizeChanged);
}

void PixelPainterDialog::setupMainPanels() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Add menu bar
    m_mainLayout->setMenuBar(m_menuBar);

    // Add toolbar
    m_mainLayout->addWidget(m_toolBar);

    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupToolPanel();
    setupCanvasPanel();
    setupLayerPanel();
    setupColorPanel();

    // Set splitter proportions
    m_mainSplitter->setSizes({200, 600, 200, 200});
    m_mainSplitter->setStretchFactor(0, 0);  // Tool panel fixed
    m_mainSplitter->setStretchFactor(1, 1);  // Canvas stretches
    m_mainSplitter->setStretchFactor(2, 0);  // Layer panel fixed
    m_mainSplitter->setStretchFactor(3, 0);  // Color panel fixed
}

void PixelPainterDialog::setupToolPanel() {
    m_toolPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_toolPanel);

    // Tool properties group
    QGroupBox* toolGroup = new QGroupBox("Tool Properties");
    QVBoxLayout* toolLayout = new QVBoxLayout(toolGroup);

    // Additional tool controls can be added here

    layout->addWidget(toolGroup);
    layout->addStretch();

    m_mainSplitter->addWidget(m_toolPanel);
}

void PixelPainterDialog::setupCanvasPanel() {
    QWidget* canvasWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(canvasWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_canvas = new PixelCanvas();
    layout->addWidget(m_canvas);

    m_mainSplitter->addWidget(canvasWidget);
}

void PixelPainterDialog::setupLayerPanel() {
    m_layerPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_layerPanel);

    // Layer list group
    QGroupBox* layerGroup = new QGroupBox("Layers");
    QVBoxLayout* layerLayout = new QVBoxLayout(layerGroup);

    // Layer list
    m_layerList = new QListWidget();
    m_layerList->setMaximumHeight(200);
    layerLayout->addWidget(m_layerList);

    // Layer buttons
    QHBoxLayout* layerButtonLayout = new QHBoxLayout();
    m_addLayerButton = new QPushButton("+");
    m_addLayerButton->setMaximumWidth(30);
    m_removeLayerButton = new QPushButton("-");
    m_removeLayerButton->setMaximumWidth(30);
    m_duplicateLayerButton = new QPushButton("Dup");
    m_moveLayerUpButton = new QPushButton("↑");
    m_moveLayerUpButton->setMaximumWidth(30);
    m_moveLayerDownButton = new QPushButton("↓");
    m_moveLayerDownButton->setMaximumWidth(30);

    layerButtonLayout->addWidget(m_addLayerButton);
    layerButtonLayout->addWidget(m_removeLayerButton);
    layerButtonLayout->addWidget(m_duplicateLayerButton);
    layerButtonLayout->addWidget(m_moveLayerUpButton);
    layerButtonLayout->addWidget(m_moveLayerDownButton);
    layerButtonLayout->addStretch();

    layerLayout->addLayout(layerButtonLayout);

    // Layer properties
    QLabel* opacityLabel = new QLabel("Opacity:");
    layerLayout->addWidget(opacityLabel);

    m_layerOpacitySlider = new QSlider(Qt::Horizontal);
    m_layerOpacitySlider->setRange(0, 100);
    m_layerOpacitySlider->setValue(100);
    layerLayout->addWidget(m_layerOpacitySlider);

    QLabel* blendLabel = new QLabel("Blend Mode:");
    layerLayout->addWidget(blendLabel);

    m_layerBlendModeCombo = new QComboBox();
    m_layerBlendModeCombo->addItems({"Normal", "Multiply", "Overlay", "Soft Light"});
    layerLayout->addWidget(m_layerBlendModeCombo);

    m_layerAlphaLockCheck = new QCheckBox("Alpha Lock");
    layerLayout->addWidget(m_layerAlphaLockCheck);

    m_layerClippingMaskCheck = new QCheckBox("Clipping Mask");
    layerLayout->addWidget(m_layerClippingMaskCheck);

    layout->addWidget(layerGroup);
    layout->addStretch();

    // Connect layer signals
    connect(m_addLayerButton, &QPushButton::clicked, this, &PixelPainterDialog::onAddLayer);
    connect(m_removeLayerButton, &QPushButton::clicked, this, &PixelPainterDialog::onRemoveLayer);
    connect(m_duplicateLayerButton, &QPushButton::clicked, this, &PixelPainterDialog::onDuplicateLayer);
    connect(m_moveLayerUpButton, &QPushButton::clicked, this, &PixelPainterDialog::onMoveLayerUp);
    connect(m_moveLayerDownButton, &QPushButton::clicked, this, &PixelPainterDialog::onMoveLayerDown);
    connect(m_layerList, &QListWidget::currentRowChanged, this, &PixelPainterDialog::onLayerSelectionChanged);
    connect(m_layerOpacitySlider, &QSlider::valueChanged, this, &PixelPainterDialog::onLayerOpacityChanged);
    connect(m_layerBlendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PixelPainterDialog::onLayerBlendModeChanged);
    connect(m_layerAlphaLockCheck, &QCheckBox::toggled, this, &PixelPainterDialog::onLayerAlphaLockChanged);
    connect(m_layerClippingMaskCheck, &QCheckBox::toggled, this, &PixelPainterDialog::onLayerClippingMaskChanged);

    m_mainSplitter->addWidget(m_layerPanel);
}

void PixelPainterDialog::setupColorPanel() {
    m_colorPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_colorPanel);

    // Color picker group
    QGroupBox* colorGroup = new QGroupBox("Colors");
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

    // Primary/Secondary color buttons
    QHBoxLayout* colorButtonLayout = new QHBoxLayout();
    m_primaryColorButton = new QPushButton();
    m_primaryColorButton->setFixedSize(40, 40);
    m_primaryColorButton->setStyleSheet("background-color: black; border: 2px solid gray;");

    m_secondaryColorButton = new QPushButton();
    m_secondaryColorButton->setFixedSize(40, 40);
    m_secondaryColorButton->setStyleSheet("background-color: white; border: 2px solid gray;");

    colorButtonLayout->addWidget(new QLabel("Primary:"));
    colorButtonLayout->addWidget(m_primaryColorButton);
    colorButtonLayout->addWidget(new QLabel("Secondary:"));
    colorButtonLayout->addWidget(m_secondaryColorButton);
    colorButtonLayout->addStretch();

    colorLayout->addLayout(colorButtonLayout);

    // Color wheel
    m_colorWheel = new ColorWheelWidget();
    colorLayout->addWidget(m_colorWheel);

    layout->addWidget(colorGroup);

    // Color palette group
    QGroupBox* paletteGroup = new QGroupBox("Palette");
    QVBoxLayout* paletteLayout = new QVBoxLayout(paletteGroup);

    m_colorPalette = new ColorPaletteWidget();
    paletteLayout->addWidget(m_colorPalette);

    QHBoxLayout* paletteButtonLayout = new QHBoxLayout();
    m_loadPaletteButton = new QPushButton("Load");
    m_savePaletteButton = new QPushButton("Save");

    paletteButtonLayout->addWidget(m_loadPaletteButton);
    paletteButtonLayout->addWidget(m_savePaletteButton);
    paletteButtonLayout->addStretch();

    paletteLayout->addLayout(paletteButtonLayout);

    layout->addWidget(paletteGroup);
    layout->addStretch();

    // Connect color signals
    connect(m_primaryColorButton, &QPushButton::clicked, this, &PixelPainterDialog::onPrimaryColorChanged);
    connect(m_secondaryColorButton, &QPushButton::clicked, this, &PixelPainterDialog::onSecondaryColorChanged);
    connect(m_colorWheel, &ColorWheelWidget::colorChanged, [this](const QColor& color) {
        if (m_canvas) {
            m_canvas->SetPrimaryColor(color);
            m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
        }
    });
    connect(m_colorPalette, &ColorPaletteWidget::colorSelected, [this](const QColor& color) {
        if (m_canvas) {
            m_canvas->SetPrimaryColor(color);
            m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
            m_colorWheel->SetSelectedColor(color);
        }
    });
    connect(m_loadPaletteButton, &QPushButton::clicked, [this]() {
        QString filepath = QFileDialog::getOpenFileName(
            this,
            "Load Color Palette",
            QDir::currentPath(),
            "Palette Files (*.palette);;All Files (*)"
        );
        if (!filepath.isEmpty()) {
            m_colorPalette->LoadPalette(filepath);
        }
    });
    connect(m_savePaletteButton, &QPushButton::clicked, [this]() {
        QString filepath = QFileDialog::getSaveFileName(
            this,
            "Save Color Palette",
            QDir::currentPath(),
            "Palette Files (*.palette);;All Files (*)"
        );
        if (!filepath.isEmpty()) {
            m_colorPalette->SavePalette(filepath);
        }
    });

    m_mainSplitter->addWidget(m_colorPanel);
}

// Slot implementations
void PixelPainterDialog::onNewCanvas() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    // Create custom dialog for canvas size
    QDialog dialog(this);
    dialog.setWindowTitle("New Canvas");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Width input
    QHBoxLayout* widthLayout = new QHBoxLayout();
    widthLayout->addWidget(new QLabel("Width:"));
    QSpinBox* widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(1, 4096);
    widthSpinBox->setValue(32);
    widthLayout->addWidget(widthSpinBox);
    layout->addLayout(widthLayout);

    // Height input
    QHBoxLayout* heightLayout = new QHBoxLayout();
    heightLayout->addWidget(new QLabel("Height:"));
    QSpinBox* heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(1, 4096);
    heightSpinBox->setValue(32);
    heightLayout->addWidget(heightSpinBox);
    layout->addLayout(heightLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int width = widthSpinBox->value();
        int height = heightSpinBox->value();

        if (m_canvas) {
            m_canvas->NewCanvas(width, height);
        }

        m_currentFilePath.clear();
        setModified(false);
        updateWindowTitle();
        updateLayerList();
    }
}

void PixelPainterDialog::onOpenFile() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Pixel Art",
        QDir::currentPath(),
        "Pixel Art Files (*.pixelart);;Image Files (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (m_canvas->LoadFromFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
            updateLayerList();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load file.");
        }
    }
}

void PixelPainterDialog::onSaveFile() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
    } else if (m_canvas) {
        if (m_canvas->SaveToFile(m_currentFilePath)) {
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void PixelPainterDialog::onSaveAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Pixel Art",
        QDir::currentPath(),
        "Pixel Art Files (*.pixelart);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (m_canvas->SaveToFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void PixelPainterDialog::onExportImage() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Export Image",
        QDir::currentPath(),
        "PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;ICO Files (*.ico);;BMP Files (*.bmp);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (!m_canvas->ExportToImage(filepath)) {
            QMessageBox::warning(this, "Error", "Failed to export image.");
        }
    }
}

void PixelPainterDialog::onToolChanged(QAction* action) {
    if (!action || !m_canvas) return;

    if (action == m_brushAction) {
        m_currentTool = PixelTool::Brush;
        m_canvas->SetCurrentTool(PixelTool::Brush);
    } else if (action == m_eraserAction) {
        m_currentTool = PixelTool::Eraser;
        m_canvas->SetCurrentTool(PixelTool::Eraser);
    } else if (action == m_bucketAction) {
        m_currentTool = PixelTool::Bucket;
        m_canvas->SetCurrentTool(PixelTool::Bucket);
    } else if (action == m_eyedropperAction) {
        m_currentTool = PixelTool::Eyedropper;
        m_canvas->SetCurrentTool(PixelTool::Eyedropper);
    } else if (action == m_wandAction) {
        m_currentTool = PixelTool::Wand;
        m_canvas->SetCurrentTool(PixelTool::Wand);
    }

    updateToolStates();
}

void PixelPainterDialog::onBrushSizeChanged() {
    int size = 1;

    QObject* sender = this->sender();
    if (sender == m_brushSizeSlider) {
        size = m_brushSizeSlider->value();
        m_brushSizeSpinBox->blockSignals(true);
        m_brushSizeSpinBox->setValue(size);
        m_brushSizeSpinBox->blockSignals(false);
    } else if (sender == m_brushSizeSpinBox) {
        size = m_brushSizeSpinBox->value();
        m_brushSizeSlider->blockSignals(true);
        m_brushSizeSlider->setValue(size);
        m_brushSizeSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetBrushSize(size);
    }
}

void PixelPainterDialog::onPrimaryColorChanged() {
    QColor color = QColorDialog::getColor(m_canvas ? m_canvas->GetPrimaryColor() : Qt::black, this);
    if (color.isValid() && m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
        m_colorWheel->SetSelectedColor(color);
    }
}

void PixelPainterDialog::onSecondaryColorChanged() {
    QColor color = QColorDialog::getColor(m_canvas ? m_canvas->GetSecondaryColor() : Qt::white, this);
    if (color.isValid() && m_canvas) {
        m_canvas->SetSecondaryColor(color);
        m_secondaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

// Layer management slots
void PixelPainterDialog::onAddLayer() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Layer", "Layer name:", QLineEdit::Normal, "New Layer", &ok);
    if (ok && !name.isEmpty() && m_canvas) {
        m_canvas->AddLayer(name);
        updateLayerList();
        setModified(true);
    }
}

void PixelPainterDialog::onRemoveLayer() {
    if (!m_canvas || m_canvas->GetLayerCount() <= 1) return;

    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0) {
        m_canvas->RemoveLayer(currentRow);
        updateLayerList();
        setModified(true);
    }
}

void PixelPainterDialog::onDuplicateLayer() {
    // TODO: Implement layer duplication
}

void PixelPainterDialog::onMoveLayerUp() {
    // TODO: Implement layer reordering
}

void PixelPainterDialog::onMoveLayerDown() {
    // TODO: Implement layer reordering
}

void PixelPainterDialog::onLayerSelectionChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        m_canvas->SetActiveLayer(currentRow);

        PixelLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            m_layerOpacitySlider->blockSignals(true);
            m_layerOpacitySlider->setValue(static_cast<int>(layer->GetOpacity() * 100));
            m_layerOpacitySlider->blockSignals(false);

            m_layerBlendModeCombo->blockSignals(true);
            m_layerBlendModeCombo->setCurrentIndex(static_cast<int>(layer->GetBlendMode()));
            m_layerBlendModeCombo->blockSignals(false);

            m_layerAlphaLockCheck->blockSignals(true);
            m_layerAlphaLockCheck->setChecked(layer->IsAlphaLocked());
            m_layerAlphaLockCheck->blockSignals(false);

            m_layerClippingMaskCheck->blockSignals(true);
            m_layerClippingMaskCheck->setChecked(layer->HasClippingMask());
            m_layerClippingMaskCheck->blockSignals(false);
        }
    }
}

void PixelPainterDialog::onLayerVisibilityChanged() {
    // TODO: Implement layer visibility toggle
}

void PixelPainterDialog::onLayerOpacityChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        PixelLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            float opacity = m_layerOpacitySlider->value() / 100.0f;
            layer->SetOpacity(opacity);
            setModified(true);
        }
    }
}

void PixelPainterDialog::onLayerBlendModeChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        PixelLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            BlendMode mode = static_cast<BlendMode>(m_layerBlendModeCombo->currentIndex());
            layer->SetBlendMode(mode);
            setModified(true);
        }
    }
}

void PixelPainterDialog::onLayerAlphaLockChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        PixelLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            layer->SetAlphaLocked(m_layerAlphaLockCheck->isChecked());
            setModified(true);
        }
    }
}

void PixelPainterDialog::onLayerClippingMaskChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        PixelLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            layer->SetClippingMask(m_layerClippingMaskCheck->isChecked());
            setModified(true);
        }
    }
}

// Canvas event slots
void PixelPainterDialog::onCanvasModified() {
    setModified(true);
}

void PixelPainterDialog::onLayerChanged() {
    updateLayerList();
}

void PixelPainterDialog::onColorPicked(const QColor& color) {
    if (m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
        m_colorWheel->SetSelectedColor(color);
    }
}

// Utility methods
void PixelPainterDialog::updateWindowTitle() {
    QString title = "Pixel Painter";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void PixelPainterDialog::updateLayerList() {
    if (!m_canvas || !m_layerList) return;

    m_layerList->blockSignals(true);
    m_layerList->clear();

    for (int i = 0; i < m_canvas->GetLayerCount(); ++i) {
        PixelLayer* layer = m_canvas->GetLayer(i);
        if (layer) {
            m_layerList->addItem(layer->GetName());
        }
    }

    m_layerList->setCurrentRow(m_canvas->GetActiveLayerIndex());
    m_layerList->blockSignals(false);
}

void PixelPainterDialog::updateToolStates() {
    // Update tool-specific UI states if needed
}

bool PixelPainterDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool PixelPainterDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) return true;

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<PixelPainterDialog*>(this),
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (result) {
        case QMessageBox::Save:
            onSaveFile();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void PixelPainterDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

// File operations for PixelCanvas
bool PixelCanvas::LoadFromFile(const QString& filepath) {
    if (filepath.endsWith(".pixelart")) {
        // Load layered format
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) {
            return false;
        }

        QJsonObject obj = doc.object();
        int width = obj["width"].toInt();
        int height = obj["height"].toInt();

        if (width <= 0 || height <= 0) {
            return false;
        }

        NewCanvas(width, height);
        m_layers.clear();

        QJsonArray layersArray = obj["layers"].toArray();
        for (const QJsonValue& layerValue : layersArray) {
            QJsonObject layerObj = layerValue.toObject();
            QString name = layerObj["name"].toString();
            bool visible = layerObj["visible"].toBool(true);
            double opacity = layerObj["opacity"].toDouble(1.0);
            int blendMode = layerObj["blendMode"].toInt(0);
            bool alphaLocked = layerObj["alphaLocked"].toBool(false);
            bool clippingMask = layerObj["clippingMask"].toBool(false);
            QString imageData = layerObj["imageData"].toString();

            auto layer = std::make_unique<PixelLayer>(name, width, height);
            layer->SetVisible(visible);
            layer->SetOpacity(static_cast<float>(opacity));
            layer->SetBlendMode(static_cast<BlendMode>(blendMode));
            layer->SetAlphaLocked(alphaLocked);
            layer->SetClippingMask(clippingMask);

            // Decode base64 image data
            QByteArray imageBytes = QByteArray::fromBase64(imageData.toUtf8());
            QImage image;
            image.loadFromData(imageBytes, "PNG");
            if (!image.isNull()) {
                layer->GetImage() = image.convertToFormat(QImage::Format_ARGB32);
            }

            m_layers.push_back(std::move(layer));
        }

        if (m_layers.empty()) {
            AddLayer("Background");
        }

        m_activeLayerIndex = qMin(obj["activeLayer"].toInt(0), static_cast<int>(m_layers.size()) - 1);

        updateCanvas();
        emit layerChanged();
        return true;
    } else {
        // Load regular image format
        QImage image(filepath);
        if (image.isNull()) return false;

        NewCanvas(image.width(), image.height());
        PixelLayer* layer = GetActiveLayer();
        if (layer) {
            layer->GetImage() = image.convertToFormat(QImage::Format_ARGB32);
            updateCanvas();
            emit canvasModified();
        }

        return true;
    }
}

bool PixelCanvas::SaveToFile(const QString& filepath) {
    if (filepath.endsWith(".pixelart")) {
        // Save layered format
        QJsonObject obj;
        obj["width"] = m_canvasSize.width();
        obj["height"] = m_canvasSize.height();
        obj["activeLayer"] = m_activeLayerIndex;

        QJsonArray layersArray;
        for (const auto& layer : m_layers) {
            QJsonObject layerObj;
            layerObj["name"] = layer->GetName();
            layerObj["visible"] = layer->IsVisible();
            layerObj["opacity"] = static_cast<double>(layer->GetOpacity());
            layerObj["blendMode"] = static_cast<int>(layer->GetBlendMode());
            layerObj["alphaLocked"] = layer->IsAlphaLocked();
            layerObj["clippingMask"] = layer->HasClippingMask();

            // Encode image data as base64
            QByteArray imageBytes;
            QBuffer buffer(&imageBytes);
            buffer.open(QIODevice::WriteOnly);
            layer->GetImage().save(&buffer, "PNG");
            layerObj["imageData"] = QString::fromUtf8(imageBytes.toBase64());

            layersArray.append(layerObj);
        }

        obj["layers"] = layersArray;

        QJsonDocument doc(obj);

        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }

        file.write(doc.toJson());
        return true;
    } else {
        // Save composite image
        return m_compositeImage.save(filepath);
    }
}

bool PixelCanvas::ExportToImage(const QString& filepath) {
    return m_compositeImage.save(filepath);
}

// ColorPaletteWidget file operations
bool ColorPaletteWidget::LoadPalette(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    QJsonArray colors = obj["colors"].toArray();

    m_colors.clear();
    for (const QJsonValue& value : colors) {
        QJsonObject colorObj = value.toObject();
        int r = colorObj["r"].toInt();
        int g = colorObj["g"].toInt();
        int b = colorObj["b"].toInt();
        int a = colorObj["a"].toInt(255);

        m_colors.push_back(QColor(r, g, b, a));
    }

    m_selectedIndex = -1;
    update();
    return true;
}

bool ColorPaletteWidget::SavePalette(const QString& filepath) {
    QJsonObject obj;
    QJsonArray colors;

    for (const QColor& color : m_colors) {
        QJsonObject colorObj;
        colorObj["r"] = color.red();
        colorObj["g"] = color.green();
        colorObj["b"] = color.blue();
        colorObj["a"] = color.alpha();
        colors.append(colorObj);
    }

    obj["colors"] = colors;

    QJsonDocument doc(obj);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}


