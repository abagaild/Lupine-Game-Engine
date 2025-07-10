#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QColorDialog>
#include <QtGui/QActionGroup>
#include <QtGui/QPixmap>
#include <QtGui/QColor>
#include <QtCore/QTimer>
#include <memory>
#include <vector>

/**
 * @brief Drawing tools for the pixel painter
 */
enum class PixelTool {
    Brush,          // Paint pixels
    Eraser,         // Erase pixels
    Bucket,         // Flood fill
    Eyedropper,     // Pick color
    Wand            // Magic wand selection
};

/**
 * @brief Blend modes for layers
 */
enum class BlendMode {
    Normal,
    Multiply,
    Overlay,
    SoftLight
};

/**
 * @brief Represents a single layer in the pixel art
 */
class PixelLayer {
public:
    PixelLayer(const QString& name, int width, int height);
    ~PixelLayer();

    // Properties
    QString GetName() const { return m_name; }
    void SetName(const QString& name) { m_name = name; }
    
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float opacity) { m_opacity = opacity; }
    
    BlendMode GetBlendMode() const { return m_blendMode; }
    void SetBlendMode(BlendMode mode) { m_blendMode = mode; }
    
    bool IsAlphaLocked() const { return m_alphaLocked; }
    void SetAlphaLocked(bool locked) { m_alphaLocked = locked; }
    
    bool HasClippingMask() const { return m_clippingMask; }
    void SetClippingMask(bool mask) { m_clippingMask = mask; }

    // Image data
    QImage& GetImage() { return m_image; }
    const QImage& GetImage() const { return m_image; }
    
    void SetPixel(int x, int y, const QColor& color);
    QColor GetPixel(int x, int y) const;
    void Clear();
    void Resize(int width, int height);

private:
    QString m_name;
    QImage m_image;
    bool m_visible;
    float m_opacity;
    BlendMode m_blendMode;
    bool m_alphaLocked;
    bool m_clippingMask;
};

/**
 * @brief Custom graphics view for pixel art editing
 */
class PixelCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit PixelCanvas(QWidget* parent = nullptr);
    ~PixelCanvas();

    // Canvas properties
    void SetCanvasSize(int width, int height);
    QSize GetCanvasSize() const { return m_canvasSize; }
    
    void SetCurrentTool(PixelTool tool) { m_currentTool = tool; }
    PixelTool GetCurrentTool() const { return m_currentTool; }
    
    void SetBrushSize(int size) { m_brushSize = size; }
    int GetBrushSize() const { return m_brushSize; }
    
    void SetPrimaryColor(const QColor& color) { m_primaryColor = color; }
    QColor GetPrimaryColor() const { return m_primaryColor; }
    
    void SetSecondaryColor(const QColor& color) { m_secondaryColor = color; }
    QColor GetSecondaryColor() const { return m_secondaryColor; }
    
    void SetShowGrid(bool show);
    bool GetShowGrid() const { return m_showGrid; }

    // Layer management
    void AddLayer(const QString& name);
    void RemoveLayer(int index);
    void SetActiveLayer(int index);
    int GetActiveLayerIndex() const { return m_activeLayerIndex; }
    PixelLayer* GetActiveLayer();
    PixelLayer* GetLayer(int index);
    int GetLayerCount() const { return static_cast<int>(m_layers.size()); }

    // Zoom controls
    void ZoomIn();
    void ZoomOut();
    void ZoomToFit();
    void ZoomToActual();

    // File operations
    void NewCanvas(int width, int height);
    bool LoadFromFile(const QString& filepath);
    bool SaveToFile(const QString& filepath);
    bool ExportToImage(const QString& filepath);

signals:
    void canvasModified();
    void layerChanged();
    void colorPicked(const QColor& color);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSceneChanged();

private:
    void setupScene();
    void updateCanvas();
    void updateGrid();
    void compositeImage();
    QPoint screenToPixel(const QPoint& screenPos) const;
    QPoint pixelToScreen(const QPoint& pixelPos) const;
    
    // Drawing operations
    void paintPixel(int x, int y, const QColor& color);
    void floodFill(int x, int y, const QColor& fillColor);
    void erasePixel(int x, int y);
    QColor pickColor(int x, int y);

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_canvasItem;
    QGraphicsPixmapItem* m_gridItem;
    
    QSize m_canvasSize;
    PixelTool m_currentTool;
    int m_brushSize;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    bool m_showGrid;
    bool m_painting;
    QPoint m_lastPaintPos;
    
    std::vector<std::unique_ptr<PixelLayer>> m_layers;
    int m_activeLayerIndex;
    QImage m_compositeImage;
};

