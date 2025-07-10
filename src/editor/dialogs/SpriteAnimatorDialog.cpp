#include "SpriteAnimatorDialog.h"
#include <QApplication>
#include <QFormLayout>
#include <QFileInfo>
#include <QInputDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QMouseEvent>
#include <QImageReader>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <cmath>

SpriteAnimatorDialog::SpriteAnimatorDialog(QWidget* parent)
    : QDialog(parent)
    , m_spriteResource(std::make_unique<Lupine::SpriteAnimationResource>())
    , m_isModified(false)
    , m_spriteSize(DEFAULT_SPRITE_SIZE, DEFAULT_SPRITE_SIZE)
    , m_sheetSize(0, 0)
    , m_spriteCount(0)
    , m_currentFrame(0)
    , m_frameTime(0.0f)
    , m_playbackSpeed(DEFAULT_PLAYBACK_SPEED)
    , m_isPlaying(false)
    , m_isLooping(false)
    , m_selectedSprite(-1)
    , m_selectedFrame(-1)
{
    setWindowTitle("Sprite Animator");
    setModal(false);
    resize(1400, 900);
    setAcceptDrops(true);
    
    setupUI();
    
    // Setup playback timer
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(PLAYBACK_UPDATE_INTERVAL);
    connect(m_playbackTimer, &QTimer::timeout, this, &SpriteAnimatorDialog::updatePlayback);
    
    // Initialize with empty resource
    NewSpriteAnimation();
}

SpriteAnimatorDialog::~SpriteAnimatorDialog() = default;

void SpriteAnimatorDialog::NewSpriteAnimation() {
    m_spriteResource = std::make_unique<Lupine::SpriteAnimationResource>();
    m_currentFilePath.clear();
    m_isModified = false;
    m_currentAnimation.clear();
    
    // Clear sprite sheet
    m_spriteSheetPath.clear();
    m_spriteSheetPixmap = QPixmap();
    m_spriteSize = glm::ivec2(DEFAULT_SPRITE_SIZE, DEFAULT_SPRITE_SIZE);
    m_sheetSize = glm::ivec2(0, 0);
    m_spriteCount = 0;
    m_spriteRects.clear();
    
    updateSpriteSheetView();
    updateAnimationList();
    updateFrameList();
    updatePreview();
    updateWindowTitle();
}

void SpriteAnimatorDialog::LoadSpriteAnimation(const QString& filepath) {
    if (m_spriteResource->LoadFromFile(filepath.toStdString())) {
        m_currentFilePath = filepath;
        m_isModified = false;
        
        // Load sprite sheet if specified
        QString texturePath = QString::fromStdString(m_spriteResource->GetTexturePath());
        if (!texturePath.isEmpty()) {
            loadSpriteSheet(texturePath);
        }
        
        // Update sprite size from resource
        glm::ivec2 spriteSize = m_spriteResource->GetSpriteSize();
        if (spriteSize.x > 0 && spriteSize.y > 0) {
            m_spriteSize = spriteSize;
            m_spriteSizeXSpin->setValue(spriteSize.x);
            m_spriteSizeYSpin->setValue(spriteSize.y);
        }
        
        // Select first animation if available
        auto animNames = m_spriteResource->GetAnimationNames();
        if (!animNames.empty()) {
            m_currentAnimation = QString::fromStdString(animNames[0]);
        }
        
        updateSpriteSheetView();
        updateAnimationList();
        updateFrameList();
        updatePreview();
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load sprite animation file: " + filepath);
    }
}

void SpriteAnimatorDialog::SaveSpriteAnimation() {
    if (m_currentFilePath.isEmpty()) {
        SaveSpriteAnimationAs();
        return;
    }
    
    // Update resource with current settings
    m_spriteResource->SetSpriteSize(m_spriteSize);
    m_spriteResource->SetSheetSize(m_sheetSize);
    m_spriteResource->SetTexturePath(m_spriteSheetPath.toStdString());
    
    if (m_spriteResource->SaveToFile(m_currentFilePath.toStdString())) {
        m_isModified = false;
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save sprite animation file: " + m_currentFilePath);
    }
}

void SpriteAnimatorDialog::SaveSpriteAnimationAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Sprite Animation",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Sprite Animation Files (*.spriteanim);;All Files (*)"
    );
    
    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".spriteanim")) {
            filepath += ".spriteanim";
        }
        
        m_currentFilePath = filepath;
        SaveSpriteAnimation();
    }
}

void SpriteAnimatorDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    setupToolBar();
    setupMainPanels();
    
    setLayout(m_mainLayout);
}

