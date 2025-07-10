#include "ScribblerDialog.h"
#include "PixelPainterDialog.h" // For ColorWheelWidget and ColorPaletteWidget
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
#include <QtGui/QTabletEvent>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QBuffer>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// RasterLayer Implementation
RasterLayer::RasterLayer(const QString& name, int width, int height)
    : m_name(name)
    , m_image(width, height, QImage::Format_ARGB32_Premultiplied)
    , m_visible(true)
    , m_opacity(1.0f)
    , m_blendMode(RasterBlendMode::Normal)
    , m_alphaLocked(false)
{
    m_image.fill(Qt::transparent);
}

RasterLayer::~RasterLayer() {
}

void RasterLayer::Clear() {
    m_image.fill(Qt::transparent);
}

void RasterLayer::Resize(int width, int height) {
    QImage newImage(width, height, QImage::Format_ARGB32_Premultiplied);
    newImage.fill(Qt::transparent);
    
    QPainter painter(&newImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawImage(0, 0, m_image);
    painter.end();
    
    m_image = newImage;

    // Also resize mask if it exists
    if (!m_mask.isNull()) {
        QImage newMask(width, height, QImage::Format_Grayscale8);
        newMask.fill(Qt::white);

        QPainter maskPainter(&newMask);
        maskPainter.setRenderHint(QPainter::Antialiasing);
        maskPainter.drawImage(0, 0, m_mask);
        maskPainter.end();

        m_mask = newMask;
    }
}

void RasterLayer::CreateMask() {
    m_mask = QImage(m_image.size(), QImage::Format_Grayscale8);
    m_mask.fill(Qt::white); // White = fully visible
}

void RasterLayer::DeleteMask() {
    m_mask = QImage();
}

void RasterLayer::ApplyMask() {
    if (m_mask.isNull()) return;

    // Apply mask to the layer image
    for (int y = 0; y < m_image.height(); ++y) {
        for (int x = 0; x < m_image.width(); ++x) {
            QColor pixelColor = m_image.pixelColor(x, y);
            QColor maskColor = m_mask.pixelColor(x, y);

            // Use mask's grayscale value as alpha multiplier
            float maskAlpha = maskColor.redF(); // Grayscale, so R=G=B
            pixelColor.setAlphaF(pixelColor.alphaF() * maskAlpha);

            m_image.setPixelColor(x, y, pixelColor);
        }
    }

    // Remove the mask after applying it
    DeleteMask();
}

// ScribblerCanvas Implementation
ScribblerCanvas::ScribblerCanvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_canvasItem(nullptr)
    , m_canvasSize(800, 600)
    , m_currentTool(ScribblerTool::Brush)
    , m_brushSize(10.0f)
    , m_brushHardness(0.8f)
    , m_brushOpacity(1.0f)
    , m_brushType(BrushType::Round)
    , m_eraserMode(EraserMode::Normal)
    , m_fillTolerance(0.0f)
    , m_brushSpacing(0.25f)
    , m_antiAliasing(true)
    , m_primaryColor(Qt::black)
    , m_secondaryColor(Qt::white)
    , m_drawing(false)
    , m_activeLayerIndex(0)
    , m_strokeTimer(nullptr)
    , m_currentPressure(1.0f)
    , m_pressureEnabled(true)
{
    setupScene();
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    // Create stroke timer for smooth drawing
    m_strokeTimer = new QTimer(this);
    m_strokeTimer->setSingleShot(true);
    m_strokeTimer->setInterval(16); // ~60 FPS
    connect(m_strokeTimer, &QTimer::timeout, this, &ScribblerCanvas::updateCanvas);
    
    // Create default layer
    AddLayer("Background");

    // Fill background layer with white
    RasterLayer* backgroundLayer = GetActiveLayer();
    if (backgroundLayer) {
        backgroundLayer->GetImage().fill(Qt::white);
        updateCanvas();
    }
}

ScribblerCanvas::~ScribblerCanvas() {
    if (m_scene) {
        delete m_scene;
    }
}

void ScribblerCanvas::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Create canvas item
    m_canvasItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_canvasItem);
    
    connect(m_scene, &QGraphicsScene::changed, this, &ScribblerCanvas::onSceneChanged);
}

void ScribblerCanvas::SetCanvasSize(int width, int height) {
    m_canvasSize = QSize(width, height);
    
    // Resize all layers
    for (auto& layer : m_layers) {
        layer->Resize(width, height);
    }
    
    updateCanvas();
}

void ScribblerCanvas::AddLayer(const QString& name) {
    auto layer = std::make_unique<RasterLayer>(name, m_canvasSize.width(), m_canvasSize.height());
    m_layers.push_back(std::move(layer));
    
    if (m_layers.size() == 1) {
        m_activeLayerIndex = 0;
    }
    
    updateCanvas();
    emit layerChanged();
}

void ScribblerCanvas::RemoveLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size()) && m_layers.size() > 1) {
        m_layers.erase(m_layers.begin() + index);
        
        if (m_activeLayerIndex >= static_cast<int>(m_layers.size())) {
            m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }
        
        updateCanvas();
        emit layerChanged();
    }
}

void ScribblerCanvas::SetActiveLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        m_activeLayerIndex = index;
        emit layerChanged();
    }
}

RasterLayer* ScribblerCanvas::GetActiveLayer() {
    if (m_activeLayerIndex >= 0 && m_activeLayerIndex < static_cast<int>(m_layers.size())) {
        return m_layers[m_activeLayerIndex].get();
    }
    return nullptr;
}

RasterLayer* ScribblerCanvas::GetLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        return m_layers[index].get();
    }
    return nullptr;
}

void ScribblerCanvas::ZoomIn() {
    scale(1.25, 1.25);
}

void ScribblerCanvas::ZoomOut() {
    scale(0.8, 0.8);
}

void ScribblerCanvas::ZoomToFit() {
    fitInView(m_canvasItem, Qt::KeepAspectRatio);
}

void ScribblerCanvas::ZoomToActual() {
    resetTransform();
}

void ScribblerCanvas::NewCanvas(int width, int height) {
    m_layers.clear();
    m_canvasSize = QSize(width, height);
    m_activeLayerIndex = 0;

    AddLayer("Background");

    // Fill background layer with white
    RasterLayer* backgroundLayer = GetActiveLayer();
    if (backgroundLayer) {
        backgroundLayer->GetImage().fill(Qt::white);
    }

    updateCanvas();

    emit canvasModified();
}

void ScribblerCanvas::updateCanvas() {
    compositeImage();
    
    if (m_canvasItem) {
        QPixmap pixmap = QPixmap::fromImage(m_compositeImage);
        m_canvasItem->setPixmap(pixmap);
        
        // Update scene rect
        m_scene->setSceneRect(0, 0, m_canvasSize.width(), m_canvasSize.height());
    }
}

