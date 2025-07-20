#include "AudioMixerDialog.h"
#include <QApplication>
#include <QStyle>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpacerItem>
#include <QCloseEvent>
#include <QUrl>
#include <QMimeData>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

// Include libsndfile
extern "C" {
#include <sndfile.h>
}

// AudioTrack Implementation
AudioTrack::AudioTrack()
    : name("New Track")
    , filePath("")
    , enabled(true)
    , muted(false)
    , solo(false)
    , volume(1.0f)
    , pitch(1.0f)
    , pan(0.0f)
    , looping(false)
    , startTime(0.0)
    , duration(0.0)
    , fadeInTime(0.0)
    , fadeOutTime(0.0)
    , sampleRate(44100)
    , channels(2) {
}

AudioTrack::~AudioTrack() {
    clear();
}

bool AudioTrack::loadFromFile(const QString& path) {
    // Clear existing data
    clear();
    
    // Open file with libsndfile
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* file = sf_open(path.toUtf8().constData(), SFM_READ, &sfInfo);
    if (!file) {
        std::cerr << "Failed to open audio file: " << path.toStdString() << std::endl;
        std::cerr << "Error: " << sf_strerror(nullptr) << std::endl;
        return false;
    }
    
    // Store file info
    filePath = path;
    name = QFileInfo(path).baseName();
    sampleRate = sfInfo.samplerate;
    channels = sfInfo.channels;
    duration = static_cast<double>(sfInfo.frames) / sfInfo.samplerate;
    
    // Read audio data
    audioData.resize(sfInfo.frames * sfInfo.channels);
    sf_count_t framesRead = sf_readf_float(file, audioData.data(), sfInfo.frames);
    
    sf_close(file);
    
    if (framesRead != sfInfo.frames) {
        std::cerr << "Warning: Only read " << framesRead << " of " << sfInfo.frames << " frames" << std::endl;
        audioData.resize(framesRead * sfInfo.channels);
        duration = static_cast<double>(framesRead) / sfInfo.samplerate;
    }
    
    std::cout << "Loaded audio file: " << path.toStdString() << std::endl;
    std::cout << "  Duration: " << duration << " seconds" << std::endl;
    std::cout << "  Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << channels << std::endl;
    std::cout << "  Frames: " << framesRead << std::endl;
    
    return true;
}

void AudioTrack::clear() {
    audioData.clear();
    filePath.clear();
    duration = 0.0;
}

bool AudioTrack::isValid() const {
    return !audioData.empty() && duration > 0.0;
}

// AudioTimelineWidget Implementation
AudioTimelineWidget::AudioTimelineWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_playbackPosition(0.0)
    , m_zoom(1.0)
    , m_selectionStart(0.0)
    , m_selectionEnd(0.0)
    , m_hasSelection(false)
    , m_dragging(false)
    , m_draggedTrack(-1) {
    
    setupScene();
    setAcceptDrops(true);
    setDragMode(QGraphicsView::RubberBandDrag);
}

AudioTimelineWidget::~AudioTimelineWidget() {
    delete m_scene;
}

void AudioTimelineWidget::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Set scene properties
    m_scene->setSceneRect(0, 0, 1000, 400);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void AudioTimelineWidget::addTrack(std::shared_ptr<AudioTrack> track) {
    if (track && track->isValid()) {
        m_tracks.push_back(track);
        updateScene();
    }
}

void AudioTimelineWidget::removeTrack(int trackIndex) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
        m_tracks.erase(m_tracks.begin() + trackIndex);
        updateScene();
    }
}

void AudioTimelineWidget::clearTracks() {
    m_tracks.clear();
    updateScene();
}

void AudioTimelineWidget::setPlaybackPosition(double seconds) {
    m_playbackPosition = seconds;
    drawPlaybackCursor();
}

double AudioTimelineWidget::getPlaybackPosition() const {
    return m_playbackPosition;
}

void AudioTimelineWidget::setZoom(double zoom) {
    m_zoom = std::max(0.1, std::min(10.0, zoom));
    updateScene();
}

double AudioTimelineWidget::getZoom() const {
    return m_zoom;
}

void AudioTimelineWidget::setSelection(double startTime, double endTime) {
    m_selectionStart = startTime;
    m_selectionEnd = endTime;
    m_hasSelection = true;
    updateScene();
}

void AudioTimelineWidget::clearSelection() {
    m_hasSelection = false;
    updateScene();
}

void AudioTimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        double timePos = scenePos.x() / (100.0 * m_zoom); // 100 pixels per second at zoom 1.0
        
        // Check if clicking on a track
        int trackIndex = static_cast<int>(scenePos.y() / 80); // 80 pixels per track
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            m_draggedTrack = trackIndex;
            m_dragging = true;
        } else {
            // Set playback position
            m_playbackPosition = std::max(0.0, timePos);
            emit playbackPositionChanged(m_playbackPosition);
            drawPlaybackCursor();
        }
        
        m_lastMousePos = scenePos;
    }
    
    QGraphicsView::mousePressEvent(event);
}

void AudioTimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && m_draggedTrack >= 0) {
        QPointF scenePos = mapToScene(event->pos());
        double timePos = scenePos.x() / (100.0 * m_zoom);
        
        // Update track start time
        if (m_draggedTrack < static_cast<int>(m_tracks.size())) {
            m_tracks[m_draggedTrack]->startTime = std::max(0.0, timePos);
            emit trackMoved(m_draggedTrack, m_tracks[m_draggedTrack]->startTime);
            updateScene();
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void AudioTimelineWidget::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    m_draggedTrack = -1;
    QGraphicsView::mouseReleaseEvent(event);
}

void AudioTimelineWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl+Wheel
        double scaleFactor = event->angleDelta().y() > 0 ? 1.2 : 0.8;
        setZoom(m_zoom * scaleFactor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void AudioTimelineWidget::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    updateScene();
}

void AudioTimelineWidget::updateScene() {
    if (!m_scene) return;
    
    m_scene->clear();
    
    // Calculate scene dimensions
    double maxTime = 10.0; // Minimum 10 seconds
    for (const auto& track : m_tracks) {
        if (track && track->isValid()) {
            maxTime = std::max(maxTime, track->startTime + track->duration);
        }
    }
    
    double sceneWidth = maxTime * 100.0 * m_zoom; // 100 pixels per second at zoom 1.0
    double sceneHeight = std::max(400.0, m_tracks.size() * 80.0 + 100.0); // 80 pixels per track + margin
    
    m_scene->setSceneRect(0, 0, sceneWidth, sceneHeight);
    
    // Draw timeline
    drawTimeline();
    
    // Draw tracks
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i] && m_tracks[i]->isValid()) {
            drawWaveform(m_tracks[i], static_cast<int>(i));
        }
    }
    
    // Draw playback cursor
    drawPlaybackCursor();
}

