#include "GraphCanvasWidget.h"
#include "lupine/visualscripting/VScriptGraph.h"
#include "lupine/visualscripting/VScriptNode.h"
#include "lupine/visualscripting/VScriptConnection.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QWheelEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QVector2D>
#include <QtCore/QMimeData>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsSceneHoverEvent>
#include <QtWidgets/QMenu>
#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <cmath>
#include <iostream>

GraphCanvasWidget::GraphCanvasWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_graph(nullptr)
    , m_gridVisible(true)
    , m_snapToGrid(true)
    , m_gridSize(20.0f)
    , m_gridItem(nullptr)
    , m_panning(false)
    , m_zoomFactor(1.0f)
    , m_creatingConnection(false)
    , m_tempConnectionItem(nullptr)
{
    setupScene();
    setAcceptDrops(true);
}

GraphCanvasWidget::~GraphCanvasWidget() {
}

void GraphCanvasWidget::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Configure view
    setDragMode(QGraphicsView::RubberBandDrag);
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Connect signals
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &GraphCanvasWidget::onSelectionChanged);
    
    updateGrid();
}

void GraphCanvasWidget::setGraph(Lupine::VScriptGraph* graph) {
    m_graph = graph;
    refreshGraph();
}

void GraphCanvasWidget::refreshGraph() {
    clearScene();

    if (!m_graph) {
        return;
    }

    // Create node items
    auto nodes = m_graph->GetNodes();
    for (auto* node : nodes) {
        NodeGraphicsItem* item = createNodeItem(node);
        if (item) {
            m_nodeItems.push_back(item);
        }
    }

    // Create connection items
    auto connections = m_graph->GetConnections();
    for (auto* connection : connections) {
        ConnectionGraphicsItem* item = createConnectionItem(connection);
        if (item) {
            m_connectionItems.push_back(item);
        }
    }
}

void GraphCanvasWidget::clearScene() {
    // Clear the vectors first to avoid dangling pointers
    m_nodeItems.clear();
    m_connectionItems.clear();

    // Clear the scene (this will delete all items)
    if (m_scene) {
        m_scene->clear();
        // Reset grid item pointer since it was deleted by clear()
        m_gridItem = nullptr;
        updateGrid();
    }
}

NodeGraphicsItem* GraphCanvasWidget::createNodeItem(Lupine::VScriptNode* node) {
    if (!node) {
        return nullptr;
    }

    NodeGraphicsItem* item = new NodeGraphicsItem(node);
    item->setPos(node->GetX(), node->GetY());
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    m_scene->addItem(item);

    return item;
}

ConnectionGraphicsItem* GraphCanvasWidget::createConnectionItem(Lupine::VScriptConnection* connection) {
    if (!connection) {
        return nullptr;
    }
    
    ConnectionGraphicsItem* item = new ConnectionGraphicsItem(connection);
    m_scene->addItem(item);
    
    return item;
}

void GraphCanvasWidget::updateGrid() {
    if (m_gridItem) {
        m_scene->removeItem(m_gridItem);
        delete m_gridItem;
        m_gridItem = nullptr;
    }
    
    if (!m_gridVisible) {
        return;
    }
    
    // Create grid
    QPainterPath gridPath;
    QRectF sceneRect = m_scene->sceneRect();
    
    if (sceneRect.isEmpty()) {
        sceneRect = QRectF(-1000, -1000, 2000, 2000);
    }
    
    // Vertical lines
    for (float x = std::floor(sceneRect.left() / m_gridSize) * m_gridSize; 
         x <= sceneRect.right(); x += m_gridSize) {
        gridPath.moveTo(x, sceneRect.top());
        gridPath.lineTo(x, sceneRect.bottom());
    }
    
    // Horizontal lines
    for (float y = std::floor(sceneRect.top() / m_gridSize) * m_gridSize; 
         y <= sceneRect.bottom(); y += m_gridSize) {
        gridPath.moveTo(sceneRect.left(), y);
        gridPath.lineTo(sceneRect.right(), y);
    }
    
    m_gridItem = new QGraphicsPathItem(gridPath);
    m_gridItem->setPen(QPen(QColor(100, 100, 100, 50), 1));
    m_gridItem->setZValue(-1000); // Behind everything
    m_scene->addItem(m_gridItem);
}

QPointF GraphCanvasWidget::snapToGrid(const QPointF& point) const {
    if (!m_snapToGrid) {
        return point;
    }
    
    float x = std::round(point.x() / m_gridSize) * m_gridSize;
    float y = std::round(point.y() / m_gridSize) * m_gridSize;
    
    return QPointF(x, y);
}

void GraphCanvasWidget::zoomIn() {
    scale(1.25, 1.25);
    m_zoomFactor *= 1.25f;
}

void GraphCanvasWidget::zoomOut() {
    scale(0.8, 0.8);
    m_zoomFactor *= 0.8f;
}

void GraphCanvasWidget::resetZoom() {
    resetTransform();
    m_zoomFactor = 1.0f;
}

void GraphCanvasWidget::fitToWindow() {
    if (m_scene) {
        fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11(); // Get current scale
    }
}

void GraphCanvasWidget::setGridVisible(bool visible) {
    if (m_gridVisible != visible) {
        m_gridVisible = visible;
        updateGrid();
    }
}