void ScribblerCanvas::compositeImage() {
    m_compositeImage = QImage(m_canvasSize, QImage::Format_ARGB32_Premultiplied);
    m_compositeImage.fill(Qt::transparent);
    
    QPainter painter(&m_compositeImage);
    painter.setRenderHint(QPainter::Antialiasing, m_antiAliasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // Composite layers from bottom to top
    for (const auto& layer : m_layers) {
        if (!layer->IsVisible()) continue;
        
        QImage layerImage = layer->GetImage();

        // Apply layer mask if present
        if (layer->HasMask()) {
            const QImage& mask = layer->GetMask();
            QPainter maskPainter(&layerImage);
            maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            maskPainter.drawImage(0, 0, mask);
            maskPainter.end();
        }

        // Apply opacity by setting painter opacity instead of modifying the image
        painter.setOpacity(layer->GetOpacity());
        
        // Apply blend mode
        switch (layer->GetBlendMode()) {
            case RasterBlendMode::Normal:
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                break;
            case RasterBlendMode::Multiply:
                painter.setCompositionMode(QPainter::CompositionMode_Multiply);
                break;
            case RasterBlendMode::Screen:
                painter.setCompositionMode(QPainter::CompositionMode_Screen);
                break;
            case RasterBlendMode::Overlay:
                painter.setCompositionMode(QPainter::CompositionMode_Overlay);
                break;
            case RasterBlendMode::SoftLight:
                painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
                break;
            case RasterBlendMode::HardLight:
                painter.setCompositionMode(QPainter::CompositionMode_HardLight);
                break;
            case RasterBlendMode::ColorDodge:
                painter.setCompositionMode(QPainter::CompositionMode_ColorDodge);
                break;
            case RasterBlendMode::ColorBurn:
                painter.setCompositionMode(QPainter::CompositionMode_ColorBurn);
                break;
        }
        
        painter.drawImage(0, 0, layerImage);
    }
    
    painter.end();
}

QPointF ScribblerCanvas::screenToCanvas(const QPoint& screenPos) const {
    return mapToScene(screenPos);
}

QPoint ScribblerCanvas::canvasToScreen(const QPointF& canvasPos) const {
    return mapFromScene(canvasPos);
}

void ScribblerCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF canvasPos = screenToCanvas(event->pos());
        
        if (canvasPos.x() >= 0 && canvasPos.x() < m_canvasSize.width() &&
            canvasPos.y() >= 0 && canvasPos.y() < m_canvasSize.height()) {
            
            switch (m_currentTool) {
                case ScribblerTool::Pen:
                case ScribblerTool::Brush:
                    m_drawing = true;
                    m_currentStroke.clear();
                    m_currentStroke.append(canvasPos);
                    drawStroke(canvasPos);
                    break;
                case ScribblerTool::Eraser:
                    m_drawing = true;
                    m_currentStroke.clear();
                    m_currentStroke.append(canvasPos);
                    drawStroke(canvasPos);
                    break;
                case ScribblerTool::Bucket:
                    floodFill(static_cast<int>(canvasPos.x()), static_cast<int>(canvasPos.y()), m_primaryColor);
                    break;
                case ScribblerTool::Eyedropper: {
                    QColor color = pickColor(static_cast<int>(canvasPos.x()), static_cast<int>(canvasPos.y()));
                    emit colorPicked(color);
                    break;
                }
                case ScribblerTool::Smudge:
                    m_drawing = true;
                    m_currentStroke.clear();
                    m_currentStroke.append(canvasPos);
                    drawSmudgeStroke(canvasPos);
                    break;
            }
            
            m_lastDrawPos = canvasPos;
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void ScribblerCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (m_drawing && (event->buttons() & Qt::LeftButton)) {
        QPointF canvasPos = screenToCanvas(event->pos());

        if (canvasPos.x() >= 0 && canvasPos.x() < m_canvasSize.width() &&
            canvasPos.y() >= 0 && canvasPos.y() < m_canvasSize.height()) {

            switch (m_currentTool) {
                case ScribblerTool::Pen:
                case ScribblerTool::Brush:
                case ScribblerTool::Eraser:
                    m_currentStroke.append(canvasPos);
                    drawStroke(canvasPos);
                    break;
                case ScribblerTool::Smudge:
                    m_currentStroke.append(canvasPos);
                    drawSmudgeStroke(canvasPos);
                    break;
                default:
                    break;
            }
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ScribblerCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drawing = false;
        m_currentStroke.clear();
        updateCanvas();
        emit canvasModified();
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ScribblerCanvas::tabletEvent(QTabletEvent* event) {
    if (m_pressureEnabled) {
        m_currentPressure = event->pressure();

        // Convert tablet event to mouse-like behavior
        QPointF canvasPos = screenToCanvas(event->position().toPoint());

        if (canvasPos.x() >= 0 && canvasPos.x() < m_canvasSize.width() &&
            canvasPos.y() >= 0 && canvasPos.y() < m_canvasSize.height()) {

            switch (event->type()) {
                case QEvent::TabletPress:
                    if (m_currentTool == ScribblerTool::Pen ||
                        m_currentTool == ScribblerTool::Brush ||
                        m_currentTool == ScribblerTool::Eraser ||
                        m_currentTool == ScribblerTool::Smudge) {
                        m_drawing = true;
                        m_currentStroke.clear();
                        m_currentStroke.append(canvasPos);
                        if (m_currentTool == ScribblerTool::Smudge) {
                            drawSmudgeStroke(canvasPos);
                        } else {
                            drawStroke(canvasPos, m_currentPressure);
                        }
                    } else if (m_currentTool == ScribblerTool::Bucket) {
                        floodFill(static_cast<int>(canvasPos.x()), static_cast<int>(canvasPos.y()), m_primaryColor);
                    } else if (m_currentTool == ScribblerTool::Eyedropper) {
                        QColor color = pickColor(static_cast<int>(canvasPos.x()), static_cast<int>(canvasPos.y()));
                        emit colorPicked(color);
                    }
                    break;

                case QEvent::TabletMove:
                    if (m_drawing) {
                        m_currentStroke.append(canvasPos);
                        if (m_currentTool == ScribblerTool::Smudge) {
                            drawSmudgeStroke(canvasPos);
                        } else {
                            drawStroke(canvasPos, m_currentPressure);
                        }
                    }
                    break;

                case QEvent::TabletRelease:
                    m_drawing = false;
                    m_currentStroke.clear();
                    updateCanvas();
                    emit canvasModified();
                    break;

                default:
                    break;
            }

            m_lastDrawPos = canvasPos;
        }

        event->accept();
    } else {
        QGraphicsView::tabletEvent(event);
    }
}

void ScribblerCanvas::wheelEvent(QWheelEvent* event) {
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

void ScribblerCanvas::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);
}

void ScribblerCanvas::onSceneChanged() {
    // Handle scene changes if needed
}

void ScribblerCanvas::drawStroke(const QPointF& pos, float pressure) {
    RasterLayer* layer = GetActiveLayer();
    if (!layer) return;

    QImage& image = layer->GetImage();

    QColor drawColor = m_primaryColor;
    float effectiveSize = m_brushSize * pressure;

    if (m_currentTool == ScribblerTool::Pen) {
        // Pen tool: hard edges, pixel-perfect
        drawPenStroke(image, pos, effectiveSize, drawColor);
    } else if (m_currentTool == ScribblerTool::Eraser) {
        // Eraser tool: remove pixels
        drawEraserStroke(image, pos, effectiveSize);
    } else {
        // Brush tool: smooth, anti-aliased strokes
        drawBrushStroke(image, pos, effectiveSize, drawColor, pressure);
    }

    // Update canvas with a slight delay for smooth performance
    if (!m_strokeTimer->isActive()) {
        m_strokeTimer->start();
    }
}

QBrush ScribblerCanvas::createBrush(float size, float hardness, const QColor& color) {
    int brushSize = qMax(1, static_cast<int>(size));
    QImage brushImage(brushSize, brushSize, QImage::Format_ARGB32_Premultiplied);
    brushImage.fill(Qt::transparent);

    QPainter brushPainter(&brushImage);
    brushPainter.setRenderHint(QPainter::Antialiasing, m_antiAliasing);

    QPointF center(brushSize / 2.0f, brushSize / 2.0f);
    float radius = brushSize / 2.0f;

    switch (m_brushType) {
        case BrushType::Round: {
            if (hardness >= 1.0f) {
                // Hard brush
                brushPainter.setBrush(QBrush(color));
                brushPainter.setPen(Qt::NoPen);
                brushPainter.drawEllipse(center, radius, radius);
            } else {
                // Soft brush with gradient
                QRadialGradient gradient(center, radius);
                QColor centerColor = color;
                QColor edgeColor = color;
                edgeColor.setAlphaF(0);

                float hardnessPoint = hardness * 0.8f; // Adjust hardness curve
                gradient.setColorAt(0, centerColor);
                gradient.setColorAt(hardnessPoint, centerColor);
                gradient.setColorAt(1, edgeColor);

                brushPainter.setBrush(QBrush(gradient));
                brushPainter.setPen(Qt::NoPen);
                brushPainter.drawEllipse(center, radius, radius);
            }
            break;
        }
        case BrushType::Square: {
            brushPainter.setBrush(QBrush(color));
            brushPainter.setPen(Qt::NoPen);
            brushPainter.drawRect(0, 0, brushSize, brushSize);
            break;
        }
        case BrushType::Soft: {
            QRadialGradient gradient(center, radius);
            QColor centerColor = color;
            QColor edgeColor = color;
            edgeColor.setAlphaF(0);

            gradient.setColorAt(0, centerColor);
            gradient.setColorAt(0.5f, centerColor);
            gradient.setColorAt(1, edgeColor);

            brushPainter.setBrush(QBrush(gradient));
            brushPainter.setPen(Qt::NoPen);
            brushPainter.drawEllipse(center, radius, radius);
            break;
        }
        case BrushType::Texture: {
            // Simple texture brush - could be enhanced with actual texture loading
            brushPainter.setBrush(QBrush(color));
            brushPainter.setPen(Qt::NoPen);

            // Create a simple noise pattern
            for (int x = 0; x < brushSize; x += 2) {
                for (int y = 0; y < brushSize; y += 2) {
                    if ((x + y) % 4 == 0) {
                        brushPainter.drawRect(x, y, 2, 2);
                    }
                }
            }
            break;
        }
    }

    brushPainter.end();
    return QBrush(brushImage);
}

void ScribblerCanvas::drawBrushStroke(QImage& image, const QPointF& pos, float size, const QColor& color, float pressure) {
    // Create a high-quality brush stamp
    QImage brushStamp = createBrushStamp(size, m_brushHardness, color, m_brushOpacity * pressure);

    if (m_currentStroke.size() > 1) {
        // Draw smooth interpolated line between last position and current
        QPointF lastPos = m_currentStroke[m_currentStroke.size() - 2];
        drawInterpolatedLine(image, lastPos, pos, brushStamp);
    } else {
        // First point of stroke
        applyBrushStamp(image, pos, brushStamp);
    }
}

void ScribblerCanvas::drawPenStroke(QImage& image, const QPointF& pos, float size, const QColor& color) {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.setBrush(QBrush(color));
    painter.setPen(Qt::NoPen);

    if (m_currentStroke.size() > 1) {
        QPointF lastPos = m_currentStroke[m_currentStroke.size() - 2];

        // Draw line with square caps for pixel-perfect pen
        QPen pen(color, size, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        painter.setPen(pen);
        painter.drawLine(lastPos, pos);
    } else {
        // Draw initial point
        painter.drawEllipse(pos, size / 2, size / 2);
    }

    painter.end();
}

void ScribblerCanvas::drawEraserStroke(QImage& image, const QPointF& pos, float size) {
    QImage eraserStamp;

    switch (m_eraserMode) {
        case EraserMode::Normal:
            // Standard eraser - removes pixels completely
            eraserStamp = createBrushStamp(size, m_brushHardness, Qt::white, 1.0f);
            break;
        case EraserMode::Background:
            // Erase to background color (secondary color)
            eraserStamp = createBrushStamp(size, m_brushHardness, m_secondaryColor, m_brushOpacity);
            break;
        case EraserMode::Soft:
            // Soft eraser with opacity
            eraserStamp = createBrushStamp(size, m_brushHardness * 0.5f, Qt::white, m_brushOpacity);
            break;
    }

    bool isNormalEraser = (m_eraserMode == EraserMode::Normal || m_eraserMode == EraserMode::Soft);

    if (m_currentStroke.size() > 1) {
        QPointF lastPos = m_currentStroke[m_currentStroke.size() - 2];
        drawInterpolatedLine(image, lastPos, pos, eraserStamp, isNormalEraser);
    } else {
        // Erase initial point
        applyBrushStamp(image, pos, eraserStamp, isNormalEraser);
    }
}

void ScribblerCanvas::drawSmudgeStroke(const QPointF& pos) {
    RasterLayer* layer = GetActiveLayer();
    if (!layer) return;

    QImage& image = layer->GetImage();

    if (m_currentStroke.size() > 1) {
        QPointF lastPos = m_currentStroke[m_currentStroke.size() - 2];

        // Sample colors along the path and blend them
        float distance = sqrt(pow(pos.x() - lastPos.x(), 2) + pow(pos.y() - lastPos.y(), 2));
        int steps = qMax(1, static_cast<int>(distance));

        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / steps;
            QPointF interpPos = lastPos + t * (pos - lastPos);

            // Apply smudge effect at this position
            applySmudgeEffect(image, interpPos, m_brushSize, m_brushOpacity);
        }
    } else {
        // First point - just sample the area
        applySmudgeEffect(image, pos, m_brushSize, m_brushOpacity);
    }

    // Update canvas with a slight delay for smooth performance
    if (!m_strokeTimer->isActive()) {
        m_strokeTimer->start();
    }
}

void ScribblerCanvas::applySmudgeEffect(QImage& image, const QPointF& pos, float size, float strength) {
    int radius = static_cast<int>(size / 2);
    int centerX = static_cast<int>(pos.x());
    int centerY = static_cast<int>(pos.y());

    // Sample colors in the brush area
    std::vector<QColor> sampledColors;
    std::vector<QPoint> positions;

    for (int y = centerY - radius; y <= centerY + radius; ++y) {
        for (int x = centerX - radius; x <= centerX + radius; ++x) {
            if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
                float dx = x - centerX;
                float dy = y - centerY;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance <= radius) {
                    sampledColors.push_back(image.pixelColor(x, y));
                    positions.push_back(QPoint(x, y));
                }
            }
        }
    }

    if (sampledColors.empty()) return;

    // Apply smudge effect by blending neighboring colors
    for (size_t i = 0; i < positions.size(); ++i) {
        QPoint& p = positions[i];
        QColor originalColor = sampledColors[i];

        // Calculate average color of nearby pixels
        QColor blendColor = calculateAverageColor(sampledColors, positions, p, radius * 0.5f);

        // Blend original color with average color based on strength
        QColor finalColor = blendColors(originalColor, blendColor, strength);

        image.setPixelColor(p.x(), p.y(), finalColor);
    }
}

QColor ScribblerCanvas::calculateAverageColor(const std::vector<QColor>& colors, const std::vector<QPoint>& positions, const QPoint& center, float radius) {
    float totalR = 0, totalG = 0, totalB = 0, totalA = 0;
    int count = 0;

    for (size_t i = 0; i < colors.size(); ++i) {
        float dx = positions[i].x() - center.x();
        float dy = positions[i].y() - center.y();
        float distance = sqrt(dx * dx + dy * dy);

        if (distance <= radius) {
            totalR += colors[i].redF();
            totalG += colors[i].greenF();
            totalB += colors[i].blueF();
            totalA += colors[i].alphaF();
            count++;
        }
    }

    if (count == 0) return QColor();

    return QColor::fromRgbF(totalR / count, totalG / count, totalB / count, totalA / count);
}

QColor ScribblerCanvas::blendColors(const QColor& color1, const QColor& color2, float factor) {
    float r = color1.redF() * (1.0f - factor) + color2.redF() * factor;
    float g = color1.greenF() * (1.0f - factor) + color2.greenF() * factor;
    float b = color1.blueF() * (1.0f - factor) + color2.blueF() * factor;
    float a = color1.alphaF() * (1.0f - factor) + color2.alphaF() * factor;

    return QColor::fromRgbF(r, g, b, a);
}

void ScribblerCanvas::drawInterpolatedLine(QImage& image, const QPointF& start, const QPointF& end, const QImage& brushStamp, bool isEraser) {
    // Calculate distance and number of steps based on brush spacing
    float distance = sqrt(pow(end.x() - start.x(), 2) + pow(end.y() - start.y(), 2));
    float spacingDistance = brushStamp.width() * m_brushSpacing;
    int steps = qMax(1, static_cast<int>(distance / spacingDistance));

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        QPointF interpPos = start + t * (end - start);
        applyBrushStamp(image, interpPos, brushStamp, isEraser);
    }
}

