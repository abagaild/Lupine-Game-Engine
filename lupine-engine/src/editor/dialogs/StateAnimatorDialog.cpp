#include "StateAnimatorDialog.h"
#include <QApplication>
#include <QFormLayout>
#include <QFileInfo>
#include <QInputDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <cmath>

StateAnimatorDialog::StateAnimatorDialog(QWidget* parent)
    : QDialog(parent)
    , m_stateMachineResource(std::make_unique<Lupine::StateMachineResource>())
    , m_runtime(std::make_unique<Lupine::StateMachineRuntime>())
    , m_isModified(false)
    , m_isPlaying(false)
{
    setWindowTitle("State Animator");
    setModal(false);
    resize(1600, 1000);
    
    setupUI();
    
    // Setup preview timer
    m_previewTimer = new QTimer(this);
    m_previewTimer->setInterval(PREVIEW_UPDATE_INTERVAL);
    connect(m_previewTimer, &QTimer::timeout, this, [this]() {
        if (m_isPlaying) {
            m_runtime->Update(PREVIEW_UPDATE_INTERVAL / 1000.0f);
            updatePreview();
        }
    });
    
    // Initialize with empty state machine
    NewStateMachine();
}

StateAnimatorDialog::~StateAnimatorDialog() = default;

void StateAnimatorDialog::NewStateMachine() {
    m_stateMachineResource = std::make_shared<Lupine::StateMachineResource>();
    m_runtime = std::make_unique<Lupine::StateMachineRuntime>();
    m_currentFilePath.clear();
    m_isModified = false;
    
    // Create default layer
    Lupine::StateMachineLayer defaultLayer("Base Layer");
    m_stateMachineResource->AddLayer(defaultLayer);
    m_currentLayer = "Base Layer";
    
    // Clear selections
    m_currentParameter.clear();
    m_currentState.clear();
    m_currentTransition = Lupine::UUID();
    
    clearGraph();
    updateLayerList();
    updateParameterList();
    updateStateGraph();
    updatePropertiesPanel();
    updatePreview();
    updateWindowTitle();
}

void StateAnimatorDialog::LoadStateMachine(const QString& filepath) {
    if (m_stateMachineResource->LoadFromFile(filepath.toStdString())) {
        m_currentFilePath = filepath;
        m_isModified = false;
        
        // Setup runtime
        m_runtime->SetResource(m_stateMachineResource);
        
        // Select first layer if available
        auto layerNames = m_stateMachineResource->GetLayerNames();
        if (!layerNames.empty()) {
            m_currentLayer = QString::fromStdString(layerNames[0]);
        } else {
            m_currentLayer.clear();
        }
        
        clearGraph();
        updateLayerList();
        updateParameterList();
        updateStateGraph();
        updatePropertiesPanel();
        updatePreview();
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load state machine file: " + filepath);
    }
}

void StateAnimatorDialog::SaveStateMachine() {
    if (m_currentFilePath.isEmpty()) {
        SaveStateMachineAs();
        return;
    }
    
    if (m_stateMachineResource->SaveToFile(m_currentFilePath.toStdString())) {
        m_isModified = false;
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save state machine file: " + m_currentFilePath);
    }
}

void StateAnimatorDialog::SaveStateMachineAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save State Machine",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "State Machine Files (*.statemachine);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".statemachine")) {
            filepath += ".statemachine";
        }
        
        m_currentFilePath = filepath;
        SaveStateMachine();
    }
}

void StateAnimatorDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    setupToolBar();
    setupMainPanels();
    
    setLayout(m_mainLayout);
}

void StateAnimatorDialog::setupToolBar() {
    m_toolbarLayout = new QHBoxLayout();
    
    // File operations
    m_newButton = new QPushButton("New");
    m_openButton = new QPushButton("Open");
    m_saveButton = new QPushButton("Save");
    m_saveAsButton = new QPushButton("Save As");
    
    connect(m_newButton, &QPushButton::clicked, this, &StateAnimatorDialog::onNew);
    connect(m_openButton, &QPushButton::clicked, this, &StateAnimatorDialog::onOpen);
    connect(m_saveButton, &QPushButton::clicked, this, &StateAnimatorDialog::onSave);
    connect(m_saveAsButton, &QPushButton::clicked, this, &StateAnimatorDialog::onSaveAs);
    
    m_toolbarLayout->addWidget(m_newButton);
    m_toolbarLayout->addWidget(m_openButton);
    m_toolbarLayout->addWidget(m_saveButton);
    m_toolbarLayout->addWidget(m_saveAsButton);
    m_toolbarLayout->addStretch();
    
    m_mainLayout->addLayout(m_toolbarLayout);
}

