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
#include <QtWidgets/QDoubleSpinBox>
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

// Forward declarations
class ColorWheelWidget;
class ColorPaletteWidget;

/**
 * @brief Drawing tools for the scribbler
 */
enum class ScribblerTool {
    Pen,            // Hard-edged drawing tool
    Brush,          // Soft-edged drawing tool with anti-aliasing
    Eraser,         // Erase pixels
    Bucket,         // Flood fill
    Eyedropper,     // Pick color
    Smudge          // Smudge/blend colors
};

/**
 * @brief Brush types for different effects
 */
enum class BrushType {
    Round,          // Circular brush
    Square,         // Square brush
    Soft,           // Soft round brush
    Texture         // Textured brush
};

/**
 * @brief Eraser modes for different erasing effects
 */
enum class EraserMode {
    Normal,         // Standard eraser - removes pixels
    Background,     // Erase to background color
    Soft            // Soft eraser with opacity
};

/**
 * @brief Blend modes for layers
 */
enum class RasterBlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    SoftLight,
    HardLight,
    ColorDodge,
    ColorBurn
};

/**
 * @brief Represents a single layer in the raster art
 */
class RasterLayer {
public:
    RasterLayer(const QString& name, int width, int height);
    ~RasterLayer();

    // Properties
    QString GetName() const { return m_name; }
    void SetName(const QString& name) { m_name = name; }
    
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float opacity) { m_opacity = opacity; }
    
    RasterBlendMode GetBlendMode() const { return m_blendMode; }
    void SetBlendMode(RasterBlendMode mode) { m_blendMode = mode; }
    
    bool IsAlphaLocked() const { return m_alphaLocked; }
    void SetAlphaLocked(bool locked) { m_alphaLocked = locked; }

    // Image data
    QImage& GetImage() { return m_image; }
    const QImage& GetImage() const { return m_image; }

    // Layer mask
    bool HasMask() const { return !m_mask.isNull(); }
    QImage& GetMask() { return m_mask; }
    const QImage& GetMask() const { return m_mask; }
    void CreateMask();
    void DeleteMask();
    void ApplyMask();

    void Clear();
    void Resize(int width, int height);

private:
    QString m_name;
    QImage m_image;
    QImage m_mask;  // Layer mask (grayscale)
    bool m_visible;
    float m_opacity;
    RasterBlendMode m_blendMode;
    bool m_alphaLocked;
};

/**
 * @brief Custom graphics view for raster art editing
 */
class ScribblerCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit ScribblerCanvas(QWidget* parent = nullptr);
    ~ScribblerCanvas();

    // Canvas properties
    void SetCanvasSize(int width, int height);
    QSize GetCanvasSize() const { return m_canvasSize; }
    
    void SetCurrentTool(ScribblerTool tool) { m_currentTool = tool; }
    ScribblerTool GetCurrentTool() const { return m_currentTool; }
    
    void SetBrushSize(float size) { m_brushSize = size; }
    float GetBrushSize() const { return m_brushSize; }
    
    void SetBrushHardness(float hardness) { m_brushHardness = hardness; }
    float GetBrushHardness() const { return m_brushHardness; }
    
    void SetBrushOpacity(float opacity) { m_brushOpacity = opacity; }
    float GetBrushOpacity() const { return m_brushOpacity; }

    void SetBrushType(BrushType type) { m_brushType = type; }
    BrushType GetBrushType() const { return m_brushType; }

    void SetEraserMode(EraserMode mode) { m_eraserMode = mode; }
    EraserMode GetEraserMode() const { return m_eraserMode; }

    void SetFillTolerance(float tolerance) { m_fillTolerance = tolerance; }
    float GetFillTolerance() const { return m_fillTolerance; }

    void SetBrushSpacing(float spacing) { m_brushSpacing = spacing; }
    float GetBrushSpacing() const { return m_brushSpacing; }

    void SetAntiAliasing(bool enabled) { m_antiAliasing = enabled; }
    bool GetAntiAliasing() const { return m_antiAliasing; }
    
    void SetPrimaryColor(const QColor& color) { m_primaryColor = color; }
    QColor GetPrimaryColor() const { return m_primaryColor; }
    
    void SetSecondaryColor(const QColor& color) { m_secondaryColor = color; }
    QColor GetSecondaryColor() const { return m_secondaryColor; }

    // Layer management
    void AddLayer(const QString& name);
    void RemoveLayer(int index);
    void SetActiveLayer(int index);
    int GetActiveLayerIndex() const { return m_activeLayerIndex; }
    RasterLayer* GetActiveLayer();
    RasterLayer* GetLayer(int index);
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
    void tabletEvent(QTabletEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSceneChanged();

private:
    void setupScene();
    void updateCanvas();
    void compositeImage();
    QPointF screenToCanvas(const QPoint& screenPos) const;
    QPoint canvasToScreen(const QPointF& canvasPos) const;
    
    // Drawing operations
    void drawStroke(const QPointF& pos, float pressure = 1.0f);
    void drawBrushStroke(QImage& image, const QPointF& pos, float size, const QColor& color, float pressure);
    void drawPenStroke(QImage& image, const QPointF& pos, float size, const QColor& color);
    void drawEraserStroke(QImage& image, const QPointF& pos, float size);
    void drawSmudgeStroke(const QPointF& pos);
    void applySmudgeEffect(QImage& image, const QPointF& pos, float size, float strength);
    QColor calculateAverageColor(const std::vector<QColor>& colors, const std::vector<QPoint>& positions, const QPoint& center, float radius);
    QColor blendColors(const QColor& color1, const QColor& color2, float factor);
    void drawInterpolatedLine(QImage& image, const QPointF& start, const QPointF& end, const QImage& brushStamp, bool isEraser = false);
    void applyBrushStamp(QImage& image, const QPointF& pos, const QImage& brushStamp, bool isEraser = false);
    QImage createBrushStamp(float size, float hardness, const QColor& color, float opacity);
    void floodFill(int x, int y, const QColor& fillColor);
    bool colorMatch(const QColor& color1, const QColor& color2, float tolerance);
    QColor pickColor(int x, int y);
    QBrush createBrush(float size, float hardness, const QColor& color);

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_canvasItem;
    
    QSize m_canvasSize;
    ScribblerTool m_currentTool;
    float m_brushSize;
    float m_brushHardness;
    float m_brushOpacity;
    BrushType m_brushType;
    EraserMode m_eraserMode;
    float m_fillTolerance;
    float m_brushSpacing;
    bool m_antiAliasing;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    bool m_drawing;
    QPointF m_lastDrawPos;
    
    std::vector<std::unique_ptr<RasterLayer>> m_layers;
    int m_activeLayerIndex;
    QImage m_compositeImage;
    
    // Stroke data for smooth drawing
    QVector<QPointF> m_currentStroke;
    QTimer* m_strokeTimer;

    // Pressure sensitivity
    float m_currentPressure;
    bool m_pressureEnabled;
};