void GraphCanvasWidget::setSnapToGrid(bool snap) {
    m_snapToGrid = snap;
}

void GraphCanvasWidget::selectNode(const QString& nodeId) {
    // Find and select the node item
    for (auto* item : m_nodeItems) {
        if (item->getNode() &&
            QString::fromStdString(item->getNode()->GetId()) == nodeId) {
            m_scene->clearSelection();
            item->setSelected(true);
            centerOn(item);
            break;
        }
    }
}

void GraphCanvasWidget::startConnection(const QString& nodeId, const QString& pinName) {
    if (m_creatingConnection) {
        // Cancel existing connection
        if (m_tempConnectionItem) {
            m_scene->removeItem(m_tempConnectionItem);
            delete m_tempConnectionItem;
            m_tempConnectionItem = nullptr;
        }
    }

    m_creatingConnection = true;
    m_connectionSourceNode = nodeId;
    m_connectionSourcePin = pinName;

    // Create temporary connection line
    m_tempConnectionItem = new QGraphicsPathItem();
    m_tempConnectionItem->setPen(QPen(QColor(255, 255, 255, 128), 2, Qt::DashLine));
    m_tempConnectionItem->setZValue(1000); // On top
    m_scene->addItem(m_tempConnectionItem);
}

void GraphCanvasWidget::completeConnection(const QString& nodeId, const QString& pinName) {
    if (!m_creatingConnection) {
        return;
    }

    // Clean up temporary connection
    if (m_tempConnectionItem) {
        m_scene->removeItem(m_tempConnectionItem);
        delete m_tempConnectionItem;
        m_tempConnectionItem = nullptr;
    }

    m_creatingConnection = false;

    // Emit signal to create the connection
    if (m_connectionSourceNode != nodeId) { // Don't connect to same node
        emit connectionCreated(m_connectionSourceNode, m_connectionSourcePin, nodeId, pinName);
    }

    m_connectionSourceNode.clear();
    m_connectionSourcePin.clear();
}

void GraphCanvasWidget::updateTempConnection(const QPointF& endPos) {
    if (!m_tempConnectionItem || m_connectionSourceNode.isEmpty()) {
        return;
    }

    // Get source pin position
    NodeGraphicsItem* sourceNodeItem = nullptr;
    for (auto* item : m_nodeItems) {
        if (item->getNode() && QString::fromStdString(item->getNode()->GetId()) == m_connectionSourceNode) {
            sourceNodeItem = item;
            break;
        }
    }

    if (!sourceNodeItem) {
        return;
    }

    QPointF startPos = sourceNodeItem->getPinPosition(m_connectionSourcePin);

    // Create bezier curve for the temporary connection
    QPainterPath path;
    path.moveTo(startPos);

    float dx = endPos.x() - startPos.x();
    float controlOffset = std::abs(dx) * 0.5f + 50.0f;

    QPointF control1(startPos.x() + controlOffset, startPos.y());
    QPointF control2(endPos.x() - controlOffset, endPos.y());

    path.cubicTo(control1, control2, endPos);

    // Update the temporary connection path
    m_tempConnectionItem->setPath(path);

    // Change color based on whether we're hovering over a valid target
    QGraphicsItem* itemUnderMouse = m_scene->itemAt(endPos, QTransform());
    NodeGraphicsItem* targetNodeItem = dynamic_cast<NodeGraphicsItem*>(itemUnderMouse);

    if (targetNodeItem) {
        QString targetPin = targetNodeItem->getPinAtPosition(targetNodeItem->mapFromScene(endPos));
        if (!targetPin.isEmpty() && isValidConnection(m_connectionSourceNode, m_connectionSourcePin,
                                                     QString::fromStdString(targetNodeItem->getNode()->GetId()), targetPin)) {
            // Valid connection - green color
            m_tempConnectionItem->setPen(QPen(QColor(100, 255, 100), 3, Qt::SolidLine));
        } else {
            // Invalid connection - red color
            m_tempConnectionItem->setPen(QPen(QColor(255, 100, 100), 3, Qt::SolidLine));
        }
    } else {
        // Default color while dragging
        m_tempConnectionItem->setPen(QPen(QColor(255, 255, 255, 180), 2, Qt::DashLine));
    }
}

bool GraphCanvasWidget::isValidConnection(const QString& fromNode, const QString& fromPin,
                                         const QString& toNode, const QString& toPin) const {
    if (!m_graph || fromNode == toNode) {
        return false; // Can't connect to same node
    }

    // Get the nodes
    Lupine::VScriptNode* sourceNode = m_graph->GetNode(fromNode.toStdString());
    Lupine::VScriptNode* targetNode = m_graph->GetNode(toNode.toStdString());

    if (!sourceNode || !targetNode) {
        return false;
    }

    // Get the pins
    Lupine::VScriptPin* sourcePin = sourceNode->GetPin(fromPin.toStdString());
    Lupine::VScriptPin* targetPin = targetNode->GetPin(toPin.toStdString());

    if (!sourcePin || !targetPin) {
        return false;
    }

    // Check pin compatibility
    return sourcePin->IsCompatibleWith(*targetPin);
}