void StateAnimatorDialog::setupMainPanels() {
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel (Layers + Parameters)
    m_leftSplitter = new QSplitter(Qt::Vertical);
    setupLayerPanel();
    setupParameterPanel();
    m_leftSplitter->addWidget(m_layerGroup);
    m_leftSplitter->addWidget(m_parameterGroup);
    m_leftSplitter->setSizes({200, 300});
    
    // Center panel (Graph)
    setupGraphPanel();
    
    // Right panel (Properties + Preview)
    m_rightSplitter = new QSplitter(Qt::Vertical);
    setupPropertiesPanel();
    setupPreviewPanel();
    m_rightSplitter->addWidget(m_propertiesGroup);
    m_rightSplitter->addWidget(m_previewGroup);
    m_rightSplitter->setSizes({400, 300});
    
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_graphGroup);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setSizes({300, 800, 500});
    
    m_mainLayout->addWidget(m_mainSplitter);
}

void StateAnimatorDialog::setupLayerPanel() {
    m_layerGroup = new QGroupBox("Layers");
    m_layerLayout = new QVBoxLayout(m_layerGroup);
    
    // Layer list
    m_layerList = new QListWidget();
    connect(m_layerList, &QListWidget::currentTextChanged, this, &StateAnimatorDialog::onLayerSelectionChanged);
    
    // Layer controls
    QHBoxLayout* layerButtonLayout = new QHBoxLayout();
    m_newLayerButton = new QPushButton("New Layer");
    m_deleteLayerButton = new QPushButton("Delete Layer");
    
    connect(m_newLayerButton, &QPushButton::clicked, this, &StateAnimatorDialog::onNewLayer);
    connect(m_deleteLayerButton, &QPushButton::clicked, this, &StateAnimatorDialog::onDeleteLayer);
    
    layerButtonLayout->addWidget(m_newLayerButton);
    layerButtonLayout->addWidget(m_deleteLayerButton);
    
    // Layer properties
    QFormLayout* layerPropsLayout = new QFormLayout();
    m_layerNameEdit = new QLineEdit();
    m_layerWeightSpin = new QDoubleSpinBox();
    m_layerWeightSpin->setRange(0.0, 1.0);
    m_layerWeightSpin->setSingleStep(0.1);
    m_layerWeightSpin->setValue(DEFAULT_LAYER_WEIGHT);
    m_layerAdditiveCheck = new QCheckBox("Additive");
    
    connect(m_layerNameEdit, &QLineEdit::textChanged, this, &StateAnimatorDialog::onLayerRenamed);
    connect(m_layerWeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &StateAnimatorDialog::onLayerPropertiesChanged);
    connect(m_layerAdditiveCheck, &QCheckBox::toggled, this, &StateAnimatorDialog::onLayerPropertiesChanged);
    
    layerPropsLayout->addRow("Name:", m_layerNameEdit);
    layerPropsLayout->addRow("Weight:", m_layerWeightSpin);
    layerPropsLayout->addRow(m_layerAdditiveCheck);
    
    m_layerLayout->addWidget(m_layerList);
    m_layerLayout->addLayout(layerButtonLayout);
    m_layerLayout->addLayout(layerPropsLayout);
}

