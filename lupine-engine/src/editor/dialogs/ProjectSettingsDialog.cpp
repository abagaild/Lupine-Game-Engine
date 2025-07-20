#include "ProjectSettingsDialog.h"
#include "lupine/core/Project.h"
#include <QMessageBox>
#include <QStandardPaths>

ProjectSettingsDialog::ProjectSettingsDialog(Lupine::Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project) {
    
    setWindowTitle("Project Settings");
    setModal(true);
    resize(600, 500);
    
    setupUI();
    loadSettings();
}

void ProjectSettingsDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    m_mainLayout->addWidget(m_tabWidget);
    
    // Setup tabs
    setupGeneralTab();
    setupDisplayTab();
    setupRenderingTab();
    setupEditorTab();
    
    // Create button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_applyButton = new QPushButton("Apply");
    m_buttonBox->addButton(m_applyButton, QDialogButtonBox::ApplyRole);
    
    m_mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ProjectSettingsDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ProjectSettingsDialog::onRejected);
    connect(m_applyButton, &QPushButton::clicked, this, &ProjectSettingsDialog::onApply);
    connect(m_browseMainSceneButton, &QPushButton::clicked, this, &ProjectSettingsDialog::onBrowseMainScene);
}

void ProjectSettingsDialog::setupGeneralTab() {
    m_generalTab = new QWidget();
    m_generalLayout = new QFormLayout(m_generalTab);
    
    // Project name
    m_projectNameEdit = new QLineEdit();
    m_generalLayout->addRow("Project Name:", m_projectNameEdit);
    
    // Project version
    m_projectVersionEdit = new QLineEdit();
    m_generalLayout->addRow("Project Version:", m_projectVersionEdit);
    
    // Main scene
    QHBoxLayout* mainSceneLayout = new QHBoxLayout();
    m_mainSceneEdit = new QLineEdit();
    m_browseMainSceneButton = new QPushButton("Browse...");
    mainSceneLayout->addWidget(m_mainSceneEdit);
    mainSceneLayout->addWidget(m_browseMainSceneButton);
    m_generalLayout->addRow("Main Scene:", mainSceneLayout);
    
    m_tabWidget->addTab(m_generalTab, "General");
}

void ProjectSettingsDialog::setupDisplayTab() {
    m_displayTab = new QWidget();
    m_displayLayout = new QFormLayout(m_displayTab);
    
    // Debug Window Settings
    m_debugWindowGroup = new QGroupBox("Debug Window Settings");
    m_debugWindowLayout = new QFormLayout(m_debugWindowGroup);

    m_debugWindowScaleSpin = new QDoubleSpinBox();
    m_debugWindowScaleSpin->setRange(0.1, 5.0);
    m_debugWindowScaleSpin->setSingleStep(0.1);
    m_debugWindowScaleSpin->setDecimals(1);
    m_debugWindowScaleSpin->setSuffix("x");
    m_debugWindowLayout->addRow("Window Scale:", m_debugWindowScaleSpin);

    m_debugRenderWidthSpin = new QSpinBox();
    m_debugRenderWidthSpin->setRange(320, 7680);
    m_debugRenderWidthSpin->setSingleStep(1);
    m_debugWindowLayout->addRow("Debug Render Width:", m_debugRenderWidthSpin);

    m_debugRenderHeightSpin = new QSpinBox();
    m_debugRenderHeightSpin->setRange(240, 4320);
    m_debugRenderHeightSpin->setSingleStep(1);
    m_debugWindowLayout->addRow("Debug Render Height:", m_debugRenderHeightSpin);

    m_displayLayout->addWidget(m_debugWindowGroup);
    
    // Default Window Settings
    m_defaultWindowGroup = new QGroupBox("Default Window Settings");
    m_defaultWindowLayout = new QFormLayout(m_defaultWindowGroup);
    
    m_defaultWidthSpin = new QSpinBox();
    m_defaultWidthSpin->setRange(320, 7680);
    m_defaultWidthSpin->setSingleStep(1);
    m_defaultWindowLayout->addRow("Default Width:", m_defaultWidthSpin);
    
    m_defaultHeightSpin = new QSpinBox();
    m_defaultHeightSpin->setRange(240, 4320);
    m_defaultHeightSpin->setSingleStep(1);
    m_defaultWindowLayout->addRow("Default Height:", m_defaultHeightSpin);
    
    m_defaultFullscreenCheck = new QCheckBox();
    m_defaultWindowLayout->addRow("Default Fullscreen:", m_defaultFullscreenCheck);

    m_enableHighDPICheck = new QCheckBox();
    m_enableHighDPICheck->setChecked(true); // Default to enabled
    m_defaultWindowLayout->addRow("Enable High DPI:", m_enableHighDPICheck);

    m_displayLayout->addWidget(m_defaultWindowGroup);

    // Render Resolution Settings
    m_renderResolutionGroup = new QGroupBox("Render Resolution Settings");
    m_renderResolutionLayout = new QFormLayout(m_renderResolutionGroup);

    m_renderWidthSpin = new QSpinBox();
    m_renderWidthSpin->setRange(320, 7680);
    m_renderWidthSpin->setSingleStep(1);
    m_renderResolutionLayout->addRow("Render Width:", m_renderWidthSpin);

    m_renderHeightSpin = new QSpinBox();
    m_renderHeightSpin->setRange(240, 4320);
    m_renderHeightSpin->setSingleStep(1);
    m_renderResolutionLayout->addRow("Render Height:", m_renderHeightSpin);

    m_displayLayout->addWidget(m_renderResolutionGroup);
    
    // Scale Settings
    m_scaleGroup = new QGroupBox("Scale Settings");
    m_scaleLayout = new QFormLayout(m_scaleGroup);
    
    m_scaleTypeCombo = new QComboBox();
    m_scaleTypeCombo->addItem("Letterbox", static_cast<int>(ScaleType::Letterbox));
    m_scaleTypeCombo->addItem("Stretch Keep Aspect", static_cast<int>(ScaleType::StretchKeepAspect));
    m_scaleTypeCombo->addItem("Stretch", static_cast<int>(ScaleType::Stretch));
    m_scaleTypeCombo->addItem("Crop", static_cast<int>(ScaleType::Crop));
    m_scaleLayout->addRow("Scale Type:", m_scaleTypeCombo);
    
    m_displayLayout->addWidget(m_scaleGroup);
    
    m_tabWidget->addTab(m_displayTab, "Display");
}

