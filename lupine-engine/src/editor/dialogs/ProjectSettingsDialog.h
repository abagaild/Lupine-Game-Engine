#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>

namespace Lupine {
    class Project;
}

/**
 * @brief Project Settings Dialog
 * 
 * Provides a comprehensive interface for configuring project settings
 * including display settings, rendering options, and main scene configuration.
 */
class ProjectSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProjectSettingsDialog(Lupine::Project* project, QWidget* parent = nullptr);

private slots:
    void onBrowseMainScene();
    void onAccepted();
    void onRejected();
    void onApply();

private:
    void setupUI();
    void setupDisplayTab();
    void setupRenderingTab();
    void setupGeneralTab();
    void setupEditorTab();
    void loadSettings();
    void saveSettings();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_applyButton;
    
    // General Tab
    QWidget* m_generalTab;
    QFormLayout* m_generalLayout;
    QLineEdit* m_projectNameEdit;
    QLineEdit* m_projectVersionEdit;
    QLineEdit* m_mainSceneEdit;
    QPushButton* m_browseMainSceneButton;
    
    // Display Tab
    QWidget* m_displayTab;
    QFormLayout* m_displayLayout;
    
    // Debug Window Settings
    QGroupBox* m_debugWindowGroup;
    QFormLayout* m_debugWindowLayout;
    QDoubleSpinBox* m_debugWindowScaleSpin;
    QSpinBox* m_debugRenderWidthSpin;
    QSpinBox* m_debugRenderHeightSpin;

    // Default Window Settings
    QGroupBox* m_defaultWindowGroup;
    QFormLayout* m_defaultWindowLayout;
    QSpinBox* m_defaultWidthSpin;
    QSpinBox* m_defaultHeightSpin;
    QCheckBox* m_defaultFullscreenCheck;
    QCheckBox* m_enableHighDPICheck;

    // Render Resolution Settings
    QGroupBox* m_renderResolutionGroup;
    QFormLayout* m_renderResolutionLayout;
    QSpinBox* m_renderWidthSpin;
    QSpinBox* m_renderHeightSpin;
    
    // Scale Settings
    QGroupBox* m_scaleGroup;
    QFormLayout* m_scaleLayout;
    QComboBox* m_scaleTypeCombo;
    
    // Rendering Tab
    QWidget* m_renderingTab;
    QFormLayout* m_renderingLayout;
    
    // Stretch Settings
    QGroupBox* m_stretchGroup;
    QFormLayout* m_stretchLayout;
    QComboBox* m_stretchStyleCombo;

    // Editor Tab
    QWidget* m_editorTab;
    QFormLayout* m_editorLayout;

    // Editor Settings
    QGroupBox* m_editorGroup;
    QFormLayout* m_editorGroupLayout;
    QSpinBox* m_undoDepthSpin;

    // Data
    Lupine::Project* m_project;
};

/**
 * @brief Enumeration for scale types
 */
enum class ScaleType {
    Letterbox,
    StretchKeepAspect,
    Stretch,
    Crop
};

/**
 * @brief Enumeration for stretch styles
 */
enum class StretchStyle {
    Bilinear,
    Bicubic,
    Nearest
};

/**
 * @brief Project display settings structure
 */
struct ProjectDisplaySettings {
    // Debug window settings
    float debugWindowScale = 1.0f;
    int debugRenderWidth = 1920;
    int debugRenderHeight = 1080;

    // Default window settings
    int defaultWindowWidth = 1920;
    int defaultWindowHeight = 1080;
    bool defaultFullscreen = false;
    bool enableHighDPI = true;

    // Render resolution settings
    int renderWidth = 1920;
    int renderHeight = 1080;

    // Scale settings
    ScaleType scaleType = ScaleType::Letterbox;

    // Main scene
    std::string mainScenePath;
};

/**
 * @brief Project rendering settings structure
 */
struct ProjectRenderingSettings {
    // Stretch settings
    StretchStyle stretchStyle = StretchStyle::Bilinear;
};

/**
 * @brief Complete project settings structure
 */
struct ProjectSettings {
    // General settings
    std::string projectName;
    std::string projectVersion = "1.0.0";
    
    // Display settings
    ProjectDisplaySettings display;
    
    // Rendering settings
    ProjectRenderingSettings rendering;
    
    /**
     * @brief Convert scale type to string
     */
    static std::string ScaleTypeToString(ScaleType type);
    
    /**
     * @brief Convert string to scale type
     */
    static ScaleType StringToScaleType(const std::string& str);
    
    /**
     * @brief Convert stretch style to string
     */
    static std::string StretchStyleToString(StretchStyle style);
    
    /**
     * @brief Convert string to stretch style
     */
    static StretchStyle StringToStretchStyle(const std::string& str);
};