void StateAnimatorDialog::setupParameterPanel() {
    m_parameterGroup = new QGroupBox("Parameters");
    m_parameterLayout = new QVBoxLayout(m_parameterGroup);
    
    // Parameter tree
    m_parameterTree = new QTreeWidget();
    m_parameterTree->setHeaderLabels({"Name", "Type", "Value"});
    m_parameterTree->header()->setStretchLastSection(false);
    m_parameterTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    connect(m_parameterTree, &QTreeWidget::currentItemChanged, this, &StateAnimatorDialog::onParameterSelectionChanged);
    
    // Parameter controls
    QHBoxLayout* paramButtonLayout = new QHBoxLayout();
    m_newParameterButton = new QPushButton("New Parameter");
    m_deleteParameterButton = new QPushButton("Delete Parameter");
    
    connect(m_newParameterButton, &QPushButton::clicked, this, &StateAnimatorDialog::onNewParameter);
    connect(m_deleteParameterButton, &QPushButton::clicked, this, &StateAnimatorDialog::onDeleteParameter);
    
    paramButtonLayout->addWidget(m_newParameterButton);
    paramButtonLayout->addWidget(m_deleteParameterButton);
    
    // Parameter properties
    QFormLayout* paramPropsLayout = new QFormLayout();
    m_parameterNameEdit = new QLineEdit();
    m_parameterTypeCombo = new QComboBox();
    m_parameterTypeCombo->addItem("Bool", static_cast<int>(Lupine::ConditionType::Bool));
    m_parameterTypeCombo->addItem("Int", static_cast<int>(Lupine::ConditionType::Int));
    m_parameterTypeCombo->addItem("Float", static_cast<int>(Lupine::ConditionType::Float));
    m_parameterTypeCombo->addItem("Trigger", static_cast<int>(Lupine::ConditionType::Trigger));
    
    connect(m_parameterNameEdit, &QLineEdit::textChanged, this, &StateAnimatorDialog::onParameterRenamed);
    connect(m_parameterTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StateAnimatorDialog::onParameterTypeChanged);
    
    // Parameter value widget (will be populated dynamically)
    m_parameterValueWidget = new QWidget();
    m_parameterValueLayout = new QVBoxLayout(m_parameterValueWidget);
    
    paramPropsLayout->addRow("Name:", m_parameterNameEdit);
    paramPropsLayout->addRow("Type:", m_parameterTypeCombo);
    paramPropsLayout->addRow("Default Value:", m_parameterValueWidget);
    
    m_parameterLayout->addWidget(m_parameterTree);
    m_parameterLayout->addLayout(paramButtonLayout);
    m_parameterLayout->addLayout(paramPropsLayout);
}

void StateAnimatorDialog::setupGraphPanel() {
    m_graphGroup = new QGroupBox("State Graph");
    m_graphLayout = new QVBoxLayout(m_graphGroup);
    
    // Graph view
    m_graphView = new QGraphicsView();
    m_graphScene = new QGraphicsScene();
    m_graphView->setScene(m_graphScene);
    m_graphView->setDragMode(QGraphicsView::RubberBandDrag);
    m_graphView->setRenderHint(QPainter::Antialiasing);
    
    // Graph controls
    QHBoxLayout* graphButtonLayout = new QHBoxLayout();
    m_newStateButton = new QPushButton("New State");
    m_deleteStateButton = new QPushButton("Delete State");
    m_newTransitionButton = new QPushButton("New Transition");
    m_deleteTransitionButton = new QPushButton("Delete Transition");
    
    connect(m_newStateButton, &QPushButton::clicked, this, &StateAnimatorDialog::onNewState);
    connect(m_deleteStateButton, &QPushButton::clicked, this, &StateAnimatorDialog::onDeleteState);
    connect(m_newTransitionButton, &QPushButton::clicked, this, &StateAnimatorDialog::onNewTransition);
    connect(m_deleteTransitionButton, &QPushButton::clicked, this, &StateAnimatorDialog::onDeleteTransition);
    
    graphButtonLayout->addWidget(m_newStateButton);
    graphButtonLayout->addWidget(m_deleteStateButton);
    graphButtonLayout->addWidget(m_newTransitionButton);
    graphButtonLayout->addWidget(m_deleteTransitionButton);
    graphButtonLayout->addStretch();
    
    m_graphLayout->addWidget(m_graphView);
    m_graphLayout->addLayout(graphButtonLayout);
}