void AudioTimelineWidget::drawWaveform(std::shared_ptr<AudioTrack> track, int trackIndex) {
    if (!track || !track->isValid()) return;
    
    double trackY = trackIndex * 80.0 + 10.0;
    double trackHeight = 60.0;
    double startX = track->startTime * 100.0 * m_zoom;
    double width = track->duration * 100.0 * m_zoom;
    
    // Draw track background
    QColor bgColor = track->enabled ? QColor(60, 60, 80) : QColor(40, 40, 40);
    if (track->muted) bgColor = QColor(80, 40, 40);
    if (track->solo) bgColor = QColor(40, 80, 40);
    
    m_scene->addRect(startX, trackY, width, trackHeight, QPen(Qt::black), QBrush(bgColor));
    
    // Draw simplified waveform
    if (width > 10.0) { // Only draw if track is wide enough
        QPen waveformPen(QColor(150, 200, 255), 1);
        
        int samples = std::min(static_cast<int>(width), 1000); // Limit samples for performance
        double samplesPerPixel = track->audioData.size() / (track->channels * width);
        
        for (int i = 0; i < samples; ++i) {
            double x = startX + (i * width / samples);
            int sampleIndex = static_cast<int>(i * samplesPerPixel) * track->channels;
            
            if (sampleIndex < static_cast<int>(track->audioData.size())) {
                float amplitude = 0.0f;
                // Average all channels
                for (int ch = 0; ch < track->channels; ++ch) {
                    if (sampleIndex + ch < static_cast<int>(track->audioData.size())) {
                        amplitude += std::abs(track->audioData[sampleIndex + ch]);
                    }
                }
                amplitude /= track->channels;
                
                double waveHeight = amplitude * trackHeight * 0.8;
                double centerY = trackY + trackHeight / 2.0;
                
                m_scene->addLine(x, centerY - waveHeight/2, x, centerY + waveHeight/2, waveformPen);
            }
        }
    }
    
    // Draw track name
    QGraphicsTextItem* nameText = m_scene->addText(track->name, QFont("Arial", 10));
    nameText->setPos(startX + 5, trackY + 5);
    nameText->setDefaultTextColor(Qt::white);
}

void AudioTimelineWidget::drawTimeline() {
    // Draw time markers
    double maxTime = m_scene->sceneRect().width() / (100.0 * m_zoom);
    double timeStep = 1.0; // 1 second intervals
    
    // Adjust time step based on zoom
    if (m_zoom > 5.0) timeStep = 0.1;
    else if (m_zoom > 2.0) timeStep = 0.5;
    else if (m_zoom < 0.5) timeStep = 5.0;
    else if (m_zoom < 0.2) timeStep = 10.0;
    
    QPen timelinePen(QColor(100, 100, 100), 1);
    QPen majorPen(QColor(150, 150, 150), 2);
    
    for (double t = 0; t <= maxTime; t += timeStep) {
        double x = t * 100.0 * m_zoom;
        bool isMajor = (static_cast<int>(t) % 5 == 0);
        
        QPen& pen = isMajor ? majorPen : timelinePen;
        double lineHeight = isMajor ? 20.0 : 10.0;
        
        m_scene->addLine(x, 0, x, lineHeight, pen);
        
        if (isMajor) {
            // Add time label
            QString timeStr = QString("%1s").arg(t, 0, 'f', 1);
            QGraphicsTextItem* timeText = m_scene->addText(timeStr, QFont("Arial", 8));
            timeText->setPos(x + 2, 0);
            timeText->setDefaultTextColor(Qt::lightGray);
        }
    }
}