void ScribblerCanvas::applyBrushStamp(QImage& image, const QPointF& pos, const QImage& brushStamp, bool isEraser) {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Check if we're drawing on a layer with a mask
    RasterLayer* activeLayer = GetActiveLayer();
    if (activeLayer && activeLayer->HasMask() && !isEraser) {
        // Create a temporary image for the brush stamp
        QImage tempImage(image.size(), QImage::Format_ARGB32_Premultiplied);
        tempImage.fill(Qt::transparent);

        QPainter tempPainter(&tempImage);
        tempPainter.setRenderHint(QPainter::Antialiasing, true);
        tempPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        tempPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        // Draw the brush stamp to the temporary image
        QPointF stampPos = pos - QPointF(brushStamp.width() / 2.0f, brushStamp.height() / 2.0f);
        tempPainter.drawImage(stampPos, brushStamp);
        tempPainter.end();

        // Apply the layer mask to the temporary image
        QPainter maskPainter(&tempImage);
        maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        maskPainter.drawImage(0, 0, activeLayer->GetMask());
        maskPainter.end();

        // Draw the masked result to the actual image
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, tempImage);
    } else {
        // Normal drawing without mask
        if (isEraser) {
            painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        } else {
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        }

        // Center the brush stamp on the position
        QPointF stampPos = pos - QPointF(brushStamp.width() / 2.0f, brushStamp.height() / 2.0f);
        painter.drawImage(stampPos, brushStamp);
    }

    painter.end();
}