void StateAnimatorDialog::setupPropertiesPanel() {
    m_propertiesGroup = new QGroupBox("Properties");
    m_propertiesLayout = new QVBoxLayout(m_propertiesGroup);

    // Scroll area for properties
    m_propertiesScrollArea = new QScrollArea();
    m_propertiesWidget = new QWidget();
    m_propertiesContentLayout = new QVBoxLayout(m_propertiesWidget);
    m_propertiesScrollArea->setWidget(m_propertiesWidget);
    m_propertiesScrollArea->setWidgetResizable(true);

    // State properties
    m_statePropertiesGroup = new QGroupBox("State Properties");
    m_statePropertiesGroup->setVisible(false);
    QFormLayout* statePropsLayout = new QFormLayout(m_statePropertiesGroup);

    m_stateNameEdit = new QLineEdit();
    m_stateAnimationEdit = new QLineEdit();
    m_stateSpeedSpin = new QDoubleSpinBox();
    m_stateSpeedSpin->setRange(0.1, 10.0);
    m_stateSpeedSpin->setSingleStep(0.1);
    m_stateSpeedSpin->setValue(DEFAULT_STATE_SPEED);
    m_stateLoopingCheck = new QCheckBox("Looping");
    m_setDefaultStateButton = new QPushButton("Set as Default State");

    connect(m_stateNameEdit, &QLineEdit::textChanged, this, &StateAnimatorDialog::onStateRenamed);
    connect(m_stateAnimationEdit, &QLineEdit::textChanged, this, &StateAnimatorDialog::onStatePropertiesChanged);
    connect(m_stateSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &StateAnimatorDialog::onStatePropertiesChanged);
    connect(m_stateLoopingCheck, &QCheckBox::toggled, this, &StateAnimatorDialog::onStatePropertiesChanged);
    connect(m_setDefaultStateButton, &QPushButton::clicked, this, &StateAnimatorDialog::onSetDefaultState);

    statePropsLayout->addRow("Name:", m_stateNameEdit);
    statePropsLayout->addRow("Animation:", m_stateAnimationEdit);
    statePropsLayout->addRow("Speed:", m_stateSpeedSpin);
    statePropsLayout->addRow(m_stateLoopingCheck);
    statePropsLayout->addRow(m_setDefaultStateButton);

    // Transition properties
    m_transitionPropertiesGroup = new QGroupBox("Transition Properties");
    m_transitionPropertiesGroup->setVisible(false);
    QFormLayout* transPropsLayout = new QFormLayout(m_transitionPropertiesGroup);

    m_transitionFromLabel = new QLabel();
    m_transitionToLabel = new QLabel();
    m_transitionDurationSpin = new QDoubleSpinBox();
    m_transitionDurationSpin->setRange(0.0, 10.0);
    m_transitionDurationSpin->setSingleStep(0.05);
    m_transitionDurationSpin->setValue(DEFAULT_TRANSITION_DURATION);
    m_transitionExitTimeSpin = new QDoubleSpinBox();
    m_transitionExitTimeSpin->setRange(0.0, 1.0);
    m_transitionExitTimeSpin->setSingleStep(0.1);
    m_transitionExitTimeSpin->setValue(1.0);
    m_transitionHasExitTimeCheck = new QCheckBox("Has Exit Time");
    m_transitionCanTransitionToSelfCheck = new QCheckBox("Can Transition to Self");

    connect(m_transitionDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &StateAnimatorDialog::onTransitionPropertiesChanged);
    connect(m_transitionExitTimeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &StateAnimatorDialog::onTransitionPropertiesChanged);
    connect(m_transitionHasExitTimeCheck, &QCheckBox::toggled, this, &StateAnimatorDialog::onTransitionPropertiesChanged);
    connect(m_transitionCanTransitionToSelfCheck, &QCheckBox::toggled, this, &StateAnimatorDialog::onTransitionPropertiesChanged);

    transPropsLayout->addRow("From:", m_transitionFromLabel);
    transPropsLayout->addRow("To:", m_transitionToLabel);
    transPropsLayout->addRow("Duration:", m_transitionDurationSpin);
    transPropsLayout->addRow("Exit Time:", m_transitionExitTimeSpin);
    transPropsLayout->addRow(m_transitionHasExitTimeCheck);
    transPropsLayout->addRow(m_transitionCanTransitionToSelfCheck);

    // Transition conditions
    m_conditionsGroup = new QGroupBox("Conditions");
    m_conditionsGroup->setVisible(false);
    QVBoxLayout* conditionsLayout = new QVBoxLayout(m_conditionsGroup);

    m_conditionsTable = new QTableWidget();
    m_conditionsTable->setColumnCount(3);
    m_conditionsTable->setHorizontalHeaderLabels({"Parameter", "Operator", "Value"});
    m_conditionsTable->horizontalHeader()->setStretchLastSection(true);

    QHBoxLayout* conditionButtonLayout = new QHBoxLayout();
    m_addConditionButton = new QPushButton("Add Condition");
    m_removeConditionButton = new QPushButton("Remove Condition");

    connect(m_addConditionButton, &QPushButton::clicked, this, &StateAnimatorDialog::onAddCondition);
    connect(m_removeConditionButton, &QPushButton::clicked, this, &StateAnimatorDialog::onRemoveCondition);

    conditionButtonLayout->addWidget(m_addConditionButton);
    conditionButtonLayout->addWidget(m_removeConditionButton);

    conditionsLayout->addWidget(m_conditionsTable);
    conditionsLayout->addLayout(conditionButtonLayout);

    m_propertiesContentLayout->addWidget(m_statePropertiesGroup);
    m_propertiesContentLayout->addWidget(m_transitionPropertiesGroup);
    m_propertiesContentLayout->addWidget(m_conditionsGroup);
    m_propertiesContentLayout->addStretch();

    m_propertiesLayout->addWidget(m_propertiesScrollArea);
}