void GraphCanvasWidget::cancelConnection() {
    if (m_creatingConnection) {
        m_creatingConnection = false;

        if (m_tempConnectionItem) {
            m_scene->removeItem(m_tempConnectionItem);
            delete m_tempConnectionItem;
            m_tempConnectionItem = nullptr;
        }

        m_connectionSourceNode.clear();
        m_connectionSourcePin.clear();
    }
}

void GraphCanvasWidget::wheelEvent(QWheelEvent* event) {
    // Zoom with mouse wheel
    if (event->modifiers() & Qt::ControlModifier) {
        const float scaleFactor = 1.15f;
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
            m_zoomFactor *= scaleFactor;
        } else {
            scale(1.0f / scaleFactor, 1.0f / scaleFactor);
            m_zoomFactor /= scaleFactor;
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void GraphCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else if (event->button() == Qt::RightButton && m_creatingConnection) {
        // Cancel connection creation
        cancelConnection();
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void GraphCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        QScrollBar* hBar = horizontalScrollBar();
        QScrollBar* vBar = verticalScrollBar();
        hBar->setValue(hBar->value() - delta.x());
        vBar->setValue(vBar->value() - delta.y());

        event->accept();
    } else if (m_creatingConnection && m_tempConnectionItem) {
        // Update temporary connection line to follow mouse
        QPointF scenePos = mapToScene(event->pos());
        updateTempConnection(scenePos);
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void GraphCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void GraphCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        // Delete selected items
        auto selectedItems = m_scene->selectedItems();
        for (auto* item : selectedItems) {
            // TODO: Remove from graph and emit signal
        }
        event->accept();
    } else if (event->key() == Qt::Key_Escape && m_creatingConnection) {
        // Cancel connection creation
        cancelConnection();
        event->accept();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void GraphCanvasWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void GraphCanvasWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void GraphCanvasWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasText()) {
        QString nodeType = event->mimeData()->text();
        QPointF scenePos = mapToScene(event->position().toPoint());
        scenePos = snapToGrid(scenePos);

        // Emit signal to create node - this will be handled by the dialog
        emit nodeDropped(nodeType, scenePos);

        event->acceptProposedAction();
    }
}

void GraphCanvasWidget::onSelectionChanged() {
    auto selectedItems = m_scene->selectedItems();

    if (selectedItems.isEmpty()) {
        emit nodeDeselected();
    } else {
        // Find first selected node
        for (auto* item : selectedItems) {
            NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item);
            if (nodeItem) {
                emit nodeSelected(nodeItem->getNode());
                return;
            }
        }
        emit nodeDeselected();
    }
}