/**
 * @brief Color wheel widget for color selection
 */
class ColorWheelWidget : public QWidget {
    Q_OBJECT

public:
    explicit ColorWheelWidget(QWidget* parent = nullptr);
    ~ColorWheelWidget();

    QColor GetSelectedColor() const { return m_selectedColor; }
    void SetSelectedColor(const QColor& color);

signals:
    void colorChanged(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateColor(const QPoint& pos);
    QColor colorAtPosition(const QPoint& pos) const;

    QColor m_selectedColor;
    QPoint m_selectedPos;
    bool m_dragging;
};

/**
 * @brief Color palette widget for managing color swatches
 */
class ColorPaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit ColorPaletteWidget(QWidget* parent = nullptr);
    ~ColorPaletteWidget();

    void AddColor(const QColor& color);
    void RemoveColor(int index);
    void ClearPalette();
    
    bool LoadPalette(const QString& filepath);
    bool SavePalette(const QString& filepath);

signals:
    void colorSelected(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    int getColorIndexAt(const QPoint& pos) const;

    std::vector<QColor> m_colors;
    int m_selectedIndex;
    QSize m_swatchSize;
};

/**
 * @brief Dialog for creating and editing pixel art
 * 
 * This dialog provides a complete pixel art creation environment with:
 * - Multi-layer support with blend modes and opacity
 * - Various drawing tools (brush, bucket, wand, etc.)
 * - Color wheel and palette management
 * - Layer effects (alpha lock, clipping masks)
 * - Export to multiple formats
 */
class PixelPainterDialog : public QDialog {
    Q_OBJECT

public:
    explicit PixelPainterDialog(QWidget* parent = nullptr);
    ~PixelPainterDialog();

private slots:
    // File operations
    void onNewCanvas();
    void onOpenFile();
    void onSaveFile();
    void onSaveAs();
    void onExportImage();

    // Tool selection
    void onToolChanged(QAction* action);
    void onBrushSizeChanged();
    void onPrimaryColorChanged();
    void onSecondaryColorChanged();

    // Layer management
    void onAddLayer();
    void onRemoveLayer();
    void onDuplicateLayer();
    void onMoveLayerUp();
    void onMoveLayerDown();
    void onLayerSelectionChanged();
    void onLayerVisibilityChanged();
    void onLayerOpacityChanged();
    void onLayerBlendModeChanged();
    void onLayerAlphaLockChanged();
    void onLayerClippingMaskChanged();

    // Canvas events
    void onCanvasModified();
    void onLayerChanged();
    void onColorPicked(const QColor& color);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupToolPanel();
    void setupCanvasPanel();
    void setupLayerPanel();
    void setupColorPanel();

    void updateWindowTitle();
    void updateLayerList();
    void updateToolStates();

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;

    // Canvas
    PixelCanvas* m_canvas;

    // Tool panel
    QWidget* m_toolPanel;
    QActionGroup* m_toolGroup;
    QAction* m_brushAction;
    QAction* m_eraserAction;
    QAction* m_bucketAction;
    QAction* m_eyedropperAction;
    QAction* m_wandAction;
    QSlider* m_brushSizeSlider;
    QSpinBox* m_brushSizeSpinBox;

    // Layer panel
    QWidget* m_layerPanel;
    QListWidget* m_layerList;
    QPushButton* m_addLayerButton;
    QPushButton* m_removeLayerButton;
    QPushButton* m_duplicateLayerButton;
    QPushButton* m_moveLayerUpButton;
    QPushButton* m_moveLayerDownButton;
    QSlider* m_layerOpacitySlider;
    QComboBox* m_layerBlendModeCombo;
    QCheckBox* m_layerAlphaLockCheck;
    QCheckBox* m_layerClippingMaskCheck;

    // Color panel
    QWidget* m_colorPanel;
    ColorWheelWidget* m_colorWheel;
    ColorPaletteWidget* m_colorPalette;
    QPushButton* m_primaryColorButton;
    QPushButton* m_secondaryColorButton;
    QPushButton* m_loadPaletteButton;
    QPushButton* m_savePaletteButton;

    // State
    QString m_currentFilePath;
    bool m_modified;
    PixelTool m_currentTool;
};