void StateAnimatorDialog::setupPreviewPanel() {
    m_previewGroup = new QGroupBox("Preview");
    m_previewLayout = new QVBoxLayout(m_previewGroup);

    // Current state display
    m_currentStateLabel = new QLabel("Current State: None");
    m_stateTimeLabel = new QLabel("State Time: 0.0s");

    // Preview controls
    QHBoxLayout* previewControlsLayout = new QHBoxLayout();
    m_playButton = new QPushButton("Play");
    m_stopButton = new QPushButton("Stop");

    connect(m_playButton, &QPushButton::clicked, this, &StateAnimatorDialog::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &StateAnimatorDialog::onStop);

    previewControlsLayout->addWidget(m_playButton);
    previewControlsLayout->addWidget(m_stopButton);
    previewControlsLayout->addStretch();

    // Parameter controls for runtime testing
    m_parameterControlsGroup = new QGroupBox("Parameter Controls");
    m_parameterControlsLayout = new QVBoxLayout(m_parameterControlsGroup);

    m_previewLayout->addWidget(m_currentStateLabel);
    m_previewLayout->addWidget(m_stateTimeLabel);
    m_previewLayout->addLayout(previewControlsLayout);
    m_previewLayout->addWidget(m_parameterControlsGroup);
    m_previewLayout->addStretch();
}

// Basic slot implementations for simplified functionality

void StateAnimatorDialog::onNew() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "New State Machine",
            "Current state machine has unsaved changes. Do you want to save before creating a new one?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveStateMachine();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    NewStateMachine();
}

void StateAnimatorDialog::onOpen() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "Open State Machine",
            "Current state machine has unsaved changes. Do you want to save before opening another?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveStateMachine();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open State Machine",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "State Machine Files (*.statemachine);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadStateMachine(filepath);
    }
}

void StateAnimatorDialog::onSave() {
    SaveStateMachine();
}

void StateAnimatorDialog::onSaveAs() {
    SaveStateMachineAs();
}

void StateAnimatorDialog::onClose() {
    close();
}

// Simplified implementations for basic functionality
void StateAnimatorDialog::onNewLayer() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Layer", "Layer name:", QLineEdit::Normal, "New Layer", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::StateMachineLayer layer(name.toStdString());
        m_stateMachineResource->AddLayer(layer);
        m_currentLayer = name;
        m_isModified = true;
        updateLayerList();
        updateStateGraph();
        updateWindowTitle();
    }
}

void StateAnimatorDialog::onDeleteLayer() {
    if (m_currentLayer.isEmpty()) return;

    int ret = QMessageBox::question(this, "Delete Layer",
        "Are you sure you want to delete the layer '" + m_currentLayer + "'?",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_stateMachineResource->RemoveLayer(m_currentLayer.toStdString());
        m_currentLayer.clear();
        m_isModified = true;
        clearGraph();
        updateLayerList();
        updateStateGraph();
        updateWindowTitle();
    }
}