QImage ScribblerCanvas::createBrushStamp(float size, float hardness, const QColor& color, float opacity) {
    int stampSize = qMax(1, static_cast<int>(size * 2)); // Make stamp larger for better quality
    QImage stamp(stampSize, stampSize, QImage::Format_ARGB32_Premultiplied);
    stamp.fill(Qt::transparent);

    QPainter painter(&stamp);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPointF center(stampSize / 2.0f, stampSize / 2.0f);
    float radius = size / 2.0f;

    switch (m_brushType) {
        case BrushType::Round: {
            if (hardness >= 0.99f) {
                // Hard brush - solid circle
                QColor brushColor = color;
                brushColor.setAlphaF(opacity);
                painter.setBrush(QBrush(brushColor));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(center, radius, radius);
            } else {
                // Soft brush with smooth falloff
                QRadialGradient gradient(center, radius);

                QColor centerColor = color;
                centerColor.setAlphaF(opacity);

                QColor edgeColor = color;
                edgeColor.setAlphaF(0);

                // Create smooth falloff based on hardness
                float hardnessPoint = hardness * 0.7f;
                gradient.setColorAt(0, centerColor);
                gradient.setColorAt(hardnessPoint, centerColor);
                gradient.setColorAt(1, edgeColor);

                painter.setBrush(QBrush(gradient));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(center, radius, radius);
            }
            break;
        }
        case BrushType::Square: {
            QColor brushColor = color;
            brushColor.setAlphaF(opacity);
            painter.setBrush(QBrush(brushColor));
            painter.setPen(Qt::NoPen);
            painter.drawRect(center.x() - radius, center.y() - radius, size, size);
            break;
        }
        case BrushType::Soft: {
            // Extra soft brush with extended falloff
            QRadialGradient gradient(center, radius * 1.2f);

            QColor centerColor = color;
            centerColor.setAlphaF(opacity * 0.8f);

            QColor midColor = color;
            midColor.setAlphaF(opacity * 0.4f);

            QColor edgeColor = color;
            edgeColor.setAlphaF(0);

            gradient.setColorAt(0, centerColor);
            gradient.setColorAt(0.3f, centerColor);
            gradient.setColorAt(0.7f, midColor);
            gradient.setColorAt(1, edgeColor);

            painter.setBrush(QBrush(gradient));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(center, radius * 1.2f, radius * 1.2f);
            break;
        }
        case BrushType::Texture: {
            // Enhanced texture brush with noise pattern
            QColor brushColor = color;
            brushColor.setAlphaF(opacity);

            // Create noise pattern
            for (int x = 0; x < stampSize; ++x) {
                for (int y = 0; y < stampSize; ++y) {
                    float dx = x - center.x();
                    float dy = y - center.y();
                    float distance = sqrt(dx * dx + dy * dy);

                    if (distance <= radius) {
                        // Simple noise based on position
                        float noise = (sin(x * 0.3f) * cos(y * 0.3f) + 1.0f) * 0.5f;
                        float alpha = opacity * (1.0f - distance / radius) * noise;

                        QColor pixelColor = color;
                        pixelColor.setAlphaF(alpha);
                        painter.setPen(pixelColor);
                        painter.drawPoint(x, y);
                    }
                }
            }
            break;
        }
    }

    painter.end();
    return stamp;
}