void ProjectSettingsDialog::setupRenderingTab() {
    m_renderingTab = new QWidget();
    m_renderingLayout = new QFormLayout(m_renderingTab);
    
    // Stretch Settings
    m_stretchGroup = new QGroupBox("Texture Filtering");
    m_stretchLayout = new QFormLayout(m_stretchGroup);
    
    m_stretchStyleCombo = new QComboBox();
    m_stretchStyleCombo->addItem("Bilinear", static_cast<int>(StretchStyle::Bilinear));
    m_stretchStyleCombo->addItem("Bicubic", static_cast<int>(StretchStyle::Bicubic));
    m_stretchStyleCombo->addItem("Nearest", static_cast<int>(StretchStyle::Nearest));
    m_stretchLayout->addRow("Stretch Style:", m_stretchStyleCombo);
    
    m_renderingLayout->addWidget(m_stretchGroup);
    
    m_tabWidget->addTab(m_renderingTab, "Rendering");
}

void ProjectSettingsDialog::setupEditorTab() {
    m_editorTab = new QWidget();
    m_editorLayout = new QFormLayout(m_editorTab);

    // Editor Settings Group
    m_editorGroup = new QGroupBox("Editor Settings");
    m_editorGroupLayout = new QFormLayout(m_editorGroup);

    // Undo Depth Setting
    m_undoDepthSpin = new QSpinBox();
    m_undoDepthSpin->setMinimum(1);
    m_undoDepthSpin->setMaximum(1000);
    m_undoDepthSpin->setValue(25);
    m_undoDepthSpin->setToolTip("Number of undo steps to keep in memory. Higher values use more memory.");
    m_editorGroupLayout->addRow("Undo Depth:", m_undoDepthSpin);

    m_editorLayout->addWidget(m_editorGroup);

    m_tabWidget->addTab(m_editorTab, "Editor");
}

void ProjectSettingsDialog::loadSettings() {
    if (!m_project) return;

    // Load general settings
    m_projectNameEdit->setText(QString::fromStdString(m_project->GetName()));
    m_projectVersionEdit->setText(QString::fromStdString(m_project->GetVersion()));

    // Try to get main scene from project
    std::string mainScene = m_project->GetMainScene();
    m_mainSceneEdit->setText(QString::fromStdString(mainScene));

    // Load display settings from project settings
    m_debugWindowScaleSpin->setValue(m_project->GetSettingValue<float>("display/debug_window_scale", 1.0f));
    m_debugRenderWidthSpin->setValue(m_project->GetSettingValue<int>("display/debug_render_width", 1920));
    m_debugRenderHeightSpin->setValue(m_project->GetSettingValue<int>("display/debug_render_height", 1080));

    m_defaultWidthSpin->setValue(m_project->GetSettingValue<int>("display/window_width", 1920));
    m_defaultHeightSpin->setValue(m_project->GetSettingValue<int>("display/window_height", 1080));
    m_defaultFullscreenCheck->setChecked(m_project->GetSettingValue<bool>("display/fullscreen", false));
    m_enableHighDPICheck->setChecked(m_project->GetSettingValue<bool>("display/enable_high_dpi", true));

    m_renderWidthSpin->setValue(m_project->GetSettingValue<int>("display/render_width", 1920));
    m_renderHeightSpin->setValue(m_project->GetSettingValue<int>("display/render_height", 1080));

    // Load scale type
    std::string scaleTypeStr = m_project->GetSettingValue<std::string>("display/scale_type", "letterbox");
    ScaleType scaleType = ProjectSettings::StringToScaleType(scaleTypeStr);
    int scaleTypeIndex = static_cast<int>(scaleType);
    if (scaleTypeIndex >= 0 && scaleTypeIndex < m_scaleTypeCombo->count()) {
        m_scaleTypeCombo->setCurrentIndex(scaleTypeIndex);
    }

    // Load rendering settings
    std::string stretchStyleStr = m_project->GetSettingValue<std::string>("rendering/stretch_style", "bilinear");
    StretchStyle stretchStyle = ProjectSettings::StringToStretchStyle(stretchStyleStr);
    int stretchStyleIndex = static_cast<int>(stretchStyle);
    if (stretchStyleIndex >= 0 && stretchStyleIndex < m_stretchStyleCombo->count()) {
        m_stretchStyleCombo->setCurrentIndex(stretchStyleIndex);
    }

    // Load editor settings
    m_undoDepthSpin->setValue(m_project->GetSettingValue<int>("editor/undo_depth", 25));
}

