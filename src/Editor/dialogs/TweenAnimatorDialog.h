#pragma once

#include <QMainWindow>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QSlider>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QCheckBox>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QRubberBand>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <memory>
#include "lupine/resources/AnimationResource.h"
#include "lupine/core/Scene.h"
#include "lupine/animation/PropertySystem.h"
#include "lupine/animation/AutokeySystem.h"

/**
 * @brief Autokey recording modes
 */
enum class AutokeyMode {
    All,        // Record all property changes
    Selected,   // Record only selected properties
    Modified    // Record only modified properties
};

/**
 * @brief Keyframe data for clipboard operations
 */
struct KeyframeData {
    QString nodePath;
    QString propertyName;
    float time;
    QVariant value;
    Lupine::InterpolationType interpolation;
};

/**
 * @brief Enhanced dialog for creating and editing tween animations (.anim files)
 *
 * This dialog provides a timeline-based interface for creating keyframe animations
 * with advanced features including:
 * - Dockable/undockable panels for modular workflow
 * - Dynamic property access/serialization/animation for any node or component property
 * - Enhanced timeline with better keyframe operations and rendering
 * - Blender-style autokeyer for automatic keyframe recording
 * - Proper duration control and timeline management
 * - Real-time animation preview
 * - Save/load .anim resource files
 */
class TweenAnimatorDialog : public QMainWindow {
    Q_OBJECT

public:
    explicit TweenAnimatorDialog(QWidget* parent = nullptr);
    ~TweenAnimatorDialog();
    
    // Resource management
    void SetScene(Lupine::Scene* scene);
    void NewAnimation();
    void LoadAnimation(const QString& filepath);
    void SaveAnimation();
    void SaveAnimationAs();

    // Autokey configuration
    void SetAutokeyMode(Lupine::AutokeyMode mode);
    Lupine::AutokeyMode GetAutokeyMode() const;
    void EnableAutokey(bool enabled = true);
    bool IsAutokeyEnabled() const;

    // Panel management
    void ShowPanel(const QString& panelName, bool show = true);
    void DockPanel(const QString& panelName, Qt::DockWidgetArea area);
    void UndockPanel(const QString& panelName);
    bool IsPanelDocked(const QString& panelName) const;

    // Timeline control
    void SetTimelineDuration(float duration);
    float GetTimelineDuration() const;
    void SetTimelineScale(float scale);
    float GetTimelineScale() const;

private slots:
    // Animation management
    void onNewClip();
    void onDeleteClip();
    void onClipSelectionChanged();
    void onClipRenamed();
    
    // Track management
    void onAddTrack();
    void onDeleteTrack();
    void onTrackSelectionChanged();
    
    // Keyframe management
    void onAddKeyframe();
    void onDeleteKeyframe();
    void onKeyframeSelectionChanged();
    void onKeyframeValueChanged();
    void onInterpolationChanged();
    
    // Timeline control
    void onTimelinePositionChanged(float time);
    void onPlayPause();
    void onStop();
    void onLoop();
    void onTimeChanged();
    void onTimelineSelectionChanged();
    void onTimelineClicked(QPointF position);
    void onKeyframeDragged(int trackIndex, int keyframeIndex, float newTime);
    void onTimelineZoomIn();
    void onTimelineZoomOut();
    void onTimelineZoomReset();
    
    // Property editing
    void onPropertyValueChanged();
    void onNodePathChanged();
    void onPropertyNameChanged();
    
    // File operations
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onClose();

    // Autokey operations
    void onAutokeyToggled(bool enabled);
    void onAutokeyModeChanged(int mode);
    void onRecordKeyframe();
    void onRecordAllKeyframes();
    void onPropertyChanged(const QString& nodePath, const QString& propertyName, float time);

    // Enhanced timeline operations
    void onKeyframeSelected(const QList<int>& keyframes);
    void onKeyframesMoved(const QList<int>& keyframes, float deltaTime);
    void onKeyframesCopied();
    void onKeyframesPasted();
    void onKeyframesDeleted();
    void onTimelineRightClick(const QPointF& position);
    void onTimelineDoubleClick(const QPointF& position);

    // Property filtering and discovery
    void onPropertyFilterChanged();
    void onRefreshProperties();
    void onPropertySelectionChanged();

    // Panel management
    void onPanelVisibilityChanged(const QString& panelName, bool visible);
    void onPanelDockStateChanged(const QString& panelName, bool docked);



private:
    void setupUI();
    void setupMainPanels();
    void setupMenuBar();
    void setupToolBar();
    void setupDockablePanels();
    void setupAnimationPanel();
    void setupTrackPanel();
    void setupTimelinePanel();
    void setupPropertyPanel();
    void setupPreviewPanel();
    void setupAutokeyPanel();