void SpriteAnimatorDialog::setupToolBar() {
    m_toolbarLayout = new QHBoxLayout();
    
    // File operations
    m_newButton = new QPushButton("New");
    m_openButton = new QPushButton("Open");
    m_saveButton = new QPushButton("Save");
    m_saveAsButton = new QPushButton("Save As");
    m_importButton = new QPushButton("Import Sprite Sheet");
    
    connect(m_newButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onNew);
    connect(m_openButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onOpen);
    connect(m_saveButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onSave);
    connect(m_saveAsButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onSaveAs);
    connect(m_importButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onImportSpriteSheet);
    
    m_toolbarLayout->addWidget(m_newButton);
    m_toolbarLayout->addWidget(m_openButton);
    m_toolbarLayout->addWidget(m_saveButton);
    m_toolbarLayout->addWidget(m_saveAsButton);
    m_toolbarLayout->addSpacing(10); // Add spacing instead of separator
    m_toolbarLayout->addWidget(m_importButton);
    m_toolbarLayout->addStretch();
    
    m_mainLayout->addLayout(m_toolbarLayout);
}

void SpriteAnimatorDialog::setupMainPanels() {
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel (Sprite Sheet + Animation)
    m_leftSplitter = new QSplitter(Qt::Vertical);
    setupSpriteSheetPanel();
    setupAnimationPanel();
    m_leftSplitter->addWidget(m_spriteSheetGroup);
    m_leftSplitter->addWidget(m_animationGroup);
    m_leftSplitter->setSizes({500, 300});
    
    // Right panel (Frames + Preview)
    m_rightSplitter = new QSplitter(Qt::Vertical);
    setupFramePanel();
    setupPreviewPanel();
    m_rightSplitter->addWidget(m_frameGroup);
    m_rightSplitter->addWidget(m_previewGroup);
    m_rightSplitter->setSizes({400, 300});
    
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setSizes({700, 700});
    
    m_mainLayout->addWidget(m_mainSplitter);
}

void SpriteAnimatorDialog::setupSpriteSheetPanel() {
    m_spriteSheetGroup = new QGroupBox("Sprite Sheet");
    m_spriteSheetLayout = new QVBoxLayout(m_spriteSheetGroup);

    // Sprite sheet view - use custom SpriteSheetView for mouse interaction
    m_spriteSheetView = new SpriteSheetView();
    m_spriteSheetScene = new QGraphicsScene();
    m_spriteSheetView->setScene(m_spriteSheetScene);
    m_spriteSheetView->setDragMode(QGraphicsView::RubberBandDrag);
    m_spriteSheetView->setRenderHint(QPainter::Antialiasing, false);

    // Connect sprite selection signals
    connect(m_spriteSheetView, &SpriteSheetView::spriteClicked, this, &SpriteAnimatorDialog::onSpriteClicked);
    connect(m_spriteSheetView, &SpriteSheetView::spriteDoubleClicked, this, &SpriteAnimatorDialog::onSpriteDoubleClicked);
    
    // Sprite sheet controls
    m_spriteSheetControlsLayout = new QHBoxLayout();
    
    m_spriteSheetPathLabel = new QLabel("No sprite sheet loaded");
    m_spriteSheetPathLabel->setWordWrap(true);
    
    QFormLayout* sizeLayout = new QFormLayout();
    m_spriteSizeXSpin = new QSpinBox();
    m_spriteSizeXSpin->setRange(1, 1024);
    m_spriteSizeXSpin->setValue(DEFAULT_SPRITE_SIZE);
    m_spriteSizeYSpin = new QSpinBox();
    m_spriteSizeYSpin->setRange(1, 1024);
    m_spriteSizeYSpin->setValue(DEFAULT_SPRITE_SIZE);
    
    connect(m_spriteSizeXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteAnimatorDialog::onSpriteSizeChanged);
    connect(m_spriteSizeYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteAnimatorDialog::onSpriteSizeChanged);
    
    QHBoxLayout* spriteSizeLayout = new QHBoxLayout();
    spriteSizeLayout->addWidget(new QLabel("Sprite Size:"));
    spriteSizeLayout->addWidget(m_spriteSizeXSpin);
    spriteSizeLayout->addWidget(new QLabel("x"));
    spriteSizeLayout->addWidget(m_spriteSizeYSpin);
    
    m_autoSliceButton = new QPushButton("Auto Slice");
    m_manualSliceButton = new QPushButton("Manual Slice");
    m_spriteCountLabel = new QLabel("Sprites: 0");
    
    connect(m_autoSliceButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onAutoSlice);
    connect(m_manualSliceButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onManualSlice);
    
    m_spriteSheetControlsLayout->addLayout(spriteSizeLayout);
    m_spriteSheetControlsLayout->addWidget(m_autoSliceButton);
    m_spriteSheetControlsLayout->addWidget(m_manualSliceButton);
    m_spriteSheetControlsLayout->addWidget(m_spriteCountLabel);
    m_spriteSheetControlsLayout->addStretch();
    
    m_spriteSheetLayout->addWidget(m_spriteSheetPathLabel);
    m_spriteSheetLayout->addWidget(m_spriteSheetView);
    m_spriteSheetLayout->addLayout(m_spriteSheetControlsLayout);
}