void StateAnimatorDialog::onLayerSelectionChanged() {
    QListWidgetItem* item = m_layerList->currentItem();
    if (item) {
        m_currentLayer = item->text();
        clearGraph();
        updateStateGraph();
        updatePropertiesPanel();

        // Update layer properties
        const auto* layer = m_stateMachineResource->GetLayer(m_currentLayer.toStdString());
        if (layer) {
            m_layerNameEdit->setText(QString::fromStdString(layer->name));
            m_layerWeightSpin->setValue(layer->weight);
            m_layerAdditiveCheck->setChecked(layer->additive);
        }
    }
}

// Placeholder implementations for other slots
void StateAnimatorDialog::onLayerRenamed() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onLayerPropertiesChanged() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onNewParameter() { QMessageBox::information(this, "Info", "Parameter management not yet implemented."); }
void StateAnimatorDialog::onDeleteParameter() { QMessageBox::information(this, "Info", "Parameter management not yet implemented."); }
void StateAnimatorDialog::onParameterSelectionChanged() {}
void StateAnimatorDialog::onParameterRenamed() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onParameterTypeChanged() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onParameterValueChanged() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onNewState() { QMessageBox::information(this, "Info", "State management not yet implemented."); }
void StateAnimatorDialog::onDeleteState() { QMessageBox::information(this, "Info", "State management not yet implemented."); }
void StateAnimatorDialog::onStateSelectionChanged() {}
void StateAnimatorDialog::onStateRenamed() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onStatePropertiesChanged() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onSetDefaultState() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onNewTransition() { QMessageBox::information(this, "Info", "Transition management not yet implemented."); }
void StateAnimatorDialog::onDeleteTransition() { QMessageBox::information(this, "Info", "Transition management not yet implemented."); }
void StateAnimatorDialog::onTransitionSelectionChanged() {}
void StateAnimatorDialog::onTransitionPropertiesChanged() { m_isModified = true; updateWindowTitle(); }
void StateAnimatorDialog::onAddCondition() { QMessageBox::information(this, "Info", "Condition management not yet implemented."); }
void StateAnimatorDialog::onRemoveCondition() { QMessageBox::information(this, "Info", "Condition management not yet implemented."); }
void StateAnimatorDialog::onConditionChanged() { m_isModified = true; updateWindowTitle(); }

void StateAnimatorDialog::onNodeMoved(StateNode* node, const QPointF& position) {
    Q_UNUSED(node)
    Q_UNUSED(position)
    m_isModified = true;
    updateWindowTitle();
}

void StateAnimatorDialog::onNodeSelected(StateNode* node) {
    Q_UNUSED(node)
    // Update properties panel for selected state
    updatePropertiesPanel();
}

void StateAnimatorDialog::onTransitionSelected(TransitionArrow* arrow) {
    Q_UNUSED(arrow)
    // Update properties panel for selected transition
    updatePropertiesPanel();
}

void StateAnimatorDialog::onCreateTransition(StateNode* fromNode, StateNode* toNode) {
    Q_UNUSED(fromNode)
    Q_UNUSED(toNode)
    QMessageBox::information(this, "Info", "Transition creation not yet implemented.");
}

void StateAnimatorDialog::onPlayPause() {
    if (m_isPlaying) {
        m_previewTimer->stop();
        m_isPlaying = false;
        m_playButton->setText("Play");
    } else {
        m_runtime->Play();
        m_previewTimer->start();
        m_isPlaying = true;
        m_playButton->setText("Pause");
    }
}

void StateAnimatorDialog::onStop() {
    m_previewTimer->stop();
    m_runtime->Stop();
    m_isPlaying = false;
    m_playButton->setText("Play");
    updatePreview();
}

void StateAnimatorDialog::onParameterControlChanged() {
    // Update runtime parameters based on UI controls
    // This would be implemented when parameter controls are fully implemented
}

// Update methods

void StateAnimatorDialog::updateLayerList() {
    m_layerList->clear();

    auto layerNames = m_stateMachineResource->GetLayerNames();
    for (const auto& name : layerNames) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name));
        m_layerList->addItem(item);

        if (QString::fromStdString(name) == m_currentLayer) {
            m_layerList->setCurrentItem(item);
        }
    }
}