// File operations for ScribblerCanvas
bool ScribblerCanvas::LoadFromFile(const QString& filepath) {
    QImage image(filepath);
    if (image.isNull()) return false;

    NewCanvas(image.width(), image.height());
    RasterLayer* layer = GetActiveLayer();
    if (layer) {
        layer->GetImage() = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        updateCanvas();
        emit canvasModified();
    }

    return true;
}

bool ScribblerCanvas::SaveToFile(const QString& filepath) {
    return m_compositeImage.save(filepath);
}

bool ScribblerCanvas::ExportToImage(const QString& filepath) {
    return m_compositeImage.save(filepath);
}

// ScribblerDialog Implementation
ScribblerDialog::ScribblerDialog(QWidget* parent)
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
    , m_currentTool(ScribblerTool::Brush)
{
    setWindowTitle("Scribbler");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUI();
    updateWindowTitle();
    updateLayerList();
    updateToolStates();
}

ScribblerDialog::~ScribblerDialog() {
}

void ScribblerDialog::setupUI() {
    setupMenuBar();
    setupToolBar();
    setupMainPanels();

    // Connect canvas signals
    if (m_canvas) {
        connect(m_canvas, &ScribblerCanvas::canvasModified, this, &ScribblerDialog::onCanvasModified);
        connect(m_canvas, &ScribblerCanvas::layerChanged, this, &ScribblerDialog::onLayerChanged);
        connect(m_canvas, &ScribblerCanvas::colorPicked, this, &ScribblerDialog::onColorPicked);
    }
}

void ScribblerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New Canvas...", this, &ScribblerDialog::onNewCanvas, QKeySequence::New);
    fileMenu->addAction("&Open...", this, &ScribblerDialog::onOpenFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", this, &ScribblerDialog::onSaveFile, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &ScribblerDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Export Image...", this, &ScribblerDialog::onExportImage);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo", [](){ /* TODO */ }, QKeySequence::Undo);
    editMenu->addAction("&Redo", [](){ /* TODO */ }, QKeySequence::Redo);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    QAction* antiAliasingAction = viewMenu->addAction("&Anti-aliasing");
    antiAliasingAction->setCheckable(true);
    antiAliasingAction->setChecked(true);
    connect(antiAliasingAction, &QAction::toggled, [this](bool checked) {
        if (m_canvas) m_canvas->SetAntiAliasing(checked);
        if (m_antiAliasingCheck) m_antiAliasingCheck->setChecked(checked);
    });

    viewMenu->addSeparator();
    viewMenu->addAction("Zoom &In", [this](){ if (m_canvas) m_canvas->ZoomIn(); }, QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out", [this](){ if (m_canvas) m_canvas->ZoomOut(); }, QKeySequence::ZoomOut);
    viewMenu->addAction("Zoom to &Fit", [this](){ if (m_canvas) m_canvas->ZoomToFit(); });
    viewMenu->addAction("&Actual Size", [this](){ if (m_canvas) m_canvas->ZoomToActual(); });
}

void ScribblerDialog::setupToolBar() {
    m_toolBar = new QToolBar("Tools", this);
    m_toolBar->setMaximumHeight(50);

    // Tool group for exclusive selection
    m_toolGroup = new QActionGroup(this);

    // Drawing tools
    m_penAction = m_toolBar->addAction("Pen");
    m_penAction->setCheckable(true);
    m_penAction->setToolTip("Pen Tool (P)");
    m_penAction->setShortcut(QKeySequence("P"));
    m_toolGroup->addAction(m_penAction);

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

    m_smudgeAction = m_toolBar->addAction("Smudge");
    m_smudgeAction->setCheckable(true);
    m_smudgeAction->setToolTip("Smudge Tool (S)");
    m_smudgeAction->setShortcut(QKeySequence("S"));
    m_toolGroup->addAction(m_smudgeAction);

    // Connect tool signals
    connect(m_toolGroup, &QActionGroup::triggered, this, &ScribblerDialog::onToolChanged);
}

void ScribblerDialog::setupMainPanels() {
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
    m_mainSplitter->setSizes({250, 600, 200, 200});
    m_mainSplitter->setStretchFactor(0, 0);  // Tool panel fixed
    m_mainSplitter->setStretchFactor(1, 1);  // Canvas stretches
    m_mainSplitter->setStretchFactor(2, 0);  // Layer panel fixed
    m_mainSplitter->setStretchFactor(3, 0);  // Color panel fixed
}

void ScribblerDialog::setupToolPanel() {
    m_toolPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_toolPanel);

    // Brush settings group
    QGroupBox* brushGroup = new QGroupBox("Brush Settings");
    QVBoxLayout* brushLayout = new QVBoxLayout(brushGroup);

    // Brush size
    QLabel* sizeLabel = new QLabel("Size:");
    brushLayout->addWidget(sizeLabel);

    QHBoxLayout* sizeLayout = new QHBoxLayout();
    m_brushSizeSlider = new QSlider(Qt::Horizontal);
    m_brushSizeSlider->setRange(1, 100);
    m_brushSizeSlider->setValue(10);
    sizeLayout->addWidget(m_brushSizeSlider);

    m_brushSizeSpinBox = new QDoubleSpinBox();
    m_brushSizeSpinBox->setRange(0.1, 100.0);
    m_brushSizeSpinBox->setValue(10.0);
    m_brushSizeSpinBox->setDecimals(1);
    m_brushSizeSpinBox->setMaximumWidth(80);
    sizeLayout->addWidget(m_brushSizeSpinBox);

    brushLayout->addLayout(sizeLayout);

    // Brush hardness
    QLabel* hardnessLabel = new QLabel("Hardness:");
    brushLayout->addWidget(hardnessLabel);

    QHBoxLayout* hardnessLayout = new QHBoxLayout();
    m_brushHardnessSlider = new QSlider(Qt::Horizontal);
    m_brushHardnessSlider->setRange(0, 100);
    m_brushHardnessSlider->setValue(80);
    hardnessLayout->addWidget(m_brushHardnessSlider);

    m_brushHardnessSpinBox = new QDoubleSpinBox();
    m_brushHardnessSpinBox->setRange(0.0, 1.0);
    m_brushHardnessSpinBox->setValue(0.8);
    m_brushHardnessSpinBox->setDecimals(2);
    m_brushHardnessSpinBox->setMaximumWidth(80);
    hardnessLayout->addWidget(m_brushHardnessSpinBox);

    brushLayout->addLayout(hardnessLayout);

    // Brush opacity
    QLabel* opacityLabel = new QLabel("Opacity:");
    brushLayout->addWidget(opacityLabel);

    QHBoxLayout* opacityLayout = new QHBoxLayout();
    m_brushOpacitySlider = new QSlider(Qt::Horizontal);
    m_brushOpacitySlider->setRange(0, 100);
    m_brushOpacitySlider->setValue(100);
    opacityLayout->addWidget(m_brushOpacitySlider);

    m_brushOpacitySpinBox = new QDoubleSpinBox();
    m_brushOpacitySpinBox->setRange(0.0, 1.0);
    m_brushOpacitySpinBox->setValue(1.0);
    m_brushOpacitySpinBox->setDecimals(2);
    m_brushOpacitySpinBox->setMaximumWidth(80);
    opacityLayout->addWidget(m_brushOpacitySpinBox);

    brushLayout->addLayout(opacityLayout);

    // Brush type
    QLabel* typeLabel = new QLabel("Type:");
    brushLayout->addWidget(typeLabel);

    m_brushTypeCombo = new QComboBox();
    m_brushTypeCombo->addItems({"Round", "Square", "Soft", "Texture"});
    brushLayout->addWidget(m_brushTypeCombo);

    // Brush spacing
    QLabel* spacingLabel = new QLabel("Spacing:");
    brushLayout->addWidget(spacingLabel);

    QHBoxLayout* spacingLayout = new QHBoxLayout();
    m_brushSpacingSlider = new QSlider(Qt::Horizontal);
    m_brushSpacingSlider->setRange(1, 100);
    m_brushSpacingSlider->setValue(25); // Default 0.25 * 100
    spacingLayout->addWidget(m_brushSpacingSlider);

    m_brushSpacingSpinBox = new QDoubleSpinBox();
    m_brushSpacingSpinBox->setRange(0.01, 1.0);
    m_brushSpacingSpinBox->setValue(0.25);
    m_brushSpacingSpinBox->setDecimals(2);
    m_brushSpacingSpinBox->setMaximumWidth(80);
    spacingLayout->addWidget(m_brushSpacingSpinBox);

    brushLayout->addLayout(spacingLayout);

    // Anti-aliasing
    m_antiAliasingCheck = new QCheckBox("Anti-aliasing");
    m_antiAliasingCheck->setChecked(true);
    brushLayout->addWidget(m_antiAliasingCheck);

    layout->addWidget(brushGroup);
    layout->addStretch();

    // Connect brush setting signals
    connect(m_brushSizeSlider, &QSlider::valueChanged, this, &ScribblerDialog::onBrushSizeChanged);
    connect(m_brushSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScribblerDialog::onBrushSizeChanged);
    connect(m_brushHardnessSlider, &QSlider::valueChanged, this, &ScribblerDialog::onBrushHardnessChanged);
    connect(m_brushHardnessSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScribblerDialog::onBrushHardnessChanged);
    connect(m_brushOpacitySlider, &QSlider::valueChanged, this, &ScribblerDialog::onBrushOpacityChanged);
    connect(m_brushOpacitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScribblerDialog::onBrushOpacityChanged);
    connect(m_brushSpacingSlider, &QSlider::valueChanged, this, &ScribblerDialog::onBrushSpacingChanged);
    connect(m_brushSpacingSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ScribblerDialog::onBrushSpacingChanged);
    connect(m_brushTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScribblerDialog::onBrushTypeChanged);
    connect(m_antiAliasingCheck, &QCheckBox::toggled, this, &ScribblerDialog::onAntiAliasingChanged);

    m_mainSplitter->addWidget(m_toolPanel);
}

