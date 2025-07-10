#pragma once

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
#include <QGraphicsProxyWidget>
#include <QCheckBox>
#include <QFormLayout>
#include <memory>
#include "lupine/resources/StateMachineResource.h"

class StateNode;
class TransitionArrow;

/**
 * @brief Dialog for creating and editing state machine animations (.statemachine files)
 * 
 * This dialog provides a Unity-style state machine editor with:
 * - Visual node graph for states and transitions
 * - Parameter management (bool, int, float, trigger)
 * - Transition conditions and timing
 * - Layer support for complex animations
 * - Real-time state machine preview
 */
class StateAnimatorDialog : public QDialog {
    Q_OBJECT

public:
    explicit StateAnimatorDialog(QWidget* parent = nullptr);
    ~StateAnimatorDialog();
    
    // Resource management
    void NewStateMachine();
    void LoadStateMachine(const QString& filepath);
    void SaveStateMachine();
    void SaveStateMachineAs();

private slots:
    // File operations
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onClose();
    
    // Layer management
    void onNewLayer();
    void onDeleteLayer();
    void onLayerSelectionChanged();
    void onLayerRenamed();
    void onLayerPropertiesChanged();
    
    // Parameter management
    void onNewParameter();
    void onDeleteParameter();
    void onParameterSelectionChanged();
    void onParameterRenamed();
    void onParameterTypeChanged();
    void onParameterValueChanged();
    
    // State management
    void onNewState();
    void onDeleteState();
    void onStateSelectionChanged();
    void onStateRenamed();
    void onStatePropertiesChanged();
    void onSetDefaultState();
    
    // Transition management
    void onNewTransition();
    void onDeleteTransition();
    void onTransitionSelectionChanged();
    void onTransitionPropertiesChanged();
    void onAddCondition();
    void onRemoveCondition();
    void onConditionChanged();
    
    // Graph interaction
    void onNodeMoved(StateNode* node, const QPointF& position);
    void onNodeSelected(StateNode* node);
    void onTransitionSelected(TransitionArrow* arrow);
    void onCreateTransition(StateNode* fromNode, StateNode* toNode);
    
    // Preview control
    void onPlayPause();
    void onStop();
    void onParameterControlChanged();

private:
    void setupUI();
    void setupMainPanels();
    void setupToolBar();
    void setupLayerPanel();
    void setupParameterPanel();
    void setupGraphPanel();
    void setupPropertiesPanel();
    void setupPreviewPanel();
    
    void updateLayerList();
    void updateParameterList();
    void updateStateGraph();
    void updatePropertiesPanel();
    void updatePreview();
    
    void createStateNode(const Lupine::AnimationState& state);
    void createTransitionArrow(const Lupine::StateTransition& transition);
    void clearGraph();
    
    // Utility functions
    StateNode* findStateNode(const QString& stateName) const;
    TransitionArrow* findTransitionArrow(const Lupine::UUID& transitionId) const;
    void updateWindowTitle();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    QSplitter* m_rightSplitter;
    
    // Toolbar
    QPushButton* m_newButton;
    QPushButton* m_openButton;
    QPushButton* m_saveButton;
    QPushButton* m_saveAsButton;
    
    // Layer panel
    QGroupBox* m_layerGroup;
    QVBoxLayout* m_layerLayout;
    QListWidget* m_layerList;
    QPushButton* m_newLayerButton;
    QPushButton* m_deleteLayerButton;
    QLineEdit* m_layerNameEdit;
    QDoubleSpinBox* m_layerWeightSpin;
    QCheckBox* m_layerAdditiveCheck;
    
    // Parameter panel
    QGroupBox* m_parameterGroup;
    QVBoxLayout* m_parameterLayout;
    QTreeWidget* m_parameterTree;
    QPushButton* m_newParameterButton;
    QPushButton* m_deleteParameterButton;
    QLineEdit* m_parameterNameEdit;
    QComboBox* m_parameterTypeCombo;
    QWidget* m_parameterValueWidget;
    QVBoxLayout* m_parameterValueLayout;
    
    // Graph panel
    QGroupBox* m_graphGroup;
    QVBoxLayout* m_graphLayout;
    QGraphicsView* m_graphView;
    QGraphicsScene* m_graphScene;
    QPushButton* m_newStateButton;
    QPushButton* m_deleteStateButton;
    QPushButton* m_newTransitionButton;
    QPushButton* m_deleteTransitionButton;
    