void StateAnimatorDialog::updateParameterList() {
    m_parameterTree->clear();

    auto paramNames = m_stateMachineResource->GetParameterNames();
    for (const auto& name : paramNames) {
        const auto* param = m_stateMachineResource->GetParameter(name);
        if (param) {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_parameterTree);
            item->setText(0, QString::fromStdString(param->name));

            QString typeText;
            switch (param->type) {
                case Lupine::ConditionType::Bool: typeText = "Bool"; break;
                case Lupine::ConditionType::Int: typeText = "Int"; break;
                case Lupine::ConditionType::Float: typeText = "Float"; break;
                case Lupine::ConditionType::Trigger: typeText = "Trigger"; break;
            }
            item->setText(1, typeText);

            // Display default value
            QString valueText;
            switch (param->type) {
                case Lupine::ConditionType::Bool:
                case Lupine::ConditionType::Trigger:
                    valueText = param->default_value.bool_value ? "true" : "false";
                    break;
                case Lupine::ConditionType::Int:
                    valueText = QString::number(param->default_value.int_value);
                    break;
                case Lupine::ConditionType::Float:
                    valueText = QString::number(param->default_value.float_value, 'f', 3);
                    break;
            }
            item->setText(2, valueText);
        }
    }
}

void StateAnimatorDialog::updateStateGraph() {
    clearGraph();

    if (m_currentLayer.isEmpty()) return;

    const auto* layer = m_stateMachineResource->GetLayer(m_currentLayer.toStdString());
    if (!layer) return;

    // Create state nodes
    for (const auto& state : layer->states) {
        createStateNode(state);
    }

    // Create transition arrows
    for (const auto& transition : layer->transitions) {
        createTransitionArrow(transition);
    }

    // Fit view to content
    if (!m_stateNodes.empty()) {
        m_graphView->fitInView(m_graphScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void StateAnimatorDialog::updatePropertiesPanel() {
    // Hide all property groups initially
    m_statePropertiesGroup->setVisible(false);
    m_transitionPropertiesGroup->setVisible(false);
    m_conditionsGroup->setVisible(false);

    // Show appropriate properties based on selection
    if (!m_currentState.isEmpty()) {
        m_statePropertiesGroup->setVisible(true);
        // Update state properties UI
    } else if (m_currentTransition != Lupine::UUID()) {
        m_transitionPropertiesGroup->setVisible(true);
        m_conditionsGroup->setVisible(true);
        // Update transition properties UI
    }
}

void StateAnimatorDialog::updatePreview() {
    if (!m_runtime) return;

    QString currentState = QString::fromStdString(m_runtime->GetCurrentState());
    float stateTime = m_runtime->GetCurrentStateTime();

    m_currentStateLabel->setText("Current State: " + (currentState.isEmpty() ? "None" : currentState));
    m_stateTimeLabel->setText(QString("State Time: %1s").arg(stateTime, 0, 'f', 2));

    // Update visual state in graph
    for (auto& pair : m_stateNodes) {
        pair.second->setCurrentState(pair.first == currentState);
    }
}

void StateAnimatorDialog::createStateNode(const Lupine::AnimationState& state) {
    QString stateName = QString::fromStdString(state.name);
    QString animationClip = QString::fromStdString(state.animation_clip);

    StateNode* node = new StateNode(stateName, animationClip);
    node->setPos(state.position.x, state.position.y);

    m_graphScene->addItem(node);
    m_stateNodes[stateName] = node;
}

void StateAnimatorDialog::createTransitionArrow(const Lupine::StateTransition& transition) {
    QString fromState = QString::fromStdString(transition.from_state);
    QString toState = QString::fromStdString(transition.to_state);

    StateNode* fromNode = findStateNode(fromState);
    StateNode* toNode = findStateNode(toState);

    if (fromNode && toNode) {
        TransitionArrow* arrow = new TransitionArrow(fromNode, toNode, transition.id);
        m_graphScene->addItem(arrow);
        m_transitionArrows[transition.id] = arrow;
    }
}

void StateAnimatorDialog::clearGraph() {
    m_graphScene->clear();
    m_stateNodes.clear();
    m_transitionArrows.clear();
}

StateNode* StateAnimatorDialog::findStateNode(const QString& stateName) const {
    auto it = m_stateNodes.find(stateName);
    return (it != m_stateNodes.end()) ? it->second : nullptr;
}

TransitionArrow* StateAnimatorDialog::findTransitionArrow(const Lupine::UUID& transitionId) const {
    auto it = m_transitionArrows.find(transitionId);
    return (it != m_transitionArrows.end()) ? it->second : nullptr;
}

void StateAnimatorDialog::updateWindowTitle() {
    QString title = "State Animator";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    } else {
        title += " - Untitled";
    }
    if (m_isModified) {
        title += "*";
    }
    setWindowTitle(title);
}

// Simplified StateNode implementation
StateNode::StateNode(const QString& stateName, const QString& animationClip)
    : m_stateName(stateName)
    , m_animationClip(animationClip)
    , m_isDefaultState(false)
    , m_isCurrentState(false)
    , m_isDragging(false)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
}

QRectF StateNode::boundingRect() const {
    return QRectF(0, 0, NODE_WIDTH, NODE_HEIGHT);
}

void StateNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QRectF rect = boundingRect();

    // Choose colors based on state
    QColor fillColor = Qt::lightGray;
    QColor borderColor = Qt::black;

    if (m_isCurrentState) {
        fillColor = Qt::green;
        borderColor = Qt::darkGreen;
    } else if (m_isDefaultState) {
        fillColor = Qt::yellow;
        borderColor = Qt::darkYellow;
    } else if (isSelected()) {
        fillColor = Qt::cyan;
        borderColor = Qt::blue;
    }

    // Draw node
    painter->setBrush(fillColor);
    painter->setPen(QPen(borderColor, 2));
    painter->drawRoundedRect(rect, 5, 5);

    // Draw text
    painter->setPen(Qt::black);
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);

    QRectF textRect = rect.adjusted(5, 5, -5, -25);
    painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_stateName);

    // Draw animation clip name
    font.setBold(false);
    font.setPointSize(font.pointSize() - 1);
    painter->setFont(font);

    QRectF clipRect = rect.adjusted(5, 25, -5, -5);
    painter->drawText(clipRect, Qt::AlignCenter | Qt::TextWordWrap, m_animationClip);
}