void ScribblerDialog::setupCanvasPanel() {
    QWidget* canvasWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(canvasWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_canvas = new ScribblerCanvas();
    layout->addWidget(m_canvas);

    m_mainSplitter->addWidget(canvasWidget);
}

void ScribblerDialog::setupLayerPanel() {
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
    m_layerBlendModeCombo->addItems({"Normal", "Multiply", "Screen", "Overlay", "Soft Light", "Hard Light", "Color Dodge", "Color Burn"});
    layerLayout->addWidget(m_layerBlendModeCombo);

    m_layerAlphaLockCheck = new QCheckBox("Alpha Lock");
    layerLayout->addWidget(m_layerAlphaLockCheck);

    // Layer mask controls
    QLabel* maskLabel = new QLabel("Layer Mask:");
    layerLayout->addWidget(maskLabel);

    QHBoxLayout* maskButtonLayout = new QHBoxLayout();
    m_addMaskButton = new QPushButton("Add");
    m_deleteMaskButton = new QPushButton("Delete");
    m_applyMaskButton = new QPushButton("Apply");

    m_addMaskButton->setToolTip("Add layer mask");
    m_deleteMaskButton->setToolTip("Delete layer mask");
    m_applyMaskButton->setToolTip("Apply mask to layer");

    maskButtonLayout->addWidget(m_addMaskButton);
    maskButtonLayout->addWidget(m_deleteMaskButton);
    maskButtonLayout->addWidget(m_applyMaskButton);
    layerLayout->addLayout(maskButtonLayout);

    layout->addWidget(layerGroup);
    layout->addStretch();

    // Connect layer signals
    connect(m_addLayerButton, &QPushButton::clicked, this, &ScribblerDialog::onAddLayer);
    connect(m_removeLayerButton, &QPushButton::clicked, this, &ScribblerDialog::onRemoveLayer);
    connect(m_duplicateLayerButton, &QPushButton::clicked, this, &ScribblerDialog::onDuplicateLayer);
    connect(m_moveLayerUpButton, &QPushButton::clicked, this, &ScribblerDialog::onMoveLayerUp);
    connect(m_moveLayerDownButton, &QPushButton::clicked, this, &ScribblerDialog::onMoveLayerDown);
    connect(m_layerList, &QListWidget::currentRowChanged, this, &ScribblerDialog::onLayerSelectionChanged);
    connect(m_layerOpacitySlider, &QSlider::valueChanged, this, &ScribblerDialog::onLayerOpacityChanged);
    connect(m_layerBlendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScribblerDialog::onLayerBlendModeChanged);
    connect(m_layerAlphaLockCheck, &QCheckBox::toggled, this, &ScribblerDialog::onLayerAlphaLockChanged);
    connect(m_addMaskButton, &QPushButton::clicked, this, &ScribblerDialog::onAddLayerMask);
    connect(m_deleteMaskButton, &QPushButton::clicked, this, &ScribblerDialog::onDeleteLayerMask);
    connect(m_applyMaskButton, &QPushButton::clicked, this, &ScribblerDialog::onApplyLayerMask);

    m_mainSplitter->addWidget(m_layerPanel);
}

void ScribblerDialog::setupColorPanel() {
    m_colorPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_colorPanel);

    // Color picker group
    QGroupBox* colorGroup = new QGroupBox("Colors");
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

    // Primary/Secondary color buttons
    QHBoxLayout* colorButtonLayout = new QHBoxLayout();
    m_primaryColorButton = new QPushButton();
    m_primaryColorButton->setFixedSize(60, 60);
    m_primaryColorButton->setStyleSheet("background-color: black; border: 2px solid gray;");

    m_secondaryColorButton = new QPushButton();
    m_secondaryColorButton->setFixedSize(60, 60);
    m_secondaryColorButton->setStyleSheet("background-color: white; border: 2px solid gray;");

    QVBoxLayout* primaryLayout = new QVBoxLayout();
    primaryLayout->addWidget(new QLabel("Primary"));
    primaryLayout->addWidget(m_primaryColorButton);

    QVBoxLayout* secondaryLayout = new QVBoxLayout();
    secondaryLayout->addWidget(new QLabel("Secondary"));
    secondaryLayout->addWidget(m_secondaryColorButton);

    colorButtonLayout->addLayout(primaryLayout);
    colorButtonLayout->addLayout(secondaryLayout);
    colorButtonLayout->addStretch();

    colorLayout->addLayout(colorButtonLayout);

    // Add color wheel
    m_colorWheel = new ColorWheelWidget();
    colorLayout->addWidget(m_colorWheel);

    // Add color palette
    m_colorPalette = new ColorPaletteWidget();
    colorLayout->addWidget(m_colorPalette);

    layout->addWidget(colorGroup);
    layout->addStretch();

    // Connect color signals
    connect(m_primaryColorButton, &QPushButton::clicked, this, &ScribblerDialog::onPrimaryColorChanged);
    connect(m_secondaryColorButton, &QPushButton::clicked, this, &ScribblerDialog::onSecondaryColorChanged);
    connect(m_colorWheel, &ColorWheelWidget::colorChanged, this, &ScribblerDialog::onColorWheelChanged);
    connect(m_colorPalette, &ColorPaletteWidget::colorSelected, this, &ScribblerDialog::onPaletteColorSelected);

    m_mainSplitter->addWidget(m_colorPanel);
}