    // Helper methods for enhanced functionality
    void connectPropertyMonitoring();
    void disconnectPropertyMonitoring();
    void recordNodeKeyframe(Lupine::Node* node, float time);
    void recordPropertyKeyframe(Lupine::Node* node, const QString& propertyName, float time);
    bool hasAnimatableProperties(Lupine::Node* node);
    bool isPropertyModified(Lupine::Node* node, const QString& propertyName);
    void updateTimelineDisplay();
    void setModified(bool modified);
    void updateKeyframeSelection();
    void updateKeyframeProperties(int keyframeId);
    void moveKeyframe(int keyframeId, float deltaTime);
    std::optional<KeyframeData> getKeyframeData(int keyframeId);
    int pasteKeyframe(const KeyframeData& data, float baseTime);
    void deleteKeyframe(int keyframeId);
    void setCurrentTime(float time);
    void setTimelineScale(float scale);
    void fitTimelineToContent();
    void refreshPropertyList();
    void updatePropertyDetails();
    QDockWidget* findDockWidget(const QString& panelName);
    void updatePanelMenus();
    void savePanelState();
    void setupEnhancedTimeline();
    
    void updateClipList();
    void updateTrackList();
    void updateKeyframeList();
    void updatePropertyEditor();
    void updateTimeline();
    void updatePreview();
    void updateAutokeyControls();
    void updatePropertyTree();
    void updateEnhancedTimeline();
    
    void populateNodePaths();
    void populatePropertyNames(const QString& nodePath);
    void populatePropertyTree();
    void populatePropertyTree(Lupine::Node* node, QTreeWidgetItem* parentItem = nullptr);
    void filterProperties();

    // Dynamic property access
    std::vector<Lupine::PropertyDescriptor> discoverNodeProperties(Lupine::Node* node);
    Lupine::EnhancedAnimationValue getPropertyValue(Lupine::Node* node, const std::string& propertyName);
    bool setPropertyValue(Lupine::Node* node, const std::string& propertyName, const Lupine::EnhancedAnimationValue& value);
    bool isPropertyAnimatable(const Lupine::PropertyDescriptor& desc);
    
    // Animation playback
    void startPlayback();
    void stopPlayback();
    void updatePlayback();
    
    // Enhanced timeline rendering
    void renderTimeline();
    void renderTracks();
    void renderKeyframes();
    void renderPlayhead();
    void renderTimelineGrid();
    void renderTimelineRuler();
    void renderKeyframeSelection();
    void renderKeyframeCurves();

    // Timeline interaction
    void handleTimelineClick(const QPointF& position);
    void handleTimelineDrag(const QPointF& startPos, const QPointF& currentPos);
    void handleKeyframeSelection(const QRectF& selectionRect);
    void handleKeyframeDrag(const QList<int>& keyframes, float deltaTime);
    
    // Utility functions
    QString getNodePathFromScene(Lupine::Node* node, const QString& basePath = "");
    std::vector<std::string> getAnimatableProperties(const QString& nodePath);
    Lupine::AnimationPropertyType getPropertyType(const QString& nodePath, const QString& propertyName);
    Lupine::AnimationValue getCurrentPropertyValue(const QString& nodePath, const QString& propertyName);
    void setPropertyValue(const QString& nodePath, const QString& propertyName, const Lupine::AnimationValue& value);
    Lupine::Node* findNodeByPath(const QString& nodePath);
    Lupine::AnimationValue interpolateValue(const Lupine::AnimationValue& a, const Lupine::AnimationValue& b, float t);
    Lupine::AnimationValue interpolateValueWithEasing(const Lupine::AnimationValue& a, const Lupine::AnimationValue& b, float t, Lupine::InterpolationType interpolation);
    float applyEasing(float t, Lupine::InterpolationType interpolation);
    Lupine::AnimationValue getValueFromPropertyEditor();
    void setValueInPropertyEditor(const Lupine::AnimationValue& value);
    void addBoneProperties(std::vector<std::string>& properties, const Lupine::Component* skeletonComponent);
    void updateWindowTitle();
    float getCurrentClipDuration() const;
    
    // UI Components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    QSplitter* m_rightSplitter;

    // Dockable panels
    QDockWidget* m_animationDock;
    QDockWidget* m_trackDock;
    QDockWidget* m_timelineDock;
    QDockWidget* m_propertyDock;
    QDockWidget* m_previewDock;
    QDockWidget* m_autokeyDock;
    
    // Animation panel
    QGroupBox* m_animationGroup;
    QVBoxLayout* m_animationLayout;
    QListWidget* m_clipList;
    QPushButton* m_newClipButton;
    QPushButton* m_deleteClipButton;
    QLineEdit* m_clipNameEdit;
    QDoubleSpinBox* m_clipDurationSpin;
    QCheckBox* m_clipLoopingCheck;
    
    // Track panel
    QGroupBox* m_trackGroup;
    QVBoxLayout* m_trackLayout;
    QTreeWidget* m_trackTree;
    QPushButton* m_addTrackButton;
    QPushButton* m_deleteTrackButton;
    
    // Timeline panel
    QGroupBox* m_timelineGroup;
    QVBoxLayout* m_timelineLayout;
    QGraphicsView* m_timelineView;
    QGraphicsScene* m_timelineScene;
    QScrollArea* m_timelineScrollArea;
    QWidget* m_timelineWidget;
    
