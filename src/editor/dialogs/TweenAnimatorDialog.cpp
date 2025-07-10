#include "TweenAnimatorDialog.h"
#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QCheckBox>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QColorDialog>
#include <QStandardPaths>
#include <QDir>
#include <QInputDialog>
#include <QFormLayout>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cmath>
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"

TweenAnimatorDialog::TweenAnimatorDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_animationResource(std::make_unique<Lupine::TweenAnimationResource>())
    , m_scene(nullptr)
    , m_isModified(false)
    , m_currentTime(0.0f)
    , m_playbackSpeed(1.0f)
    , m_isPlaying(false)
    , m_isLooping(false)
    , m_currentKeyframe(-1)
    , m_timelineScale(1.0f)
    , m_timelineOffset(0.0f)
    , m_trackHeight(DEFAULT_TRACK_HEIGHT)
    , m_keyframeSize(DEFAULT_KEYFRAME_SIZE)
{
    setWindowTitle("Tween Animator");
    resize(1200, 800);
    
    setupUI();
    
    // Setup playback timer
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(TIMELINE_UPDATE_INTERVAL);
    connect(m_playbackTimer, &QTimer::timeout, this, &TweenAnimatorDialog::updatePlayback);
    
    // Initialize with a default clip
    NewAnimation();
}

TweenAnimatorDialog::~TweenAnimatorDialog() = default;

void TweenAnimatorDialog::SetScene(Lupine::Scene* scene) {
    m_scene = scene;
    populateNodePaths();
}

void TweenAnimatorDialog::NewAnimation() {
    m_animationResource = std::make_unique<Lupine::TweenAnimationResource>();
    m_currentFilePath.clear();
    m_isModified = false;
    
    // Create a default clip
    Lupine::AnimationClip defaultClip("Default", DEFAULT_CLIP_DURATION, false);
    m_animationResource->AddClip(defaultClip);
    m_currentClip = "Default";
    
    updateClipList();
    updateTrackList();
    updateTimeline();
    updateWindowTitle();
}

void TweenAnimatorDialog::LoadAnimation(const QString& filepath) {
    if (m_animationResource->LoadFromFile(filepath.toStdString())) {
        m_currentFilePath = filepath;
        m_isModified = false;
        
        // Select first clip if available
        auto clipNames = m_animationResource->GetClipNames();
        if (!clipNames.empty()) {
            m_currentClip = QString::fromStdString(clipNames[0]);
        } else {
            m_currentClip.clear();
        }
        
        updateClipList();
        updateTrackList();
        updateTimeline();
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load animation file: " + filepath);
    }
}

void TweenAnimatorDialog::SaveAnimation() {
    if (m_currentFilePath.isEmpty()) {
        SaveAnimationAs();
        return;
    }
    
    if (m_animationResource->SaveToFile(m_currentFilePath.toStdString())) {
        m_isModified = false;
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save animation file: " + m_currentFilePath);
    }
}

void TweenAnimatorDialog::SaveAnimationAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Animation",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Animation Files (*.anim);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".anim")) {
            filepath += ".anim";
        }
        
        m_currentFilePath = filepath;
        SaveAnimation();
    }
}

void TweenAnimatorDialog::setupUI() {
    setupMenuBar();
    setupToolBar();
    setupMainPanels();

    // QMainWindow uses setCentralWidget, not setLayout
    setCentralWidget(m_mainSplitter);
}

void TweenAnimatorDialog::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("File");

    QAction* newAction = fileMenu->addAction("New");
    QAction* openAction = fileMenu->addAction("Open");
    QAction* saveAction = fileMenu->addAction("Save");
    QAction* saveAsAction = fileMenu->addAction("Save As");
    fileMenu->addSeparator();
    QAction* closeAction = fileMenu->addAction("Close");

    connect(newAction, &QAction::triggered, this, &TweenAnimatorDialog::onNew);
    connect(openAction, &QAction::triggered, this, &TweenAnimatorDialog::onOpen);
    connect(saveAction, &QAction::triggered, this, &TweenAnimatorDialog::onSave);
    connect(saveAsAction, &QAction::triggered, this, &TweenAnimatorDialog::onSaveAs);
    connect(closeAction, &QAction::triggered, this, &TweenAnimatorDialog::onClose);

    // Edit menu
    QMenu* editMenu = menuBar->addMenu("Edit");

    QAction* addTrackAction = editMenu->addAction("Add Track");
    QAction* deleteTrackAction = editMenu->addAction("Delete Track");
    editMenu->addSeparator();
    QAction* addKeyframeAction = editMenu->addAction("Add Keyframe");
    QAction* deleteKeyframeAction = editMenu->addAction("Delete Keyframe");

    connect(addTrackAction, &QAction::triggered, this, &TweenAnimatorDialog::onAddTrack);
    connect(deleteTrackAction, &QAction::triggered, this, &TweenAnimatorDialog::onDeleteTrack);
    connect(addKeyframeAction, &QAction::triggered, this, &TweenAnimatorDialog::onAddKeyframe);
    connect(deleteKeyframeAction, &QAction::triggered, this, &TweenAnimatorDialog::onDeleteKeyframe);

    // Animation menu
    QMenu* animationMenu = menuBar->addMenu("Animation");

    QAction* newClipAction = animationMenu->addAction("New Clip");
    QAction* deleteClipAction = animationMenu->addAction("Delete Clip");
    animationMenu->addSeparator();
    QAction* playAction = animationMenu->addAction("Play/Pause");
    QAction* stopAction = animationMenu->addAction("Stop");

    connect(newClipAction, &QAction::triggered, this, &TweenAnimatorDialog::onNewClip);
    connect(deleteClipAction, &QAction::triggered, this, &TweenAnimatorDialog::onDeleteClip);
    connect(playAction, &QAction::triggered, this, &TweenAnimatorDialog::onPlayPause);
    connect(stopAction, &QAction::triggered, this, &TweenAnimatorDialog::onStop);

    setMenuBar(menuBar);
}

void TweenAnimatorDialog::setupToolBar() {
    QToolBar* toolBar = addToolBar("Main");

    // File operations
    m_newButton = new QPushButton("New");
    m_openButton = new QPushButton("Open");
    m_saveButton = new QPushButton("Save");
    m_saveAsButton = new QPushButton("Save As");

    connect(m_newButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onNew);
    connect(m_openButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onOpen);
    connect(m_saveButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onSave);
    connect(m_saveAsButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onSaveAs);

    toolBar->addWidget(m_newButton);
    toolBar->addWidget(m_openButton);
    toolBar->addWidget(m_saveButton);
    toolBar->addWidget(m_saveAsButton);
    toolBar->addSeparator();

    // Playback controls
    m_playButton = new QPushButton("Play");
    m_stopButton = new QPushButton("Stop");
    m_loopButton = new QPushButton("Loop");
    m_loopButton->setCheckable(true);

    connect(m_playButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onStop);
    connect(m_loopButton, &QPushButton::toggled, this, &TweenAnimatorDialog::onLoop);

    toolBar->addWidget(m_playButton);
    toolBar->addWidget(m_stopButton);
    toolBar->addWidget(m_loopButton);
}

void TweenAnimatorDialog::setupMainPanels() {
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel (Animation + Tracks)
    m_leftSplitter = new QSplitter(Qt::Vertical);
    setupAnimationPanel();
    setupTrackPanel();
    m_leftSplitter->addWidget(m_animationGroup);
    m_leftSplitter->addWidget(m_trackGroup);
    m_leftSplitter->setSizes({200, 300});
    
    // Right panel (Timeline + Properties)
    m_rightSplitter = new QSplitter(Qt::Vertical);
    setupTimelinePanel();
    setupPropertyPanel();
    m_rightSplitter->addWidget(m_timelineGroup);
    m_rightSplitter->addWidget(m_propertyGroup);
    m_rightSplitter->setSizes({400, 200});
    
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setSizes({300, 900});
}

void TweenAnimatorDialog::setupAnimationPanel() {
    m_animationGroup = new QGroupBox("Animation Clips");
    m_animationLayout = new QVBoxLayout(m_animationGroup);
    
    // Clip list
    m_clipList = new QListWidget();
    connect(m_clipList, &QListWidget::currentTextChanged, this, &TweenAnimatorDialog::onClipSelectionChanged);
    
    // Clip controls
    QHBoxLayout* clipButtonLayout = new QHBoxLayout();
    m_newClipButton = new QPushButton("New Clip");
    m_deleteClipButton = new QPushButton("Delete Clip");
    
    connect(m_newClipButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onNewClip);
    connect(m_deleteClipButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onDeleteClip);
    
    clipButtonLayout->addWidget(m_newClipButton);
    clipButtonLayout->addWidget(m_deleteClipButton);
    
    // Clip properties
    QFormLayout* clipPropsLayout = new QFormLayout();
    m_clipNameEdit = new QLineEdit();
    m_clipDurationSpin = new QDoubleSpinBox();
    m_clipDurationSpin->setRange(0.1, 3600.0);
    m_clipDurationSpin->setSingleStep(0.1);
    m_clipDurationSpin->setSuffix(" s");
    
    m_clipLoopingCheck = new QCheckBox("Looping");
    
    connect(m_clipNameEdit, &QLineEdit::textChanged, this, &TweenAnimatorDialog::onClipRenamed);
    connect(m_clipDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onTimeChanged);
    
    clipPropsLayout->addRow("Name:", m_clipNameEdit);
    clipPropsLayout->addRow("Duration:", m_clipDurationSpin);
    clipPropsLayout->addRow(m_clipLoopingCheck);
    
    m_animationLayout->addWidget(m_clipList);
    m_animationLayout->addLayout(clipButtonLayout);
    m_animationLayout->addLayout(clipPropsLayout);
}

void TweenAnimatorDialog::setupTrackPanel() {
    m_trackGroup = new QGroupBox("Animation Tracks");
    m_trackLayout = new QVBoxLayout(m_trackGroup);
    
    // Track tree
    m_trackTree = new QTreeWidget();
    m_trackTree->setHeaderLabels({"Node Path", "Property", "Type"});
    m_trackTree->header()->setStretchLastSection(false);
    m_trackTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    connect(m_trackTree, &QTreeWidget::currentItemChanged, this, &TweenAnimatorDialog::onTrackSelectionChanged);
    
    // Track controls
    QHBoxLayout* trackButtonLayout = new QHBoxLayout();
    m_addTrackButton = new QPushButton("Add Track");
    m_deleteTrackButton = new QPushButton("Delete Track");
    
    connect(m_addTrackButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onAddTrack);
    connect(m_deleteTrackButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onDeleteTrack);
    
    trackButtonLayout->addWidget(m_addTrackButton);
    trackButtonLayout->addWidget(m_deleteTrackButton);
    
    m_trackLayout->addWidget(m_trackTree);
    m_trackLayout->addLayout(trackButtonLayout);
}