// Slot implementations
void ScribblerDialog::onNewCanvas() {
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
    widthSpinBox->setValue(800);
    widthLayout->addWidget(widthSpinBox);
    layout->addLayout(widthLayout);

    // Height input
    QHBoxLayout* heightLayout = new QHBoxLayout();
    heightLayout->addWidget(new QLabel("Height:"));
    QSpinBox* heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(1, 4096);
    heightSpinBox->setValue(600);
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

void ScribblerDialog::onOpenFile() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QDir::currentPath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tiff);;All Files (*)"
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

void ScribblerDialog::onSaveFile() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
    } else if (m_canvas) {
        if (m_canvas->SaveToFile(m_currentFilePath)) {
            setModified(false);
            updateWindowTitle();
            emit imageSaved(m_currentFilePath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void ScribblerDialog::onSaveAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Image",
        QDir::currentPath(),
        "PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;BMP Files (*.bmp);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (m_canvas->SaveToFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
            emit imageSaved(filepath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void ScribblerDialog::onExportImage() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Export Image",
        QDir::currentPath(),
        "PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;BMP Files (*.bmp);;TIFF Files (*.tiff);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (!m_canvas->ExportToImage(filepath)) {
            QMessageBox::warning(this, "Error", "Failed to export image.");
        }
    }
}

void ScribblerDialog::onToolChanged(QAction* action) {
    if (!action || !m_canvas) return;

    if (action == m_penAction) {
        m_currentTool = ScribblerTool::Pen;
        m_canvas->SetCurrentTool(ScribblerTool::Pen);
    } else if (action == m_brushAction) {
        m_currentTool = ScribblerTool::Brush;
        m_canvas->SetCurrentTool(ScribblerTool::Brush);
    } else if (action == m_eraserAction) {
        m_currentTool = ScribblerTool::Eraser;
        m_canvas->SetCurrentTool(ScribblerTool::Eraser);
    } else if (action == m_bucketAction) {
        m_currentTool = ScribblerTool::Bucket;
        m_canvas->SetCurrentTool(ScribblerTool::Bucket);
    } else if (action == m_eyedropperAction) {
        m_currentTool = ScribblerTool::Eyedropper;
        m_canvas->SetCurrentTool(ScribblerTool::Eyedropper);
    } else if (action == m_smudgeAction) {
        m_currentTool = ScribblerTool::Smudge;
        m_canvas->SetCurrentTool(ScribblerTool::Smudge);
    }

    updateToolStates();
}

void ScribblerDialog::onBrushSizeChanged() {
    float size = 10.0f;

    QObject* sender = this->sender();
    if (sender == m_brushSizeSlider) {
        size = static_cast<float>(m_brushSizeSlider->value());
        m_brushSizeSpinBox->blockSignals(true);
        m_brushSizeSpinBox->setValue(size);
        m_brushSizeSpinBox->blockSignals(false);
    } else if (sender == m_brushSizeSpinBox) {
        size = static_cast<float>(m_brushSizeSpinBox->value());
        m_brushSizeSlider->blockSignals(true);
        m_brushSizeSlider->setValue(static_cast<int>(size));
        m_brushSizeSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetBrushSize(size);
    }
}

void ScribblerDialog::onBrushHardnessChanged() {
    float hardness = 0.8f;

    QObject* sender = this->sender();
    if (sender == m_brushHardnessSlider) {
        hardness = m_brushHardnessSlider->value() / 100.0f;
        m_brushHardnessSpinBox->blockSignals(true);
        m_brushHardnessSpinBox->setValue(hardness);
        m_brushHardnessSpinBox->blockSignals(false);
    } else if (sender == m_brushHardnessSpinBox) {
        hardness = static_cast<float>(m_brushHardnessSpinBox->value());
        m_brushHardnessSlider->blockSignals(true);
        m_brushHardnessSlider->setValue(static_cast<int>(hardness * 100));
        m_brushHardnessSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetBrushHardness(hardness);
    }
}

void ScribblerDialog::onBrushOpacityChanged() {
    float opacity = 1.0f;

    QObject* sender = this->sender();
    if (sender == m_brushOpacitySlider) {
        opacity = m_brushOpacitySlider->value() / 100.0f;
        m_brushOpacitySpinBox->blockSignals(true);
        m_brushOpacitySpinBox->setValue(opacity);
        m_brushOpacitySpinBox->blockSignals(false);
    } else if (sender == m_brushOpacitySpinBox) {
        opacity = static_cast<float>(m_brushOpacitySpinBox->value());
        m_brushOpacitySlider->blockSignals(true);
        m_brushOpacitySlider->setValue(static_cast<int>(opacity * 100));
        m_brushOpacitySlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetBrushOpacity(opacity);
    }
}

void ScribblerDialog::onBrushSpacingChanged() {
    float spacing = 0.25f;

    QObject* sender = this->sender();
    if (sender == m_brushSpacingSlider) {
        spacing = m_brushSpacingSlider->value() / 100.0f;
        m_brushSpacingSpinBox->blockSignals(true);
        m_brushSpacingSpinBox->setValue(spacing);
        m_brushSpacingSpinBox->blockSignals(false);
    } else if (sender == m_brushSpacingSpinBox) {
        spacing = m_brushSpacingSpinBox->value();
        m_brushSpacingSlider->blockSignals(true);
        m_brushSpacingSlider->setValue(static_cast<int>(spacing * 100));
        m_brushSpacingSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetBrushSpacing(spacing);
    }
}

void ScribblerDialog::onBrushTypeChanged() {
    if (m_canvas) {
        BrushType type = static_cast<BrushType>(m_brushTypeCombo->currentIndex());
        m_canvas->SetBrushType(type);
    }
}

void ScribblerDialog::onAntiAliasingChanged() {
    if (m_canvas) {
        m_canvas->SetAntiAliasing(m_antiAliasingCheck->isChecked());
    }
}

void ScribblerDialog::onPrimaryColorChanged() {
    QColor color = QColorDialog::getColor(m_canvas ? m_canvas->GetPrimaryColor() : Qt::black, this);
    if (color.isValid() && m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

void ScribblerDialog::onSecondaryColorChanged() {
    QColor color = QColorDialog::getColor(m_canvas ? m_canvas->GetSecondaryColor() : Qt::white, this);
    if (color.isValid() && m_canvas) {
        m_canvas->SetSecondaryColor(color);
        m_secondaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

void ScribblerDialog::onColorWheelChanged(const QColor& color) {
    if (m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

void ScribblerDialog::onPaletteColorSelected(const QColor& color) {
    if (m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
        // Also update the color wheel to reflect the selected color
        m_colorWheel->SetSelectedColor(color);
    }
}

// Layer management slots
void ScribblerDialog::onAddLayer() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Layer", "Layer name:", QLineEdit::Normal, "New Layer", &ok);
    if (ok && !name.isEmpty() && m_canvas) {
        m_canvas->AddLayer(name);
        updateLayerList();
        setModified(true);
    }
}

void ScribblerDialog::onRemoveLayer() {
    if (!m_canvas || m_canvas->GetLayerCount() <= 1) return;

    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0) {
        m_canvas->RemoveLayer(currentRow);
        updateLayerList();
        setModified(true);
    }
}

void ScribblerDialog::onDuplicateLayer() {
    // TODO: Implement layer duplication
}

void ScribblerDialog::onMoveLayerUp() {
    // TODO: Implement layer reordering
}

void ScribblerDialog::onMoveLayerDown() {
    // TODO: Implement layer reordering
}

void ScribblerDialog::onLayerSelectionChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        m_canvas->SetActiveLayer(currentRow);

        RasterLayer* layer = m_canvas->GetLayer(currentRow);
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
        }
    }
}

void ScribblerDialog::onLayerVisibilityChanged() {
    // TODO: Implement layer visibility toggle
}

void ScribblerDialog::onLayerOpacityChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            float opacity = m_layerOpacitySlider->value() / 100.0f;
            layer->SetOpacity(opacity);
            setModified(true);
        }
    }
}