void SpriteAnimatorDialog::setupAnimationPanel() {
    m_animationGroup = new QGroupBox("Animations");
    m_animationLayout = new QVBoxLayout(m_animationGroup);
    
    // Animation list
    m_animationList = new QListWidget();
    connect(m_animationList, &QListWidget::currentTextChanged, this, &SpriteAnimatorDialog::onAnimationSelectionChanged);
    
    // Animation controls
    QHBoxLayout* animButtonLayout = new QHBoxLayout();
    m_newAnimationButton = new QPushButton("New Animation");
    m_deleteAnimationButton = new QPushButton("Delete Animation");
    
    connect(m_newAnimationButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onNewAnimation);
    connect(m_deleteAnimationButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onDeleteAnimation);
    
    animButtonLayout->addWidget(m_newAnimationButton);
    animButtonLayout->addWidget(m_deleteAnimationButton);
    
    // Animation properties
    QFormLayout* animPropsLayout = new QFormLayout();
    m_animationNameEdit = new QLineEdit();
    m_animationLoopingCheck = new QCheckBox("Looping");
    m_animationSpeedSpin = new QDoubleSpinBox();
    m_animationSpeedSpin->setRange(0.1, 10.0);
    m_animationSpeedSpin->setSingleStep(0.1);
    m_animationSpeedSpin->setValue(1.0);
    m_defaultAnimationCheck = new QCheckBox("Default Animation");
    
    connect(m_animationNameEdit, &QLineEdit::textChanged, this, &SpriteAnimatorDialog::onAnimationRenamed);
    connect(m_animationLoopingCheck, &QCheckBox::toggled, this, &SpriteAnimatorDialog::onAnimationPropertiesChanged);
    connect(m_animationSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteAnimatorDialog::onAnimationPropertiesChanged);
    connect(m_defaultAnimationCheck, &QCheckBox::toggled, this, &SpriteAnimatorDialog::onAnimationPropertiesChanged);
    
    animPropsLayout->addRow("Name:", m_animationNameEdit);
    animPropsLayout->addRow(m_animationLoopingCheck);
    animPropsLayout->addRow("Speed:", m_animationSpeedSpin);
    animPropsLayout->addRow(m_defaultAnimationCheck);
    
    m_animationLayout->addWidget(m_animationList);
    m_animationLayout->addLayout(animButtonLayout);
    m_animationLayout->addLayout(animPropsLayout);
}

void SpriteAnimatorDialog::setupFramePanel() {
    m_frameGroup = new QGroupBox("Animation Frames");
    m_frameLayout = new QVBoxLayout(m_frameGroup);

    // Frame table
    m_frameTable = new QTableWidget();
    m_frameTable->setColumnCount(3);
    m_frameTable->setHorizontalHeaderLabels({"Frame", "Duration", "Sprite Index"});
    m_frameTable->horizontalHeader()->setStretchLastSection(true);
    m_frameTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(m_frameTable, &QTableWidget::currentCellChanged, this, &SpriteAnimatorDialog::onFrameSelectionChanged);

    // Frame controls
    QHBoxLayout* frameButtonLayout = new QHBoxLayout();
    m_addFrameButton = new QPushButton("Add Frame");
    m_removeFrameButton = new QPushButton("Remove Frame");
    m_moveFrameUpButton = new QPushButton("Move Up");
    m_moveFrameDownButton = new QPushButton("Move Down");

    connect(m_addFrameButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onAddFrame);
    connect(m_removeFrameButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onRemoveFrame);
    connect(m_moveFrameUpButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onMoveFrameUp);
    connect(m_moveFrameDownButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onMoveFrameDown);

    frameButtonLayout->addWidget(m_addFrameButton);
    frameButtonLayout->addWidget(m_removeFrameButton);
    frameButtonLayout->addWidget(m_moveFrameUpButton);
    frameButtonLayout->addWidget(m_moveFrameDownButton);

    // Frame properties
    QFormLayout* framePropsLayout = new QFormLayout();
    m_frameDurationSpin = new QDoubleSpinBox();
    m_frameDurationSpin->setRange(0.01, 10.0);
    m_frameDurationSpin->setSingleStep(0.01);
    m_frameDurationSpin->setValue(DEFAULT_FRAME_DURATION);
    m_frameDurationSpin->setSuffix(" s");

    connect(m_frameDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteAnimatorDialog::onFrameDurationChanged);

    framePropsLayout->addRow("Duration:", m_frameDurationSpin);

    m_frameLayout->addWidget(m_frameTable);
    m_frameLayout->addLayout(frameButtonLayout);
    m_frameLayout->addLayout(framePropsLayout);
}