void TweenAnimatorDialog::setupTimelinePanel() {
    m_timelineGroup = new QGroupBox("Timeline");
    m_timelineLayout = new QVBoxLayout(m_timelineGroup);
    
    // Timeline controls
    m_timelineControlsLayout = new QHBoxLayout();
    
    m_currentTimeSpin = new QDoubleSpinBox();
    m_currentTimeSpin->setRange(0.0, 3600.0);
    m_currentTimeSpin->setSingleStep(0.01);
    m_currentTimeSpin->setSuffix(" s");
    
    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 1000);
    
    m_totalTimeLabel = new QLabel("/ 1.0 s");
    
    connect(m_currentTimeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onTimeChanged);
    connect(m_timeSlider, &QSlider::valueChanged, this, [this](int value) {
        float time = (value / 1000.0f) * getCurrentClipDuration();
        onTimelinePositionChanged(time);
    });
    
    m_timelineControlsLayout->addWidget(new QLabel("Time:"));
    m_timelineControlsLayout->addWidget(m_currentTimeSpin);
    m_timelineControlsLayout->addWidget(m_timeSlider);
    m_timelineControlsLayout->addWidget(m_totalTimeLabel);

    // Add zoom controls
    m_timelineControlsLayout->addStretch();
    QPushButton* zoomInButton = new QPushButton("Zoom In");
    QPushButton* zoomOutButton = new QPushButton("Zoom Out");
    QPushButton* zoomResetButton = new QPushButton("Reset Zoom");

    connect(zoomInButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onTimelineZoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onTimelineZoomOut);
    connect(zoomResetButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onTimelineZoomReset);

    m_timelineControlsLayout->addWidget(zoomInButton);
    m_timelineControlsLayout->addWidget(zoomOutButton);
    m_timelineControlsLayout->addWidget(zoomResetButton);
    
    // Timeline view
    m_timelineView = new QGraphicsView();
    m_timelineScene = new QGraphicsScene();
    m_timelineView->setScene(m_timelineScene);
    m_timelineView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_timelineView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    m_timelineLayout->addLayout(m_timelineControlsLayout);
    m_timelineLayout->addWidget(m_timelineView);
}

void TweenAnimatorDialog::setupPropertyPanel() {
    m_propertyGroup = new QGroupBox("Keyframe Properties");
    m_propertyLayout = new QVBoxLayout(m_propertyGroup);
    
    // Property selection
    QFormLayout* propSelectLayout = new QFormLayout();
    m_nodePathCombo = new QComboBox();
    m_nodePathCombo->setEditable(true);
    m_propertyNameCombo = new QComboBox();
    m_interpolationCombo = new QComboBox();
    
    // Populate interpolation types
    m_interpolationCombo->addItem("Linear", static_cast<int>(Lupine::InterpolationType::Linear));
    m_interpolationCombo->addItem("Ease", static_cast<int>(Lupine::InterpolationType::Ease));
    m_interpolationCombo->addItem("Ease In", static_cast<int>(Lupine::InterpolationType::EaseIn));
    m_interpolationCombo->addItem("Ease Out", static_cast<int>(Lupine::InterpolationType::EaseOut));
    m_interpolationCombo->addItem("Ease In Out", static_cast<int>(Lupine::InterpolationType::EaseInOut));
    m_interpolationCombo->addItem("Bounce", static_cast<int>(Lupine::InterpolationType::Bounce));
    m_interpolationCombo->addItem("Elastic", static_cast<int>(Lupine::InterpolationType::Elastic));
    
    connect(m_nodePathCombo, &QComboBox::currentTextChanged, this, &TweenAnimatorDialog::onNodePathChanged);
    connect(m_propertyNameCombo, &QComboBox::currentTextChanged, this, &TweenAnimatorDialog::onPropertyNameChanged);
    connect(m_interpolationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TweenAnimatorDialog::onInterpolationChanged);
    
    propSelectLayout->addRow("Node Path:", m_nodePathCombo);
    propSelectLayout->addRow("Property:", m_propertyNameCombo);
    propSelectLayout->addRow("Interpolation:", m_interpolationCombo);
    
    // Value editor (will be populated dynamically)
    m_valueWidget = new QWidget();
    m_valueLayout = new QVBoxLayout(m_valueWidget);
    
    // Keyframe table
    m_keyframeTable = new QTableWidget();
    m_keyframeTable->setColumnCount(3);
    m_keyframeTable->setHorizontalHeaderLabels({"Time", "Value", "Interpolation"});
    m_keyframeTable->horizontalHeader()->setStretchLastSection(true);
    
    connect(m_keyframeTable, &QTableWidget::currentCellChanged, this, &TweenAnimatorDialog::onKeyframeSelectionChanged);
    
    // Keyframe controls
    QHBoxLayout* keyframeButtonLayout = new QHBoxLayout();
    m_addKeyframeButton = new QPushButton("Add Keyframe");
    m_deleteKeyframeButton = new QPushButton("Delete Keyframe");
    
    connect(m_addKeyframeButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onAddKeyframe);
    connect(m_deleteKeyframeButton, &QPushButton::clicked, this, &TweenAnimatorDialog::onDeleteKeyframe);
    
    keyframeButtonLayout->addWidget(m_addKeyframeButton);
    keyframeButtonLayout->addWidget(m_deleteKeyframeButton);
    
    m_propertyLayout->addLayout(propSelectLayout);
    m_propertyLayout->addWidget(m_valueWidget);
    m_propertyLayout->addWidget(m_keyframeTable);
    m_propertyLayout->addLayout(keyframeButtonLayout);
}

float TweenAnimatorDialog::getCurrentClipDuration() const {
    if (m_currentClip.isEmpty()) return 1.0f;
    
    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    return clip ? clip->duration : 1.0f;
}

void TweenAnimatorDialog::updateWindowTitle() {
    QString title = "Tween Animator";
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

// Slot implementations

void TweenAnimatorDialog::onNewClip() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Clip", "Clip name:", QLineEdit::Normal, "New Clip", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::AnimationClip clip(name.toStdString(), DEFAULT_CLIP_DURATION, false);
        m_animationResource->AddClip(clip);
        m_currentClip = name;
        m_isModified = true;
        updateClipList();
        updateWindowTitle();
    }
}

void TweenAnimatorDialog::onDeleteClip() {
    if (m_currentClip.isEmpty()) return;

    int ret = QMessageBox::question(this, "Delete Clip",
        "Are you sure you want to delete the clip '" + m_currentClip + "'?",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_animationResource->RemoveClip(m_currentClip.toStdString());
        m_currentClip.clear();
        m_isModified = true;
        updateClipList();
        updateTrackList();
        updateTimeline();
        updateWindowTitle();
    }
}

void TweenAnimatorDialog::onClipSelectionChanged() {
    QListWidgetItem* item = m_clipList->currentItem();
    if (item) {
        m_currentClip = item->text();
        updateTrackList();
        updateTimeline();
        updatePropertyEditor();

        // Update clip properties
        const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
        if (clip) {
            m_clipNameEdit->setText(QString::fromStdString(clip->name));
            m_clipDurationSpin->setValue(clip->duration);
            m_clipLoopingCheck->setChecked(clip->looping);
        }
    }
}

void TweenAnimatorDialog::onClipRenamed() {
    if (m_currentClip.isEmpty()) return;

    QString newName = m_clipNameEdit->text();
    if (newName.isEmpty() || newName == m_currentClip) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (clip) {
        // Create new clip with new name
        Lupine::AnimationClip newClip = *clip;
        newClip.name = newName.toStdString();

        // Remove old clip and add new one
        m_animationResource->RemoveClip(m_currentClip.toStdString());
        m_animationResource->AddClip(newClip);

        m_currentClip = newName;
        m_isModified = true;
        updateClipList();
        updateWindowTitle();
    }
}

void TweenAnimatorDialog::onAddTrack() {
    if (m_currentClip.isEmpty()) return;

    QString nodePath = m_nodePathCombo->currentText();
    QString propertyName = m_propertyNameCombo->currentText();

    if (nodePath.isEmpty() || propertyName.isEmpty()) {
        QMessageBox::information(this, "Add Track", "Please select a node path and property name.");
        return;
    }

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (clip) {
        Lupine::AnimationPropertyType propType = getPropertyType(nodePath, propertyName);
        Lupine::AnimationTrack track(nodePath.toStdString(), propertyName.toStdString(), propType);

        // Add initial keyframe at current time
        Lupine::AnimationValue currentValue = getCurrentPropertyValue(nodePath, propertyName);
        Lupine::AnimationKeyframe keyframe(m_currentTime, currentValue);
        track.keyframes.push_back(keyframe);

        clip->tracks.push_back(track);
        m_isModified = true;
        updateTrackList();
        updateTimeline();
        updateWindowTitle();
    }
}

void TweenAnimatorDialog::onDeleteTrack() {
    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty()) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (clip) {
        // Find and remove the track
        auto it = std::find_if(clip->tracks.begin(), clip->tracks.end(),
            [this](const Lupine::AnimationTrack& track) {
                QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
                return trackId == m_currentTrack;
            });

        if (it != clip->tracks.end()) {
            clip->tracks.erase(it);
            m_currentTrack.clear();
            m_isModified = true;
            updateTrackList();
            updateTimeline();
            updateWindowTitle();
        }
    }
}

void TweenAnimatorDialog::onTrackSelectionChanged() {
    QTreeWidgetItem* item = m_trackTree->currentItem();
    if (item && item->parent() == nullptr) { // Top-level item (track)
        QString nodePath = item->text(0);
        QString propertyName = item->text(1);
        m_currentTrack = nodePath + "/" + propertyName;

        // Update property editor
        m_nodePathCombo->setCurrentText(nodePath);
        populatePropertyNames(nodePath);
        m_propertyNameCombo->setCurrentText(propertyName);

        updateKeyframeList();
    }
}

void TweenAnimatorDialog::onAddKeyframe() {
    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty()) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (clip) {
        // Find the track
        for (auto& track : clip->tracks) {
            QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
            if (trackId == m_currentTrack) {
                // Get current property value
                QString nodePath = QString::fromStdString(track.node_path);
                QString propertyName = QString::fromStdString(track.property_name);
                Lupine::AnimationValue currentValue = getCurrentPropertyValue(nodePath, propertyName);

                // Create keyframe
                Lupine::AnimationKeyframe keyframe(m_currentTime, currentValue);

                // Insert keyframe in sorted order
                auto it = std::lower_bound(track.keyframes.begin(), track.keyframes.end(), keyframe,
                    [](const Lupine::AnimationKeyframe& a, const Lupine::AnimationKeyframe& b) {
                        return a.time < b.time;
                    });
                track.keyframes.insert(it, keyframe);

                m_isModified = true;
                updateKeyframeList();
                updateTimeline();
                updateWindowTitle();
                break;
            }
        }
    }
}

void TweenAnimatorDialog::onDeleteKeyframe() {
    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty() || m_currentKeyframe < 0) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Find the track
    for (auto& track : clip->tracks) {
        QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
        if (trackId == m_currentTrack) {
            // Remove the keyframe if it exists
            if (m_currentKeyframe >= 0 && m_currentKeyframe < static_cast<int>(track.keyframes.size())) {
                track.keyframes.erase(track.keyframes.begin() + m_currentKeyframe);

                // Adjust selection
                if (m_currentKeyframe >= static_cast<int>(track.keyframes.size())) {
                    m_currentKeyframe = static_cast<int>(track.keyframes.size()) - 1;
                }

                m_isModified = true;
                updateKeyframeList();
                updateTimeline();
                updatePropertyEditor();
                updateWindowTitle();
            }
            break;
        }
    }
}

void TweenAnimatorDialog::onKeyframeSelectionChanged() {
    m_currentKeyframe = m_keyframeTable->currentRow();
    updatePropertyEditor();
}

void TweenAnimatorDialog::onKeyframeValueChanged() {
    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty() || m_currentKeyframe < 0) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Find the track
    for (auto& track : clip->tracks) {
        QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
        if (trackId == m_currentTrack) {
            // Update the keyframe value if it exists
            if (m_currentKeyframe >= 0 && m_currentKeyframe < static_cast<int>(track.keyframes.size())) {
                auto& keyframe = track.keyframes[m_currentKeyframe];

                // Get the new value from the property editor widgets
                Lupine::AnimationValue newValue = getValueFromPropertyEditor();
                keyframe.value = newValue;

                m_isModified = true;
                updateKeyframeList();
                updateTimeline();
                updatePreview();
                updateWindowTitle();
            }
            break;
        }
    }
}

