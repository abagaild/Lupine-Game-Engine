#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QListWidget>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <memory>
#include <vector>
#include <map>
#include <SDL2/SDL.h>
extern "C" {
#include <sndfile.h>
}

namespace stk {
    class FileWvIn;
    class FileWvOut;
    class Effect;
}

/**
 * @brief Represents a single audio track in the mixer
 */
class AudioTrack {
public:
    AudioTrack();
    ~AudioTrack();

    // Basic properties
    QString name;
    QString filePath;
    bool enabled;
    bool muted;
    bool solo;
    
    // Audio properties
    float volume;           // 0.0 to 1.0
    float pitch;           // 0.1 to 3.0 (pitch multiplier)
    float pan;             // -1.0 (left) to 1.0 (right)
    bool looping;
    
    // Timeline properties
    double startTime;      // Start time in seconds
    double duration;       // Duration in seconds
    double fadeInTime;     // Fade in duration
    double fadeOutTime;    // Fade out duration
    
    // Audio data
    std::vector<float> audioData;
    int sampleRate;
    int channels;
    
    // Methods
    bool loadFromFile(const QString& path);
    void clear();
    bool isValid() const;
};

/**
 * @brief Custom widget for displaying audio waveform and timeline
 */
class AudioTimelineWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit AudioTimelineWidget(QWidget* parent = nullptr);
    ~AudioTimelineWidget();

    void addTrack(std::shared_ptr<AudioTrack> track);
    void removeTrack(int trackIndex);
    void clearTracks();
    
    void setPlaybackPosition(double seconds);
    double getPlaybackPosition() const;
    
    void setZoom(double zoom);
    double getZoom() const;
    
    void setSelection(double startTime, double endTime);
    void clearSelection();

signals:
    void playbackPositionChanged(double seconds);
    void selectionChanged(double startTime, double endTime);
    void trackMoved(int trackIndex, double newStartTime);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateScene();

private:
    void setupScene();
    void drawWaveform(std::shared_ptr<AudioTrack> track, int trackIndex);
    void drawTimeline();
    void drawPlaybackCursor();
    
    QGraphicsScene* m_scene;
    std::vector<std::shared_ptr<AudioTrack>> m_tracks;
    double m_playbackPosition;
    double m_zoom;
    double m_selectionStart;
    double m_selectionEnd;
    bool m_hasSelection;
    
    // Interaction state
    bool m_dragging;
    QPointF m_lastMousePos;
    int m_draggedTrack;
};

/**
 * @brief Control panel for individual audio track
 */
class AudioTrackControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit AudioTrackControlWidget(std::shared_ptr<AudioTrack> track, QWidget* parent = nullptr);
    ~AudioTrackControlWidget();

    void updateFromTrack();
    std::shared_ptr<AudioTrack> getTrack() const { return m_track; }

signals:
    void trackChanged();
    void removeRequested();
    void soloChanged(bool solo);
    void muteChanged(bool mute);

private slots:
    void onVolumeChanged(int value);
    void onPitchChanged(double value);
    void onPanChanged(int value);
    void onLoopingChanged(bool enabled);
    void onEnabledChanged(bool enabled);
    void onMuteChanged(bool mute);
    void onSoloChanged(bool solo);
    void onRemoveClicked();

private:
    void setupUI();
    
    std::shared_ptr<AudioTrack> m_track;
    
    // UI elements
    QLabel* m_nameLabel;
    QCheckBox* m_enabledCheckBox;
    QCheckBox* m_muteCheckBox;
    QCheckBox* m_soloCheckBox;
    QSlider* m_volumeSlider;
    QLabel* m_volumeLabel;
    QDoubleSpinBox* m_pitchSpinBox;
    QSlider* m_panSlider;
    QLabel* m_panLabel;
    QCheckBox* m_loopingCheckBox;
    QPushButton* m_removeButton;
};

/**
 * @brief Main Audio Mixer Dialog
 */
class AudioMixerDialog : public QDialog {
    Q_OBJECT

public:
    explicit AudioMixerDialog(QWidget* parent = nullptr);
    ~AudioMixerDialog();

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    // File menu
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onImportAudio();
    void onExportAudio();
    
    // Edit menu
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onSelectAll();
    
    // Transport controls
    void onPlay();
    void onPause();
    void onStop();
    void onRecord();
    void onRewind();
    void onFastForward();
    
    // Track management
    void onAddTrack();
    void onRemoveTrack();
    void onDuplicateTrack();
    void onTrackChanged();
    void onTrackSoloChanged(bool solo);
    void onTrackMuteChanged(bool mute);
    
    // Playback
    void onPlaybackTimer();
    void onPlaybackPositionChanged(double seconds);
    
    // Master controls
    void onMasterVolumeChanged(int value);
    void onMasterPanChanged(int value);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainLayout();
    void setupTransportControls();
    void setupMasterControls();
    void setupTrackArea();
    
    void updateWindowTitle();
    void updateTransportButtons();
    void updateMasterControls();
    
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    
    // Project management
    void newProject();
    bool loadProject(const QString& filePath);
    bool saveProject(const QString& filePath);
    void addAudioFile(const QString& filePath);
    
    // Audio playback
    void startPlayback();
    void stopPlayback();
    void pausePlayback();

    // Audio export
    bool exportMixedAudio(const QString& filePath);
    void mixTracksToBuffer(float* outputBuffer, sf_count_t frameOffset, sf_count_t frameCount, int sampleRate, int channels);

    // Audio system
    void initializeAudio();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    
    // Timeline and tracks
    AudioTimelineWidget* m_timelineWidget;
    QScrollArea* m_trackControlsArea;
    QWidget* m_trackControlsWidget;
    QVBoxLayout* m_trackControlsLayout;
    
    // Transport controls
    QWidget* m_transportWidget;
    QPushButton* m_playButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    QPushButton* m_recordButton;
    QPushButton* m_rewindButton;
    QPushButton* m_fastForwardButton;
    QLabel* m_timeLabel;
    
    // Master controls
    QWidget* m_masterWidget;
    QSlider* m_masterVolumeSlider;
    QLabel* m_masterVolumeLabel;
    QSlider* m_masterPanSlider;
    QLabel* m_masterPanLabel;
    
    // Data
    std::vector<std::shared_ptr<AudioTrack>> m_tracks;
    std::vector<AudioTrackControlWidget*> m_trackControls;
    QString m_currentProjectPath;
    bool m_modified;
    
    // Playback state
    bool m_playing;
    bool m_paused;
    double m_playbackPosition;
    QTimer* m_playbackTimer;
    
    // Master settings
    float m_masterVolume;
    float m_masterPan;

    // Audio playback system
    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_audioSpec;
    bool m_audioInitialized;

    // Audio mixing
    static void AudioCallback(void* userdata, Uint8* stream, int len);
    void MixAudioToStream(Uint8* stream, int len);
};