void ScribblerDialog::onLayerBlendModeChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            RasterBlendMode mode = static_cast<RasterBlendMode>(m_layerBlendModeCombo->currentIndex());
            layer->SetBlendMode(mode);
            setModified(true);
        }
    }
}

void ScribblerDialog::onLayerAlphaLockChanged() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer) {
            layer->SetAlphaLocked(m_layerAlphaLockCheck->isChecked());
            setModified(true);
        }
    }
}

void ScribblerDialog::onAddLayerMask() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer && !layer->HasMask()) {
            layer->CreateMask();
            updateLayerList();
            setModified(true);
            QMessageBox::information(this, "Layer Mask", "Layer mask created. Use black to hide, white to show.");
        }
    }
}

void ScribblerDialog::onDeleteLayerMask() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer && layer->HasMask()) {
            int result = QMessageBox::question(this, "Delete Layer Mask",
                "Are you sure you want to delete the layer mask?",
                QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::Yes) {
                layer->DeleteMask();
                updateLayerList();
                setModified(true);
            }
        }
    }
}

void ScribblerDialog::onApplyLayerMask() {
    int currentRow = m_layerList->currentRow();
    if (currentRow >= 0 && m_canvas) {
        RasterLayer* layer = m_canvas->GetLayer(currentRow);
        if (layer && layer->HasMask()) {
            int result = QMessageBox::question(this, "Apply Layer Mask",
                "Apply mask to layer? This cannot be undone.",
                QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::Yes) {
                layer->ApplyMask();
                updateLayerList();
                setModified(true);
            }
        }
    }
}

// Canvas event slots
void ScribblerDialog::onCanvasModified() {
    setModified(true);
}

void ScribblerDialog::onLayerChanged() {
    updateLayerList();
}

void ScribblerDialog::onColorPicked(const QColor& color) {
    if (m_canvas) {
        m_canvas->SetPrimaryColor(color);
        m_primaryColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

// Utility methods
void ScribblerDialog::updateWindowTitle() {
    QString title = "Scribbler";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void ScribblerDialog::updateLayerList() {
    if (!m_canvas || !m_layerList) return;

    m_layerList->blockSignals(true);
    m_layerList->clear();

    for (int i = 0; i < m_canvas->GetLayerCount(); ++i) {
        RasterLayer* layer = m_canvas->GetLayer(i);
        if (layer) {
            m_layerList->addItem(layer->GetName());
        }
    }

    m_layerList->setCurrentRow(m_canvas->GetActiveLayerIndex());
    m_layerList->blockSignals(false);
}

void ScribblerDialog::updateToolStates() {
    // Update tool-specific UI states if needed
}

bool ScribblerDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool ScribblerDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) return true;

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<ScribblerDialog*>(this),
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

void ScribblerDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

bool ScribblerDialog::LoadImage(const QString& filepath) {
    if (m_canvas) {
        if (m_canvas->LoadFromFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
            updateLayerList();
            return true;
        }
    }
    return false;
}

// Missing ScribblerCanvas methods
void ScribblerCanvas::floodFill(int x, int y, const QColor& fillColor) {
    RasterLayer* layer = GetActiveLayer();
    if (!layer) return;

    QImage& image = layer->GetImage();
    if (x < 0 || x >= image.width() || y < 0 || y >= image.height()) return;

    QColor targetColor = image.pixelColor(x, y);
    if (colorMatch(targetColor, fillColor, m_fillTolerance)) return;

    // Enhanced flood fill with tolerance
    std::vector<QPoint> stack;
    std::vector<std::vector<bool>> visited(image.width(), std::vector<bool>(image.height(), false));

    stack.push_back(QPoint(x, y));

    while (!stack.empty()) {
        QPoint p = stack.back();
        stack.pop_back();

        if (p.x() < 0 || p.x() >= image.width() ||
            p.y() < 0 || p.y() >= image.height()) {
            continue;
        }

        if (visited[p.x()][p.y()]) {
            continue;
        }

        QColor currentColor = image.pixelColor(p.x(), p.y());
        if (!colorMatch(currentColor, targetColor, m_fillTolerance)) {
            continue;
        }

        visited[p.x()][p.y()] = true;
        image.setPixelColor(p.x(), p.y(), fillColor);

        // Add neighboring pixels
        stack.push_back(QPoint(p.x() + 1, p.y()));
        stack.push_back(QPoint(p.x() - 1, p.y()));
        stack.push_back(QPoint(p.x(), p.y() + 1));
        stack.push_back(QPoint(p.x(), p.y() - 1));
    }

    updateCanvas();
    emit canvasModified();
}

bool ScribblerCanvas::colorMatch(const QColor& color1, const QColor& color2, float tolerance) {
    if (tolerance <= 0.0f) {
        return color1 == color2;
    }

    // Calculate color difference using Euclidean distance in RGB space
    float dr = (color1.redF() - color2.redF()) * 255.0f;
    float dg = (color1.greenF() - color2.greenF()) * 255.0f;
    float db = (color1.blueF() - color2.blueF()) * 255.0f;
    float da = (color1.alphaF() - color2.alphaF()) * 255.0f;

    float distance = sqrt(dr * dr + dg * dg + db * db + da * da);
    return distance <= tolerance * 255.0f;
}

QColor ScribblerCanvas::pickColor(int x, int y) {
    if (x >= 0 && x < m_compositeImage.width() && y >= 0 && y < m_compositeImage.height()) {
        return m_compositeImage.pixelColor(x, y);
    }
    return QColor();
}