void TweenAnimatorDialog::onInterpolationChanged() {
    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty() || m_currentKeyframe < 0) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Find the track
    for (auto& track : clip->tracks) {
        QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
        if (trackId == m_currentTrack) {
            // Update the keyframe interpolation if it exists
            if (m_currentKeyframe >= 0 && m_currentKeyframe < static_cast<int>(track.keyframes.size())) {
                auto& keyframe = track.keyframes[m_currentKeyframe];

                // Get the new interpolation type from the combo box
                int interpIndex = m_interpolationCombo->currentIndex();
                switch (interpIndex) {
                    case 0: keyframe.interpolation = Lupine::InterpolationType::Linear; break;
                    case 1: keyframe.interpolation = Lupine::InterpolationType::Ease; break;
                    case 2: keyframe.interpolation = Lupine::InterpolationType::EaseIn; break;
                    case 3: keyframe.interpolation = Lupine::InterpolationType::EaseOut; break;
                    case 4: keyframe.interpolation = Lupine::InterpolationType::EaseInOut; break;
                    case 5: keyframe.interpolation = Lupine::InterpolationType::Bounce; break;
                    case 6: keyframe.interpolation = Lupine::InterpolationType::Elastic; break;
                    default: keyframe.interpolation = Lupine::InterpolationType::Linear; break;
                }

                m_isModified = true;
                updateKeyframeList();
                updateTimeline();
                updatePreview();
                updateWindowTitle();
            }
            break;
        }
    }
}

void TweenAnimatorDialog::onTimelinePositionChanged(float time) {
    m_currentTime = time;
    m_currentTimeSpin->setValue(time);

    float duration = getCurrentClipDuration();
    if (duration > 0) {
        int sliderValue = static_cast<int>((time / duration) * 1000);
        m_timeSlider->setValue(sliderValue);
    }

    updatePreview();
}

void TweenAnimatorDialog::onPlayPause() {
    if (m_isPlaying) {
        m_playbackTimer->stop();
        m_isPlaying = false;
        m_playButton->setText("Play");
    } else {
        startPlayback();
    }
}

void TweenAnimatorDialog::onStop() {
    stopPlayback();
    m_currentTime = 0.0f;
    onTimelinePositionChanged(m_currentTime);
}

void TweenAnimatorDialog::onLoop() {
    m_isLooping = m_loopButton->isChecked();
}

void TweenAnimatorDialog::onTimeChanged() {
    float newTime = m_currentTimeSpin->value();
    onTimelinePositionChanged(newTime);
}

void TweenAnimatorDialog::onTimelineSelectionChanged() {
    // Handle timeline selection changes
    QList<QGraphicsItem*> selectedItems = m_timelineScene->selectedItems();

    for (QGraphicsItem* item : selectedItems) {
        // Check if it's a keyframe item
        if (item->data(0).isValid() && item->data(1).isValid()) {
            int trackIndex = item->data(0).toInt();
            int keyframeIndex = item->data(1).toInt();

            // Update current selection
            if (m_currentClip.isEmpty()) return;

            const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
            if (!clip || trackIndex >= static_cast<int>(clip->tracks.size())) return;

            const auto& track = clip->tracks[trackIndex];
            m_currentTrack = QString::fromStdString(track.node_path + "/" + track.property_name);
            m_currentKeyframe = keyframeIndex;

            updatePropertyEditor();
            break; // Only handle first selected item
        }
    }
}

void TweenAnimatorDialog::onTimelineClicked(QPointF position) {
    // Convert click position to time
    float time = position.x() / 100.0f; // Assuming 100 pixels per second
    onTimelinePositionChanged(time);
}

void TweenAnimatorDialog::onKeyframeDragged(int trackIndex, int keyframeIndex, float newTime) {
    if (m_currentClip.isEmpty()) return;

    auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip || trackIndex >= static_cast<int>(clip->tracks.size())) return;

    auto& track = clip->tracks[trackIndex];
    if (keyframeIndex >= 0 && keyframeIndex < static_cast<int>(track.keyframes.size())) {
        // Update keyframe time
        track.keyframes[keyframeIndex].time = std::max(0.0f, newTime);

        // Sort keyframes by time to maintain order
        std::sort(track.keyframes.begin(), track.keyframes.end(),
                 [](const Lupine::AnimationKeyframe& a, const Lupine::AnimationKeyframe& b) {
                     return a.time < b.time;
                 });

        m_isModified = true;
        updateKeyframeList();
        updateTimeline();
        updateWindowTitle();
    }
}

void TweenAnimatorDialog::onTimelineZoomIn() {
    m_timelineScale *= 1.2f;
    updateTimeline();
}

void TweenAnimatorDialog::onTimelineZoomOut() {
    m_timelineScale /= 1.2f;
    m_timelineScale = std::max(0.1f, m_timelineScale); // Minimum zoom
    updateTimeline();
}

void TweenAnimatorDialog::onTimelineZoomReset() {
    m_timelineScale = 1.0f;
    updateTimeline();
}

void TweenAnimatorDialog::onPropertyValueChanged() {
    // Implementation for property value changes
    m_isModified = true;
    updateWindowTitle();
}

void TweenAnimatorDialog::onNodePathChanged() {
    QString nodePath = m_nodePathCombo->currentText();
    populatePropertyNames(nodePath);
}

void TweenAnimatorDialog::onPropertyNameChanged() {
    // Update property type and value editor based on selected property
    updatePropertyEditor();
}

// File operation slots
void TweenAnimatorDialog::onNew() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "New Animation",
            "Current animation has unsaved changes. Do you want to save before creating a new animation?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveAnimation();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    NewAnimation();
}

void TweenAnimatorDialog::onOpen() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "Open Animation",
            "Current animation has unsaved changes. Do you want to save before opening another animation?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveAnimation();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Animation",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Animation Files (*.anim);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadAnimation(filepath);
    }
}

void TweenAnimatorDialog::onSave() {
    SaveAnimation();
}

void TweenAnimatorDialog::onSaveAs() {
    SaveAnimationAs();
}

void TweenAnimatorDialog::onClose() {
    close();
}

// Update methods

void TweenAnimatorDialog::updateClipList() {
    m_clipList->clear();

    auto clipNames = m_animationResource->GetClipNames();
    for (const auto& name : clipNames) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name));
        m_clipList->addItem(item);

        if (QString::fromStdString(name) == m_currentClip) {
            m_clipList->setCurrentItem(item);
        }
    }
}