    // Timeline controls
    QHBoxLayout* m_timelineControlsLayout;
    QPushButton* m_playButton;
    QPushButton* m_stopButton;
    QPushButton* m_loopButton;
    QSlider* m_timeSlider;
    QDoubleSpinBox* m_currentTimeSpin;
    QLabel* m_totalTimeLabel;
    
    // Property panel
    QGroupBox* m_propertyGroup;
    QVBoxLayout* m_propertyLayout;
    QComboBox* m_nodePathCombo;
    QComboBox* m_propertyNameCombo;
    QComboBox* m_interpolationCombo;
    QWidget* m_valueWidget;
    QVBoxLayout* m_valueLayout;
    
    // Keyframe list
    QTableWidget* m_keyframeTable;
    QPushButton* m_addKeyframeButton;
    QPushButton* m_deleteKeyframeButton;
    
    // Preview panel
    QGroupBox* m_previewGroup;
    QVBoxLayout* m_previewLayout;
    QLabel* m_previewLabel;
    
    // File operations
    QPushButton* m_newButton;
    QPushButton* m_openButton;
    QPushButton* m_saveButton;
    QPushButton* m_saveAsButton;
    
    // Data
    std::unique_ptr<Lupine::TweenAnimationResource> m_animationResource;
    Lupine::Scene* m_scene;
    QString m_currentFilePath;
    bool m_isModified;

    // Enhanced animation systems
    std::unique_ptr<Lupine::AutokeySystem> m_autokeySystem;
    std::unique_ptr<Lupine::PropertyReflectionSystem> m_reflectionSystem;
    std::unique_ptr<Lupine::PropertyStateManager> m_stateManager;
    std::unique_ptr<Lupine::PropertyInterpolator> m_interpolator;
    
    // Animation playback
    QTimer* m_playbackTimer;
    float m_currentTime;
    float m_playbackSpeed;
    bool m_isPlaying;
    bool m_isLooping;
    
    // Current selection
    QString m_currentClip;
    QString m_currentTrack;
    int m_currentKeyframe;
    
    // Timeline rendering
    float m_timelineScale;
    float m_timelineOffset;
    int m_trackHeight;
    int m_keyframeSize;

    // Autokey system
    bool m_autokeyEnabled;
    AutokeyMode m_autokeyMode;
    QPushButton* m_autokeyButton;
    QComboBox* m_autokeyModeCombo;
    QLabel* m_statusLabel;

    // Selection and clipboard
    QList<Lupine::Node*> m_selectedNodes;
    QList<int> m_selectedKeyframes;
    QList<QString> m_selectedProperties;
    QList<KeyframeData> m_keyframeClipboard;

    // Property management
    QLineEdit* m_propertyFilterEdit;
    QListWidget* m_propertyList;
    QString m_propertyFilter;
    QMap<QString, QVariant> m_cachedProperties;

    // Keyframe operations
    QPushButton* m_copyKeyframeButton;
    QPushButton* m_pasteKeyframeButton;

    // Constants
    static constexpr float DEFAULT_CLIP_DURATION = 1.0f;
    static constexpr float MIN_TIMELINE_SCALE = 0.1f;
    static constexpr float MAX_TIMELINE_SCALE = 10.0f;
    static constexpr int DEFAULT_TRACK_HEIGHT = 24;
    static constexpr int DEFAULT_KEYFRAME_SIZE = 8;
    static constexpr int TIMELINE_UPDATE_INTERVAL = 16; // ~60 FPS
};

/**
 * @brief Custom graphics item for timeline keyframes
 */
class TimelineKeyframeItem : public QGraphicsItem {
public:
    TimelineKeyframeItem(float time, const Lupine::AnimationValue& value, Lupine::InterpolationType interpolation);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    float getTime() const { return m_time; }
    void setTime(float time) { m_time = time; }
    
    const Lupine::AnimationValue& getValue() const { return m_value; }
    void setValue(const Lupine::AnimationValue& value) { m_value = value; }
    
    Lupine::InterpolationType getInterpolation() const { return m_interpolation; }
    void setInterpolation(Lupine::InterpolationType interpolation) { m_interpolation = interpolation; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    float m_time;
    Lupine::AnimationValue m_value;
    Lupine::InterpolationType m_interpolation;
    bool m_isDragging;
    QPointF m_dragStartPos;
};

/**
 * @brief Custom graphics item for timeline tracks
 */
class TimelineTrackItem : public QGraphicsItem {
public:
    TimelineTrackItem(const QString& nodePath, const QString& propertyName);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    void addKeyframe(TimelineKeyframeItem* keyframe);
    void removeKeyframe(TimelineKeyframeItem* keyframe);
    void clearKeyframes();
    
    const QString& getNodePath() const { return m_nodePath; }
    const QString& getPropertyName() const { return m_propertyName; }

private:
    QString m_nodePath;
    QString m_propertyName;
    std::vector<TimelineKeyframeItem*> m_keyframes;
};
