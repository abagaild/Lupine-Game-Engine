#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QMutex>

#include "lupine/export/ExportManager.h"
#include "lupine/core/Project.h"

namespace Lupine {

/**
 * @brief Worker thread for export operations
 */
class ExportWorker : public QObject {
    Q_OBJECT

public:
    ExportWorker(ExportManager* manager, const Project* project, const ExportConfig& config);

public slots:
    void doExport();

signals:
    void progressUpdated(float progress, const QString& status);
    void exportFinished(const ExportResult& result);

private:
    ExportManager* m_export_manager;
    const Project* m_project;
    ExportConfig m_config;
};

/**
 * @brief Export dialog for configuring and running project exports
 */
class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(const Project* project, QWidget* parent = nullptr);
    ~ExportDialog();

private slots:
    void onTargetChanged();
    void onBrowseOutputDirectory();
    void onBrowseIcon();
    void onBrowseMacIcon();
    void onExport();
    void onCancel();
    void onProgressUpdated(float progress, const QString& status);
    void onExportFinished(const ExportResult& result);

private:
    void setupUI();
    void setupGeneralTab();
    void setupPlatformSpecificTab();
    void setupAdvancedTab();
    void updatePlatformSpecificSettings();
    void populateTargetComboBox();
    ExportConfig getConfigFromUI() const;
    void setConfigToUI(const ExportConfig& config);
    void validateAndEnableExport();

    // UI Components
    QTabWidget* m_tabWidget;
    
    // General tab
    QComboBox* m_targetComboBox;
    QLineEdit* m_outputDirectoryEdit;
    QPushButton* m_browseOutputButton;
    QLineEdit* m_executableNameEdit;
    QCheckBox* m_includeDebugSymbolsCheck;
    QCheckBox* m_optimizeAssetsCheck;
    QCheckBox* m_embedAssetsCheck;
    
    // Platform-specific tab
    QGroupBox* m_windowsGroup;
    QLineEdit* m_windowsIconEdit;
    QPushButton* m_browseIconButton;
    QLineEdit* m_versionInfoEdit;
    QCheckBox* m_consoleSubsystemCheck;
    
    QGroupBox* m_linuxGroup;
    QLineEdit* m_desktopFileNameEdit;
    QLineEdit* m_appCategoryEdit;
    QTextEdit* m_dependenciesEdit;

    QGroupBox* m_macGroup;
    QLineEdit* m_macIconEdit;
    QPushButton* m_browseMacIconButton;
    QLineEdit* m_bundleIdentifierEdit;
    QLineEdit* m_macVersionEdit;
    QLineEdit* m_minimumOSEdit;
    QCheckBox* m_createDMGCheck;
    QCheckBox* m_codeSignCheck;
    QLineEdit* m_developerIdEdit;

    QGroupBox* m_webGroup;
    QLineEdit* m_canvasSizeEdit;
    QCheckBox* m_enableThreadsCheck;
    QCheckBox* m_enableSimdCheck;
    QSpinBox* m_memorySizeSpinBox;
    
    // Advanced tab
    QCheckBox* m_createInstallerCheck;
    QTextEdit* m_additionalFilesEdit;
    
    // Error display
    QLabel* m_errorLabel;

    // Progress and controls
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
    QPushButton* m_closeButton;
    
    // Data
    const Project* m_project;
    ExportManager* m_export_manager;
    QThread* m_export_thread;
    ExportWorker* m_export_worker;
    bool m_export_in_progress;
};

} // namespace Lupine