void TweenAnimatorDialog::updateTrackList() {
    m_trackTree->clear();

    if (m_currentClip.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Group tracks by node path for better organization
    QMap<QString, QTreeWidgetItem*> nodeGroups;

    for (const auto& track : clip->tracks) {
        QString nodePath = QString::fromStdString(track.node_path);
        QString propertyName = QString::fromStdString(track.property_name);
        QString trackId = nodePath + "/" + propertyName;

        // Create or get node group
        QTreeWidgetItem* nodeGroup = nullptr;
        if (nodeGroups.contains(nodePath)) {
            nodeGroup = nodeGroups[nodePath];
        } else {
            nodeGroup = new QTreeWidgetItem(m_trackTree);
            nodeGroup->setText(0, nodePath);
            nodeGroup->setExpanded(true);
            nodeGroup->setFlags(nodeGroup->flags() & ~Qt::ItemIsSelectable);

            // Style the group item
            QFont font = nodeGroup->font(0);
            font.setBold(true);
            nodeGroup->setFont(0, font);
            nodeGroup->setBackground(0, QColor(230, 230, 230));

            nodeGroups[nodePath] = nodeGroup;
        }

        // Create property item under the node group
        QTreeWidgetItem* propertyItem = new QTreeWidgetItem(nodeGroup);

        // Format property name nicely
        QString displayName = propertyName;
        if (propertyName.contains(".")) {
            QStringList parts = propertyName.split(".");
            displayName = parts.last(); // Show only the last part for cleaner display

            // Add component prefix if it's a component property
            if (parts.size() > 1) {
                displayName = parts[0] + ": " + displayName;
            }
        }

        propertyItem->setText(0, displayName);
        propertyItem->setData(0, Qt::UserRole, trackId);

        // Set property type text
        QString typeText;
        switch (track.property_type) {
            case Lupine::AnimationPropertyType::Float: typeText = "Float"; break;
            case Lupine::AnimationPropertyType::Vec2: typeText = "Vector2"; break;
            case Lupine::AnimationPropertyType::Vec3: typeText = "Vector3"; break;
            case Lupine::AnimationPropertyType::Vec4: typeText = "Vector4"; break;
            case Lupine::AnimationPropertyType::Quaternion: typeText = "Quaternion"; break;
            case Lupine::AnimationPropertyType::Color: typeText = "Color"; break;
            case Lupine::AnimationPropertyType::Bool: typeText = "Bool"; break;
            case Lupine::AnimationPropertyType::Int: typeText = "Int"; break;
        }
        propertyItem->setText(1, typeText);
        propertyItem->setText(2, QString::number(track.keyframes.size()) + " keys");

        if (trackId == m_currentTrack) {
            m_trackTree->setCurrentItem(propertyItem);
        }
    }
}

void TweenAnimatorDialog::updateKeyframeList() {
    m_keyframeTable->setRowCount(0);

    if (m_currentClip.isEmpty() || m_currentTrack.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Find the current track
    for (const auto& track : clip->tracks) {
        QString trackId = QString::fromStdString(track.node_path + "/" + track.property_name);
        if (trackId == m_currentTrack) {
            m_keyframeTable->setRowCount(track.keyframes.size());

            for (size_t i = 0; i < track.keyframes.size(); ++i) {
                const auto& keyframe = track.keyframes[i];

                // Time column
                QTableWidgetItem* timeItem = new QTableWidgetItem(QString::number(keyframe.time, 'f', 3));
                m_keyframeTable->setItem(i, 0, timeItem);

                // Value column (simplified display)
                QString valueText;
                switch (keyframe.value.type) {
                    case Lupine::AnimationPropertyType::Float:
                        valueText = QString::number(keyframe.value.float_value, 'f', 3);
                        break;
                    case Lupine::AnimationPropertyType::Vec2:
                        valueText = QString("(%1, %2)")
                            .arg(keyframe.value.vec2_value.x, 0, 'f', 3)
                            .arg(keyframe.value.vec2_value.y, 0, 'f', 3);
                        break;
                    case Lupine::AnimationPropertyType::Vec3:
                        valueText = QString("(%1, %2, %3)")
                            .arg(keyframe.value.vec3_value.x, 0, 'f', 3)
                            .arg(keyframe.value.vec3_value.y, 0, 'f', 3)
                            .arg(keyframe.value.vec3_value.z, 0, 'f', 3);
                        break;
                    case Lupine::AnimationPropertyType::Bool:
                        valueText = keyframe.value.bool_value ? "true" : "false";
                        break;
                    case Lupine::AnimationPropertyType::Int:
                        valueText = QString::number(keyframe.value.int_value);
                        break;
                    default:
                        valueText = "...";
                        break;
                }
                QTableWidgetItem* valueItem = new QTableWidgetItem(valueText);
                m_keyframeTable->setItem(i, 1, valueItem);

                // Interpolation column
                QString interpText;
                switch (keyframe.interpolation) {
                    case Lupine::InterpolationType::Linear: interpText = "Linear"; break;
                    case Lupine::InterpolationType::Ease: interpText = "Ease"; break;
                    case Lupine::InterpolationType::EaseIn: interpText = "Ease In"; break;
                    case Lupine::InterpolationType::EaseOut: interpText = "Ease Out"; break;
                    case Lupine::InterpolationType::EaseInOut: interpText = "Ease In Out"; break;
                    case Lupine::InterpolationType::Bounce: interpText = "Bounce"; break;
                    case Lupine::InterpolationType::Elastic: interpText = "Elastic"; break;
                    default: interpText = "Linear"; break;
                }
                QTableWidgetItem* interpItem = new QTableWidgetItem(interpText);
                m_keyframeTable->setItem(i, 2, interpItem);
            }
            break;
        }
    }
}

void TweenAnimatorDialog::updatePropertyEditor() {
    // Clear existing value widgets
    QLayoutItem* item;
    while ((item = m_valueLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (m_currentKeyframe >= 0 && !m_currentTrack.isEmpty()) {
        // Create appropriate value editor based on property type
        QString nodePath = m_nodePathCombo->currentText();
        QString propertyName = m_propertyNameCombo->currentText();
        Lupine::AnimationPropertyType propType = getPropertyType(nodePath, propertyName);

        switch (propType) {
            case Lupine::AnimationPropertyType::Float: {
                QDoubleSpinBox* spinBox = new QDoubleSpinBox();
                spinBox->setRange(-999999.0, 999999.0);
                spinBox->setDecimals(3);
                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                m_valueLayout->addWidget(spinBox);
                break;
            }
            case Lupine::AnimationPropertyType::Vec2: {
                QHBoxLayout* vecLayout = new QHBoxLayout();
                QDoubleSpinBox* xSpin = new QDoubleSpinBox();
                QDoubleSpinBox* ySpin = new QDoubleSpinBox();
                xSpin->setRange(-999999.0, 999999.0);
                ySpin->setRange(-999999.0, 999999.0);
                xSpin->setDecimals(3);
                ySpin->setDecimals(3);
                connect(xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                vecLayout->addWidget(new QLabel("X:"));
                vecLayout->addWidget(xSpin);
                vecLayout->addWidget(new QLabel("Y:"));
                vecLayout->addWidget(ySpin);
                m_valueLayout->addLayout(vecLayout);
                break;
            }
            case Lupine::AnimationPropertyType::Vec3: {
                QHBoxLayout* vecLayout = new QHBoxLayout();
                QDoubleSpinBox* xSpin = new QDoubleSpinBox();
                QDoubleSpinBox* ySpin = new QDoubleSpinBox();
                QDoubleSpinBox* zSpin = new QDoubleSpinBox();
                xSpin->setRange(-999999.0, 999999.0);
                ySpin->setRange(-999999.0, 999999.0);
                zSpin->setRange(-999999.0, 999999.0);
                xSpin->setDecimals(3);
                ySpin->setDecimals(3);
                zSpin->setDecimals(3);
                connect(xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(zSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                vecLayout->addWidget(new QLabel("X:"));
                vecLayout->addWidget(xSpin);
                vecLayout->addWidget(new QLabel("Y:"));
                vecLayout->addWidget(ySpin);
                vecLayout->addWidget(new QLabel("Z:"));
                vecLayout->addWidget(zSpin);
                m_valueLayout->addLayout(vecLayout);
                break;
            }
            case Lupine::AnimationPropertyType::Vec4:
            case Lupine::AnimationPropertyType::Color: {
                QHBoxLayout* vecLayout = new QHBoxLayout();
                QDoubleSpinBox* xSpin = new QDoubleSpinBox();
                QDoubleSpinBox* ySpin = new QDoubleSpinBox();
                QDoubleSpinBox* zSpin = new QDoubleSpinBox();
                QDoubleSpinBox* wSpin = new QDoubleSpinBox();
                xSpin->setRange(propType == Lupine::AnimationPropertyType::Color ? 0.0 : -999999.0,
                               propType == Lupine::AnimationPropertyType::Color ? 1.0 : 999999.0);
                ySpin->setRange(propType == Lupine::AnimationPropertyType::Color ? 0.0 : -999999.0,
                               propType == Lupine::AnimationPropertyType::Color ? 1.0 : 999999.0);
                zSpin->setRange(propType == Lupine::AnimationPropertyType::Color ? 0.0 : -999999.0,
                               propType == Lupine::AnimationPropertyType::Color ? 1.0 : 999999.0);
                wSpin->setRange(propType == Lupine::AnimationPropertyType::Color ? 0.0 : -999999.0,
                               propType == Lupine::AnimationPropertyType::Color ? 1.0 : 999999.0);
                xSpin->setDecimals(3);
                ySpin->setDecimals(3);
                zSpin->setDecimals(3);
                wSpin->setDecimals(3);
                connect(xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(zSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                connect(wSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);

                if (propType == Lupine::AnimationPropertyType::Color) {
                    vecLayout->addWidget(new QLabel("R:"));
                    vecLayout->addWidget(xSpin);
                    vecLayout->addWidget(new QLabel("G:"));
                    vecLayout->addWidget(ySpin);
                    vecLayout->addWidget(new QLabel("B:"));
                    vecLayout->addWidget(zSpin);
                    vecLayout->addWidget(new QLabel("A:"));
                    vecLayout->addWidget(wSpin);
                } else {
                    vecLayout->addWidget(new QLabel("X:"));
                    vecLayout->addWidget(xSpin);
                    vecLayout->addWidget(new QLabel("Y:"));
                    vecLayout->addWidget(ySpin);
                    vecLayout->addWidget(new QLabel("Z:"));
                    vecLayout->addWidget(zSpin);
                    vecLayout->addWidget(new QLabel("W:"));
                    vecLayout->addWidget(wSpin);
                }
                m_valueLayout->addLayout(vecLayout);
                break;
            }
            case Lupine::AnimationPropertyType::Int: {
                QSpinBox* spinBox = new QSpinBox();
                spinBox->setRange(-999999, 999999);
                connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TweenAnimatorDialog::onPropertyValueChanged);
                m_valueLayout->addWidget(spinBox);
                break;
            }
            case Lupine::AnimationPropertyType::Bool: {
                QCheckBox* checkBox = new QCheckBox();
                connect(checkBox, &QCheckBox::toggled, this, &TweenAnimatorDialog::onPropertyValueChanged);
                m_valueLayout->addWidget(checkBox);
                break;
            }
            default:
                m_valueLayout->addWidget(new QLabel("Unsupported property type"));
                break;
        }
    }
}

void TweenAnimatorDialog::updateTimeline() {
    renderTimeline();
}

void TweenAnimatorDialog::updatePreview() {
    // Apply current animation state to scene nodes
    if (!m_scene || m_currentClip.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Evaluate all tracks at current time
    for (const auto& track : clip->tracks) {
        if (track.keyframes.empty()) continue;

        // Find keyframes to interpolate between
        Lupine::AnimationValue value;
        if (track.keyframes.size() == 1) {
            value = track.keyframes[0].value;
        } else {
            // Find surrounding keyframes
            auto it = std::lower_bound(track.keyframes.begin(), track.keyframes.end(), m_currentTime,
                [](const Lupine::AnimationKeyframe& kf, float time) {
                    return kf.time < time;
                });

            if (it == track.keyframes.begin()) {
                value = track.keyframes[0].value;
            } else if (it == track.keyframes.end()) {
                value = track.keyframes.back().value;
            } else {
                // Interpolate between keyframes
                const auto& kf1 = *(it - 1);
                const auto& kf2 = *it;

                float t = (m_currentTime - kf1.time) / (kf2.time - kf1.time);
                t = std::clamp(t, 0.0f, 1.0f);

                // Use advanced interpolation with easing
                value = interpolateValueWithEasing(kf1.value, kf2.value, t, kf1.interpolation);
            }
        }

        // Apply value to scene node
        QString nodePath = QString::fromStdString(track.node_path);
        QString propertyName = QString::fromStdString(track.property_name);
        setPropertyValue(nodePath, propertyName, value);
    }
}

// Utility methods

void TweenAnimatorDialog::populateNodePaths() {
    m_nodePathCombo->clear();

    if (!m_scene || !m_scene->GetRootNode()) return;

    std::function<void(Lupine::Node*, const QString&)> addNodePaths = [&](Lupine::Node* node, const QString& basePath) {
        QString nodePath = basePath.isEmpty() ? QString::fromStdString(node->GetName()) : basePath + "/" + QString::fromStdString(node->GetName());
        m_nodePathCombo->addItem(nodePath);

        for (const auto& child : node->GetChildren()) {
            addNodePaths(child.get(), nodePath);
        }
    };

    addNodePaths(m_scene->GetRootNode(), "");
}

void TweenAnimatorDialog::populatePropertyNames(const QString& nodePath) {
    m_propertyNameCombo->clear();

    if (nodePath.isEmpty()) return;

    // Add common animatable properties based on node type
    auto properties = getAnimatableProperties(nodePath);
    for (const auto& prop : properties) {
        m_propertyNameCombo->addItem(QString::fromStdString(prop));
    }
}

std::vector<std::string> TweenAnimatorDialog::getAnimatableProperties(const QString& nodePath) {
    std::vector<std::string> properties;

    // Find the node in the scene
    if (!m_scene) return properties;

    Lupine::Node* node = findNodeByPath(nodePath);
    if (!node) return properties;

    // Add common properties based on node type
    if (dynamic_cast<Lupine::Node2D*>(node)) {
        properties.push_back("position");
        properties.push_back("rotation");
        properties.push_back("scale");
        properties.push_back("z_index");
        properties.push_back("visible");
    } else if (dynamic_cast<Lupine::Node3D*>(node)) {
        properties.push_back("position");
        properties.push_back("rotation");
        properties.push_back("scale");
        properties.push_back("visible");
    } else if (dynamic_cast<Lupine::Control*>(node)) {
        properties.push_back("position");
        properties.push_back("size");
        properties.push_back("rotation");
        properties.push_back("anchor_left");
        properties.push_back("anchor_top");
        properties.push_back("anchor_right");
        properties.push_back("anchor_bottom");
        properties.push_back("margin_left");
        properties.push_back("margin_top");
        properties.push_back("margin_right");
        properties.push_back("margin_bottom");
        properties.push_back("modulate");
        properties.push_back("visible");
    }

    // Add component-specific properties
    auto components = node->GetComponents<Lupine::Component>();
    for (const auto* component : components) {
        // Add properties based on component type
        if (component->GetTypeName() == "Sprite2D") {
            properties.push_back("sprite.color");
            properties.push_back("sprite.modulate");
            properties.push_back("sprite.texture_region");
            properties.push_back("sprite.flip_h");
            properties.push_back("sprite.flip_v");
            properties.push_back("sprite.centered");
            properties.push_back("sprite.offset");
        } else if (component->GetTypeName() == "Sprite3D") {
            properties.push_back("sprite3d.color");
            properties.push_back("sprite3d.modulate");
            properties.push_back("sprite3d.texture_region");
            properties.push_back("sprite3d.flip_h");
            properties.push_back("sprite3d.flip_v");
            properties.push_back("sprite3d.centered");
            properties.push_back("sprite3d.offset");
        } else if (component->GetTypeName() == "Label") {
            properties.push_back("label.color");
            properties.push_back("label.font_size");
            properties.push_back("label.text");
            properties.push_back("label.outline_color");
            properties.push_back("label.outline_size");
        } else if (component->GetTypeName() == "AnimatedSprite2D") {
            properties.push_back("animated_sprite.color");
            properties.push_back("animated_sprite.modulate");
            properties.push_back("animated_sprite.speed_scale");
            properties.push_back("animated_sprite.frame");
        } else if (component->GetTypeName() == "AnimatedSprite3D") {
            properties.push_back("animated_sprite3d.color");
            properties.push_back("animated_sprite3d.modulate");
            properties.push_back("animated_sprite3d.speed_scale");
            properties.push_back("animated_sprite3d.frame");
        } else if (component->GetTypeName() == "RigidBody2D") {
            properties.push_back("rigidbody2d.linear_velocity");
            properties.push_back("rigidbody2d.angular_velocity");
            properties.push_back("rigidbody2d.gravity_scale");
        } else if (component->GetTypeName() == "RigidBody3D") {
            properties.push_back("rigidbody3d.linear_velocity");
            properties.push_back("rigidbody3d.angular_velocity");
            properties.push_back("rigidbody3d.gravity_scale");
        } else if (component->GetTypeName() == "Camera2D") {
            properties.push_back("camera2d.zoom");
            properties.push_back("camera2d.offset");
            properties.push_back("camera2d.rotation");
        } else if (component->GetTypeName() == "Camera3D") {
            properties.push_back("camera3d.fov");
            properties.push_back("camera3d.near");
            properties.push_back("camera3d.far");
        } else if (component->GetTypeName() == "Light2D") {
            properties.push_back("light2d.color");
            properties.push_back("light2d.energy");
            properties.push_back("light2d.range");
        } else if (component->GetTypeName() == "Light3D") {
            properties.push_back("light3d.color");
            properties.push_back("light3d.energy");
            properties.push_back("light3d.range");
            properties.push_back("light3d.spot_angle");
        } else if (component->GetTypeName() == "AudioSource") {
            properties.push_back("audio.volume");
            properties.push_back("audio.pitch");
        } else if (component->GetTypeName() == "Skeleton") {
            // Add bone properties for skeletal animation
            properties.push_back("skeleton.bone_count");
            // We would need to enumerate actual bones here
            // For now, add some common bone properties
            addBoneProperties(properties, component);
        } else if (component->GetTypeName() == "MeshRenderer") {
            properties.push_back("mesh.material_color");
            properties.push_back("mesh.material_metallic");
            properties.push_back("mesh.material_roughness");
        }
        // Add more component properties as needed
    }

    return properties;
}

void TweenAnimatorDialog::addBoneProperties(std::vector<std::string>& properties, const Lupine::Component* skeletonComponent) {
    // This would typically enumerate the actual bones in the skeleton
    // For now, we'll add some common bone names that might exist
    std::vector<std::string> commonBones = {
        "root", "spine", "chest", "neck", "head",
        "shoulder_l", "shoulder_r", "arm_l", "arm_r",
        "forearm_l", "forearm_r", "hand_l", "hand_r",
        "hip_l", "hip_r", "thigh_l", "thigh_r",
        "shin_l", "shin_r", "foot_l", "foot_r"
    };

    for (const auto& boneName : commonBones) {
        // Add position, rotation, and scale properties for each bone
        properties.push_back("bone." + boneName + ".position");
        properties.push_back("bone." + boneName + ".rotation");
        properties.push_back("bone." + boneName + ".scale");
    }

    // In a real implementation, you would:
    // 1. Cast the component to the actual Skeleton type
    // 2. Enumerate the actual bone hierarchy
    // 3. Add properties for each bone that exists
    //
    // Example:
    // if (auto* skeleton = dynamic_cast<const Lupine::Skeleton*>(skeletonComponent)) {
    //     for (const auto& bone : skeleton->GetBones()) {
    //         properties.push_back("bone." + bone.name + ".position");
    //         properties.push_back("bone." + bone.name + ".rotation");
    //         properties.push_back("bone." + bone.name + ".scale");
    //     }
    // }
}

Lupine::AnimationPropertyType TweenAnimatorDialog::getPropertyType(const QString& nodePath, const QString& propertyName) {
    // Determine property type based on property name

    // Vector2 properties
    if (propertyName == "position" || propertyName == "scale" || propertyName == "size" ||
        propertyName == "offset" || propertyName == "sprite.offset" || propertyName == "sprite3d.offset" ||
        propertyName == "camera2d.offset" || propertyName == "linear_velocity" ||
        propertyName == "rigidbody2d.linear_velocity" || propertyName == "camera2d.zoom") {

        // Check if it's a 3D node for Vec3 properties
        Lupine::Node* node = findNodeByPath(nodePath);
        if (node && dynamic_cast<Lupine::Node3D*>(node) &&
            (propertyName == "position" || propertyName == "scale")) {
            return Lupine::AnimationPropertyType::Vec3;
        }
        return Lupine::AnimationPropertyType::Vec2;
    }

    // Vector3 properties
    else if (propertyName == "rigidbody3d.linear_velocity" || propertyName == "rigidbody3d.angular_velocity") {
        return Lupine::AnimationPropertyType::Vec3;
    }

    // Color properties
    else if (propertyName == "color" || propertyName == "modulate" ||
             propertyName == "sprite.color" || propertyName == "sprite.modulate" ||
             propertyName == "sprite3d.color" || propertyName == "sprite3d.modulate" ||
             propertyName == "label.color" || propertyName == "label.outline_color" ||
             propertyName == "animated_sprite.color" || propertyName == "animated_sprite.modulate" ||
             propertyName == "animated_sprite3d.color" || propertyName == "animated_sprite3d.modulate" ||
             propertyName == "light2d.color" || propertyName == "light3d.color") {
        return Lupine::AnimationPropertyType::Color;
    }

    // Vector4 properties (texture regions)
    else if (propertyName == "sprite.texture_region" || propertyName == "sprite3d.texture_region") {
        return Lupine::AnimationPropertyType::Vec4;
    }

    // Integer properties
    else if (propertyName == "font_size" || propertyName == "label.font_size" ||
             propertyName == "z_index" || propertyName == "animated_sprite.frame" ||
             propertyName == "animated_sprite3d.frame" || propertyName == "label.outline_size") {
        return Lupine::AnimationPropertyType::Int;
    }

    // Boolean properties
    else if (propertyName == "visible" || propertyName == "sprite.flip_h" || propertyName == "sprite.flip_v" ||
             propertyName == "sprite.centered" || propertyName == "sprite3d.flip_h" ||
             propertyName == "sprite3d.flip_v" || propertyName == "sprite3d.centered") {
        return Lupine::AnimationPropertyType::Bool;
    }

    // String properties (not supported in current animation system)
    // else if (propertyName == "label.text") {
    //     return Lupine::AnimationPropertyType::String;
    // }

    // Bone properties
    else if (propertyName.startsWith("bone.") && propertyName.endsWith(".position")) {
        return Lupine::AnimationPropertyType::Vec3; // Bone positions are typically 3D
    } else if (propertyName.startsWith("bone.") && propertyName.endsWith(".rotation")) {
        return Lupine::AnimationPropertyType::Vec3; // Euler angles
    } else if (propertyName.startsWith("bone.") && propertyName.endsWith(".scale")) {
        return Lupine::AnimationPropertyType::Vec3; // 3D scale
    }

    // Float properties (default and specific)
    else if (propertyName == "rotation" || propertyName == "gravity_scale" ||
             propertyName == "rigidbody2d.gravity_scale" || propertyName == "rigidbody3d.gravity_scale" ||
             propertyName == "camera2d.rotation" || propertyName == "camera3d.fov" ||
             propertyName == "camera3d.near" || propertyName == "camera3d.far" ||
             propertyName == "light2d.energy" || propertyName == "light2d.range" ||
             propertyName == "light3d.energy" || propertyName == "light3d.range" ||
             propertyName == "light3d.spot_angle" || propertyName == "audio.volume" ||
             propertyName == "audio.pitch" || propertyName == "animated_sprite.speed_scale" ||
             propertyName == "animated_sprite3d.speed_scale" ||
             propertyName.startsWith("anchor_") || propertyName.startsWith("margin_")) {
        return Lupine::AnimationPropertyType::Float;
    }

    return Lupine::AnimationPropertyType::Float; // Default
}

Lupine::AnimationValue TweenAnimatorDialog::getCurrentPropertyValue(const QString& nodePath, const QString& propertyName) {
    // Get current value from scene node
    Lupine::Node* node = findNodeByPath(nodePath);
    if (!node) return Lupine::AnimationValue(0.0f);

    // Handle Node2D properties
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        if (propertyName == "position") {
            return Lupine::AnimationValue(node2d->GetPosition());
        } else if (propertyName == "rotation") {
            return Lupine::AnimationValue(node2d->GetRotation());
        } else if (propertyName == "scale") {
            return Lupine::AnimationValue(node2d->GetScale());
        } else if (propertyName == "visible") {
            return Lupine::AnimationValue(node2d->IsActive());
        }
    }

    // Handle Node3D properties
    else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        if (propertyName == "position") {
            return Lupine::AnimationValue(node3d->GetPosition());
        } else if (propertyName == "rotation") {
            // Convert quaternion to euler angles
            glm::quat quat = node3d->GetRotation();
            glm::vec3 euler = glm::eulerAngles(quat);
            return Lupine::AnimationValue(euler);
        } else if (propertyName == "scale") {
            return Lupine::AnimationValue(node3d->GetScale());
        } else if (propertyName == "visible") {
            return Lupine::AnimationValue(node3d->IsActive());
        }
    }

    // Handle Control properties
    else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        if (propertyName == "position") {
            return Lupine::AnimationValue(control->GetPosition());
        } else if (propertyName == "size") {
            return Lupine::AnimationValue(control->GetSize());
        } else if (propertyName == "rotation") {
            // Control nodes don't have rotation in this implementation
            return Lupine::AnimationValue(0.0f);
        } else if (propertyName == "modulate") {
            // Control nodes don't have modulate in this implementation
            return Lupine::AnimationValue(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else if (propertyName == "visible") {
            return Lupine::AnimationValue(control->IsActive());
        }
        // Add anchor and margin properties
        else if (propertyName == "anchor_left") {
            return Lupine::AnimationValue(control->GetAnchorMin().x);
        } else if (propertyName == "anchor_top") {
            return Lupine::AnimationValue(control->GetAnchorMin().y);
        } else if (propertyName == "anchor_right") {
            return Lupine::AnimationValue(control->GetAnchorMax().x);
        } else if (propertyName == "anchor_bottom") {
            return Lupine::AnimationValue(control->GetAnchorMax().y);
        }
    }

    // Handle component properties
    auto components = node->GetComponents<Lupine::Component>();
    for (const auto* component : components) {
        if (component->GetTypeName() == "Sprite2D" && propertyName.startsWith("sprite.")) {
            // Handle Sprite2D component properties
            // This would require accessing the component's properties
            // For now, return default values
        } else if (component->GetTypeName() == "Label" && propertyName.startsWith("label.")) {
            // Handle Label component properties
            // This would require accessing the component's properties
        }
        // Add more component property handling as needed
    }

    return Lupine::AnimationValue(0.0f);
}

void TweenAnimatorDialog::setPropertyValue(const QString& nodePath, const QString& propertyName, const Lupine::AnimationValue& value) {
    // Set value on scene node
    Lupine::Node* node = findNodeByPath(nodePath);
    if (!node) return;

    // Handle Node2D properties
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        if (propertyName == "position" && value.type == Lupine::AnimationPropertyType::Vec2) {
            node2d->SetPosition(value.vec2_value);
        } else if (propertyName == "rotation" && value.type == Lupine::AnimationPropertyType::Float) {
            node2d->SetRotation(value.float_value);
        } else if (propertyName == "scale" && value.type == Lupine::AnimationPropertyType::Vec2) {
            node2d->SetScale(value.vec2_value);
        } else if (propertyName == "visible" && value.type == Lupine::AnimationPropertyType::Bool) {
            node2d->SetActive(value.bool_value);
        }
    }

    // Handle Node3D properties
    else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        if (propertyName == "position" && value.type == Lupine::AnimationPropertyType::Vec3) {
            node3d->SetPosition(value.vec3_value);
        } else if (propertyName == "rotation" && value.type == Lupine::AnimationPropertyType::Vec3) {
            // Convert euler angles to quaternion
            glm::quat quat = glm::quat(value.vec3_value);
            node3d->SetRotation(quat);
        } else if (propertyName == "scale" && value.type == Lupine::AnimationPropertyType::Vec3) {
            node3d->SetScale(value.vec3_value);
        } else if (propertyName == "visible" && value.type == Lupine::AnimationPropertyType::Bool) {
            node3d->SetActive(value.bool_value);
        }
    }

    // Handle Control properties
    else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        if (propertyName == "position" && value.type == Lupine::AnimationPropertyType::Vec2) {
            control->SetPosition(value.vec2_value);
        } else if (propertyName == "size" && value.type == Lupine::AnimationPropertyType::Vec2) {
            control->SetSize(value.vec2_value);
        } else if (propertyName == "rotation" && value.type == Lupine::AnimationPropertyType::Float) {
            // Control nodes don't have rotation in this implementation
            // This is a no-op
        } else if (propertyName == "modulate" && value.type == Lupine::AnimationPropertyType::Color) {
            // Control nodes don't have modulate in this implementation
            // This is a no-op
        } else if (propertyName == "visible" && value.type == Lupine::AnimationPropertyType::Bool) {
            control->SetActive(value.bool_value);
        }
        // Handle anchor and margin properties
        else if (propertyName == "anchor_left" && value.type == Lupine::AnimationPropertyType::Float) {
            glm::vec2 anchor_min = control->GetAnchorMin();
            anchor_min.x = value.float_value;
            control->SetAnchorMin(anchor_min);
        } else if (propertyName == "anchor_top" && value.type == Lupine::AnimationPropertyType::Float) {
            glm::vec2 anchor_min = control->GetAnchorMin();
            anchor_min.y = value.float_value;
            control->SetAnchorMin(anchor_min);
        } else if (propertyName == "anchor_right" && value.type == Lupine::AnimationPropertyType::Float) {
            glm::vec2 anchor_max = control->GetAnchorMax();
            anchor_max.x = value.float_value;
            control->SetAnchorMax(anchor_max);
        } else if (propertyName == "anchor_bottom" && value.type == Lupine::AnimationPropertyType::Float) {
            glm::vec2 anchor_max = control->GetAnchorMax();
            anchor_max.y = value.float_value;
            control->SetAnchorMax(anchor_max);
        }
    }

    // Handle component properties
    auto components = node->GetComponents<Lupine::Component>();
    for (auto* component : components) {
        if (component->GetTypeName() == "Sprite2D" && propertyName.startsWith("sprite.")) {
            // Handle Sprite2D component properties
            // This would require accessing the component's setter methods
            // For now, we'll leave this as a placeholder
        } else if (component->GetTypeName() == "Label" && propertyName.startsWith("label.")) {
            // Handle Label component properties
            // This would require accessing the component's setter methods
        }
        // Add more component property handling as needed
    }
}

Lupine::Node* TweenAnimatorDialog::findNodeByPath(const QString& nodePath) {
    if (!m_scene || !m_scene->GetRootNode()) return nullptr;

    QStringList pathParts = nodePath.split('/');
    if (pathParts.isEmpty()) return nullptr;

    Lupine::Node* current = m_scene->GetRootNode();

    // Skip root if path starts with root name
    int startIndex = 0;
    if (!pathParts.isEmpty() && pathParts[0] == QString::fromStdString(current->GetName())) {
        startIndex = 1;
    }

    for (int i = startIndex; i < pathParts.size(); ++i) {
        const QString& partName = pathParts[i];
        bool found = false;

        for (const auto& child : current->GetChildren()) {
            if (QString::fromStdString(child->GetName()) == partName) {
                current = child.get();
                found = true;
                break;
            }
        }

        if (!found) return nullptr;
    }

    return current;
}

Lupine::AnimationValue TweenAnimatorDialog::interpolateValue(const Lupine::AnimationValue& a, const Lupine::AnimationValue& b, float t) {
    if (a.type != b.type) return a;

    switch (a.type) {
        case Lupine::AnimationPropertyType::Float:
            return Lupine::AnimationValue(a.float_value + (b.float_value - a.float_value) * t);
        case Lupine::AnimationPropertyType::Vec2:
            return Lupine::AnimationValue(glm::mix(a.vec2_value, b.vec2_value, t));
        case Lupine::AnimationPropertyType::Vec3:
            return Lupine::AnimationValue(glm::mix(a.vec3_value, b.vec3_value, t));
        case Lupine::AnimationPropertyType::Vec4:
        case Lupine::AnimationPropertyType::Color:
            return Lupine::AnimationValue(glm::mix(a.vec4_value, b.vec4_value, t));
        case Lupine::AnimationPropertyType::Bool:
            return Lupine::AnimationValue(t < 0.5f ? a.bool_value : b.bool_value);
        case Lupine::AnimationPropertyType::Int:
            return Lupine::AnimationValue(static_cast<int>(a.int_value + (b.int_value - a.int_value) * t));
        default:
            return a;
    }
}

// Advanced interpolation function with easing support
Lupine::AnimationValue TweenAnimatorDialog::interpolateValueWithEasing(const Lupine::AnimationValue& a, const Lupine::AnimationValue& b, float t, Lupine::InterpolationType interpolation) {
    if (a.type != b.type) return a;

    // Apply easing function to t
    float easedT = applyEasing(t, interpolation);

    switch (a.type) {
        case Lupine::AnimationPropertyType::Float:
            return Lupine::AnimationValue(a.float_value + (b.float_value - a.float_value) * easedT);
        case Lupine::AnimationPropertyType::Vec2:
            return Lupine::AnimationValue(glm::mix(a.vec2_value, b.vec2_value, easedT));
        case Lupine::AnimationPropertyType::Vec3:
            return Lupine::AnimationValue(glm::mix(a.vec3_value, b.vec3_value, easedT));
        case Lupine::AnimationPropertyType::Vec4:
        case Lupine::AnimationPropertyType::Color:
            return Lupine::AnimationValue(glm::mix(a.vec4_value, b.vec4_value, easedT));
        case Lupine::AnimationPropertyType::Bool:
            return Lupine::AnimationValue(easedT < 0.5f ? a.bool_value : b.bool_value);
        case Lupine::AnimationPropertyType::Int:
            return Lupine::AnimationValue(static_cast<int>(a.int_value + (b.int_value - a.int_value) * easedT));
        default:
            return a;
    }
}

// Easing functions implementation
float TweenAnimatorDialog::applyEasing(float t, Lupine::InterpolationType interpolation) {
    t = std::clamp(t, 0.0f, 1.0f);

    switch (interpolation) {
        case Lupine::InterpolationType::Linear:
            return t;

        case Lupine::InterpolationType::Ease:
            // Smooth step (ease in-out)
            return t * t * (3.0f - 2.0f * t);

        case Lupine::InterpolationType::EaseIn:
            // Quadratic ease in
            return t * t;

        case Lupine::InterpolationType::EaseOut:
            // Quadratic ease out
            return 1.0f - (1.0f - t) * (1.0f - t);

        case Lupine::InterpolationType::EaseInOut:
            // Cubic ease in-out
            if (t < 0.5f) {
                return 4.0f * t * t * t;
            } else {
                float p = 2.0f * t - 2.0f;
                return 1.0f + p * p * p / 2.0f;
            }

        case Lupine::InterpolationType::Bounce:
            // Bounce ease out
            if (t < 1.0f / 2.75f) {
                return 7.5625f * t * t;
            } else if (t < 2.0f / 2.75f) {
                t -= 1.5f / 2.75f;
                return 7.5625f * t * t + 0.75f;
            } else if (t < 2.5f / 2.75f) {
                t -= 2.25f / 2.75f;
                return 7.5625f * t * t + 0.9375f;
            } else {
                t -= 2.625f / 2.75f;
                return 7.5625f * t * t + 0.984375f;
            }

        case Lupine::InterpolationType::Elastic:
        {
            // Elastic ease out
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;

            float p = 0.3f;
            float s = p / 4.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p) + 1.0f;
        }

        default:
            return t;
    }
}

Lupine::AnimationValue TweenAnimatorDialog::getValueFromPropertyEditor() {
    // Get the current property type
    QString nodePath = m_nodePathCombo->currentText();
    QString propertyName = m_propertyNameCombo->currentText();
    Lupine::AnimationPropertyType propType = getPropertyType(nodePath, propertyName);

    // Extract value from the appropriate widget
    switch (propType) {
        case Lupine::AnimationPropertyType::Float: {
            QDoubleSpinBox* spinBox = m_valueWidget->findChild<QDoubleSpinBox*>();
            if (spinBox) {
                return Lupine::AnimationValue(static_cast<float>(spinBox->value()));
            }
            break;
        }
        case Lupine::AnimationPropertyType::Vec2: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 2) {
                glm::vec2 value(static_cast<float>(spinBoxes[0]->value()),
                               static_cast<float>(spinBoxes[1]->value()));
                return Lupine::AnimationValue(value);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Vec3: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 3) {
                glm::vec3 value(static_cast<float>(spinBoxes[0]->value()),
                               static_cast<float>(spinBoxes[1]->value()),
                               static_cast<float>(spinBoxes[2]->value()));
                return Lupine::AnimationValue(value);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Color:
        case Lupine::AnimationPropertyType::Vec4: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 4) {
                glm::vec4 value(static_cast<float>(spinBoxes[0]->value()),
                               static_cast<float>(spinBoxes[1]->value()),
                               static_cast<float>(spinBoxes[2]->value()),
                               static_cast<float>(spinBoxes[3]->value()));
                return Lupine::AnimationValue(value);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Bool: {
            QCheckBox* checkBox = m_valueWidget->findChild<QCheckBox*>();
            if (checkBox) {
                return Lupine::AnimationValue(checkBox->isChecked());
            }
            break;
        }
        case Lupine::AnimationPropertyType::Int: {
            QSpinBox* spinBox = m_valueWidget->findChild<QSpinBox*>();
            if (spinBox) {
                return Lupine::AnimationValue(spinBox->value());
            }
            break;
        }
        default:
            break;
    }

    return Lupine::AnimationValue(0.0f);
}

void TweenAnimatorDialog::setValueInPropertyEditor(const Lupine::AnimationValue& value) {
    // Set value in the appropriate widget based on type
    switch (value.type) {
        case Lupine::AnimationPropertyType::Float: {
            QDoubleSpinBox* spinBox = m_valueWidget->findChild<QDoubleSpinBox*>();
            if (spinBox) {
                spinBox->setValue(value.float_value);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Vec2: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 2) {
                spinBoxes[0]->setValue(value.vec2_value.x);
                spinBoxes[1]->setValue(value.vec2_value.y);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Vec3: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 3) {
                spinBoxes[0]->setValue(value.vec3_value.x);
                spinBoxes[1]->setValue(value.vec3_value.y);
                spinBoxes[2]->setValue(value.vec3_value.z);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Color:
        case Lupine::AnimationPropertyType::Vec4: {
            QList<QDoubleSpinBox*> spinBoxes = m_valueWidget->findChildren<QDoubleSpinBox*>();
            if (spinBoxes.size() >= 4) {
                spinBoxes[0]->setValue(value.vec4_value.x);
                spinBoxes[1]->setValue(value.vec4_value.y);
                spinBoxes[2]->setValue(value.vec4_value.z);
                spinBoxes[3]->setValue(value.vec4_value.w);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Bool: {
            QCheckBox* checkBox = m_valueWidget->findChild<QCheckBox*>();
            if (checkBox) {
                checkBox->setChecked(value.bool_value);
            }
            break;
        }
        case Lupine::AnimationPropertyType::Int: {
            QSpinBox* spinBox = m_valueWidget->findChild<QSpinBox*>();
            if (spinBox) {
                spinBox->setValue(value.int_value);
            }
            break;
        }
        default:
            break;
    }
}

// Animation playback methods

void TweenAnimatorDialog::startPlayback() {
    m_isPlaying = true;
    m_playButton->setText("Pause");
    m_playbackTimer->start();
}

void TweenAnimatorDialog::stopPlayback() {
    m_isPlaying = false;
    m_playButton->setText("Play");
    m_playbackTimer->stop();
}

void TweenAnimatorDialog::updatePlayback() {
    if (!m_isPlaying) return;

    float deltaTime = TIMELINE_UPDATE_INTERVAL / 1000.0f; // Convert ms to seconds
    m_currentTime += deltaTime * m_playbackSpeed;

    float duration = getCurrentClipDuration();
    if (m_currentTime >= duration) {
        if (m_isLooping) {
            m_currentTime = 0.0f;
        } else {
            m_currentTime = duration;
            stopPlayback();
        }
    }

    onTimelinePositionChanged(m_currentTime);
}

// Timeline rendering methods

void TweenAnimatorDialog::renderTimeline() {
    m_timelineScene->clear();

    if (m_currentClip.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Set scene size with zoom
    float duration = clip->duration;
    int trackCount = clip->tracks.size();
    float scaledWidth = duration * 100 * m_timelineScale;
    m_timelineScene->setSceneRect(0, 0, scaledWidth, trackCount * m_trackHeight);

    // Render time ruler with major and minor ticks
    for (float time = 0; time <= duration; time += 0.1f) {
        float x = time * 100 * m_timelineScale;
        bool isMajorTick = (fmod(time, 1.0f) < 0.01f); // Major tick every second

        if (isMajorTick) {
            // Major tick - full height with time label
            m_timelineScene->addLine(x, 0, x, trackCount * m_trackHeight, QPen(Qt::darkGray, 2));
            QGraphicsTextItem* timeLabel = m_timelineScene->addText(QString::number(time, 'f', 1) + "s");
            timeLabel->setPos(x + 2, -20);
            QFont font = timeLabel->font();
            font.setPointSize(8);
            timeLabel->setFont(font);
        } else {
            // Minor tick - shorter
            m_timelineScene->addLine(x, 0, x, 20, QPen(Qt::gray));
        }
    }

    // Render tracks and keyframes using dedicated methods
    renderTracks();
    renderKeyframes();

    // Current time indicator
    float currentX = m_currentTime * 100 * m_timelineScale;
    m_timelineScene->addLine(currentX, 0, currentX, trackCount * m_trackHeight, QPen(Qt::red, 3));

    // Add time indicator label
    QGraphicsTextItem* timeIndicator = m_timelineScene->addText(QString::number(m_currentTime, 'f', 2) + "s");
    timeIndicator->setPos(currentX + 5, -20);
    timeIndicator->setDefaultTextColor(Qt::red);
    QFont font = timeIndicator->font();
    font.setPointSize(8);
    font.setBold(true);
    timeIndicator->setFont(font);
}

void TweenAnimatorDialog::renderTracks() {
    if (m_currentClip.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Render track backgrounds and labels
    for (size_t i = 0; i < clip->tracks.size(); ++i) {
        const auto& track = clip->tracks[i];
        float y = i * m_trackHeight;

        // Track background - alternate colors for better visibility
        QColor bgColor = (i % 2 == 0) ? QColor(240, 240, 240) : QColor(220, 220, 220);
        m_timelineScene->addRect(0, y, clip->duration * 100 * m_timelineScale, m_trackHeight,
                                QPen(Qt::darkGray), QBrush(bgColor));

        // Track label with node path and property
        QString trackLabel = QString::fromStdString(track.node_path + "/" + track.property_name);
        QGraphicsTextItem* labelItem = m_timelineScene->addText(trackLabel);
        labelItem->setPos(5, y + 2);

        // Make label smaller to fit better
        QFont font = labelItem->font();
        font.setPointSize(8);
        labelItem->setFont(font);

        // Track separator line
        m_timelineScene->addLine(0, y + m_trackHeight, clip->duration * 100 * m_timelineScale, y + m_trackHeight,
                                QPen(Qt::gray));
    }
}

void TweenAnimatorDialog::renderKeyframes() {
    if (m_currentClip.isEmpty()) return;

    const auto* clip = m_animationResource->GetClip(m_currentClip.toStdString());
    if (!clip) return;

    // Render keyframes for each track
    for (size_t i = 0; i < clip->tracks.size(); ++i) {
        const auto& track = clip->tracks[i];
        float y = i * m_trackHeight;

        // Render keyframes
        for (size_t j = 0; j < track.keyframes.size(); ++j) {
            const auto& keyframe = track.keyframes[j];
            float x = keyframe.time * 100 * m_timelineScale;

            // Keyframe diamond shape
            QPolygonF diamond;
            float halfSize = m_keyframeSize / 2;
            diamond << QPointF(x, y + m_trackHeight/2 - halfSize)
                   << QPointF(x + halfSize, y + m_trackHeight/2)
                   << QPointF(x, y + m_trackHeight/2 + halfSize)
                   << QPointF(x - halfSize, y + m_trackHeight/2);

            // Color based on interpolation type
            QColor keyframeColor = Qt::yellow;
            switch (keyframe.interpolation) {
                case Lupine::InterpolationType::Linear: keyframeColor = Qt::yellow; break;
                case Lupine::InterpolationType::Ease: keyframeColor = Qt::cyan; break;
                case Lupine::InterpolationType::EaseIn: keyframeColor = Qt::green; break;
                case Lupine::InterpolationType::EaseOut: keyframeColor = Qt::blue; break;
                case Lupine::InterpolationType::EaseInOut: keyframeColor = Qt::magenta; break;
                case Lupine::InterpolationType::Bounce: keyframeColor = Qt::red; break;
                case Lupine::InterpolationType::Elastic: keyframeColor = QColor(255, 165, 0); break; // Orange
                default: keyframeColor = Qt::yellow; break;
            }

            QGraphicsPolygonItem* kfItem = m_timelineScene->addPolygon(diamond,
                                                                      QPen(Qt::black),
                                                                      QBrush(keyframeColor));
            kfItem->setFlag(QGraphicsItem::ItemIsMovable);
            kfItem->setFlag(QGraphicsItem::ItemIsSelectable);

            // Store keyframe data for interaction
            kfItem->setData(0, static_cast<int>(i)); // Track index
            kfItem->setData(1, static_cast<int>(j)); // Keyframe index
        }

        // Draw interpolation curves between keyframes
        if (track.keyframes.size() > 1) {
            for (size_t j = 0; j < track.keyframes.size() - 1; ++j) {
                const auto& kf1 = track.keyframes[j];
                const auto& kf2 = track.keyframes[j + 1];

                float x1 = kf1.time * 100 * m_timelineScale;
                float x2 = kf2.time * 100 * m_timelineScale;
                float yCenter = y + m_trackHeight / 2;

                // Draw connection line
                QPen curvePen(Qt::darkBlue, 1, Qt::DashLine);
                m_timelineScene->addLine(x1, yCenter, x2, yCenter, curvePen);
            }
        }
    }
}

// ============================================================================
// AUTOKEY OPERATIONS
// ============================================================================

void TweenAnimatorDialog::onAutokeyToggled(bool enabled) {
    m_autokeyEnabled = enabled;

    // Update UI state
    if (m_autokeyButton) {
        m_autokeyButton->setChecked(enabled);
        m_autokeyButton->setStyleSheet(enabled ?
            "QPushButton { background-color: #ff4444; color: white; font-weight: bold; }" :
            "QPushButton { background-color: #666666; color: white; }");
    }

    // Update status
    if (m_statusLabel) {
        m_statusLabel->setText(enabled ? "Autokey: ON" : "Autokey: OFF");
    }

    // Connect/disconnect property change monitoring
    if (enabled) {
        connectPropertyMonitoring();
    } else {
        disconnectPropertyMonitoring();
    }
}

void TweenAnimatorDialog::onAutokeyModeChanged(int mode) {
    m_autokeyMode = static_cast<AutokeyMode>(mode);

    // Update UI to reflect mode
    if (m_autokeyModeCombo) {
        m_autokeyModeCombo->setCurrentIndex(mode);
    }

    QString modeText;
    switch (m_autokeyMode) {
        case AutokeyMode::All:
            modeText = "Recording all property changes";
            break;
        case AutokeyMode::Selected:
            modeText = "Recording selected properties only";
            break;
        case AutokeyMode::Modified:
            modeText = "Recording modified properties only";
            break;
    }

    if (m_statusLabel) {
        m_statusLabel->setText(modeText);
    }
}

void TweenAnimatorDialog::onRecordKeyframe() {
    if (!m_scene || m_selectedNodes.isEmpty()) {
        QMessageBox::warning(this, "Record Keyframe", "No nodes selected for keyframe recording.");
        return;
    }

    // Record keyframes for all selected nodes at current time
    for (Lupine::Node* node : m_selectedNodes) {
        recordNodeKeyframe(node, m_currentTime);
    }

    // Update timeline display
    updateTimelineDisplay();
    setModified(true);
}

void TweenAnimatorDialog::onRecordAllKeyframes() {
    if (!m_scene) {
        QMessageBox::warning(this, "Record All Keyframes", "No scene loaded.");
        return;
    }

    // Record keyframes for all animatable nodes in the scene
    std::function<void(Lupine::Node*)> recordNodeRecursive = [&](Lupine::Node* node) {
        if (node && hasAnimatableProperties(node)) {
            recordNodeKeyframe(node, m_currentTime);
        }

        // Recurse through children
        for (auto& child : node->GetChildren()) {
            recordNodeRecursive(child.get());
        }
    };

    recordNodeRecursive(m_scene->GetRootNode());

    // Update timeline display
    updateTimelineDisplay();
    setModified(true);
}

void TweenAnimatorDialog::onPropertyChanged(const QString& nodePath, const QString& propertyName, float time) {
    if (!m_autokeyEnabled) {
        return;
    }

    // Find the node by path
    Lupine::Node* node = findNodeByPath(nodePath);
    if (!node) {
        return;
    }

    // Check if we should record this property based on autokey mode
    bool shouldRecord = false;
    switch (m_autokeyMode) {
        case AutokeyMode::All:
            shouldRecord = true;
            break;
        case AutokeyMode::Selected:
            shouldRecord = m_selectedNodes.contains(node);
            break;
        case AutokeyMode::Modified:
            shouldRecord = isPropertyModified(node, propertyName);
            break;
    }

    if (shouldRecord) {
        recordPropertyKeyframe(node, propertyName, time);
        updateTimelineDisplay();
        setModified(true);
    }
}

// ============================================================================
// TIMELINE OPERATIONS
// ============================================================================

void TweenAnimatorDialog::onKeyframeSelected(const QList<int>& keyframes) {
    m_selectedKeyframes = keyframes;

    // Update UI to show selection
    updateKeyframeSelection();

    // Enable/disable keyframe operations based on selection
    bool hasSelection = !keyframes.isEmpty();
    if (m_deleteKeyframeButton) m_deleteKeyframeButton->setEnabled(hasSelection);
    if (m_copyKeyframeButton) m_copyKeyframeButton->setEnabled(hasSelection);

    // Update property panel to show selected keyframe properties
    if (hasSelection && !keyframes.isEmpty()) {
        updateKeyframeProperties(keyframes.first());
    }
}

void TweenAnimatorDialog::onKeyframesMoved(const QList<int>& keyframes, float deltaTime) {
    if (keyframes.isEmpty()) return;

    // Move selected keyframes by deltaTime
    for (int keyframeId : keyframes) {
        moveKeyframe(keyframeId, deltaTime);
    }

    // Update timeline display
    updateTimelineDisplay();
    setModified(true);

    // Update selection to reflect new positions
    onKeyframeSelected(keyframes);
}

void TweenAnimatorDialog::onKeyframesCopied() {
    if (m_selectedKeyframes.isEmpty()) {
        QMessageBox::information(this, "Copy Keyframes", "No keyframes selected to copy.");
        return;
    }

    // Store selected keyframes in clipboard
    m_keyframeClipboard.clear();
    for (int keyframeId : m_selectedKeyframes) {
        auto keyframeData = getKeyframeData(keyframeId);
        if (keyframeData.has_value()) {
            m_keyframeClipboard.append(keyframeData.value());
        }
    }

    // Enable paste button
    if (m_pasteKeyframeButton) {
        m_pasteKeyframeButton->setEnabled(!m_keyframeClipboard.isEmpty());
    }

    // Update status
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Copied %1 keyframes").arg(m_keyframeClipboard.size()));
    }
}

void TweenAnimatorDialog::onKeyframesPasted() {
    if (m_keyframeClipboard.isEmpty()) {
        QMessageBox::information(this, "Paste Keyframes", "No keyframes in clipboard to paste.");
        return;
    }

    // Paste keyframes at current time
    float baseTime = m_currentTime;
    QList<int> newKeyframes;

    for (const auto& keyframeData : m_keyframeClipboard) {
        int newKeyframeId = pasteKeyframe(keyframeData, baseTime);
        if (newKeyframeId >= 0) {
            newKeyframes.append(newKeyframeId);
        }
    }

    // Update timeline and select pasted keyframes
    updateTimelineDisplay();
    onKeyframeSelected(newKeyframes);
    setModified(true);

    // Update status
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Pasted %1 keyframes").arg(newKeyframes.size()));
    }
}

void TweenAnimatorDialog::onKeyframesDeleted() {
    if (m_selectedKeyframes.isEmpty()) {
        QMessageBox::information(this, "Delete Keyframes", "No keyframes selected to delete.");
        return;
    }

    // Confirm deletion
    int result = QMessageBox::question(this, "Delete Keyframes",
        QString("Delete %1 selected keyframes?").arg(m_selectedKeyframes.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (result != QMessageBox::Yes) {
        return;
    }

    // Delete keyframes (in reverse order to maintain indices)
    auto sortedKeyframes = m_selectedKeyframes;
    std::sort(sortedKeyframes.begin(), sortedKeyframes.end(), std::greater<int>());

    for (int keyframeId : sortedKeyframes) {
        deleteKeyframe(keyframeId);
    }

    // Clear selection and update display
    m_selectedKeyframes.clear();
    updateTimelineDisplay();
    setModified(true);

    // Update status
    if (m_statusLabel) {
        m_statusLabel->setText("Keyframes deleted");
    }
}

void TweenAnimatorDialog::onTimelineRightClick(const QPointF& position) {
    // Create context menu for timeline operations
    QMenu contextMenu(this);

    // Convert position to time
    float clickTime = (position.x() / m_timelineScale) / 100.0f;

    // Add keyframe at position
    QAction* addKeyframeAction = contextMenu.addAction("Add Keyframe");
    connect(addKeyframeAction, &QAction::triggered, [this, clickTime]() {
        setCurrentTime(clickTime);
        onRecordKeyframe();
    });

    contextMenu.addSeparator();

    // Timeline view options
    QAction* zoomInAction = contextMenu.addAction("Zoom In");
    connect(zoomInAction, &QAction::triggered, [this]() {
        setTimelineScale(m_timelineScale * 1.5f);
    });

    QAction* zoomOutAction = contextMenu.addAction("Zoom Out");
    connect(zoomOutAction, &QAction::triggered, [this]() {
        setTimelineScale(m_timelineScale / 1.5f);
    });

    QAction* fitAllAction = contextMenu.addAction("Fit All");
    connect(fitAllAction, &QAction::triggered, [this]() {
        fitTimelineToContent();
    });

    contextMenu.addSeparator();

    // Keyframe operations (if keyframes are selected)
    if (!m_selectedKeyframes.isEmpty()) {
        QAction* deleteAction = contextMenu.addAction("Delete Selected Keyframes");
        connect(deleteAction, &QAction::triggered, this, &TweenAnimatorDialog::onKeyframesDeleted);

        QAction* copyAction = contextMenu.addAction("Copy Selected Keyframes");
        connect(copyAction, &QAction::triggered, this, &TweenAnimatorDialog::onKeyframesCopied);
    }

    // Paste option (if clipboard has keyframes)
    if (!m_keyframeClipboard.isEmpty()) {
        QAction* pasteAction = contextMenu.addAction("Paste Keyframes");
        connect(pasteAction, &QAction::triggered, this, &TweenAnimatorDialog::onKeyframesPasted);
    }

    // Show context menu
    contextMenu.exec(QCursor::pos());
}

void TweenAnimatorDialog::onTimelineDoubleClick(const QPointF& position) {
    // Convert position to time and set current time
    float clickTime = (position.x() / m_timelineScale) / 100.0f;
    setCurrentTime(clickTime);

    // Auto-record keyframe if autokey is enabled
    if (m_autokeyEnabled) {
        onRecordKeyframe();
    }
}

// ============================================================================
// PROPERTY MANAGEMENT
// ============================================================================

void TweenAnimatorDialog::onPropertyFilterChanged() {
    if (!m_propertyFilterEdit) return;

    QString filterText = m_propertyFilterEdit->text().toLower();
    m_propertyFilter = filterText;

    // Refresh property list with filter applied
    refreshPropertyList();
}

void TweenAnimatorDialog::onRefreshProperties() {
    // Clear current property cache
    m_cachedProperties.clear();

    // Rebuild property list from selected nodes
    refreshPropertyList();

    // Update status
    if (m_statusLabel) {
        m_statusLabel->setText("Properties refreshed");
    }
}

void TweenAnimatorDialog::onPropertySelectionChanged() {
    if (!m_propertyList) return;

    // Get selected properties
    QList<QListWidgetItem*> selectedItems = m_propertyList->selectedItems();
    m_selectedProperties.clear();

    for (QListWidgetItem* item : selectedItems) {
        QString propertyPath = item->data(Qt::UserRole).toString();
        m_selectedProperties.append(propertyPath);
    }

    // Update property details panel
    updatePropertyDetails();

    // Enable/disable property operations
    bool hasSelection = !selectedItems.isEmpty();
    if (m_addKeyframeButton) m_addKeyframeButton->setEnabled(hasSelection);
}

// ============================================================================
// PANEL MANAGEMENT
// ============================================================================

void TweenAnimatorDialog::onPanelVisibilityChanged(const QString& panelName, bool visible) {
    QDockWidget* dock = findDockWidget(panelName);
    if (!dock) return;

    dock->setVisible(visible);

    // Update menu checkboxes
    updatePanelMenus();

    // Save panel state
    savePanelState();
}

void TweenAnimatorDialog::onPanelDockStateChanged(const QString& panelName, bool docked) {
    QDockWidget* dock = findDockWidget(panelName);
    if (!dock) return;

    if (docked) {
        // Re-dock the panel
        if (!dock->parent()) {
            addDockWidget(Qt::LeftDockWidgetArea, dock);
        }
        dock->setFloating(false);
    } else {
        // Undock the panel (make it floating)
        dock->setFloating(true);
    }

    // Update menu checkboxes
    updatePanelMenus();

    // Save panel state
    savePanelState();
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void TweenAnimatorDialog::connectPropertyMonitoring() {
    // Connect to property change signals from the scene
    // This would typically connect to signals from nodes when their properties change
    // For now, this is a placeholder for the monitoring system
}

void TweenAnimatorDialog::disconnectPropertyMonitoring() {
    // Disconnect property change monitoring
    // This would disconnect the signals connected in connectPropertyMonitoring()
}

void TweenAnimatorDialog::recordNodeKeyframe(Lupine::Node* node, float time) {
    if (!node || !m_reflectionSystem) return;

    // Get all animatable properties for this node
    auto properties = m_reflectionSystem->DiscoverProperties(node);

    for (const auto& prop : properties) {
        if (m_reflectionSystem->IsPropertyAnimatable(prop)) {
            recordPropertyKeyframe(node, QString::fromStdString(prop.name), time);
        }
    }
}

void TweenAnimatorDialog::recordPropertyKeyframe(Lupine::Node* node, const QString& propertyName, float time) {
    if (!node || !m_reflectionSystem) return;

    // Get current property value
    auto value = m_reflectionSystem->GetPropertyValue(node, propertyName.toStdString());
    if (!value.IsValid()) return;

    // Create keyframe data
    // This would typically add the keyframe to the animation resource
    // For now, this is a placeholder implementation
}

bool TweenAnimatorDialog::hasAnimatableProperties(Lupine::Node* node) {
    if (!node || !m_reflectionSystem) return false;

    auto properties = m_reflectionSystem->DiscoverProperties(node);
    for (const auto& prop : properties) {
        if (m_reflectionSystem->IsPropertyAnimatable(prop)) {
            return true;
        }
    }
    return false;
}



bool TweenAnimatorDialog::isPropertyModified(Lupine::Node* node, const QString& propertyName) {
    // Check if property has been modified from its default value
    // This would typically compare against a baseline state
    // For now, return true as placeholder
    return true;
}

void TweenAnimatorDialog::updateTimelineDisplay() {
    // Update the timeline widget to reflect current keyframes
    // This would redraw the timeline with current keyframe positions
}

void TweenAnimatorDialog::setModified(bool modified) {
    m_isModified = modified;

    // Update window title to show modified state
    QString title = "Tween Animator";
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
    }
    if (modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void TweenAnimatorDialog::updateKeyframeSelection() {
    // Update UI to show selected keyframes
    // This would highlight selected keyframes in the timeline
}

void TweenAnimatorDialog::updateKeyframeProperties(int keyframeId) {
    // Update property panel to show properties of selected keyframe
    // This would populate the property editor with keyframe data
}

void TweenAnimatorDialog::moveKeyframe(int keyframeId, float deltaTime) {
    // Move a keyframe by the specified time delta
    // This would update the keyframe's time position in the animation data
}

std::optional<KeyframeData> TweenAnimatorDialog::getKeyframeData(int keyframeId) {
    // Get keyframe data for clipboard operations
    // This would retrieve the keyframe data from the animation resource
    return std::nullopt; // Placeholder
}

int TweenAnimatorDialog::pasteKeyframe(const KeyframeData& data, float baseTime) {
    // Paste keyframe data at the specified time
    // This would create a new keyframe in the animation resource
    return -1; // Placeholder - return new keyframe ID
}

void TweenAnimatorDialog::deleteKeyframe(int keyframeId) {
    // Delete the specified keyframe
    // This would remove the keyframe from the animation resource
}

void TweenAnimatorDialog::setCurrentTime(float time) {
    m_currentTime = time;

    // Update UI to reflect current time
    if (m_currentTimeSpin) {
        m_currentTimeSpin->setValue(time);
    }
    if (m_timeSlider) {
        m_timeSlider->setValue(static_cast<int>(time * 100));
    }
}

void TweenAnimatorDialog::setTimelineScale(float scale) {
    m_timelineScale = qBound(MIN_TIMELINE_SCALE, scale, MAX_TIMELINE_SCALE);
    updateTimelineDisplay();
}

void TweenAnimatorDialog::fitTimelineToContent() {
    // Calculate appropriate scale to fit all keyframes
    // This would analyze the animation data and set an appropriate scale
    setTimelineScale(1.0f); // Placeholder
}

void TweenAnimatorDialog::refreshPropertyList() {
    if (!m_propertyList) return;

    m_propertyList->clear();

    // Populate property list from selected nodes
    for (Lupine::Node* node : m_selectedNodes) {
        if (!node || !m_reflectionSystem) continue;

        auto properties = m_reflectionSystem->DiscoverProperties(node);
        for (const auto& prop : properties) {
            // Apply filter
            if (!m_propertyFilter.isEmpty() &&
                !QString::fromStdString(prop.name).toLower().contains(m_propertyFilter)) {
                continue;
            }

            QString displayText = QString("%1.%2").arg(QString::fromStdString(node->GetName()),
                                                      QString::fromStdString(prop.displayName));
            QListWidgetItem* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, QString::fromStdString(prop.name));
            m_propertyList->addItem(item);
        }
    }
}

void TweenAnimatorDialog::updatePropertyDetails() {
    // Update property details panel based on selection
    // This would show detailed information about selected properties
}

QDockWidget* TweenAnimatorDialog::findDockWidget(const QString& panelName) {
    // Find dock widget by name
    if (panelName == "Animation") return m_animationDock;
    if (panelName == "Track") return m_trackDock;
    if (panelName == "Timeline") return m_timelineDock;
    if (panelName == "Property") return m_propertyDock;
    if (panelName == "Preview") return m_previewDock;
    if (panelName == "Autokey") return m_autokeyDock;
    return nullptr;
}

void TweenAnimatorDialog::updatePanelMenus() {
    // Update panel visibility checkboxes in menus
    // This would sync menu items with actual panel visibility
}

void TweenAnimatorDialog::savePanelState() {
    // Save current panel layout and visibility state
    // This would typically save to QSettings or similar
}