// NodeGraphicsItem Implementation
NodeGraphicsItem::NodeGraphicsItem(Lupine::VScriptNode* node, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_node(node)
    , m_dragging(false)
    , m_showingTooltip(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    updateGeometry();
}

QRectF NodeGraphicsItem::boundingRect() const {
    return m_boundingRect;
}

void NodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!m_node) {
        return;
    }

    QRectF rect = boundingRect();

    // Special rendering for comment nodes
    if (m_node->IsCommentNode()) {
        paintCommentNode(painter, rect);
        return;
    }

    // Regular node background
    QColor bgColor = isSelected() ? QColor(100, 150, 255) : QColor(60, 60, 60);
    painter->setBrush(QBrush(bgColor));
    painter->setPen(QPen(QColor(200, 200, 200), 2));
    painter->drawRoundedRect(rect, 5, 5);

    // Node title
    painter->setPen(QPen(Qt::white));
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);

    QRectF titleRect = rect.adjusted(5, 5, -5, -rect.height() + 25);
    painter->drawText(titleRect, Qt::AlignCenter, QString::fromStdString(m_node->GetDisplayName()));

    // Draw pins
    painter->setFont(QFont()); // Reset font

    auto inputPins = m_node->GetInputPins();
    auto outputPins = m_node->GetOutputPins();

    float pinY = 35; // Start below title
    const float pinSpacing = 22;
    const float pinRadius = 8;
    const float pinOutlineWidth = 2;

    // Draw input pins (left side)
    for (auto* pin : inputPins) {
        QColor pinColor = getPinColor(pin->GetDataType());
        QPointF pinCenter(pinRadius + 2, pinY + pinRadius);

        // Check if we should highlight this pin for connection compatibility
        bool isCompatible = false;
        bool isHighlighted = false;

        // Get the canvas widget to check connection state
        GraphCanvasWidget* canvas = nullptr;
        if (scene() && !scene()->views().isEmpty()) {
            canvas = qobject_cast<GraphCanvasWidget*>(scene()->views().first());
        }

        if (canvas && canvas->isCreatingConnection()) {
            QString pinName = QString::fromStdString(pin->GetName());
            QString nodeId = QString::fromStdString(m_node->GetId());
            isCompatible = canvas->isValidConnection(canvas->getConnectionSourceNode(),
                                                   canvas->getConnectionSourcePin(),
                                                   nodeId, pinName);
            isHighlighted = true;
        }

        // Draw pin outline with compatibility feedback
        painter->setBrush(QBrush(Qt::transparent));
        if (isHighlighted) {
            if (isCompatible) {
                painter->setPen(QPen(QColor(100, 255, 100), pinOutlineWidth + 1)); // Green for compatible
            } else {
                painter->setPen(QPen(QColor(255, 100, 100), pinOutlineWidth + 1)); // Red for incompatible
            }
        } else {
            painter->setPen(QPen(Qt::white, pinOutlineWidth));
        }
        painter->drawEllipse(pinCenter, pinRadius + pinOutlineWidth, pinRadius + pinOutlineWidth);

        // Draw pin based on type
        if (pin->IsExecutionPin()) {
            // Draw execution pin as diamond shape
            painter->setBrush(QBrush(Qt::white));
            painter->setPen(QPen(Qt::white, 2));
            QPolygonF diamond;
            diamond << QPointF(pinCenter.x(), pinCenter.y() - pinRadius)
                   << QPointF(pinCenter.x() + pinRadius, pinCenter.y())
                   << QPointF(pinCenter.x(), pinCenter.y() + pinRadius)
                   << QPointF(pinCenter.x() - pinRadius, pinCenter.y());
            painter->drawPolygon(diamond);
        } else if (pin->IsWildcardPin()) {
            // Draw wildcard pin as diamond with question mark
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            QPolygonF diamond;
            diamond << QPointF(pinCenter.x(), pinCenter.y() - pinRadius)
                   << QPointF(pinCenter.x() + pinRadius, pinCenter.y())
                   << QPointF(pinCenter.x(), pinCenter.y() + pinRadius)
                   << QPointF(pinCenter.x() - pinRadius, pinCenter.y());
            painter->drawPolygon(diamond);

            // Draw question mark
            painter->setPen(QPen(Qt::white, 1));
            QFont smallFont = painter->font();
            smallFont.setPointSize(6);
            painter->setFont(smallFont);
            painter->drawText(QPointF(pinCenter.x() - 3, pinCenter.y() + 2), "?");
        } else if (pin->IsDelegatePin()) {
            // Draw delegate pin as square
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 2));
            QRectF square(pinCenter.x() - pinRadius, pinCenter.y() - pinRadius,
                         pinRadius * 2, pinRadius * 2);
            painter->drawRect(square);
        } else if (pin->IsArrayPin()) {
            // Draw array pin as circle with brackets
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            painter->drawEllipse(pinCenter, pinRadius, pinRadius);

            // Draw array brackets
            painter->setPen(QPen(Qt::white, 2));
            painter->drawText(QPointF(pinCenter.x() - 6, pinCenter.y() + 2), "[]");
        } else {
            // Draw regular pin as circle
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            painter->drawEllipse(pinCenter, pinRadius, pinRadius);
        }

        // Pin label with better positioning
        painter->setPen(QPen(Qt::white));
        QFont pinFont = painter->font();
        pinFont.setPointSize(9);
        painter->setFont(pinFont);
        painter->drawText(QPointF(pinRadius * 2 + 8, pinY + pinRadius + 3), QString::fromStdString(pin->GetLabel()));

        pinY += pinSpacing;
    }

    // Draw output pins (right side)
    pinY = 35; // Reset for output pins
    for (auto* pin : outputPins) {
        QColor pinColor = getPinColor(pin->GetDataType());
        QPointF pinCenter(rect.width() - pinRadius - 2, pinY + pinRadius);

        // Check if we should highlight this pin for connection compatibility
        bool isCompatible = false;
        bool isHighlighted = false;

        // Get the canvas widget to check connection state (same as for input pins)
        GraphCanvasWidget* canvas = nullptr;
        if (scene() && !scene()->views().isEmpty()) {
            canvas = qobject_cast<GraphCanvasWidget*>(scene()->views().first());
        }

        if (canvas && canvas->isCreatingConnection()) {
            QString pinName = QString::fromStdString(pin->GetName());
            QString nodeId = QString::fromStdString(m_node->GetId());
            isCompatible = canvas->isValidConnection(canvas->getConnectionSourceNode(),
                                                   canvas->getConnectionSourcePin(),
                                                   nodeId, pinName);
            isHighlighted = true;
        }

        // Draw pin outline with compatibility feedback
        painter->setBrush(QBrush(Qt::transparent));
        if (isHighlighted) {
            if (isCompatible) {
                painter->setPen(QPen(QColor(100, 255, 100), pinOutlineWidth + 1)); // Green for compatible
            } else {
                painter->setPen(QPen(QColor(255, 100, 100), pinOutlineWidth + 1)); // Red for incompatible
            }
        } else {
            painter->setPen(QPen(Qt::white, pinOutlineWidth));
        }
        painter->drawEllipse(pinCenter, pinRadius + pinOutlineWidth, pinRadius + pinOutlineWidth);

        // Draw pin based on type
        if (pin->IsExecutionPin()) {
            // Draw execution pin as diamond shape
            painter->setBrush(QBrush(Qt::white));
            painter->setPen(QPen(Qt::white, 2));
            QPolygonF diamond;
            diamond << QPointF(pinCenter.x(), pinCenter.y() - pinRadius)
                   << QPointF(pinCenter.x() + pinRadius, pinCenter.y())
                   << QPointF(pinCenter.x(), pinCenter.y() + pinRadius)
                   << QPointF(pinCenter.x() - pinRadius, pinCenter.y());
            painter->drawPolygon(diamond);
        } else if (pin->IsWildcardPin()) {
            // Draw wildcard pin as diamond with question mark
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            QPolygonF diamond;
            diamond << QPointF(pinCenter.x(), pinCenter.y() - pinRadius)
                   << QPointF(pinCenter.x() + pinRadius, pinCenter.y())
                   << QPointF(pinCenter.x(), pinCenter.y() + pinRadius)
                   << QPointF(pinCenter.x() - pinRadius, pinCenter.y());
            painter->drawPolygon(diamond);

            // Draw question mark
            painter->setPen(QPen(Qt::white, 1));
            QFont smallFont = painter->font();
            smallFont.setPointSize(6);
            painter->setFont(smallFont);
            painter->drawText(QPointF(pinCenter.x() - 3, pinCenter.y() + 2), "?");
        } else if (pin->IsDelegatePin()) {
            // Draw delegate pin as square
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 2));
            QRectF square(pinCenter.x() - pinRadius, pinCenter.y() - pinRadius,
                         pinRadius * 2, pinRadius * 2);
            painter->drawRect(square);
        } else if (pin->IsArrayPin()) {
            // Draw array pin as circle with brackets
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            painter->drawEllipse(pinCenter, pinRadius, pinRadius);

            // Draw array brackets
            painter->setPen(QPen(Qt::white, 2));
            painter->drawText(QPointF(pinCenter.x() - 6, pinCenter.y() + 2), "[]");
        } else {
            // Draw regular pin as circle
            painter->setBrush(QBrush(pinColor));
            painter->setPen(QPen(pinColor.darker(150), 1));
            painter->drawEllipse(pinCenter, pinRadius, pinRadius);
        }

        // Pin label (right-aligned) with better positioning
        painter->setPen(QPen(Qt::white));
        QFont pinFont = painter->font();
        pinFont.setPointSize(9);
        painter->setFont(pinFont);
        QFontMetrics fm(pinFont);
        QString label = QString::fromStdString(pin->GetLabel());
        int labelWidth = fm.boundingRect(label).width();
        painter->drawText(QPointF(rect.width() - pinRadius * 2 - 8 - labelWidth, pinY + pinRadius + 3), label);

        pinY += pinSpacing;
    }
}