void StateNode::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStartPos = event->pos();
    }
    QGraphicsItem::mousePressEvent(event);
}

void StateNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isDragging) {
        QPointF newPos = event->scenePos() - m_dragStartPos;
        setPos(newPos);
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void StateNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant StateNode::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        // Update any connected transition arrows
        // This would be implemented when transitions are fully supported
    }
    return QGraphicsItem::itemChange(change, value);
}

// Simplified TransitionArrow implementation
TransitionArrow::TransitionArrow(StateNode* fromNode, StateNode* toNode, const Lupine::UUID& transitionId)
    : m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_transitionId(transitionId)
{
    setFlag(ItemIsSelectable);
    updatePosition();
}

QRectF TransitionArrow::boundingRect() const {
    return QRectF(m_startPoint, m_endPoint).normalized().adjusted(-10, -10, 10, 10);
}

void TransitionArrow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QPen pen(Qt::black, 2);
    if (isSelected()) {
        pen.setColor(Qt::blue);
        pen.setWidth(3);
    }

    painter->setPen(pen);
    painter->drawLine(m_startPoint, m_endPoint);

    // Draw arrow head
    QPointF arrowHead1 = m_endPoint + QPointF(-10, -5);
    QPointF arrowHead2 = m_endPoint + QPointF(-10, 5);
    painter->drawLine(m_endPoint, arrowHead1);
    painter->drawLine(m_endPoint, arrowHead2);
}

void TransitionArrow::updatePosition() {
    if (m_fromNode && m_toNode) {
        QRectF fromRect = m_fromNode->boundingRect().translated(m_fromNode->pos());
        QRectF toRect = m_toNode->boundingRect().translated(m_toNode->pos());

        m_startPoint = fromRect.center();
        m_endPoint = toRect.center();

        update();
    }
}

void TransitionArrow::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event)
    setSelected(true);
}