void SpriteAnimatorDialog::setupPreviewPanel() {
    m_previewGroup = new QGroupBox("Animation Preview");
    m_previewLayout = new QVBoxLayout(m_previewGroup);

    // Preview view
    m_previewView = new QGraphicsView();
    m_previewScene = new QGraphicsScene();
    m_previewView->setScene(m_previewScene);
    m_previewView->setFixedSize(200, 200);
    m_previewView->setRenderHint(QPainter::Antialiasing, false);

    m_previewPixmapItem = nullptr;

    // Preview controls
    m_previewControlsLayout = new QHBoxLayout();

    m_playButton = new QPushButton("Play");
    m_stopButton = new QPushButton("Stop");
    m_loopButton = new QPushButton("Loop");
    m_loopButton->setCheckable(true);
    m_loopButton->setChecked(true);

    connect(m_playButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &SpriteAnimatorDialog::onStop);
    connect(m_loopButton, &QPushButton::toggled, this, &SpriteAnimatorDialog::onLoop);

    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(10, 300); // 0.1x to 3.0x speed
    m_speedSlider->setValue(100); // 1.0x speed

    connect(m_speedSlider, &QSlider::valueChanged, this, &SpriteAnimatorDialog::onSpeedChanged);

    m_frameSlider = new QSlider(Qt::Horizontal);
    m_frameSlider->setRange(0, 0);

    connect(m_frameSlider, &QSlider::valueChanged, this, &SpriteAnimatorDialog::onFrameChanged);

    m_currentFrameLabel = new QLabel("0");
    m_totalFramesLabel = new QLabel("/ 0");

    m_previewControlsLayout->addWidget(m_playButton);
    m_previewControlsLayout->addWidget(m_stopButton);
    m_previewControlsLayout->addWidget(m_loopButton);
    m_previewControlsLayout->addStretch();

    QVBoxLayout* sliderLayout = new QVBoxLayout();
    QHBoxLayout* speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Speed:"));
    speedLayout->addWidget(m_speedSlider);

    QHBoxLayout* frameLayout = new QHBoxLayout();
    frameLayout->addWidget(new QLabel("Frame:"));
    frameLayout->addWidget(m_currentFrameLabel);
    frameLayout->addWidget(m_totalFramesLabel);
    frameLayout->addWidget(m_frameSlider);

    sliderLayout->addLayout(speedLayout);
    sliderLayout->addLayout(frameLayout);

    m_previewLayout->addWidget(m_previewView);
    m_previewLayout->addLayout(m_previewControlsLayout);
    m_previewLayout->addLayout(sliderLayout);
}

// Slot implementations

void SpriteAnimatorDialog::onNew() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "New Sprite Animation",
            "Current animation has unsaved changes. Do you want to save before creating a new animation?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveSpriteAnimation();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    NewSpriteAnimation();
}

void SpriteAnimatorDialog::onOpen() {
    if (m_isModified) {
        int ret = QMessageBox::question(this, "Open Sprite Animation",
            "Current animation has unsaved changes. Do you want to save before opening another animation?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            SaveSpriteAnimation();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Sprite Animation",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Sprite Animation Files (*.spriteanim);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadSpriteAnimation(filepath);
    }
}

void SpriteAnimatorDialog::onSave() {
    SaveSpriteAnimation();
}

void SpriteAnimatorDialog::onSaveAs() {
    SaveSpriteAnimationAs();
}

void SpriteAnimatorDialog::onClose() {
    close();
}

void SpriteAnimatorDialog::onImportSpriteSheet() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Import Sprite Sheet",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tiff);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        loadSpriteSheet(filepath);
    }
}

void SpriteAnimatorDialog::onSpriteSheetChanged() {
    updateSpriteSheetView();
    m_isModified = true;
    updateWindowTitle();
}

void SpriteAnimatorDialog::onSpriteSizeChanged() {
    m_spriteSize.x = m_spriteSizeXSpin->value();
    m_spriteSize.y = m_spriteSizeYSpin->value();

    // Update the sprite sheet view with new sprite size
    m_spriteSheetView->setSpriteSize(m_spriteSize);

    if (!m_spriteSheetPixmap.isNull()) {
        sliceSpriteSheet();
        updateSpriteSheetView();
    }

    m_isModified = true;
    updateWindowTitle();
}