QRectF NodeGraphicsItem::calculateBoundingRect() const {
    if (!m_node) {
        return QRectF(0, 0, 100, 50);
    }

    // Special handling for comment nodes
    if (m_node->IsCommentNode()) {
        float width = std::stof(m_node->GetProperty("width", "200"));
        float height = std::stof(m_node->GetProperty("height", "100"));
        return QRectF(0, 0, width, height);
    }

    // Calculate size based on node content
    QString title = QString::fromStdString(m_node->GetDisplayName());
    QFont font;
    QFontMetrics fm(font);
    int titleWidth = fm.boundingRect(title).width();

    int width = std::max(120, titleWidth + 20);
    int height = 60; // Base height

    // Add height for pins
    auto inputPins = m_node->GetInputPins();
    auto outputPins = m_node->GetOutputPins();
    int maxPins = std::max(inputPins.size(), outputPins.size());
    height += maxPins * 22; // Updated spacing

    return QRectF(0, 0, width, height);
}

void NodeGraphicsItem::updateGeometry() {
    prepareGeometryChange();
    m_boundingRect = calculateBoundingRect();
}

QVariant NodeGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && m_node) {
        QPointF newPos = value.toPointF();
        // Update node position
        m_node->SetPosition(newPos.x(), newPos.y());
    }

    return QGraphicsItem::itemChange(change, value);
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Check if we clicked on a pin
        QString pinName = getPinAtPosition(event->pos());
        if (!pinName.isEmpty()) {
            // Start connection from this pin
            GraphCanvasWidget* canvas = qobject_cast<GraphCanvasWidget*>(scene()->views().first());
            if (canvas) {
                canvas->startConnection(QString::fromStdString(m_node->GetId()), pinName);
                event->accept();
                return;
            }
        }
    }

    m_dragging = true;
    QGraphicsItem::mousePressEvent(event);
}

void NodeGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseMoveEvent(event);
}

void NodeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_dragging) {
        // Check if we're completing a connection
        QString pinName = getPinAtPosition(event->pos());
        if (!pinName.isEmpty()) {
            GraphCanvasWidget* canvas = qobject_cast<GraphCanvasWidget*>(scene()->views().first());
            if (canvas) {
                canvas->completeConnection(QString::fromStdString(m_node->GetId()), pinName);
                event->accept();
                return;
            }
        }
    }

    m_dragging = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

void NodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverEnterEvent(event);
}

void NodeGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    updateTooltip(event->pos());
    QGraphicsItem::hoverMoveEvent(event);
}

void NodeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if (m_showingTooltip) {
        setToolTip("");
        m_showingTooltip = false;
    }
    m_hoveredPin.clear();
    QGraphicsItem::hoverLeaveEvent(event);
}