    // Properties panel
    QGroupBox* m_propertiesGroup;
    QVBoxLayout* m_propertiesLayout;
    QScrollArea* m_propertiesScrollArea;
    QWidget* m_propertiesWidget;
    QVBoxLayout* m_propertiesContentLayout;
    
    // State properties
    QGroupBox* m_statePropertiesGroup;
    QLineEdit* m_stateNameEdit;
    QLineEdit* m_stateAnimationEdit;
    QDoubleSpinBox* m_stateSpeedSpin;
    QCheckBox* m_stateLoopingCheck;
    QPushButton* m_setDefaultStateButton;
    
    // Transition properties
    QGroupBox* m_transitionPropertiesGroup;
    QLabel* m_transitionFromLabel;
    QLabel* m_transitionToLabel;
    QDoubleSpinBox* m_transitionDurationSpin;
    QDoubleSpinBox* m_transitionExitTimeSpin;
    QCheckBox* m_transitionHasExitTimeCheck;
    QCheckBox* m_transitionCanTransitionToSelfCheck;
    
    // Transition conditions
    QGroupBox* m_conditionsGroup;
    QTableWidget* m_conditionsTable;
    QPushButton* m_addConditionButton;
    QPushButton* m_removeConditionButton;
    
    // Preview panel
    QGroupBox* m_previewGroup;
    QVBoxLayout* m_previewLayout;
    QLabel* m_currentStateLabel;
    QLabel* m_stateTimeLabel;
    QPushButton* m_playButton;
    QPushButton* m_stopButton;
    
    // Parameter controls for preview
    QGroupBox* m_parameterControlsGroup;
    QVBoxLayout* m_parameterControlsLayout;
    std::map<QString, QWidget*> m_parameterControls;
    
    // Data
    std::shared_ptr<Lupine::StateMachineResource> m_stateMachineResource;
    std::unique_ptr<Lupine::StateMachineRuntime> m_runtime;
    QString m_currentFilePath;
    bool m_isModified;
    
    // Current selections
    QString m_currentLayer;
    QString m_currentParameter;
    QString m_currentState;
    Lupine::UUID m_currentTransition;
    
    // Graph nodes and arrows
    std::map<QString, StateNode*> m_stateNodes;
    std::map<Lupine::UUID, TransitionArrow*> m_transitionArrows;
    
    // Preview
    QTimer* m_previewTimer;
    bool m_isPlaying;
    
    // Constants
    static constexpr float DEFAULT_LAYER_WEIGHT = 1.0f;
    static constexpr float DEFAULT_STATE_SPEED = 1.0f;
    static constexpr float DEFAULT_TRANSITION_DURATION = 0.25f;
    static constexpr int PREVIEW_UPDATE_INTERVAL = 16; // ~60 FPS
};

/**
 * @brief Custom graphics item for state nodes
 */
class StateNode : public QGraphicsItem {
public:
    StateNode(const QString& stateName, const QString& animationClip);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    QString getStateName() const { return m_stateName; }
    void setStateName(const QString& name) { m_stateName = name; update(); }
    
    QString getAnimationClip() const { return m_animationClip; }
    void setAnimationClip(const QString& clip) { m_animationClip = clip; update(); }
    
    bool isDefaultState() const { return m_isDefaultState; }
    void setDefaultState(bool isDefault) { m_isDefaultState = isDefault; update(); }
    
    bool isCurrentState() const { return m_isCurrentState; }
    void setCurrentState(bool isCurrent) { m_isCurrentState = isCurrent; update(); }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString m_stateName;
    QString m_animationClip;
    bool m_isDefaultState;
    bool m_isCurrentState;
    bool m_isDragging;
    QPointF m_dragStartPos;
    
    static constexpr float NODE_WIDTH = 120.0f;
    static constexpr float NODE_HEIGHT = 60.0f;
};

/**
 * @brief Custom graphics item for transition arrows
 */
class TransitionArrow : public QGraphicsItem {
public:
    TransitionArrow(StateNode* fromNode, StateNode* toNode, const Lupine::UUID& transitionId);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    StateNode* getFromNode() const { return m_fromNode; }
    StateNode* getToNode() const { return m_toNode; }
    Lupine::UUID getTransitionId() const { return m_transitionId; }
    
    void updatePosition();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    StateNode* m_fromNode;
    StateNode* m_toNode;
    Lupine::UUID m_transitionId;
    QPointF m_startPoint;
    QPointF m_endPoint;
};
