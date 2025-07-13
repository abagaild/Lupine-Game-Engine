#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
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
#include <QGraphicsPixmapItem>
#include <QCheckBox>
#include <QProgressBar>
#include <memory>
#include "lupine/resources/AnimationResource.h"

// Forward declarations
class SpriteSheetView;

/**
 * @brief Dialog for creating and editing sprite animations (.spriteanim files)
 * 
 * This dialog provides tools for:
 * - Importing sprite sheets
 * - Auto-slicing sprites based on grid size
 * - Creating named animations with frame sequences
 * - Setting frame durations and playback speed
 * - Previewing animations in real-time
 * - Saving/loading .spriteanim resource files
 */
class SpriteAnimatorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SpriteAnimatorDialog(QWidget* parent = nullptr);
    ~SpriteAnimatorDialog();
    
    // Resource management
    void NewSpriteAnimation();
    void LoadSpriteAnimation(const QString& filepath);
    void SaveSpriteAnimation();
    void SaveSpriteAnimationAs();

private slots:
    // File operations
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onClose();
    
    // Sprite sheet management
    void onImportSpriteSheet();
    void onSpriteSheetChanged();
    void onSpriteSizeChanged();
    void onAutoSlice();
    void onManualSlice();
    
    // Animation management
    void onNewAnimation();
    void onDeleteAnimation();
    void onAnimationSelectionChanged();
    void onAnimationRenamed();
    void onAnimationPropertiesChanged();
    
    // Frame management
    void onAddFrame();
    void onRemoveFrame();
    void onMoveFrameUp();
    void onMoveFrameDown();
    void onFrameSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onFrameDurationChanged();
    
    // Preview control
    void onPlayPause();
    void onStop();
    void onLoop();
    void onSpeedChanged();
    void onFrameChanged();
    
    // Sprite selection
    void onSpriteClicked(int spriteIndex);
    void onSpriteDoubleClicked(int spriteIndex);

private:
    void setupUI();
    void setupMainPanels();
    void setupToolBar();
    void setupSpriteSheetPanel();
    void setupAnimationPanel();
    void setupFramePanel();
    void setupPreviewPanel();
    
    void updateSpriteSheetView();
    void updateAnimationList();
    void updateFrameList();
    void updatePreview();
    void updateSpriteGrid();
    
    void loadSpriteSheet(const QString& filepath);
    void sliceSpriteSheet();
    void renderSpriteGrid();
    void renderFramePreview();
    
    // Animation playback
    void startPlayback();
    void stopPlayback();
    void updatePlayback();
    
    // Utility functions
    QPixmap getSpriteAtIndex(int index) const;
    glm::vec4 getSpriteRegion(int index) const;
    int getSpriteIndexAt(const QPoint& position) const;
    int getSpriteIndexFromRegion(const glm::vec4& region) const;
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
    QPushButton* m_importButton;
    
    // Sprite sheet panel
    QGroupBox* m_spriteSheetGroup;
    QVBoxLayout* m_spriteSheetLayout;
    QScrollArea* m_spriteSheetScrollArea;
    QLabel* m_spriteSheetLabel;
    SpriteSheetView* m_spriteSheetView;
    QGraphicsScene* m_spriteSheetScene;
    QGraphicsPixmapItem* m_spriteSheetPixmapItem;
    
    // Sprite sheet controls
    QHBoxLayout* m_spriteSheetControlsLayout;
    QLabel* m_spriteSheetPathLabel;
    QSpinBox* m_spriteSizeXSpin;
    QSpinBox* m_spriteSizeYSpin;
    QPushButton* m_autoSliceButton;
    QPushButton* m_manualSliceButton;
    QLabel* m_spriteCountLabel;
    
    // Animation panel
    QGroupBox* m_animationGroup;
    QVBoxLayout* m_animationLayout;
    QListWidget* m_animationList;
    QPushButton* m_newAnimationButton;
    QPushButton* m_deleteAnimationButton;
    
    // Animation properties
    QLineEdit* m_animationNameEdit;
    QCheckBox* m_animationLoopingCheck;
    QDoubleSpinBox* m_animationSpeedSpin;
    QCheckBox* m_defaultAnimationCheck;
    
    // Frame panel
    QGroupBox* m_frameGroup;
    QVBoxLayout* m_frameLayout;
    QTableWidget* m_frameTable;
    QPushButton* m_addFrameButton;
    QPushButton* m_removeFrameButton;
    QPushButton* m_moveFrameUpButton;
    QPushButton* m_moveFrameDownButton;
    
    // Frame properties
    QDoubleSpinBox* m_frameDurationSpin;
    
    // Preview panel
    QGroupBox* m_previewGroup;
    QVBoxLayout* m_previewLayout;
    QLabel* m_previewLabel;
    QGraphicsView* m_previewView;
    QGraphicsScene* m_previewScene;
    QGraphicsPixmapItem* m_previewPixmapItem;
    
    // Preview controls
    QHBoxLayout* m_previewControlsLayout;
    QPushButton* m_playButton;
    QPushButton* m_stopButton;
    QPushButton* m_loopButton;
    QSlider* m_speedSlider;
    QSlider* m_frameSlider;
    QLabel* m_currentFrameLabel;
    QLabel* m_totalFramesLabel;
    
    // Data
    std::unique_ptr<Lupine::SpriteAnimationResource> m_spriteResource;
    QString m_currentFilePath;
    bool m_isModified;
    
    // Sprite sheet data
    QString m_spriteSheetPath;
    QPixmap m_spriteSheetPixmap;
    glm::ivec2 m_spriteSize;
    glm::ivec2 m_sheetSize;
    int m_spriteCount;
    std::vector<QRect> m_spriteRects;
    
    // Animation playback
    QTimer* m_playbackTimer;
    QString m_currentAnimation;
    int m_currentFrame;
    float m_frameTime;
    float m_playbackSpeed;
    bool m_isPlaying;
    bool m_isLooping;
    
    // Selection
    int m_selectedSprite;
    int m_selectedFrame;
    
    // Constants
    static constexpr int DEFAULT_SPRITE_SIZE = 32;
    static constexpr float DEFAULT_FRAME_DURATION = 0.1f;
    static constexpr float DEFAULT_PLAYBACK_SPEED = 1.0f;
    static constexpr int PLAYBACK_UPDATE_INTERVAL = 16; // ~60 FPS
    static constexpr int MAX_SPRITE_DISPLAY_SIZE = 512;
};

/**
 * @brief Custom graphics view for sprite sheet interaction
 */
class SpriteSheetView : public QGraphicsView {
    Q_OBJECT

public:
    explicit SpriteSheetView(QWidget* parent = nullptr);
    
    void setSpriteSize(const glm::ivec2& size) { m_spriteSize = size; }
    void setSheetSize(const glm::ivec2& size) { m_sheetSize = size; }

signals:
    void spriteClicked(int spriteIndex);
    void spriteDoubleClicked(int spriteIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int getSpriteIndexAt(const QPoint& position) const;
    
    glm::ivec2 m_spriteSize;
    glm::ivec2 m_sheetSize;
};