/**
 * @brief Dialog for creating and editing raster art
 * 
 * This dialog provides a complete raster art creation environment with:
 * - Multi-layer support with advanced blend modes
 * - Pen and brush tools with anti-aliasing options
 * - Pressure sensitivity simulation
 * - Advanced brush settings (hardness, opacity, type)
 * - Export to multiple formats
 */
class ScribblerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScribblerDialog(QWidget* parent = nullptr);
    ~ScribblerDialog();

    /**
     * @brief Load an image file into the canvas
     * @param filepath Path to image file
     * @return True if successful
     */
    bool LoadImage(const QString& filepath);

    /**
     * @brief Get the canvas widget
     * @return Pointer to canvas widget
     */
    ScribblerCanvas* GetCanvas() const { return m_canvas; }

signals:
    /**
     * @brief Emitted when an image is saved
     * @param filepath Path where image was saved
     */
    void imageSaved(const QString& filepath);

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
    void onBrushHardnessChanged();
    void onBrushOpacityChanged();
    void onBrushSpacingChanged();
    void onBrushTypeChanged();
    void onAntiAliasingChanged();
    void onPrimaryColorChanged();
    void onSecondaryColorChanged();
    void onColorWheelChanged(const QColor& color);
    void onPaletteColorSelected(const QColor& color);

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
    void onAddLayerMask();
    void onDeleteLayerMask();
    void onApplyLayerMask();

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
    ScribblerCanvas* m_canvas;

    // Tool panel
    QWidget* m_toolPanel;
    QActionGroup* m_toolGroup;
    QAction* m_penAction;
    QAction* m_brushAction;
    QAction* m_eraserAction;
    QAction* m_bucketAction;
    QAction* m_eyedropperAction;
    QAction* m_smudgeAction;
    
    // Brush settings
    QSlider* m_brushSizeSlider;
    QDoubleSpinBox* m_brushSizeSpinBox;
    QSlider* m_brushHardnessSlider;
    QDoubleSpinBox* m_brushHardnessSpinBox;
    QSlider* m_brushOpacitySlider;
    QDoubleSpinBox* m_brushOpacitySpinBox;
    QSlider* m_brushSpacingSlider;
    QDoubleSpinBox* m_brushSpacingSpinBox;
    QComboBox* m_brushTypeCombo;
    QCheckBox* m_antiAliasingCheck;

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
    QPushButton* m_addMaskButton;
    QPushButton* m_deleteMaskButton;
    QPushButton* m_applyMaskButton;

    // Color panel
    QWidget* m_colorPanel;
    QPushButton* m_primaryColorButton;
    QPushButton* m_secondaryColorButton;
    ColorWheelWidget* m_colorWheel;
    ColorPaletteWidget* m_colorPalette;

    // State
    QString m_currentFilePath;
    bool m_modified;
    ScribblerTool m_currentTool;
};
