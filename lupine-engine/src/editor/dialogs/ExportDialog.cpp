#include "ExportDialog.h"
#include <QApplication>
#include <QStandardPaths>

namespace Lupine {

// ExportWorker implementation
ExportWorker::ExportWorker(ExportManager* manager, const Project* project, const ExportConfig& config)
    : m_export_manager(manager), m_project(project), m_config(config) {
}

void ExportWorker::doExport() {
    auto progress_callback = [this](float progress, const std::string& status) {
        emit progressUpdated(progress, QString::fromStdString(status));
    };
    
    ExportResult result = m_export_manager->ExportProject(m_project, m_config, progress_callback);
    emit exportFinished(result);
}

// ExportDialog implementation
ExportDialog::ExportDialog(const Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
    , m_export_manager(new ExportManager())
    , m_export_thread(nullptr)
    , m_export_worker(nullptr)
    , m_export_in_progress(false) {
    
    setWindowTitle("Export Project");
    setModal(true);
    resize(600, 500);
    
    setupUI();
    populateTargetComboBox();
    
    // Set default configuration
    if (!m_targetComboBox->currentData().isNull()) {
        ExportTarget target = static_cast<ExportTarget>(m_targetComboBox->currentData().toInt());
        ExportConfig defaultConfig = m_export_manager->GetDefaultConfig(target);
        
        // Set default output directory
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        defaultConfig.output_directory = (documentsPath + "/LupineExports").toStdString();
        
        setConfigToUI(defaultConfig);
    }
    
    validateAndEnableExport();
}

ExportDialog::~ExportDialog() {
    if (m_export_thread && m_export_thread->isRunning()) {
        m_export_thread->quit();
        m_export_thread->wait();
    }
    
    delete m_export_manager;
}

void ExportDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupGeneralTab();
    setupPlatformSpecificTab();
    setupAdvancedTab();
    
    // Progress section
    auto* progressLayout = new QVBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_statusLabel = new QLabel();
    m_statusLabel->setVisible(false);
    
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(progressLayout);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_exportButton = new QPushButton("Export");
    m_cancelButton = new QPushButton("Cancel");
    m_closeButton = new QPushButton("Close");
    
    m_cancelButton->setVisible(false);
    
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_targetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onTargetChanged);
    connect(m_browseOutputButton, &QPushButton::clicked,
            this, &ExportDialog::onBrowseOutputDirectory);
    connect(m_browseIconButton, &QPushButton::clicked,
            this, &ExportDialog::onBrowseIcon);
    connect(m_exportButton, &QPushButton::clicked,
            this, &ExportDialog::onExport);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &ExportDialog::onCancel);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
    
    // Connect validation signals
    connect(m_outputDirectoryEdit, &QLineEdit::textChanged,
            this, &ExportDialog::validateAndEnableExport);
    connect(m_executableNameEdit, &QLineEdit::textChanged,
            this, &ExportDialog::validateAndEnableExport);
}

void ExportDialog::setupGeneralTab() {
    auto* generalWidget = new QWidget();
    auto* layout = new QGridLayout(generalWidget);
    
    int row = 0;
    
    // Target platform
    layout->addWidget(new QLabel("Target Platform:"), row, 0);
    m_targetComboBox = new QComboBox();
    layout->addWidget(m_targetComboBox, row++, 1);
    
    // Output directory
    layout->addWidget(new QLabel("Output Directory:"), row, 0);
    auto* outputLayout = new QHBoxLayout();
    m_outputDirectoryEdit = new QLineEdit();
    m_browseOutputButton = new QPushButton("Browse...");
    outputLayout->addWidget(m_outputDirectoryEdit);
    outputLayout->addWidget(m_browseOutputButton);
    layout->addLayout(outputLayout, row++, 1);
    
    // Executable name
    layout->addWidget(new QLabel("Executable Name:"), row, 0);
    m_executableNameEdit = new QLineEdit();
    layout->addWidget(m_executableNameEdit, row++, 1);
    
    // Options
    m_includeDebugSymbolsCheck = new QCheckBox("Include Debug Symbols");
    layout->addWidget(m_includeDebugSymbolsCheck, row++, 1);
    
    m_optimizeAssetsCheck = new QCheckBox("Optimize Assets");
    m_optimizeAssetsCheck->setChecked(true);
    layout->addWidget(m_optimizeAssetsCheck, row++, 1);
    
    m_embedAssetsCheck = new QCheckBox("Embed Assets in Executable");
    m_embedAssetsCheck->setChecked(true);
    layout->addWidget(m_embedAssetsCheck, row++, 1);
    
    layout->setRowStretch(row, 1);
    
    m_tabWidget->addTab(generalWidget, "General");
}