void NodeGraphicsItem::updateTooltip(const QPointF& position) {
    QString pinName = getPinAtPosition(position);

    if (pinName != m_hoveredPin) {
        m_hoveredPin = pinName;

        if (!pinName.isEmpty() && m_node) {
            Lupine::VScriptPin* pin = m_node->GetPin(pinName.toStdString());
            if (pin) {
                QString tooltip = QString("Pin: %1\nType: %2\nDirection: %3")
                    .arg(QString::fromStdString(pin->GetLabel()))
                    .arg(DataTypeToString(pin->GetDataType()))
                    .arg(pin->GetDirection() == Lupine::VScriptPinDirection::Input ? "Input" : "Output");

                if (!pin->GetDefaultValue().empty()) {
                    tooltip += QString("\nDefault: %1").arg(QString::fromStdString(pin->GetDefaultValue()));
                }

                if (!pin->GetTooltip().empty()) {
                    tooltip += QString("\n%1").arg(QString::fromStdString(pin->GetTooltip()));
                }

                setToolTip(tooltip);
                m_showingTooltip = true;
            }
        } else {
            if (m_showingTooltip) {
                setToolTip("");
                m_showingTooltip = false;
            }
        }
    }
}

QString NodeGraphicsItem::DataTypeToString(Lupine::VScriptDataType dataType) const {
    switch (dataType) {
        case Lupine::VScriptDataType::Execution:
            return "Execution";
        case Lupine::VScriptDataType::Boolean:
            return "Boolean";
        case Lupine::VScriptDataType::Integer:
            return "Integer";
        case Lupine::VScriptDataType::Float:
            return "Float";
        case Lupine::VScriptDataType::String:
            return "String";
        case Lupine::VScriptDataType::Vector2:
            return "Vector2";
        case Lupine::VScriptDataType::Vector3:
            return "Vector3";
        case Lupine::VScriptDataType::Vector4:
            return "Vector4";
        case Lupine::VScriptDataType::Transform:
            return "Transform";
        case Lupine::VScriptDataType::Rotator:
            return "Rotator";
        case Lupine::VScriptDataType::Color:
            return "Color";
        case Lupine::VScriptDataType::Object:
            return "Object";
        case Lupine::VScriptDataType::Class:
            return "Class";
        case Lupine::VScriptDataType::Enum:
            return "Enum";
        case Lupine::VScriptDataType::Struct:
            return "Struct";
        case Lupine::VScriptDataType::Array:
            return "Array";
        case Lupine::VScriptDataType::Map:
            return "Map";
        case Lupine::VScriptDataType::Set:
            return "Set";
        case Lupine::VScriptDataType::Delegate:
            return "Delegate";
        case Lupine::VScriptDataType::Event:
            return "Event";
        case Lupine::VScriptDataType::Wildcard:
            return "Wildcard";
        case Lupine::VScriptDataType::Any:
        default:
            return "Any";
    }
}

QColor NodeGraphicsItem::getPinColor(Lupine::VScriptDataType dataType) const {
    switch (dataType) {
        case Lupine::VScriptDataType::Execution:
            return QColor(255, 255, 255); // White
        case Lupine::VScriptDataType::Boolean:
            return QColor(255, 100, 100); // Red
        case Lupine::VScriptDataType::Integer:
            return QColor(100, 255, 255); // Cyan
        case Lupine::VScriptDataType::Float:
            return QColor(100, 255, 100); // Green
        case Lupine::VScriptDataType::String:
            return QColor(255, 100, 255); // Magenta
        case Lupine::VScriptDataType::Vector2:
            return QColor(255, 255, 100); // Yellow
        case Lupine::VScriptDataType::Vector3:
            return QColor(200, 100, 255); // Purple
        case Lupine::VScriptDataType::Vector4:
            return QColor(255, 150, 100); // Orange
        case Lupine::VScriptDataType::Transform:
            return QColor(139, 69, 19); // Brown
        case Lupine::VScriptDataType::Rotator:
            return QColor(255, 192, 203); // Pink
        case Lupine::VScriptDataType::Color:
            return QColor(255, 255, 255); // White (will show rainbow effect)
        case Lupine::VScriptDataType::Object:
            return QColor(100, 150, 255); // Blue
        case Lupine::VScriptDataType::Class:
            return QColor(50, 100, 200); // Dark Blue
        case Lupine::VScriptDataType::Enum:
            return QColor(150, 200, 255); // Light Blue
        case Lupine::VScriptDataType::Struct:
            return QColor(100, 200, 200); // Teal
        case Lupine::VScriptDataType::Array:
            return QColor(255, 255, 255); // White (will be modified based on element type)
        case Lupine::VScriptDataType::Map:
            return QColor(200, 150, 255); // Light Purple
        case Lupine::VScriptDataType::Set:
            return QColor(255, 200, 150); // Light Orange
        case Lupine::VScriptDataType::Delegate:
            return QColor(255, 100, 100); // Red
        case Lupine::VScriptDataType::Event:
            return QColor(255, 50, 50); // Bright Red
        case Lupine::VScriptDataType::Wildcard:
            return QColor(180, 180, 180); // Light Gray
        case Lupine::VScriptDataType::Any:
        default:
            return QColor(150, 150, 150); // Gray
    }
}

QPointF NodeGraphicsItem::getPinPosition(const QString& pinName) const {
    if (!m_node) {
        return QPointF();
    }

    auto inputPins = m_node->GetInputPins();
    auto outputPins = m_node->GetOutputPins();

    float pinY = 35; // Start below title
    const float pinSpacing = 22;
    const float pinRadius = 8;

    // Check input pins
    for (auto* pin : inputPins) {
        if (QString::fromStdString(pin->GetName()) == pinName) {
            return mapToScene(QPointF(pinRadius + 2, pinY + pinRadius));
        }
        pinY += pinSpacing;
    }

    // Check output pins
    pinY = 35; // Reset for output pins
    QRectF rect = boundingRect();
    for (auto* pin : outputPins) {
        if (QString::fromStdString(pin->GetName()) == pinName) {
            return mapToScene(QPointF(rect.width() - pinRadius - 2, pinY + pinRadius));
        }
        pinY += pinSpacing;
    }

    return QPointF();
}