void SpriteAnimatorDialog::onAutoSlice() {
    if (!m_spriteSheetPixmap.isNull()) {
        sliceSpriteSheet();
        updateSpriteSheetView();
        m_isModified = true;
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onManualSlice() {
    if (m_spriteSheetPixmap.isNull()) {
        QMessageBox::information(this, "Manual Slice", "Please load a sprite sheet first.");
        return;
    }

    // Create a simple dialog for manual slicing parameters
    QDialog dialog(this);
    dialog.setWindowTitle("Manual Slice Configuration");
    dialog.setModal(true);

    QFormLayout* layout = new QFormLayout(&dialog);

    // Grid configuration
    QSpinBox* colsSpin = new QSpinBox();
    colsSpin->setRange(1, 100);
    colsSpin->setValue(m_sheetSize.x / m_spriteSize.x);

    QSpinBox* rowsSpin = new QSpinBox();
    rowsSpin->setRange(1, 100);
    rowsSpin->setValue(m_sheetSize.y / m_spriteSize.y);

    // Offset configuration
    QSpinBox* offsetXSpin = new QSpinBox();
    offsetXSpin->setRange(0, m_sheetSize.x);
    offsetXSpin->setValue(0);

    QSpinBox* offsetYSpin = new QSpinBox();
    offsetYSpin->setRange(0, m_sheetSize.y);
    offsetYSpin->setValue(0);

    // Spacing configuration
    QSpinBox* spacingXSpin = new QSpinBox();
    spacingXSpin->setRange(0, 100);
    spacingXSpin->setValue(0);

    QSpinBox* spacingYSpin = new QSpinBox();
    spacingYSpin->setRange(0, 100);
    spacingYSpin->setValue(0);

    layout->addRow("Columns:", colsSpin);
    layout->addRow("Rows:", rowsSpin);
    layout->addRow("Offset X:", offsetXSpin);
    layout->addRow("Offset Y:", offsetYSpin);
    layout->addRow("Spacing X:", spacingXSpin);
    layout->addRow("Spacing Y:", spacingYSpin);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // Apply manual slicing
        m_spriteRects.clear();
        m_spriteCount = 0;

        int cols = colsSpin->value();
        int rows = rowsSpin->value();
        int offsetX = offsetXSpin->value();
        int offsetY = offsetYSpin->value();
        int spacingX = spacingXSpin->value();
        int spacingY = spacingYSpin->value();

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                int x = offsetX + col * (m_spriteSize.x + spacingX);
                int y = offsetY + row * (m_spriteSize.y + spacingY);

                // Check bounds
                if (x + m_spriteSize.x <= m_sheetSize.x && y + m_spriteSize.y <= m_sheetSize.y) {
                    QRect rect(x, y, m_spriteSize.x, m_spriteSize.y);
                    m_spriteRects.push_back(rect);
                }
            }
        }

        m_spriteCount = m_spriteRects.size();
        m_spriteCountLabel->setText("Sprites: " + QString::number(m_spriteCount));
        updateSpriteSheetView();
        m_isModified = true;
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onNewAnimation() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Animation", "Animation name:", QLineEdit::Normal, "New Animation", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::SpriteAnimation animation(name.toStdString(), true, 1.0f);
        m_spriteResource->AddAnimation(animation);
        m_currentAnimation = name;
        m_isModified = true;
        updateAnimationList();
        updateFrameList();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onDeleteAnimation() {
    if (m_currentAnimation.isEmpty()) return;

    int ret = QMessageBox::question(this, "Delete Animation",
        "Are you sure you want to delete the animation '" + m_currentAnimation + "'?",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_spriteResource->RemoveAnimation(m_currentAnimation.toStdString());
        m_currentAnimation.clear();
        m_isModified = true;
        updateAnimationList();
        updateFrameList();
        updatePreview();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onAnimationSelectionChanged() {
    QListWidgetItem* item = m_animationList->currentItem();
    if (item) {
        m_currentAnimation = item->text();

        // Update animation properties
        const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
        if (animation) {
            m_animationNameEdit->setText(QString::fromStdString(animation->name));
            m_animationLoopingCheck->setChecked(animation->looping);
            m_animationSpeedSpin->setValue(animation->speed_scale);
            m_defaultAnimationCheck->setChecked(m_spriteResource->GetDefaultAnimation() == animation->name);
        }

        updateFrameList();
        updatePreview();
    }
}

void SpriteAnimatorDialog::onAnimationRenamed() {
    if (m_currentAnimation.isEmpty()) return;

    QString newName = m_animationNameEdit->text();
    if (newName.isEmpty() || newName == m_currentAnimation) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation) {
        // Create new animation with new name
        Lupine::SpriteAnimation newAnimation = *animation;
        newAnimation.name = newName.toStdString();

        // Remove old animation and add new one
        m_spriteResource->RemoveAnimation(m_currentAnimation.toStdString());
        m_spriteResource->AddAnimation(newAnimation);

        m_currentAnimation = newName;
        m_isModified = true;
        updateAnimationList();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onAnimationPropertiesChanged() {
    if (m_currentAnimation.isEmpty()) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation) {
        animation->looping = m_animationLoopingCheck->isChecked();
        animation->speed_scale = m_animationSpeedSpin->value();

        if (m_defaultAnimationCheck->isChecked()) {
            m_spriteResource->SetDefaultAnimation(animation->name);
        }

        m_isModified = true;
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onAddFrame() {
    if (m_currentAnimation.isEmpty() || m_selectedSprite < 0) {
        QMessageBox::information(this, "Add Frame", "Please select an animation and a sprite from the sprite sheet.");
        return;
    }

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation) {
        // Create frame from selected sprite
        glm::vec4 region = getSpriteRegion(m_selectedSprite);
        Lupine::SpriteFrame frame(region, DEFAULT_FRAME_DURATION);
        animation->frames.push_back(frame);

        m_isModified = true;
        updateFrameList();
        updatePreview();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onRemoveFrame() {
    if (m_currentAnimation.isEmpty() || m_selectedFrame < 0) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation && m_selectedFrame < static_cast<int>(animation->frames.size())) {
        animation->frames.erase(animation->frames.begin() + m_selectedFrame);
        m_selectedFrame = -1;
        m_isModified = true;
        updateFrameList();
        updatePreview();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onMoveFrameUp() {
    if (m_currentAnimation.isEmpty() || m_selectedFrame <= 0) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation && m_selectedFrame < static_cast<int>(animation->frames.size())) {
        std::swap(animation->frames[m_selectedFrame], animation->frames[m_selectedFrame - 1]);
        m_selectedFrame--;
        m_isModified = true;
        updateFrameList();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onMoveFrameDown() {
    if (m_currentAnimation.isEmpty() || m_selectedFrame < 0) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation && m_selectedFrame < static_cast<int>(animation->frames.size()) - 1) {
        std::swap(animation->frames[m_selectedFrame], animation->frames[m_selectedFrame + 1]);
        m_selectedFrame++;
        m_isModified = true;
        updateFrameList();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onFrameSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) {
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);
    m_selectedFrame = currentRow;

    if (m_selectedFrame >= 0 && !m_currentAnimation.isEmpty()) {
        const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
        if (animation && m_selectedFrame < static_cast<int>(animation->frames.size())) {
            const auto& frame = animation->frames[m_selectedFrame];
            m_frameDurationSpin->setValue(frame.duration);
        }
    }
}

void SpriteAnimatorDialog::onFrameDurationChanged() {
    if (m_currentAnimation.isEmpty() || m_selectedFrame < 0) return;

    auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (animation && m_selectedFrame < static_cast<int>(animation->frames.size())) {
        animation->frames[m_selectedFrame].duration = m_frameDurationSpin->value();
        m_isModified = true;
        updateFrameList();
        updateWindowTitle();
    }
}

void SpriteAnimatorDialog::onPlayPause() {
    if (m_isPlaying) {
        m_playbackTimer->stop();
        m_isPlaying = false;
        m_playButton->setText("Play");
    } else {
        startPlayback();
    }
}

void SpriteAnimatorDialog::onStop() {
    stopPlayback();
    m_currentFrame = 0;
    m_frameTime = 0.0f;
    updatePreview();
}

void SpriteAnimatorDialog::onLoop() {
    m_isLooping = m_loopButton->isChecked();
}

void SpriteAnimatorDialog::onSpeedChanged() {
    m_playbackSpeed = m_speedSlider->value() / 100.0f; // Convert to 0.1x - 3.0x range
}

void SpriteAnimatorDialog::onFrameChanged() {
    if (!m_isPlaying) {
        m_currentFrame = m_frameSlider->value();
        updatePreview();
    }
}

void SpriteAnimatorDialog::onSpriteClicked(int spriteIndex) {
    m_selectedSprite = spriteIndex;
    updateSpriteSheetView(); // Highlight selected sprite
}

void SpriteAnimatorDialog::onSpriteDoubleClicked(int spriteIndex) {
    m_selectedSprite = spriteIndex;
    onAddFrame(); // Automatically add frame when double-clicking sprite
}

// Update methods

void SpriteAnimatorDialog::updateSpriteSheetView() {
    m_spriteSheetScene->clear();

    if (m_spriteSheetPixmap.isNull()) {
        return;
    }

    // Add sprite sheet pixmap
    m_spriteSheetPixmapItem = m_spriteSheetScene->addPixmap(m_spriteSheetPixmap);

    // Draw sprite grid
    renderSpriteGrid();

    // Fit view to content
    m_spriteSheetView->fitInView(m_spriteSheetScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void SpriteAnimatorDialog::updateAnimationList() {
    m_animationList->clear();

    auto animNames = m_spriteResource->GetAnimationNames();
    for (const auto& name : animNames) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name));
        m_animationList->addItem(item);

        if (QString::fromStdString(name) == m_currentAnimation) {
            m_animationList->setCurrentItem(item);
        }
    }
}

void SpriteAnimatorDialog::updateFrameList() {
    m_frameTable->setRowCount(0);

    if (m_currentAnimation.isEmpty()) {
        m_frameSlider->setRange(0, 0);
        m_totalFramesLabel->setText("/ 0");
        return;
    }

    const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (!animation) return;

    m_frameTable->setRowCount(animation->frames.size());

    for (size_t i = 0; i < animation->frames.size(); ++i) {
        const auto& frame = animation->frames[i];

        // Frame number
        QTableWidgetItem* frameItem = new QTableWidgetItem(QString::number(i + 1));
        frameItem->setFlags(frameItem->flags() & ~Qt::ItemIsEditable);
        m_frameTable->setItem(i, 0, frameItem);

        // Duration
        QTableWidgetItem* durationItem = new QTableWidgetItem(QString::number(frame.duration, 'f', 3) + " s");
        durationItem->setFlags(durationItem->flags() & ~Qt::ItemIsEditable);
        m_frameTable->setItem(i, 1, durationItem);

        // Sprite index (calculated from texture region)
        int spriteIndex = getSpriteIndexFromRegion(frame.texture_region);
        QTableWidgetItem* spriteItem = new QTableWidgetItem(QString::number(spriteIndex));
        spriteItem->setFlags(spriteItem->flags() & ~Qt::ItemIsEditable);
        m_frameTable->setItem(i, 2, spriteItem);
    }

    // Update frame slider
    m_frameSlider->setRange(0, std::max(0, static_cast<int>(animation->frames.size()) - 1));
    m_totalFramesLabel->setText("/ " + QString::number(animation->frames.size()));

    // Restore frame selection if valid
    if (m_selectedFrame >= 0 && m_selectedFrame < static_cast<int>(animation->frames.size())) {
        m_frameTable->selectRow(m_selectedFrame);
    } else if (animation->frames.size() > 0) {
        // Select first frame if current selection is invalid
        m_selectedFrame = 0;
        m_frameTable->selectRow(0);
    } else {
        m_selectedFrame = -1;
    }
}

void SpriteAnimatorDialog::updatePreview() {
    m_previewScene->clear();

    if (m_currentAnimation.isEmpty()) {
        m_previewPixmapItem = nullptr;
        return;
    }

    const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (!animation || animation->frames.empty()) {
        m_previewPixmapItem = nullptr;
        return;
    }

    // Get current frame
    int frameIndex = std::clamp(m_currentFrame, 0, static_cast<int>(animation->frames.size()) - 1);
    const auto& frame = animation->frames[frameIndex];

    // Get sprite pixmap for this frame
    int spriteIndex = getSpriteIndexFromRegion(frame.texture_region);
    QPixmap spritePixmap = getSpriteAtIndex(spriteIndex);

    if (!spritePixmap.isNull()) {
        m_previewPixmapItem = m_previewScene->addPixmap(spritePixmap);
        m_previewView->fitInView(m_previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    // Update frame display
    m_currentFrameLabel->setText(QString::number(frameIndex + 1));
    m_frameSlider->setValue(frameIndex);
}

void SpriteAnimatorDialog::loadSpriteSheet(const QString& filepath) {
    QPixmap pixmap(filepath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to load sprite sheet: " + filepath);
        return;
    }

    m_spriteSheetPath = filepath;
    m_spriteSheetPixmap = pixmap;
    m_sheetSize = glm::ivec2(pixmap.width(), pixmap.height());

    // Update the sprite sheet view with new sheet size and sprite size
    m_spriteSheetView->setSheetSize(m_sheetSize);
    m_spriteSheetView->setSpriteSize(m_spriteSize);

    // Update UI
    QFileInfo fileInfo(filepath);
    m_spriteSheetPathLabel->setText("Sprite Sheet: " + fileInfo.fileName());

    // Auto-slice the sprite sheet
    sliceSpriteSheet();
    updateSpriteSheetView();

    m_isModified = true;
    updateWindowTitle();
}

void SpriteAnimatorDialog::sliceSpriteSheet() {
    m_spriteRects.clear();
    m_spriteCount = 0;

    if (m_spriteSheetPixmap.isNull() || m_spriteSize.x <= 0 || m_spriteSize.y <= 0) {
        return;
    }

    int cols = m_sheetSize.x / m_spriteSize.x;
    int rows = m_sheetSize.y / m_spriteSize.y;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QRect rect(col * m_spriteSize.x, row * m_spriteSize.y, m_spriteSize.x, m_spriteSize.y);
            m_spriteRects.push_back(rect);
        }
    }

    m_spriteCount = m_spriteRects.size();
    m_spriteCountLabel->setText("Sprites: " + QString::number(m_spriteCount));
}

void SpriteAnimatorDialog::renderSpriteGrid() {
    if (m_spriteRects.empty()) return;

    QPen gridPen(Qt::red, 1);
    QPen selectedPen(Qt::yellow, 2);

    for (size_t i = 0; i < m_spriteRects.size(); ++i) {
        const QRect& rect = m_spriteRects[i];
        QPen pen = (static_cast<int>(i) == m_selectedSprite) ? selectedPen : gridPen;
        m_spriteSheetScene->addRect(rect, pen);

        // Add sprite index label
        QGraphicsTextItem* textItem = m_spriteSheetScene->addText(QString::number(i));
        textItem->setPos(rect.x() + 2, rect.y() + 2);
        textItem->setScale(0.8);
    }
}

QPixmap SpriteAnimatorDialog::getSpriteAtIndex(int index) const {
    if (index < 0 || index >= static_cast<int>(m_spriteRects.size()) || m_spriteSheetPixmap.isNull()) {
        return QPixmap();
    }

    const QRect& rect = m_spriteRects[index];
    return m_spriteSheetPixmap.copy(rect);
}

glm::vec4 SpriteAnimatorDialog::getSpriteRegion(int index) const {
    if (index < 0 || index >= static_cast<int>(m_spriteRects.size()) || m_sheetSize.x <= 0 || m_sheetSize.y <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const QRect& rect = m_spriteRects[index];
    float u = static_cast<float>(rect.x()) / static_cast<float>(m_sheetSize.x);
    float v = static_cast<float>(rect.y()) / static_cast<float>(m_sheetSize.y);
    float w = static_cast<float>(rect.width()) / static_cast<float>(m_sheetSize.x);
    float h = static_cast<float>(rect.height()) / static_cast<float>(m_sheetSize.y);

    return glm::vec4(u, v, w, h);
}

int SpriteAnimatorDialog::getSpriteIndexAt(const QPoint& position) const {
    for (size_t i = 0; i < m_spriteRects.size(); ++i) {
        if (m_spriteRects[i].contains(position)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int SpriteAnimatorDialog::getSpriteIndexFromRegion(const glm::vec4& region) const {
    if (m_sheetSize.x <= 0 || m_sheetSize.y <= 0 || m_spriteSize.x <= 0 || m_spriteSize.y <= 0) {
        return 0;
    }

    int x = static_cast<int>(region.x * m_sheetSize.x);
    int y = static_cast<int>(region.y * m_sheetSize.y);

    int col = x / m_spriteSize.x;
    int row = y / m_spriteSize.y;
    int cols = m_sheetSize.x / m_spriteSize.x;

    return row * cols + col;
}

void SpriteAnimatorDialog::updateWindowTitle() {
    QString title = "Sprite Animator";
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

// Animation playback methods

void SpriteAnimatorDialog::startPlayback() {
    if (m_currentAnimation.isEmpty()) return;

    const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (!animation || animation->frames.empty()) return;

    m_isPlaying = true;
    m_playButton->setText("Pause");
    m_playbackTimer->start();
}

void SpriteAnimatorDialog::stopPlayback() {
    m_isPlaying = false;
    m_playButton->setText("Play");
    m_playbackTimer->stop();
}

void SpriteAnimatorDialog::updatePlayback() {
    if (!m_isPlaying || m_currentAnimation.isEmpty()) return;

    const auto* animation = m_spriteResource->GetAnimation(m_currentAnimation.toStdString());
    if (!animation || animation->frames.empty()) return;

    // Update frame time
    float deltaTime = (PLAYBACK_UPDATE_INTERVAL / 1000.0f) * m_playbackSpeed * animation->speed_scale;
    m_frameTime += deltaTime;

    // Get current frame duration
    int frameIndex = std::clamp(m_currentFrame, 0, static_cast<int>(animation->frames.size()) - 1);
    float frameDuration = animation->frames[frameIndex].duration;

    // Check if we need to advance to next frame
    if (m_frameTime >= frameDuration) {
        m_frameTime = 0.0f;
        m_currentFrame++;

        // Handle looping
        if (m_currentFrame >= static_cast<int>(animation->frames.size())) {
            if (m_isLooping && animation->looping) {
                m_currentFrame = 0;
            } else {
                m_currentFrame = static_cast<int>(animation->frames.size()) - 1;
                stopPlayback();
            }
        }

        updatePreview();
    }
}

// SpriteSheetView implementation

SpriteSheetView::SpriteSheetView(QWidget* parent)
    : QGraphicsView(parent)
    , m_spriteSize(32, 32)
    , m_sheetSize(0, 0)
{
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, false);
}

void SpriteSheetView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        int spriteIndex = getSpriteIndexAt(scenePos.toPoint());
        if (spriteIndex >= 0) {
            emit spriteClicked(spriteIndex);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void SpriteSheetView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        int spriteIndex = getSpriteIndexAt(scenePos.toPoint());
        if (spriteIndex >= 0) {
            emit spriteDoubleClicked(spriteIndex);
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void SpriteSheetView::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);
}

int SpriteSheetView::getSpriteIndexAt(const QPoint& position) const {
    if (m_spriteSize.x <= 0 || m_spriteSize.y <= 0) return -1;

    int col = position.x() / m_spriteSize.x;
    int row = position.y() / m_spriteSize.y;
    int cols = m_sheetSize.x / m_spriteSize.x;
    int rows = m_sheetSize.y / m_spriteSize.y;

    if (col < 0 || col >= cols || row < 0 || row >= rows) return -1;

    return row * cols + col;
}