void ProjectSettingsDialog::saveSettings() {
    if (!m_project) return;

    // Save general settings
    m_project->SetName(m_projectNameEdit->text().toStdString());
    m_project->SetVersion(m_projectVersionEdit->text().toStdString());
    m_project->SetMainScene(m_mainSceneEdit->text().toStdString());

    // Save display settings
    m_project->SetSetting("display/debug_window_scale", static_cast<float>(m_debugWindowScaleSpin->value()));
    m_project->SetSetting("display/debug_render_width", m_debugRenderWidthSpin->value());
    m_project->SetSetting("display/debug_render_height", m_debugRenderHeightSpin->value());

    m_project->SetSetting("display/window_width", m_defaultWidthSpin->value());
    m_project->SetSetting("display/window_height", m_defaultHeightSpin->value());
    m_project->SetSetting("display/fullscreen", m_defaultFullscreenCheck->isChecked());
    m_project->SetSetting("display/enable_high_dpi", m_enableHighDPICheck->isChecked());

    m_project->SetSetting("display/render_width", m_renderWidthSpin->value());
    m_project->SetSetting("display/render_height", m_renderHeightSpin->value());

    // Save scale type
    int scaleTypeIndex = m_scaleTypeCombo->currentIndex();
    if (scaleTypeIndex >= 0) {
        ScaleType scaleType = static_cast<ScaleType>(m_scaleTypeCombo->currentData().toInt());
        m_project->SetSetting("display/scale_type", ProjectSettings::ScaleTypeToString(scaleType));
    }

    // Save rendering settings
    int stretchStyleIndex = m_stretchStyleCombo->currentIndex();
    if (stretchStyleIndex >= 0) {
        StretchStyle stretchStyle = static_cast<StretchStyle>(m_stretchStyleCombo->currentData().toInt());
        m_project->SetSetting("rendering/stretch_style", ProjectSettings::StretchStyleToString(stretchStyle));
    }

    // Save editor settings
    m_project->SetSetting("editor/undo_depth", m_undoDepthSpin->value());

    // Actually save the project to file
    if (!m_project->SaveToFile()) {
        // TODO: Show error message to user
        qWarning() << "Failed to save project settings to file";
    }
}

void ProjectSettingsDialog::onBrowseMainScene() {
    QString projectDir = QString::fromStdString(m_project->GetProjectDirectory());
    QString sceneFile = QFileDialog::getOpenFileName(
        this,
        "Select Main Scene",
        projectDir,
        "Scene Files (*.scene);;All Files (*)"
    );
    
    if (!sceneFile.isEmpty()) {
        // Make path relative to project directory if possible
        QDir projectDirObj(projectDir);
        QString relativePath = projectDirObj.relativeFilePath(sceneFile);
        m_mainSceneEdit->setText(relativePath);
    }
}

void ProjectSettingsDialog::onAccepted() {
    saveSettings();
    accept();
}

void ProjectSettingsDialog::onRejected() {
    reject();
}

void ProjectSettingsDialog::onApply() {
    saveSettings();
}

// ProjectSettings utility methods
std::string ProjectSettings::ScaleTypeToString(ScaleType type) {
    switch (type) {
        case ScaleType::Letterbox: return "letterbox";
        case ScaleType::StretchKeepAspect: return "stretch_keep_aspect";
        case ScaleType::Stretch: return "stretch";
        case ScaleType::Crop: return "crop";
        default: return "letterbox";
    }
}

ScaleType ProjectSettings::StringToScaleType(const std::string& str) {
    if (str == "letterbox") return ScaleType::Letterbox;
    if (str == "stretch_keep_aspect") return ScaleType::StretchKeepAspect;
    if (str == "stretch") return ScaleType::Stretch;
    if (str == "crop") return ScaleType::Crop;
    return ScaleType::Letterbox; // Default
}

std::string ProjectSettings::StretchStyleToString(StretchStyle style) {
    switch (style) {
        case StretchStyle::Bilinear: return "bilinear";
        case StretchStyle::Bicubic: return "bicubic";
        case StretchStyle::Nearest: return "nearest";
        default: return "bilinear";
    }
}

StretchStyle ProjectSettings::StringToStretchStyle(const std::string& str) {
    if (str == "bilinear") return StretchStyle::Bilinear;
    if (str == "bicubic") return StretchStyle::Bicubic;
    if (str == "nearest") return StretchStyle::Nearest;
    return StretchStyle::Bilinear; // Default
}