QString NodeGraphicsItem::getPinAtPosition(const QPointF& position) const {
    if (!m_node) {
        return QString();
    }

    auto inputPins = m_node->GetInputPins();
    auto outputPins = m_node->GetOutputPins();

    float pinY = 35; // Start below title
    const float pinSpacing = 22;
    const float pinRadius = 8;
    const float hitRadius = pinRadius + 4; // Larger hit area for easier clicking

    // Check input pins
    for (auto* pin : inputPins) {
        QPointF pinPos(pinRadius + 2, pinY + pinRadius);
        if (QVector2D(position - pinPos).length() <= hitRadius) {
            return QString::fromStdString(pin->GetName());
        }
        pinY += pinSpacing;
    }

    // Check output pins
    pinY = 35; // Reset for output pins
    QRectF rect = boundingRect();
    for (auto* pin : outputPins) {
        QPointF pinPos(rect.width() - pinRadius - 2, pinY + pinRadius);
        if (QVector2D(position - pinPos).length() <= hitRadius) {
            return QString::fromStdString(pin->GetName());
        }
        pinY += pinSpacing;
    }

    return QString();
}

// ConnectionGraphicsItem Implementation
ConnectionGraphicsItem::ConnectionGraphicsItem(Lupine::VScriptConnection* connection, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_connection(connection)
{
    setZValue(-1); // Behind nodes
    updatePath();
    updateAppearance();
}

void ConnectionGraphicsItem::updatePath() {
    if (!m_connection) {
        return;
    }

    QPainterPath path = calculatePath();
    setPath(path);
}

QPainterPath ConnectionGraphicsItem::calculatePath() const {
    QPainterPath path;

    if (!m_connection) {
        return path;
    }

    // Get pin positions
    QPointF startPos = getPinPosition(
        QString::fromStdString(m_connection->GetFromNodeId()),
        QString::fromStdString(m_connection->GetFromPinName())
    );

    QPointF endPos = getPinPosition(
        QString::fromStdString(m_connection->GetToNodeId()),
        QString::fromStdString(m_connection->GetToPinName())
    );

    // If we couldn't find pin positions, return empty path
    if (startPos.isNull() || endPos.isNull()) {
        return path;
    }

    // Create improved bezier curve with better visual flow
    path.moveTo(startPos);

    float dx = endPos.x() - startPos.x();
    float dy = endPos.y() - startPos.y();
    float distance = std::sqrt(dx * dx + dy * dy);

    // Adaptive control point calculation for better curves
    float controlOffset = std::max(std::abs(dx) * 0.6f, 80.0f);

    // Adjust control points based on vertical distance and connection direction
    if (std::abs(dy) > 100.0f) {
        controlOffset = std::max(controlOffset, std::abs(dy) * 0.3f);
    }

    // For very long connections, increase the control offset
    if (distance > 300.0f) {
        controlOffset = std::max(controlOffset, distance * 0.4f);
    }

    // Create smooth S-curve for better visual flow
    QPointF control1(startPos.x() + controlOffset, startPos.y());
    QPointF control2(endPos.x() - controlOffset, endPos.y());

    // Add slight vertical offset for better separation when connections overlap
    if (std::abs(dy) < 20.0f && dx > 0) {
        control1.setY(control1.y() + 10);
        control2.setY(control2.y() - 10);
    }

    path.cubicTo(control1, control2, endPos);

    return path;
}

QPointF ConnectionGraphicsItem::getPinPosition(const QString& nodeId, const QString& pinName) const {
    // Find the node graphics item in the scene
    QGraphicsScene* graphicsScene = scene();
    if (!graphicsScene) {
        return QPointF();
    }

    // Search through all items to find the matching node
    for (QGraphicsItem* item : graphicsScene->items()) {
        NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item);
        if (nodeItem && nodeItem->getNode() &&
            QString::fromStdString(nodeItem->getNode()->GetId()) == nodeId) {
            return nodeItem->getPinPosition(pinName);
        }
    }

    return QPointF();
}

void ConnectionGraphicsItem::updateAppearance() {
    if (!m_connection) {
        return;
    }

    // Get the data type of the connection for color coding
    QColor connectionColor = getConnectionColor();
    Lupine::VScriptDataType dataType = getConnectionDataType();

    // Determine line thickness based on data type
    float lineWidth = 3.0f;
    if (dataType == Lupine::VScriptDataType::Execution) {
        lineWidth = 4.0f; // Thicker for execution flow
    } else if (dataType == Lupine::VScriptDataType::Array) {
        lineWidth = 3.5f; // Slightly thicker for arrays
    }

    // Set pen with appropriate color and style
    QPen connectionPen(connectionColor, lineWidth);
    connectionPen.setCapStyle(Qt::RoundCap);
    connectionPen.setJoinStyle(Qt::RoundJoin);

    // Add special effects for certain data types
    if (dataType == Lupine::VScriptDataType::Execution) {
        // Execution connections get a slight glow effect
        connectionPen.setStyle(Qt::SolidLine);
    } else if (dataType == Lupine::VScriptDataType::Delegate || dataType == Lupine::VScriptDataType::Event) {
        // Delegate connections get a dashed line
        connectionPen.setStyle(Qt::DashLine);
    }

    setPen(connectionPen);

    // Set brush for filled connections (like execution flow)
    if (dataType == Lupine::VScriptDataType::Execution) {
        setBrush(QBrush(connectionColor.lighter(120)));
    } else {
        setBrush(QBrush(Qt::NoBrush));
    }
}