void ExportDialog::setupPlatformSpecificTab() {
    auto* platformWidget = new QWidget();
    auto* layout = new QVBoxLayout(platformWidget);
    
    // Windows settings
    m_windowsGroup = new QGroupBox("Windows Settings");
    auto* windowsLayout = new QGridLayout(m_windowsGroup);
    
    int row = 0;
    windowsLayout->addWidget(new QLabel("Icon:"), row, 0);
    auto* iconLayout = new QHBoxLayout();
    m_windowsIconEdit = new QLineEdit();
    m_browseIconButton = new QPushButton("Browse...");
    iconLayout->addWidget(m_windowsIconEdit);
    iconLayout->addWidget(m_browseIconButton);
    windowsLayout->addLayout(iconLayout, row++, 1);
    
    windowsLayout->addWidget(new QLabel("Version Info:"), row, 0);
    m_versionInfoEdit = new QLineEdit("1.0.0.0");
    windowsLayout->addWidget(m_versionInfoEdit, row++, 1);
    
    m_consoleSubsystemCheck = new QCheckBox("Console Subsystem");
    windowsLayout->addWidget(m_consoleSubsystemCheck, row++, 1);
    
    layout->addWidget(m_windowsGroup);
    
    // Linux settings
    m_linuxGroup = new QGroupBox("Linux Settings");
    auto* linuxLayout = new QGridLayout(m_linuxGroup);
    
    row = 0;
    linuxLayout->addWidget(new QLabel("Desktop File Name:"), row, 0);
    m_desktopFileNameEdit = new QLineEdit();
    linuxLayout->addWidget(m_desktopFileNameEdit, row++, 1);
    
    linuxLayout->addWidget(new QLabel("App Category:"), row, 0);
    m_appCategoryEdit = new QLineEdit("Game");
    linuxLayout->addWidget(m_appCategoryEdit, row++, 1);
    
    linuxLayout->addWidget(new QLabel("Dependencies:"), row, 0);
    m_dependenciesEdit = new QTextEdit();
    m_dependenciesEdit->setMaximumHeight(60);
    linuxLayout->addWidget(m_dependenciesEdit, row++, 1);
    
    layout->addWidget(m_linuxGroup);
    
    // Web settings
    m_webGroup = new QGroupBox("Web Settings");
    auto* webLayout = new QGridLayout(m_webGroup);
    
    row = 0;
    webLayout->addWidget(new QLabel("Canvas Size:"), row, 0);
    m_canvasSizeEdit = new QLineEdit("1920x1080");
    webLayout->addWidget(m_canvasSizeEdit, row++, 1);
    
    webLayout->addWidget(new QLabel("Memory Size (MB):"), row, 0);
    m_memorySizeSpinBox = new QSpinBox();
    m_memorySizeSpinBox->setRange(64, 2048);
    m_memorySizeSpinBox->setValue(512);
    webLayout->addWidget(m_memorySizeSpinBox, row++, 1);
    
    m_enableThreadsCheck = new QCheckBox("Enable Threads");
    webLayout->addWidget(m_enableThreadsCheck, row++, 1);
    
    m_enableSimdCheck = new QCheckBox("Enable SIMD");
    m_enableSimdCheck->setChecked(true);
    webLayout->addWidget(m_enableSimdCheck, row++, 1);
    
    layout->addWidget(m_webGroup);
    
    layout->addStretch();
    
    m_tabWidget->addTab(platformWidget, "Platform Settings");
}

void ExportDialog::setupAdvancedTab() {
    auto* advancedWidget = new QWidget();
    auto* layout = new QVBoxLayout(advancedWidget);
    
    m_createInstallerCheck = new QCheckBox("Create Installer Package");
    layout->addWidget(m_createInstallerCheck);
    
    layout->addWidget(new QLabel("Additional Files to Include:"));
    m_additionalFilesEdit = new QTextEdit();
    m_additionalFilesEdit->setPlaceholderText("Enter file paths, one per line...");
    layout->addWidget(m_additionalFilesEdit);
    
    layout->addStretch();
    
    m_tabWidget->addTab(advancedWidget, "Advanced");
}

void ExportDialog::populateTargetComboBox() {
    auto exporters = m_export_manager->GetAvailableExporters();
    
    for (auto* exporter : exporters) {
        m_targetComboBox->addItem(QString::fromStdString(exporter->GetName()),
                                 static_cast<int>(exporter->GetTarget()));
    }
    
    if (m_targetComboBox->count() == 0) {
        m_targetComboBox->addItem("No exporters available", -1);
        m_exportButton->setEnabled(false);
    }
}