void AudioTimelineWidget::drawPlaybackCursor() {
    // Remove existing cursor
    for (auto item : m_scene->items()) {
        if (item->data(0).toString() == "playback_cursor") {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    // Draw new cursor
    double x = m_playbackPosition * 100.0 * m_zoom;
    QPen cursorPen(QColor(255, 100, 100), 3);
    
    QGraphicsLineItem* cursor = m_scene->addLine(x, 0, x, m_scene->sceneRect().height(), cursorPen);
    cursor->setData(0, "playback_cursor");
    cursor->setZValue(1000); // Bring to front
}



// AudioTrackControlWidget Implementation
AudioTrackControlWidget::AudioTrackControlWidget(std::shared_ptr<AudioTrack> track, QWidget* parent)
    : QWidget(parent)
    , m_track(track)
    , m_nameLabel(nullptr)
    , m_enabledCheckBox(nullptr)
    , m_muteCheckBox(nullptr)
    , m_soloCheckBox(nullptr)
    , m_volumeSlider(nullptr)
    , m_volumeLabel(nullptr)
    , m_pitchSpinBox(nullptr)
    , m_panSlider(nullptr)
    , m_panLabel(nullptr)
    , m_loopingCheckBox(nullptr)
    , m_removeButton(nullptr) {

    setupUI();
    updateFromTrack();
}

AudioTrackControlWidget::~AudioTrackControlWidget() {
}

void AudioTrackControlWidget::setupUI() {
    setFixedHeight(120);
    setStyleSheet("QWidget { border: 1px solid gray; margin: 2px; padding: 4px; }");

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(8);

    // Track name and controls
    QVBoxLayout* nameLayout = new QVBoxLayout();
    m_nameLabel = new QLabel(m_track ? m_track->name : "Track");
    m_nameLabel->setFont(QFont("Arial", 10, QFont::Bold));
    m_nameLabel->setMinimumWidth(100);
    nameLayout->addWidget(m_nameLabel);

    QHBoxLayout* checkboxLayout = new QHBoxLayout();
    m_enabledCheckBox = new QCheckBox("On");
    m_muteCheckBox = new QCheckBox("Mute");
    m_soloCheckBox = new QCheckBox("Solo");
    checkboxLayout->addWidget(m_enabledCheckBox);
    checkboxLayout->addWidget(m_muteCheckBox);
    checkboxLayout->addWidget(m_soloCheckBox);
    nameLayout->addLayout(checkboxLayout);

    mainLayout->addLayout(nameLayout);

    // Volume control
    QVBoxLayout* volumeLayout = new QVBoxLayout();
    volumeLayout->addWidget(new QLabel("Volume"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setMinimumWidth(80);
    volumeLayout->addWidget(m_volumeSlider);
    m_volumeLabel = new QLabel("100%");
    m_volumeLabel->setAlignment(Qt::AlignCenter);
    volumeLayout->addWidget(m_volumeLabel);
    mainLayout->addLayout(volumeLayout);

    // Pitch control
    QVBoxLayout* pitchLayout = new QVBoxLayout();
    pitchLayout->addWidget(new QLabel("Pitch"));
    m_pitchSpinBox = new QDoubleSpinBox();
    m_pitchSpinBox->setRange(0.1, 3.0);
    m_pitchSpinBox->setSingleStep(0.1);
    m_pitchSpinBox->setValue(1.0);
    m_pitchSpinBox->setDecimals(2);
    m_pitchSpinBox->setMinimumWidth(70);
    pitchLayout->addWidget(m_pitchSpinBox);
    pitchLayout->addStretch();
    mainLayout->addLayout(pitchLayout);

    // Pan control
    QVBoxLayout* panLayout = new QVBoxLayout();
    panLayout->addWidget(new QLabel("Pan"));
    m_panSlider = new QSlider(Qt::Horizontal);
    m_panSlider->setRange(-100, 100);
    m_panSlider->setValue(0);
    m_panSlider->setMinimumWidth(80);
    panLayout->addWidget(m_panSlider);
    m_panLabel = new QLabel("Center");
    m_panLabel->setAlignment(Qt::AlignCenter);
    panLayout->addWidget(m_panLabel);
    mainLayout->addLayout(panLayout);

    // Additional controls
    QVBoxLayout* extraLayout = new QVBoxLayout();
    m_loopingCheckBox = new QCheckBox("Loop");
    extraLayout->addWidget(m_loopingCheckBox);
    m_removeButton = new QPushButton("Remove");
    m_removeButton->setMaximumWidth(60);
    extraLayout->addWidget(m_removeButton);
    mainLayout->addLayout(extraLayout);

    // Connect signals
    connect(m_enabledCheckBox, &QCheckBox::toggled, this, &AudioTrackControlWidget::onEnabledChanged);
    connect(m_muteCheckBox, &QCheckBox::toggled, this, &AudioTrackControlWidget::onMuteChanged);
    connect(m_soloCheckBox, &QCheckBox::toggled, this, &AudioTrackControlWidget::onSoloChanged);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioTrackControlWidget::onVolumeChanged);
    connect(m_pitchSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AudioTrackControlWidget::onPitchChanged);
    connect(m_panSlider, &QSlider::valueChanged, this, &AudioTrackControlWidget::onPanChanged);
    connect(m_loopingCheckBox, &QCheckBox::toggled, this, &AudioTrackControlWidget::onLoopingChanged);
    connect(m_removeButton, &QPushButton::clicked, this, &AudioTrackControlWidget::onRemoveClicked);
}

void AudioTrackControlWidget::updateFromTrack() {
    if (!m_track) return;

    m_nameLabel->setText(m_track->name);
    m_enabledCheckBox->setChecked(m_track->enabled);
    m_muteCheckBox->setChecked(m_track->muted);
    m_soloCheckBox->setChecked(m_track->solo);
    m_volumeSlider->setValue(static_cast<int>(m_track->volume * 100));
    m_pitchSpinBox->setValue(m_track->pitch);
    m_panSlider->setValue(static_cast<int>(m_track->pan * 100));
    m_loopingCheckBox->setChecked(m_track->looping);

    // Update labels
    m_volumeLabel->setText(QString("%1%").arg(static_cast<int>(m_track->volume * 100)));

    if (m_track->pan < -0.01) {
        m_panLabel->setText(QString("L%1").arg(static_cast<int>(-m_track->pan * 100)));
    } else if (m_track->pan > 0.01) {
        m_panLabel->setText(QString("R%1").arg(static_cast<int>(m_track->pan * 100)));
    } else {
        m_panLabel->setText("Center");
    }
}

void AudioTrackControlWidget::onVolumeChanged(int value) {
    if (m_track) {
        m_track->volume = value / 100.0f;
        m_volumeLabel->setText(QString("%1%").arg(value));
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onPitchChanged(double value) {
    if (m_track) {
        m_track->pitch = static_cast<float>(value);
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onPanChanged(int value) {
    if (m_track) {
        m_track->pan = value / 100.0f;

        if (value < -1) {
            m_panLabel->setText(QString("L%1").arg(-value));
        } else if (value > 1) {
            m_panLabel->setText(QString("R%1").arg(value));
        } else {
            m_panLabel->setText("Center");
        }

        emit trackChanged();
    }
}

void AudioTrackControlWidget::onLoopingChanged(bool enabled) {
    if (m_track) {
        m_track->looping = enabled;
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onEnabledChanged(bool enabled) {
    if (m_track) {
        m_track->enabled = enabled;
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onMuteChanged(bool mute) {
    if (m_track) {
        m_track->muted = mute;
        emit muteChanged(mute);
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onSoloChanged(bool solo) {
    if (m_track) {
        m_track->solo = solo;
        emit soloChanged(solo);
        emit trackChanged();
    }
}

void AudioTrackControlWidget::onRemoveClicked() {
    emit removeRequested();
}

// AudioMixerDialog Implementation
AudioMixerDialog::AudioMixerDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_timelineWidget(nullptr)
    , m_trackControlsArea(nullptr)
    , m_trackControlsWidget(nullptr)
    , m_trackControlsLayout(nullptr)
    , m_transportWidget(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_stopButton(nullptr)
    , m_recordButton(nullptr)
    , m_rewindButton(nullptr)
    , m_fastForwardButton(nullptr)
    , m_timeLabel(nullptr)
    , m_masterWidget(nullptr)
    , m_masterVolumeSlider(nullptr)
    , m_masterVolumeLabel(nullptr)
    , m_masterPanSlider(nullptr)
    , m_masterPanLabel(nullptr)
    , m_currentProjectPath("")
    , m_modified(false)
    , m_playing(false)
    , m_paused(false)
    , m_playbackPosition(0.0)
    , m_playbackTimer(nullptr)
    , m_masterVolume(1.0f)
    , m_masterPan(0.0f)
    , m_audioDevice(0)
    , m_audioInitialized(false) {

    setWindowTitle("Audio Mixer");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    setAcceptDrops(true);

    setupUI();
    updateWindowTitle();
    updateTransportButtons();

    // Setup playback timer
    m_playbackTimer = new QTimer(this);
    connect(m_playbackTimer, &QTimer::timeout, this, &AudioMixerDialog::onPlaybackTimer);

    // Initialize audio system
    initializeAudio();
}

AudioMixerDialog::~AudioMixerDialog() {
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }

    // Cleanup audio system
    if (m_audioInitialized && m_audioDevice != 0) {
        SDL_CloseAudioDevice(m_audioDevice);
    }
}

void AudioMixerDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();
    setupMainLayout();
    setupTransportControls();
    setupMasterControls();
}

void AudioMixerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New Project", this, &AudioMixerDialog::onNewProject, QKeySequence::New);
    fileMenu->addAction("&Open Project...", this, &AudioMixerDialog::onOpenProject, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Project", this, &AudioMixerDialog::onSaveProject, QKeySequence::Save);
    fileMenu->addAction("Save Project &As...", this, &AudioMixerDialog::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Import Audio...", this, &AudioMixerDialog::onImportAudio, QKeySequence("Ctrl+I"));
    fileMenu->addAction("&Export Audio...", this, &AudioMixerDialog::onExportAudio, QKeySequence("Ctrl+E"));

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo", this, &AudioMixerDialog::onUndo, QKeySequence::Undo);
    editMenu->addAction("&Redo", this, &AudioMixerDialog::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction("Cu&t", this, &AudioMixerDialog::onCut, QKeySequence::Cut);
    editMenu->addAction("&Copy", this, &AudioMixerDialog::onCopy, QKeySequence::Copy);
    editMenu->addAction("&Paste", this, &AudioMixerDialog::onPaste, QKeySequence::Paste);
    editMenu->addAction("&Delete", this, &AudioMixerDialog::onDelete, QKeySequence::Delete);
    editMenu->addSeparator();
    editMenu->addAction("Select &All", this, &AudioMixerDialog::onSelectAll, QKeySequence::SelectAll);

    // Track menu
    QMenu* trackMenu = m_menuBar->addMenu("&Track");
    trackMenu->addAction("&Add Track", this, &AudioMixerDialog::onAddTrack, QKeySequence("Ctrl+T"));
    trackMenu->addAction("&Remove Track", this, &AudioMixerDialog::onRemoveTrack, QKeySequence("Ctrl+R"));
    trackMenu->addAction("&Duplicate Track", this, &AudioMixerDialog::onDuplicateTrack, QKeySequence("Ctrl+D"));

    // Transport menu
    QMenu* transportMenu = m_menuBar->addMenu("&Transport");
    transportMenu->addAction("&Play", this, &AudioMixerDialog::onPlay, QKeySequence("Space"));
    transportMenu->addAction("P&ause", this, &AudioMixerDialog::onPause, QKeySequence("Ctrl+Space"));
    transportMenu->addAction("&Stop", this, &AudioMixerDialog::onStop, QKeySequence("Ctrl+."));
    transportMenu->addAction("&Record", this, &AudioMixerDialog::onRecord, QKeySequence("Ctrl+R"));
    transportMenu->addSeparator();
    transportMenu->addAction("Re&wind", this, &AudioMixerDialog::onRewind, QKeySequence("Home"));
    transportMenu->addAction("&Fast Forward", this, &AudioMixerDialog::onFastForward, QKeySequence("End"));
}

void AudioMixerDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_mainLayout->addWidget(m_toolBar);

    // Transport controls in toolbar
    m_toolBar->addAction("New", this, &AudioMixerDialog::onNewProject);
    m_toolBar->addAction("Open", this, &AudioMixerDialog::onOpenProject);
    m_toolBar->addAction("Save", this, &AudioMixerDialog::onSaveProject);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Import", this, &AudioMixerDialog::onImportAudio);
    m_toolBar->addAction("Export", this, &AudioMixerDialog::onExportAudio);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Add Track", this, &AudioMixerDialog::onAddTrack);
}

void AudioMixerDialog::setupMainLayout() {
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupTrackArea();

    // Timeline widget
    m_timelineWidget = new AudioTimelineWidget(this);
    m_mainSplitter->addWidget(m_timelineWidget);

    // Set splitter proportions
    m_mainSplitter->setSizes({300, 900});

    // Connect timeline signals
    connect(m_timelineWidget, &AudioTimelineWidget::playbackPositionChanged,
            this, &AudioMixerDialog::onPlaybackPositionChanged);
    connect(m_timelineWidget, &AudioTimelineWidget::trackMoved,
            this, [this](int trackIndex, double newStartTime) {
                Q_UNUSED(trackIndex)
                Q_UNUSED(newStartTime)
                setModified(true);
            });
}

void AudioMixerDialog::setupTrackArea() {
    // Track controls area
    m_trackControlsArea = new QScrollArea(this);
    m_trackControlsArea->setWidgetResizable(true);
    m_trackControlsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_trackControlsArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_trackControlsArea->setMinimumWidth(300);
    m_trackControlsArea->setMaximumWidth(400);

    m_trackControlsWidget = new QWidget();
    m_trackControlsLayout = new QVBoxLayout(m_trackControlsWidget);
    m_trackControlsLayout->setAlignment(Qt::AlignTop);
    m_trackControlsLayout->setSpacing(2);

    m_trackControlsArea->setWidget(m_trackControlsWidget);
    m_mainSplitter->addWidget(m_trackControlsArea);
}

void AudioMixerDialog::setupTransportControls() {
    m_transportWidget = new QWidget(this);
    QHBoxLayout* transportLayout = new QHBoxLayout(m_transportWidget);

    // Transport buttons
    m_rewindButton = new QPushButton("⏮");
    m_playButton = new QPushButton("▶");
    m_pauseButton = new QPushButton("⏸");
    m_stopButton = new QPushButton("⏹");
    m_recordButton = new QPushButton("⏺");
    m_fastForwardButton = new QPushButton("⏭");

    // Style buttons
    QString buttonStyle = "QPushButton { font-size: 16px; min-width: 40px; min-height: 30px; }";
    m_rewindButton->setStyleSheet(buttonStyle);
    m_playButton->setStyleSheet(buttonStyle);
    m_pauseButton->setStyleSheet(buttonStyle);
    m_stopButton->setStyleSheet(buttonStyle);
    m_recordButton->setStyleSheet(buttonStyle + "QPushButton { color: red; }");
    m_fastForwardButton->setStyleSheet(buttonStyle);

    // Time display
    m_timeLabel = new QLabel("00:00.000");
    m_timeLabel->setFont(QFont("Courier", 12));
    m_timeLabel->setStyleSheet("QLabel { background: black; color: lime; padding: 4px; }");

    // Layout
    transportLayout->addWidget(m_rewindButton);
    transportLayout->addWidget(m_playButton);
    transportLayout->addWidget(m_pauseButton);
    transportLayout->addWidget(m_stopButton);
    transportLayout->addWidget(m_recordButton);
    transportLayout->addWidget(m_fastForwardButton);
    transportLayout->addSpacing(20);
    transportLayout->addWidget(m_timeLabel);
    transportLayout->addStretch();

    m_mainLayout->addWidget(m_transportWidget);

    // Connect signals
    connect(m_rewindButton, &QPushButton::clicked, this, &AudioMixerDialog::onRewind);
    connect(m_playButton, &QPushButton::clicked, this, &AudioMixerDialog::onPlay);
    connect(m_pauseButton, &QPushButton::clicked, this, &AudioMixerDialog::onPause);
    connect(m_stopButton, &QPushButton::clicked, this, &AudioMixerDialog::onStop);
    connect(m_recordButton, &QPushButton::clicked, this, &AudioMixerDialog::onRecord);
    connect(m_fastForwardButton, &QPushButton::clicked, this, &AudioMixerDialog::onFastForward);
}

void AudioMixerDialog::setupMasterControls() {
    m_masterWidget = new QWidget(this);
    QHBoxLayout* masterLayout = new QHBoxLayout(m_masterWidget);

    // Master volume
    QVBoxLayout* volumeLayout = new QVBoxLayout();
    volumeLayout->addWidget(new QLabel("Master Volume"));
    m_masterVolumeSlider = new QSlider(Qt::Horizontal);
    m_masterVolumeSlider->setRange(0, 100);
    m_masterVolumeSlider->setValue(100);
    m_masterVolumeSlider->setMinimumWidth(150);
    volumeLayout->addWidget(m_masterVolumeSlider);
    m_masterVolumeLabel = new QLabel("100%");
    m_masterVolumeLabel->setAlignment(Qt::AlignCenter);
    volumeLayout->addWidget(m_masterVolumeLabel);

    // Master pan
    QVBoxLayout* panLayout = new QVBoxLayout();
    panLayout->addWidget(new QLabel("Master Pan"));
    m_masterPanSlider = new QSlider(Qt::Horizontal);
    m_masterPanSlider->setRange(-100, 100);
    m_masterPanSlider->setValue(0);
    m_masterPanSlider->setMinimumWidth(150);
    panLayout->addWidget(m_masterPanSlider);
    m_masterPanLabel = new QLabel("Center");
    m_masterPanLabel->setAlignment(Qt::AlignCenter);
    panLayout->addWidget(m_masterPanLabel);

    masterLayout->addLayout(volumeLayout);
    masterLayout->addLayout(panLayout);
    masterLayout->addStretch();

    m_mainLayout->addWidget(m_masterWidget);

    // Connect signals
    connect(m_masterVolumeSlider, &QSlider::valueChanged, this, &AudioMixerDialog::onMasterVolumeChanged);
    connect(m_masterPanSlider, &QSlider::valueChanged, this, &AudioMixerDialog::onMasterPanChanged);
}

void AudioMixerDialog::updateWindowTitle() {
    QString title = "Audio Mixer";
    if (!m_currentProjectPath.isEmpty()) {
        title += " - " + QFileInfo(m_currentProjectPath).baseName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void AudioMixerDialog::updateTransportButtons() {
    if (m_playButton && m_pauseButton && m_stopButton) {
        m_playButton->setEnabled(!m_playing || m_paused);
        m_pauseButton->setEnabled(m_playing && !m_paused);
        m_stopButton->setEnabled(m_playing || m_paused);
    }
}

void AudioMixerDialog::updateMasterControls() {
    if (m_masterVolumeSlider && m_masterVolumeLabel) {
        m_masterVolumeSlider->setValue(static_cast<int>(m_masterVolume * 100));
        m_masterVolumeLabel->setText(QString("%1%").arg(static_cast<int>(m_masterVolume * 100)));
    }

    if (m_masterPanSlider && m_masterPanLabel) {
        m_masterPanSlider->setValue(static_cast<int>(m_masterPan * 100));

        if (m_masterPan < -0.01) {
            m_masterPanLabel->setText(QString("L%1").arg(static_cast<int>(-m_masterPan * 100)));
        } else if (m_masterPan > 0.01) {
            m_masterPanLabel->setText(QString("R%1").arg(static_cast<int>(m_masterPan * 100)));
        } else {
            m_masterPanLabel->setText("Center");
        }
    }
}

bool AudioMixerDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool AudioMixerDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<AudioMixerDialog*>(this),
        "Unsaved Changes",
        "The project has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    switch (result) {
        case QMessageBox::Save:
            if (m_currentProjectPath.isEmpty()) {
                onSaveProjectAs();
            } else {
                onSaveProject();
            }
            return !hasUnsavedChanges(); // Return true if save succeeded
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void AudioMixerDialog::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

// Event handlers
void AudioMixerDialog::closeEvent(QCloseEvent* event) {
    if (promptSaveChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void AudioMixerDialog::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        // Check if any of the URLs are audio files
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QString extension = QFileInfo(filePath).suffix().toLower();
                if (extension == "wav" || extension == "mp3" || extension == "flac" ||
                    extension == "ogg" || extension == "aiff" || extension == "au") {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    event->ignore();
}

void AudioMixerDialog::dropEvent(QDropEvent* event) {
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            addAudioFile(filePath);
        }
    }
    event->acceptProposedAction();
}

// File menu slots
void AudioMixerDialog::onNewProject() {
    if (promptSaveChanges()) {
        newProject();
    }
}

void AudioMixerDialog::onOpenProject() {
    if (!promptSaveChanges()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Audio Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Audio Project Files (*.amp);;All Files (*)"
    );

    if (!filePath.isEmpty()) {
        loadProject(filePath);
    }
}

void AudioMixerDialog::onSaveProject() {
    if (m_currentProjectPath.isEmpty()) {
        onSaveProjectAs();
    } else {
        saveProject(m_currentProjectPath);
    }
}

void AudioMixerDialog::onSaveProjectAs() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Audio Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Audio Project Files (*.amp);;All Files (*)"
    );

    if (!filePath.isEmpty()) {
        if (!filePath.endsWith(".amp", Qt::CaseInsensitive)) {
            filePath += ".amp";
        }
        saveProject(filePath);
    }
}

void AudioMixerDialog::onImportAudio() {
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Import Audio Files",
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        "Audio Files (*.wav *.mp3 *.flac *.ogg *.aiff *.au);;All Files (*)"
    );

    for (const QString& filePath : filePaths) {
        addAudioFile(filePath);
    }
}

void AudioMixerDialog::onExportAudio() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Mixed Audio",
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        "Audio Files (*.wav *.flac *.ogg);;All Files (*)"
    );

    if (!filePath.isEmpty()) {
        exportMixedAudio(filePath);
    }
}

// Edit menu slots
void AudioMixerDialog::onUndo() {
    // TODO: Implement undo functionality
    QMessageBox::information(this, "Undo", "Undo functionality will be implemented in a future update.");
}

void AudioMixerDialog::onRedo() {
    // TODO: Implement redo functionality
    QMessageBox::information(this, "Redo", "Redo functionality will be implemented in a future update.");
}

void AudioMixerDialog::onCut() {
    // TODO: Implement cut functionality
    QMessageBox::information(this, "Cut", "Cut functionality will be implemented in a future update.");
}

void AudioMixerDialog::onCopy() {
    // TODO: Implement copy functionality
    QMessageBox::information(this, "Copy", "Copy functionality will be implemented in a future update.");
}

void AudioMixerDialog::onPaste() {
    // TODO: Implement paste functionality
    QMessageBox::information(this, "Paste", "Paste functionality will be implemented in a future update.");
}

void AudioMixerDialog::onDelete() {
    // TODO: Implement delete functionality
    QMessageBox::information(this, "Delete", "Delete functionality will be implemented in a future update.");
}

void AudioMixerDialog::onSelectAll() {
    // TODO: Implement select all functionality
    QMessageBox::information(this, "Select All", "Select All functionality will be implemented in a future update.");
}

// Transport control slots
void AudioMixerDialog::onPlay() {
    if (m_paused) {
        // Resume playback
        m_paused = false;
        m_playbackTimer->start(50); // Update every 50ms
    } else {
        // Start playback
        startPlayback();
    }
    updateTransportButtons();
}

void AudioMixerDialog::onPause() {
    if (m_playing && !m_paused) {
        m_paused = true;
        m_playbackTimer->stop();
        updateTransportButtons();
    }
}

void AudioMixerDialog::onStop() {
    stopPlayback();
    updateTransportButtons();
}

void AudioMixerDialog::onRecord() {
    // TODO: Implement recording functionality
    QMessageBox::information(this, "Record", "Recording functionality will be implemented in a future update.");
}

void AudioMixerDialog::onRewind() {
    m_playbackPosition = 0.0;
    if (m_timelineWidget) {
        m_timelineWidget->setPlaybackPosition(m_playbackPosition);
    }

    // Update time display
    int minutes = static_cast<int>(m_playbackPosition) / 60;
    int seconds = static_cast<int>(m_playbackPosition) % 60;
    int milliseconds = static_cast<int>((m_playbackPosition - static_cast<int>(m_playbackPosition)) * 1000);

    if (m_timeLabel) {
        m_timeLabel->setText(QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0')));
    }
}

void AudioMixerDialog::onFastForward() {
    // Fast forward by 10 seconds
    m_playbackPosition += 10.0;
    if (m_timelineWidget) {
        m_timelineWidget->setPlaybackPosition(m_playbackPosition);
    }

    // Update time display
    int minutes = static_cast<int>(m_playbackPosition) / 60;
    int seconds = static_cast<int>(m_playbackPosition) % 60;
    int milliseconds = static_cast<int>((m_playbackPosition - static_cast<int>(m_playbackPosition)) * 1000);

    if (m_timeLabel) {
        m_timeLabel->setText(QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0')));
    }
}

// Track management slots
void AudioMixerDialog::onAddTrack() {
    onImportAudio(); // For now, adding a track means importing audio
}

void AudioMixerDialog::onRemoveTrack() {
    // TODO: Implement track removal
    QMessageBox::information(this, "Remove Track", "Track removal functionality will be implemented in a future update.");
}

void AudioMixerDialog::onDuplicateTrack() {
    // TODO: Implement track duplication
    QMessageBox::information(this, "Duplicate Track", "Track duplication functionality will be implemented in a future update.");
}

void AudioMixerDialog::onTrackChanged() {
    setModified(true);
}

void AudioMixerDialog::onTrackSoloChanged(bool solo) {
    Q_UNUSED(solo)
    // TODO: Handle solo logic (mute other tracks when one is soloed)
    setModified(true);
}

void AudioMixerDialog::onTrackMuteChanged(bool mute) {
    Q_UNUSED(mute)
    setModified(true);
}

// Playback slots
void AudioMixerDialog::onPlaybackTimer() {
    if (m_playing && !m_paused) {
        m_playbackPosition += 0.05; // 50ms increment

        if (m_timelineWidget) {
            m_timelineWidget->setPlaybackPosition(m_playbackPosition);
        }

        // Update time display
        int minutes = static_cast<int>(m_playbackPosition) / 60;
        int seconds = static_cast<int>(m_playbackPosition) % 60;
        int milliseconds = static_cast<int>((m_playbackPosition - static_cast<int>(m_playbackPosition)) * 1000);

        if (m_timeLabel) {
            m_timeLabel->setText(QString("%1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0')));
        }

        // TODO: Check if we've reached the end of all tracks and stop playback
    }
}

void AudioMixerDialog::onPlaybackPositionChanged(double seconds) {
    m_playbackPosition = seconds;

    // Update time display
    int minutes = static_cast<int>(m_playbackPosition) / 60;
    int secs = static_cast<int>(m_playbackPosition) % 60;
    int milliseconds = static_cast<int>((m_playbackPosition - static_cast<int>(m_playbackPosition)) * 1000);

    if (m_timeLabel) {
        m_timeLabel->setText(QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0')));
    }
}

// Master control slots
void AudioMixerDialog::onMasterVolumeChanged(int value) {
    m_masterVolume = value / 100.0f;
    if (m_masterVolumeLabel) {
        m_masterVolumeLabel->setText(QString("%1%").arg(value));
    }
    setModified(true);
}

void AudioMixerDialog::onMasterPanChanged(int value) {
    m_masterPan = value / 100.0f;

    if (m_masterPanLabel) {
        if (value < -1) {
            m_masterPanLabel->setText(QString("L%1").arg(-value));
        } else if (value > 1) {
            m_masterPanLabel->setText(QString("R%1").arg(value));
        } else {
            m_masterPanLabel->setText("Center");
        }
    }
    setModified(true);
}

// Project management methods
void AudioMixerDialog::newProject() {
    // Clear all tracks
    m_tracks.clear();

    // Clear track controls
    for (AudioTrackControlWidget* control : m_trackControls) {
        m_trackControlsLayout->removeWidget(control);
        delete control;
    }
    m_trackControls.clear();

    // Clear timeline
    if (m_timelineWidget) {
        m_timelineWidget->clearTracks();
    }

    // Reset state
    m_currentProjectPath.clear();
    m_playbackPosition = 0.0;
    m_playing = false;
    m_paused = false;
    m_masterVolume = 1.0f;
    m_masterPan = 0.0f;

    // Update UI
    updateWindowTitle();
    updateTransportButtons();
    updateMasterControls();
    setModified(false);

    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }
}

bool AudioMixerDialog::loadProject(const QString& filePath) {
    // TODO: Implement project loading from JSON file
    QMessageBox::information(this, "Load Project",
        QString("Project loading will be implemented in a future update.\nFile: %1").arg(filePath));
    return false;
}

bool AudioMixerDialog::saveProject(const QString& filePath) {
    // TODO: Implement project saving to JSON file
    QMessageBox::information(this, "Save Project",
        QString("Project saving will be implemented in a future update.\nFile: %1").arg(filePath));

    // For now, just mark as saved
    m_currentProjectPath = filePath;
    setModified(false);
    return true;
}

void AudioMixerDialog::addAudioFile(const QString& filePath) {
    // Create new audio track
    auto track = std::make_shared<AudioTrack>();

    if (track->loadFromFile(filePath)) {
        // Add to tracks list
        m_tracks.push_back(track);

        // Create track control widget
        AudioTrackControlWidget* controlWidget = new AudioTrackControlWidget(track, this);
        m_trackControls.push_back(controlWidget);
        m_trackControlsLayout->addWidget(controlWidget);

        // Connect signals
        connect(controlWidget, &AudioTrackControlWidget::trackChanged,
                this, &AudioMixerDialog::onTrackChanged);
        connect(controlWidget, &AudioTrackControlWidget::removeRequested,
                this, [this, controlWidget]() {
                    // Find and remove the track
                    for (size_t i = 0; i < m_trackControls.size(); ++i) {
                        if (m_trackControls[i] == controlWidget) {
                            // Remove from timeline
                            if (m_timelineWidget) {
                                m_timelineWidget->removeTrack(static_cast<int>(i));
                            }

                            // Remove from lists
                            m_tracks.erase(m_tracks.begin() + i);
                            m_trackControls.erase(m_trackControls.begin() + i);

                            // Remove from layout and delete
                            m_trackControlsLayout->removeWidget(controlWidget);
                            delete controlWidget;

                            setModified(true);
                            break;
                        }
                    }
                });
        connect(controlWidget, &AudioTrackControlWidget::soloChanged,
                this, &AudioMixerDialog::onTrackSoloChanged);
        connect(controlWidget, &AudioTrackControlWidget::muteChanged,
                this, &AudioMixerDialog::onTrackMuteChanged);

        // Add to timeline
        if (m_timelineWidget) {
            m_timelineWidget->addTrack(track);
        }

        setModified(true);

        std::cout << "Added audio track: " << filePath.toStdString() << std::endl;
    } else {
        QMessageBox::warning(this, "Error",
            QString("Failed to load audio file:\n%1").arg(filePath));
    }
}

// Audio playback methods
void AudioMixerDialog::startPlayback() {
    m_playing = true;
    m_paused = false;

    if (m_playbackTimer) {
        m_playbackTimer->start(50); // Update every 50ms
    }

    // Start audio device
    if (m_audioInitialized && m_audioDevice != 0) {
        SDL_PauseAudioDevice(m_audioDevice, 0); // 0 = unpause
    }

    std::cout << "Starting audio playback at position: " << m_playbackPosition << std::endl;
}

void AudioMixerDialog::stopPlayback() {
    m_playing = false;
    m_paused = false;

    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }

    // Pause audio device
    if (m_audioInitialized && m_audioDevice != 0) {
        SDL_PauseAudioDevice(m_audioDevice, 1); // 1 = pause
    }

    std::cout << "Stopping audio playback" << std::endl;
}

void AudioMixerDialog::pausePlayback() {
    if (m_playing) {
        m_paused = true;

        if (m_playbackTimer) {
            m_playbackTimer->stop();
        }

        // Pause audio device
        if (m_audioInitialized && m_audioDevice != 0) {
            SDL_PauseAudioDevice(m_audioDevice, 1); // 1 = pause
        }

        std::cout << "Pausing audio playback at position: " << m_playbackPosition << std::endl;
    }
}

bool AudioMixerDialog::exportMixedAudio(const QString& filePath) {
    if (m_tracks.empty()) {
        QMessageBox::warning(this, "Export Error", "No audio tracks to export.");
        return false;
    }

    // Determine output format from file extension
    QString extension = QFileInfo(filePath).suffix().toLower();
    int format = SF_FORMAT_WAV | SF_FORMAT_PCM_16; // Default to WAV 16-bit

    if (extension == "flac") {
        format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    } else if (extension == "ogg") {
        format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
    }

    // Calculate total duration and find maximum sample rate
    double totalDuration = 0.0;
    int maxSampleRate = 44100;
    int outputChannels = 2; // Stereo output

    for (const auto& track : m_tracks) {
        if (track && track->isValid() && track->enabled && !track->muted) {
            double trackEnd = track->startTime + track->duration;
            totalDuration = std::max(totalDuration, trackEnd);
            maxSampleRate = std::max(maxSampleRate, track->sampleRate);
        }
    }

    if (totalDuration <= 0.0) {
        QMessageBox::warning(this, "Export Error", "No valid audio tracks found.");
        return false;
    }

    // Setup output file
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    sfInfo.samplerate = maxSampleRate;
    sfInfo.channels = outputChannels;
    sfInfo.format = format;

    SNDFILE* outFile = sf_open(filePath.toUtf8().constData(), SFM_WRITE, &sfInfo);
    if (!outFile) {
        QMessageBox::critical(this, "Export Error",
            QString("Failed to create output file:\n%1\nError: %2")
            .arg(filePath).arg(sf_strerror(nullptr)));
        return false;
    }

    // Calculate total frames to export
    sf_count_t totalFrames = static_cast<sf_count_t>(totalDuration * maxSampleRate);

    // Create output buffer (process in chunks for memory efficiency)
    const int bufferSize = 4096;
    std::vector<float> outputBuffer(bufferSize * outputChannels);

    // Show progress dialog
    QProgressDialog progress("Exporting audio...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    try {
        // Process audio in chunks
        for (sf_count_t frameOffset = 0; frameOffset < totalFrames; frameOffset += bufferSize) {
            // Update progress
            int progressValue = static_cast<int>((frameOffset * 100) / totalFrames);
            progress.setValue(progressValue);
            QApplication::processEvents();

            if (progress.wasCanceled()) {
                sf_close(outFile);
                QFile::remove(filePath);
                return false;
            }

            // Calculate actual chunk size
            sf_count_t chunkSize = std::min(static_cast<sf_count_t>(bufferSize), totalFrames - frameOffset);

            // Clear output buffer
            std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);

            // Mix all tracks into output buffer
            mixTracksToBuffer(outputBuffer.data(), frameOffset, chunkSize, maxSampleRate, outputChannels);

            // Write to file
            sf_count_t written = sf_writef_float(outFile, outputBuffer.data(), chunkSize);
            if (written != chunkSize) {
                sf_close(outFile);
                QFile::remove(filePath);
                QMessageBox::critical(this, "Export Error", "Failed to write audio data to file.");
                return false;
            }
        }

        progress.setValue(100);

    } catch (const std::exception& e) {
        sf_close(outFile);
        QFile::remove(filePath);
        QMessageBox::critical(this, "Export Error",
            QString("Error during export: %1").arg(e.what()));
        return false;
    }

    sf_close(outFile);

    QMessageBox::information(this, "Export Complete",
        QString("Audio successfully exported to:\n%1").arg(filePath));

    return true;
}

void AudioMixerDialog::mixTracksToBuffer(float* outputBuffer, sf_count_t frameOffset, sf_count_t frameCount, int sampleRate, int channels) {
    double timeOffset = static_cast<double>(frameOffset) / sampleRate;

    // Check if any track has solo enabled
    bool hasSolo = false;
    for (const auto& track : m_tracks) {
        if (track && track->isValid() && track->enabled && track->solo) {
            hasSolo = true;
            break;
        }
    }

    // Mix each track
    for (const auto& track : m_tracks) {
        if (!track || !track->isValid() || !track->enabled) continue;

        // Skip muted tracks, unless they're soloed
        if (track->muted && !track->solo) continue;

        // If there are solo tracks, only play solo tracks
        if (hasSolo && !track->solo) continue;

        // Calculate track timing
        double trackStartTime = track->startTime;
        double trackEndTime = track->startTime + track->duration;
        double currentTime = timeOffset;

        // Skip if we're before this track starts or after it ends (unless looping)
        if (!track->looping && (currentTime < trackStartTime || currentTime >= trackEndTime)) {
            continue;
        }

        // Calculate track volume with master volume
        float trackVolume = track->volume * m_masterVolume;
        if (trackVolume <= 0.0f) continue;

        // Process each frame
        for (sf_count_t frame = 0; frame < frameCount; ++frame) {
            double frameTime = timeOffset + (static_cast<double>(frame) / sampleRate);

            // Skip if before track start
            if (frameTime < trackStartTime) continue;

            // Calculate position within track
            double trackTime = frameTime - trackStartTime;

            // Handle looping
            if (track->looping && trackTime >= track->duration) {
                trackTime = fmod(trackTime, track->duration);
            } else if (trackTime >= track->duration) {
                break; // Past end of non-looping track
            }

            // Calculate source sample position with pitch adjustment
            double sourceSamplePos = trackTime * track->sampleRate * track->pitch;
            sf_count_t sourceFrame = static_cast<sf_count_t>(sourceSamplePos);

            // Skip if past end of source data
            if (sourceFrame >= static_cast<sf_count_t>(track->audioData.size() / track->channels)) {
                continue;
            }

            // Get source samples with interpolation
            float leftSample = 0.0f, rightSample = 0.0f;

            if (track->channels == 1) {
                // Mono source
                leftSample = rightSample = track->audioData[sourceFrame];
            } else if (track->channels == 2) {
                // Stereo source
                sf_count_t sampleIndex = sourceFrame * 2;
                if (sampleIndex + 1 < track->audioData.size()) {
                    leftSample = track->audioData[sampleIndex];
                    rightSample = track->audioData[sampleIndex + 1];
                }
            }

            // Apply panning
            float leftGain = 1.0f, rightGain = 1.0f;
            if (track->pan < 0.0f) {
                // Pan left
                rightGain = 1.0f + track->pan;
            } else if (track->pan > 0.0f) {
                // Pan right
                leftGain = 1.0f - track->pan;
            }

            // Apply volume and panning
            leftSample *= trackVolume * leftGain;
            rightSample *= trackVolume * rightGain;

            // Mix into output buffer
            sf_count_t outputIndex = frame * channels;
            if (channels == 1) {
                // Mono output - mix left and right
                outputBuffer[outputIndex] += (leftSample + rightSample) * 0.5f;
            } else if (channels >= 2) {
                // Stereo output
                outputBuffer[outputIndex] += leftSample;      // Left channel
                outputBuffer[outputIndex + 1] += rightSample; // Right channel
            }
        }
    }

    // Apply master panning to final output
    if (channels >= 2 && m_masterPan != 0.0f) {
        for (sf_count_t frame = 0; frame < frameCount; ++frame) {
            sf_count_t outputIndex = frame * channels;
            float left = outputBuffer[outputIndex];
            float right = outputBuffer[outputIndex + 1];

            if (m_masterPan < 0.0f) {
                // Pan left - reduce right channel
                right *= (1.0f + m_masterPan);
            } else {
                // Pan right - reduce left channel
                left *= (1.0f - m_masterPan);
            }

            outputBuffer[outputIndex] = left;
            outputBuffer[outputIndex + 1] = right;
        }
    }

    // Clamp output to prevent clipping
    for (sf_count_t i = 0; i < frameCount * channels; ++i) {
        outputBuffer[i] = std::clamp(outputBuffer[i], -1.0f, 1.0f);
    }
}

void AudioMixerDialog::initializeAudio() {
    // Initialize SDL audio subsystem if not already done
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
            return;
        }
    }

    // Set up audio specification
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_F32SYS; // 32-bit float
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = AudioCallback;
    desired.userdata = this;

    // Open audio device
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &m_audioSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (m_audioDevice == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        return;
    }

    m_audioInitialized = true;
    std::cout << "Audio mixer initialized: " << m_audioSpec.freq << "Hz, "
              << (int)m_audioSpec.channels << " channels, " << m_audioSpec.samples << " samples" << std::endl;
}

void AudioMixerDialog::AudioCallback(void* userdata, Uint8* stream, int len) {
    AudioMixerDialog* mixer = static_cast<AudioMixerDialog*>(userdata);
    if (mixer) {
        mixer->MixAudioToStream(stream, len);
    }
}

void AudioMixerDialog::MixAudioToStream(Uint8* stream, int len) {
    // Clear the stream
    SDL_memset(stream, 0, len);

    if (!m_playing || m_paused || m_tracks.empty()) {
        return;
    }

    // Calculate number of frames
    int frameSize = m_audioSpec.channels * sizeof(float);
    int frameCount = len / frameSize;

    // Create temporary buffer for mixing
    std::vector<float> mixBuffer(frameCount * m_audioSpec.channels, 0.0f);

    // Mix tracks to buffer
    sf_count_t frameOffset = static_cast<sf_count_t>(m_playbackPosition * m_audioSpec.freq);
    mixTracksToBuffer(mixBuffer.data(), frameOffset, frameCount, m_audioSpec.freq, m_audioSpec.channels);

    // Copy mixed audio to output stream
    memcpy(stream, mixBuffer.data(), len);
}


