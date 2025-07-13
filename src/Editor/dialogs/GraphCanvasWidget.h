#pragma once

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsPathItem>
#include <QtCore/QPointF>
#include "lupine/visualscripting/VScriptNode.h"
#include <memory>

namespace Lupine {
    class VScriptGraph;
    class VScriptConnection;
}

// Forward declarations
class NodeGraphicsItem;
class ConnectionGraphicsItem;

/**
 * @brief Graphics view widget for editing visual script graphs
 * 
 * Provides a canvas for visual node editing with:
 * - Zoom and pan functionality
 * - Node drag and drop
 * - Connection drawing
 * - Grid display
 * - Selection handling
 */
class GraphCanvasWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphCanvasWidget(QWidget* parent = nullptr);
    ~GraphCanvasWidget();

    /**
     * @brief Set the graph to display and edit
     * @param graph Graph to display (can be nullptr)
     */
    void setGraph(Lupine::VScriptGraph* graph);

    /**
     * @brief Get the current graph
     * @return Current graph or nullptr
     */
    Lupine::VScriptGraph* getGraph() const { return m_graph; }

    /**
     * @brief Zoom functions
     */
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

    /**
     * @brief Grid and snap settings
     */
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

    void setSnapToGrid(bool snap);
    bool isSnapToGrid() const { return m_snapToGrid; }

    /**
     * @brief Select a node by ID
     * @param nodeId ID of the node to select
     */
    void selectNode(const QString& nodeId);

    /**
     * @brief Start creating a connection from a pin
     * @param nodeId Source node ID
     * @param pinName Source pin name
     */
    void startConnection(const QString& nodeId, const QString& pinName);

    /**
     * @brief Complete a connection to a pin
     * @param nodeId Target node ID
     * @param pinName Target pin name
     */
    void completeConnection(const QString& nodeId, const QString& pinName);

    /**
     * @brief Update temporary connection line during dragging
     * @param endPos Current mouse position in scene coordinates
     */
    void updateTempConnection(const QPointF& endPos);

    /**
     * @brief Check if a connection between two pins is valid
     * @param fromNode Source node ID
     * @param fromPin Source pin name
     * @param toNode Target node ID
     * @param toPin Target pin name
     * @return True if connection is valid
     */
    bool isValidConnection(const QString& fromNode, const QString& fromPin,
                          const QString& toNode, const QString& toPin) const;

    /**
     * @brief Cancel current connection creation
     */
    void cancelConnection();

    /**
     * @brief Check if currently creating a connection
     */
    bool isCreatingConnection() const { return m_creatingConnection; }

    /**
     * @brief Get the source node of current connection
     */
    QString getConnectionSourceNode() const { return m_connectionSourceNode; }

    /**
     * @brief Get the source pin of current connection
     */
    QString getConnectionSourcePin() const { return m_connectionSourcePin; }

signals:
    void nodeSelected(Lupine::VScriptNode* node);
    void nodeDeselected();
    void nodeDropped(const QString& nodeType, const QPointF& position);
    void nodeDeleted(const QString& nodeId);
    void connectionCreated(const QString& fromNode, const QString& fromPin,
                          const QString& toNode, const QString& toPin);
    void graphModified();

protected:
    // Event handlers
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onSelectionChanged();

private:
    void setupScene();
    void updateGrid();
    void refreshGraph();
    void clearScene();

    /**
     * @brief Snap point to grid if snap is enabled
     * @param point Point to snap
     * @return Snapped point
     */
    QPointF snapToGrid(const QPointF& point) const;

    /**
     * @brief Create graphics item for a node
     * @param node Node to create item for
     * @return Created graphics item
     */
    NodeGraphicsItem* createNodeItem(Lupine::VScriptNode* node);

    /**
     * @brief Create graphics item for a connection
     * @param connection Connection to create item for
     * @return Created graphics item
     */
    ConnectionGraphicsItem* createConnectionItem(Lupine::VScriptConnection* connection);

    QGraphicsScene* m_scene;
    Lupine::VScriptGraph* m_graph;

    // Grid settings
    bool m_gridVisible;
    bool m_snapToGrid;
    float m_gridSize;
    QGraphicsPathItem* m_gridItem;

    // Interaction state
    bool m_panning;
    QPoint m_lastPanPoint;
    float m_zoomFactor;

    // Connection state
    bool m_creatingConnection;
    QString m_connectionSourceNode;
    QString m_connectionSourcePin;
    QGraphicsPathItem* m_tempConnectionItem;

    // Graphics items
    std::vector<NodeGraphicsItem*> m_nodeItems;
    std::vector<ConnectionGraphicsItem*> m_connectionItems;
};

/**
 * @brief Graphics item representing a visual script node
 */
class NodeGraphicsItem : public QGraphicsItem {
public:
    explicit NodeGraphicsItem(Lupine::VScriptNode* node, QGraphicsItem* parent = nullptr);

    /**
     * @brief Get the associated node
     */
    Lupine::VScriptNode* getNode() const { return m_node; }

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    /**
     * @brief Get pin position in scene coordinates
     * @param pinName Name of the pin
     * @return Pin position or invalid point if not found
     */
    QPointF getPinPosition(const QString& pinName) const;

    /**
     * @brief Get pin at a given position
     * @param position Position in item coordinates
     * @return Pin name or empty string if no pin at position
     */
    QString getPinAtPosition(const QPointF& position) const;

    /**
     * @brief Get color for a data type
     * @param dataType The data type
     * @return Color for the data type
     */
    QColor getPinColor(Lupine::VScriptDataType dataType) const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    void updateGeometry();
    QRectF calculateBoundingRect() const;
    void updateTooltip(const QPointF& position);
    QString DataTypeToString(Lupine::VScriptDataType dataType) const;
    void paintCommentNode(QPainter* painter, const QRectF& rect);

    Lupine::VScriptNode* m_node;
    QRectF m_boundingRect;
    bool m_dragging;
    QString m_hoveredPin;
    bool m_showingTooltip;
};

/**
 * @brief Graphics item representing a connection between nodes
 */
class ConnectionGraphicsItem : public QGraphicsPathItem {
public:
    explicit ConnectionGraphicsItem(Lupine::VScriptConnection* connection, QGraphicsItem* parent = nullptr);

    /**
     * @brief Get the associated connection
     */
    Lupine::VScriptConnection* getConnection() const { return m_connection; }

    /**
     * @brief Update the connection path
     */
    void updatePath();

    /**
     * @brief Update the visual appearance of the connection
     */
    void updateAppearance();

private:
    /**
     * @brief Calculate the path for the connection
     * @return QPainterPath for the connection
     */
    QPainterPath calculatePath() const;

    /**
     * @brief Get pin position for a node
     * @param nodeId Node ID
     * @param pinName Pin name
     * @return Pin position in scene coordinates
     */
    QPointF getPinPosition(const QString& nodeId, const QString& pinName) const;

    /**
     * @brief Get the color for this connection based on data type
     * @return QColor for the connection
     */
    QColor getConnectionColor() const;

    /**
     * @brief Get the data type for this connection
     * @return VScriptDataType for the connection
     */
    Lupine::VScriptDataType getConnectionDataType() const;

    Lupine::VScriptConnection* m_connection;
};