void ExportDialog::onTargetChanged() {
    updatePlatformSpecificSettings();
    validateAndEnableExport();
}

void ExportDialog::updatePlatformSpecificSettings() {
    if (m_targetComboBox->currentData().isNull()) {
        return;
    }
    
    ExportTarget target = static_cast<ExportTarget>(m_targetComboBox->currentData().toInt());
    
    // Hide all platform groups
    m_windowsGroup->setVisible(false);
    m_linuxGroup->setVisible(false);
    m_webGroup->setVisible(false);
    
    // Show relevant group and update executable name
    switch (target) {
        case ExportTarget::Windows_x64:
            m_windowsGroup->setVisible(true);
            if (m_executableNameEdit->text().isEmpty()) {
                m_executableNameEdit->setText("Game.exe");
            }
            break;
            
        case ExportTarget::Linux_x64:
            m_linuxGroup->setVisible(true);
            if (m_executableNameEdit->text().isEmpty()) {
                m_executableNameEdit->setText("Game.AppImage");
            }
            if (m_desktopFileNameEdit->text().isEmpty()) {
                m_desktopFileNameEdit->setText(QString::fromStdString(m_project->GetName()));
            }
            break;
            
        case ExportTarget::Web_HTML5:
            m_webGroup->setVisible(true);
            if (m_executableNameEdit->text().isEmpty()) {
                m_executableNameEdit->setText("index.html");
            }
            break;
    }
}

void ExportDialog::onBrowseOutputDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory",
                                                   m_outputDirectoryEdit->text());
    if (!dir.isEmpty()) {
        m_outputDirectoryEdit->setText(dir);
    }
}

void ExportDialog::onBrowseIcon() {
    QString file = QFileDialog::getOpenFileName(this, "Select Icon File",
                                               QString(),
                                               "Icon Files (*.ico *.png *.jpg *.bmp)");
    if (!file.isEmpty()) {
        m_windowsIconEdit->setText(file);
    }
}

void ExportDialog::onExport() {
    if (m_export_in_progress) {
        return;
    }

    ExportConfig config = getConfigFromUI();

    // Validate configuration
    QString validation_error = QString::fromStdString(m_export_manager->ValidateConfig(config));
    if (!validation_error.isEmpty()) {
        QMessageBox::warning(this, "Export Configuration Error", validation_error);
        return;
    }

    // Start export in worker thread
    m_export_in_progress = true;
    m_exportButton->setVisible(false);
    m_cancelButton->setVisible(true);
    m_progressBar->setVisible(true);
    m_statusLabel->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Preparing export...");

    // Create worker thread
    m_export_thread = new QThread();
    m_export_worker = new ExportWorker(m_export_manager, m_project, config);
    m_export_worker->moveToThread(m_export_thread);

    connect(m_export_thread, &QThread::started, m_export_worker, &ExportWorker::doExport);
    connect(m_export_worker, &ExportWorker::progressUpdated, this, &ExportDialog::onProgressUpdated);
    connect(m_export_worker, &ExportWorker::exportFinished, this, &ExportDialog::onExportFinished);
    connect(m_export_worker, &ExportWorker::exportFinished, m_export_thread, &QThread::quit);
    connect(m_export_thread, &QThread::finished, m_export_worker, &QObject::deleteLater);
    connect(m_export_thread, &QThread::finished, m_export_thread, &QObject::deleteLater);

    m_export_thread->start();
}

void ExportDialog::onCancel() {
    if (m_export_thread && m_export_thread->isRunning()) {
        m_export_thread->requestInterruption();
        m_export_thread->quit();
        m_export_thread->wait(5000); // Wait up to 5 seconds
    }

    m_export_in_progress = false;
    m_exportButton->setVisible(true);
    m_cancelButton->setVisible(false);
    m_progressBar->setVisible(false);
    m_statusLabel->setVisible(false);
}

void ExportDialog::onProgressUpdated(float progress, const QString& status) {
    m_progressBar->setValue(static_cast<int>(progress * 100));
    m_statusLabel->setText(status);
}