QColor ConnectionGraphicsItem::getConnectionColor() const {
    if (!m_connection) {
        return QColor(255, 255, 255); // Default white
    }

    // Find the source node and pin to get the data type
    QGraphicsScene* graphicsScene = scene();
    if (!graphicsScene) {
        return QColor(255, 255, 255);
    }

    for (QGraphicsItem* item : graphicsScene->items()) {
        NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item);
        if (nodeItem && nodeItem->getNode() &&
            QString::fromStdString(nodeItem->getNode()->GetId()) == QString::fromStdString(m_connection->GetFromNodeId())) {

            Lupine::VScriptPin* pin = nodeItem->getNode()->GetPin(m_connection->GetFromPinName());
            if (pin) {
                return nodeItem->getPinColor(pin->GetDataType());
            }
        }
    }

    return QColor(255, 255, 255); // Default white
}

Lupine::VScriptDataType ConnectionGraphicsItem::getConnectionDataType() const {
    if (!m_connection) {
        return Lupine::VScriptDataType::Any;
    }

    // Find the source node and pin to get the data type
    QGraphicsScene* graphicsScene = scene();
    if (!graphicsScene) {
        return Lupine::VScriptDataType::Any;
    }

    for (QGraphicsItem* item : graphicsScene->items()) {
        NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item);
        if (nodeItem && nodeItem->getNode() &&
            QString::fromStdString(nodeItem->getNode()->GetId()) == QString::fromStdString(m_connection->GetFromNodeId())) {

            Lupine::VScriptPin* pin = nodeItem->getNode()->GetPin(m_connection->GetFromPinName());
            if (pin) {
                return pin->GetDataType();
            }
        }
    }

    return Lupine::VScriptDataType::Any;
}

void GraphCanvasWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu contextMenu(this);

    // Add common actions
    QAction* addCommentAction = contextMenu.addAction("Add Comment");
    QAction* addRerouteAction = contextMenu.addAction("Add Reroute Node");
    contextMenu.addSeparator();

    QAction* resetZoomAction = contextMenu.addAction("Reset Zoom");
    QAction* fitToViewAction = contextMenu.addAction("Fit to View");
    contextMenu.addSeparator();

    QAction* deleteSelectedAction = contextMenu.addAction("Delete Selected");
    deleteSelectedAction->setEnabled(!m_scene->selectedItems().isEmpty());

    // Show context menu and handle selection
    QAction* selectedAction = contextMenu.exec(event->globalPos());

    if (selectedAction == addCommentAction) {
        QPointF scenePos = mapToScene(event->pos());
        emit nodeDropped("Comment", scenePos);
    } else if (selectedAction == addRerouteAction) {
        QPointF scenePos = mapToScene(event->pos());
        emit nodeDropped("Reroute", scenePos);
    } else if (selectedAction == resetZoomAction) {
        resetTransform();
        m_zoomFactor = 1.0f;
    } else if (selectedAction == fitToViewAction) {
        fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
    } else if (selectedAction == deleteSelectedAction) {
        // Delete selected items
        for (QGraphicsItem* item : m_scene->selectedItems()) {
            NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item);
            if (nodeItem && nodeItem->getNode()) {
                // Emit signal to delete node
                emit nodeDeleted(QString::fromStdString(nodeItem->getNode()->GetId()));
            }
        }
    }
}

void NodeGraphicsItem::paintCommentNode(QPainter* painter, const QRectF& rect) {
    if (!m_node) {
        return;
    }

    // Get comment properties
    std::string colorStr = m_node->GetProperty("comment_color", "#FFFF88");
    std::string commentText = m_node->GetProperty("comment_text", "Comment");

    // Parse color
    QColor commentColor(QString::fromStdString(colorStr));
    if (!commentColor.isValid()) {
        commentColor = QColor(255, 255, 136); // Default yellow
    }

    // Make background semi-transparent
    commentColor.setAlpha(180);

    // Draw comment background
    painter->setBrush(QBrush(commentColor));
    painter->setPen(QPen(commentColor.darker(150), 2));
    painter->drawRoundedRect(rect, 8, 8);

    // Draw comment text
    painter->setPen(QPen(Qt::black));
    QFont font = painter->font();
    font.setPointSize(10);
    painter->setFont(font);

    QRectF textRect = rect.adjusted(10, 10, -10, -10);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     QString::fromStdString(commentText));

    // Draw resize handle in bottom-right corner
    if (isSelected()) {
        QRectF handleRect(rect.right() - 10, rect.bottom() - 10, 8, 8);
        painter->setBrush(QBrush(Qt::darkGray));
        painter->setPen(QPen(Qt::black, 1));
        painter->drawRect(handleRect);
    }
}