void ExportDialog::onExportFinished(const ExportResult& result) {
    m_export_in_progress = false;
    m_exportButton->setVisible(true);
    m_cancelButton->setVisible(false);

    if (result.success) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("Export completed successfully!");

        QString message = QString("Export completed successfully!\n\nOutput: %1\nSize: %2 bytes\nFiles: %3")
                         .arg(QString::fromStdString(result.output_path))
                         .arg(result.total_size_bytes)
                         .arg(result.generated_files.size());

        QMessageBox::information(this, "Export Successful", message);
    } else {
        m_statusLabel->setText("Export failed!");
        QMessageBox::critical(this, "Export Failed",
                             QString("Export failed:\n%1").arg(QString::fromStdString(result.error_message)));
    }
}

ExportConfig ExportDialog::getConfigFromUI() const {
    ExportConfig config;

    if (!m_targetComboBox->currentData().isNull()) {
        config.target = static_cast<ExportTarget>(m_targetComboBox->currentData().toInt());
    }

    config.output_directory = m_outputDirectoryEdit->text().toStdString();
    config.executable_name = m_executableNameEdit->text().toStdString();
    config.include_debug_symbols = m_includeDebugSymbolsCheck->isChecked();
    config.optimize_assets = m_optimizeAssetsCheck->isChecked();
    config.embed_assets = m_embedAssetsCheck->isChecked();
    config.create_installer = m_createInstallerCheck->isChecked();

    // Platform-specific settings
    config.windows.icon_path = m_windowsIconEdit->text().toStdString();
    config.windows.version_info = m_versionInfoEdit->text().toStdString();
    config.windows.console_subsystem = m_consoleSubsystemCheck->isChecked();

    config.linux_config.desktop_file_name = m_desktopFileNameEdit->text().toStdString();
    config.linux_config.app_category = m_appCategoryEdit->text().toStdString();

    // Parse dependencies
    QString deps = m_dependenciesEdit->toPlainText();
    QStringList depList = deps.split('\n', Qt::SkipEmptyParts);
    for (const QString& dep : depList) {
        config.linux_config.dependencies.push_back(dep.trimmed().toStdString());
    }

    config.web.canvas_size = m_canvasSizeEdit->text().toStdString();
    config.web.enable_threads = m_enableThreadsCheck->isChecked();
    config.web.enable_simd = m_enableSimdCheck->isChecked();
    config.web.memory_size_mb = m_memorySizeSpinBox->value();

    return config;
}

void ExportDialog::setConfigToUI(const ExportConfig& config) {
    // Find and set target
    for (int i = 0; i < m_targetComboBox->count(); ++i) {
        if (m_targetComboBox->itemData(i).toInt() == static_cast<int>(config.target)) {
            m_targetComboBox->setCurrentIndex(i);
            break;
        }
    }

    m_outputDirectoryEdit->setText(QString::fromStdString(config.output_directory));
    m_executableNameEdit->setText(QString::fromStdString(config.executable_name));
    m_includeDebugSymbolsCheck->setChecked(config.include_debug_symbols);
    m_optimizeAssetsCheck->setChecked(config.optimize_assets);
    m_embedAssetsCheck->setChecked(config.embed_assets);
    m_createInstallerCheck->setChecked(config.create_installer);

    // Platform-specific settings
    m_windowsIconEdit->setText(QString::fromStdString(config.windows.icon_path));
    m_versionInfoEdit->setText(QString::fromStdString(config.windows.version_info));
    m_consoleSubsystemCheck->setChecked(config.windows.console_subsystem);

    m_desktopFileNameEdit->setText(QString::fromStdString(config.linux_config.desktop_file_name));
    m_appCategoryEdit->setText(QString::fromStdString(config.linux_config.app_category));

    // Set dependencies
    QStringList deps;
    for (const auto& dep : config.linux_config.dependencies) {
        deps << QString::fromStdString(dep);
    }
    m_dependenciesEdit->setPlainText(deps.join('\n'));

    m_canvasSizeEdit->setText(QString::fromStdString(config.web.canvas_size));
    m_enableThreadsCheck->setChecked(config.web.enable_threads);
    m_enableSimdCheck->setChecked(config.web.enable_simd);
    m_memorySizeSpinBox->setValue(config.web.memory_size_mb);

    updatePlatformSpecificSettings();
}

void ExportDialog::validateAndEnableExport() {
    bool valid = true;

    if (m_outputDirectoryEdit->text().isEmpty()) {
        valid = false;
    }

    if (m_executableNameEdit->text().isEmpty()) {
        valid = false;
    }

    if (m_targetComboBox->currentData().isNull() || m_targetComboBox->currentData().toInt() == -1) {
        valid = false;
    }

    m_exportButton->setEnabled(valid && !m_export_in_progress);
}

} // namespace Lupine


