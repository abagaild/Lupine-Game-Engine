#include "LocalizationSettingsDialog.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QCloseEvent>

LocalizationSettingsDialog::LocalizationSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_localeGroup(nullptr)
    , m_settingsGroup(nullptr)
    , m_fileGroup(nullptr)
    , m_autoDetectLocale(true)
    , m_fallbackToDefault(true)
    , m_showMissingKeys(false)
    , m_settingsChanged(false) {
    
    setWindowTitle("Localization Settings");
    setModal(true);
    resize(600, 500);
    
    setupUI();
    loadSettings();
}

LocalizationSettingsDialog::~LocalizationSettingsDialog() = default;

void LocalizationSettingsDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupLocaleManagement();
    setupSettings();
    setupFileOperations();
    setupDialogButtons();
    
    setLayout(m_mainLayout);
}

void LocalizationSettingsDialog::setupLocaleManagement() {
    m_localeGroup = new QGroupBox("Supported Locales");
    m_localeLayout = new QVBoxLayout(m_localeGroup);
    
    // Locale list
    m_localeList = new QListWidget();
    m_localeList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_localeLayout->addWidget(m_localeList);
    
    // Buttons
    m_localeButtonLayout = new QHBoxLayout();
    m_addLocaleButton = new QPushButton("Add Locale");
    m_removeLocaleButton = new QPushButton("Remove Locale");
    m_editLocaleButton = new QPushButton("Edit Locale");
    
    m_removeLocaleButton->setEnabled(false);
    m_editLocaleButton->setEnabled(false);
    
    m_localeButtonLayout->addWidget(m_addLocaleButton);
    m_localeButtonLayout->addWidget(m_removeLocaleButton);
    m_localeButtonLayout->addWidget(m_editLocaleButton);
    m_localeButtonLayout->addStretch();
    
    m_localeLayout->addLayout(m_localeButtonLayout);
    m_mainLayout->addWidget(m_localeGroup);
    
    // Connect signals
    connect(m_addLocaleButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onAddLocale);
    connect(m_removeLocaleButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onRemoveLocale);
    connect(m_editLocaleButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onEditLocale);
    connect(m_localeList, &QListWidget::itemSelectionChanged, this, &LocalizationSettingsDialog::onLocaleSelectionChanged);
}

void LocalizationSettingsDialog::setupSettings() {
    m_settingsGroup = new QGroupBox("Localization Settings");
    m_settingsLayout = new QGridLayout(m_settingsGroup);
    
    // Default locale
    m_defaultLocaleLabel = new QLabel("Default Locale:");
    m_defaultLocaleCombo = new QComboBox();
    m_settingsLayout->addWidget(m_defaultLocaleLabel, 0, 0);
    m_settingsLayout->addWidget(m_defaultLocaleCombo, 0, 1);
    
    // Auto-detect locale
    m_autoDetectLocaleCheck = new QCheckBox("Auto-detect system locale on startup");
    m_settingsLayout->addWidget(m_autoDetectLocaleCheck, 1, 0, 1, 2);
    
    // Fallback to default
    m_fallbackToDefaultCheck = new QCheckBox("Fallback to default locale for missing keys");
    m_settingsLayout->addWidget(m_fallbackToDefaultCheck, 2, 0, 1, 2);
    
    // Show missing keys
    m_showMissingKeysCheck = new QCheckBox("Show missing localization keys in console");
    m_settingsLayout->addWidget(m_showMissingKeysCheck, 3, 0, 1, 2);
    
    m_mainLayout->addWidget(m_settingsGroup);
    
    // Connect signals
    connect(m_defaultLocaleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &LocalizationSettingsDialog::onDefaultLocaleChanged);
    connect(m_autoDetectLocaleCheck, &QCheckBox::toggled, [this]() { m_settingsChanged = true; });
    connect(m_fallbackToDefaultCheck, &QCheckBox::toggled, [this]() { m_settingsChanged = true; });
    connect(m_showMissingKeysCheck, &QCheckBox::toggled, [this]() { m_settingsChanged = true; });
}

void LocalizationSettingsDialog::setupFileOperations() {
    m_fileGroup = new QGroupBox("File Operations");
    m_fileLayout = new QHBoxLayout(m_fileGroup);
    
    m_importButton = new QPushButton("Import Localization Data");
    m_exportButton = new QPushButton("Export Localization Data");
    m_resetButton = new QPushButton("Reset to Defaults");
    
    m_fileLayout->addWidget(m_importButton);
    m_fileLayout->addWidget(m_exportButton);
    m_fileLayout->addWidget(m_resetButton);
    m_fileLayout->addStretch();
    
    m_mainLayout->addWidget(m_fileGroup);
    
    // Connect signals
    connect(m_importButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onImportLocalization);
    connect(m_exportButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onExportLocalization);
    connect(m_resetButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onResetToDefaults);
}

void LocalizationSettingsDialog::setupDialogButtons() {
    m_buttonLayout = new QHBoxLayout();
    
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    m_applyButton = new QPushButton("Apply");
    
    m_okButton->setDefault(true);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_okButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &LocalizationSettingsDialog::onApplyClicked);
}

void LocalizationSettingsDialog::loadSettings() {
    auto& locManager = Lupine::LocalizationManager::Instance();
    
    // Load supported locales
    m_supportedLocales = locManager.GetSupportedLocales();
    m_defaultLocale = locManager.GetDefaultLocale();
    
    // Load settings (these would come from project settings in a real implementation)
    m_autoDetectLocale = true;
    m_fallbackToDefault = true;
    m_showMissingKeys = false;
    
    // Update UI
    refreshLocaleList();
    refreshDefaultLocaleCombo();
    
    m_autoDetectLocaleCheck->setChecked(m_autoDetectLocale);
    m_fallbackToDefaultCheck->setChecked(m_fallbackToDefault);
    m_showMissingKeysCheck->setChecked(m_showMissingKeys);
    
    m_settingsChanged = false;
}

void LocalizationSettingsDialog::refreshLocaleList() {
    m_localeList->clear();
    
    for (const auto& locale : m_supportedLocales) {
        QString displayText = QString::fromStdString(locale.display_name);
        if (displayText.isEmpty()) {
            displayText = QString::fromStdString(locale.GetIdentifier());
        }
        
        QListWidgetItem* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QString::fromStdString(locale.GetIdentifier()));
        m_localeList->addItem(item);
    }
}

void LocalizationSettingsDialog::refreshDefaultLocaleCombo() {
    m_defaultLocaleCombo->clear();
    
    int defaultIndex = 0;
    for (size_t i = 0; i < m_supportedLocales.size(); ++i) {
        const auto& locale = m_supportedLocales[i];
        QString displayText = QString::fromStdString(locale.display_name);
        if (displayText.isEmpty()) {
            displayText = QString::fromStdString(locale.GetIdentifier());
        }
        
        m_defaultLocaleCombo->addItem(displayText, QString::fromStdString(locale.GetIdentifier()));
        
        if (locale == m_defaultLocale) {
            defaultIndex = static_cast<int>(i);
        }
    }
    
    m_defaultLocaleCombo->setCurrentIndex(defaultIndex);
}

void LocalizationSettingsDialog::onLocaleSelectionChanged() {
    bool hasSelection = !m_localeList->selectedItems().isEmpty();
    m_removeLocaleButton->setEnabled(hasSelection);
    m_editLocaleButton->setEnabled(hasSelection);
}

void LocalizationSettingsDialog::onDefaultLocaleChanged() {
    m_settingsChanged = true;
}

void LocalizationSettingsDialog::onAddLocale() {
    showAddLocaleDialog();
}

void LocalizationSettingsDialog::onRemoveLocale() {
    auto selectedItems = m_localeList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString identifier = selectedItems.first()->data(Qt::UserRole).toString();

    int ret = QMessageBox::question(this, "Remove Locale",
        QString("Are you sure you want to remove the locale '%1'?\nThis will delete all localization data for this locale.")
        .arg(selectedItems.first()->text()),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Find and remove locale
        auto it = std::find_if(m_supportedLocales.begin(), m_supportedLocales.end(),
            [&identifier](const Lupine::Locale& locale) {
                return locale.GetIdentifier() == identifier.toStdString();
            });

        if (it != m_supportedLocales.end()) {
            m_supportedLocales.erase(it);
            refreshLocaleList();
            refreshDefaultLocaleCombo();
            m_settingsChanged = true;
        }
    }
}

void LocalizationSettingsDialog::onEditLocale() {
    auto selectedItems = m_localeList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString identifier = selectedItems.first()->data(Qt::UserRole).toString();

    // Find locale
    auto it = std::find_if(m_supportedLocales.begin(), m_supportedLocales.end(),
        [&identifier](const Lupine::Locale& locale) {
            return locale.GetIdentifier() == identifier.toStdString();
        });

    if (it != m_supportedLocales.end()) {
        showEditLocaleDialog(*it);
    }
}

void LocalizationSettingsDialog::onImportLocalization() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Localization Data",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        if (locManager.LoadFromFile(fileName.toStdString())) {
            loadSettings(); // Refresh UI
            QMessageBox::information(this, "Import Successful",
                "Localization data imported successfully.");
        } else {
            QMessageBox::warning(this, "Import Failed",
                "Failed to import localization data. Please check the file format.");
        }
    }
}

void LocalizationSettingsDialog::onExportLocalization() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Localization Data",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/localization.json",
        "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        if (locManager.SaveToFile(fileName.toStdString())) {
            QMessageBox::information(this, "Export Successful",
                "Localization data exported successfully.");
        } else {
            QMessageBox::warning(this, "Export Failed",
                "Failed to export localization data.");
        }
    }
}

void LocalizationSettingsDialog::onResetToDefaults() {
    int ret = QMessageBox::question(this, "Reset to Defaults",
        "Are you sure you want to reset all localization settings to defaults?\nThis will remove all custom locales and localization data.",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& locManager = Lupine::LocalizationManager::Instance();
        locManager.Clear();
        locManager.Initialize();
        loadSettings();
        m_settingsChanged = true;
    }
}

void LocalizationSettingsDialog::onOkClicked() {
    if (validateSettings()) {
        applySettings();
        accept();
    }
}

void LocalizationSettingsDialog::onCancelClicked() {
    reject();
}

void LocalizationSettingsDialog::onApplyClicked() {
    if (validateSettings()) {
        applySettings();
    }
}

void LocalizationSettingsDialog::applySettings() {
    auto& locManager = Lupine::LocalizationManager::Instance();

    // Update supported locales
    locManager.Clear();
    for (const auto& locale : m_supportedLocales) {
        locManager.AddSupportedLocale(locale);
    }

    // Update default locale
    QString defaultIdentifier = m_defaultLocaleCombo->currentData().toString();
    auto it = std::find_if(m_supportedLocales.begin(), m_supportedLocales.end(),
        [&defaultIdentifier](const Lupine::Locale& locale) {
            return locale.GetIdentifier() == defaultIdentifier.toStdString();
        });

    if (it != m_supportedLocales.end()) {
        locManager.SetDefaultLocale(*it);
        locManager.SetCurrentLocale(*it);
    }

    // Apply other settings (these would be saved to project settings in a real implementation)
    m_autoDetectLocale = m_autoDetectLocaleCheck->isChecked();
    m_fallbackToDefault = m_fallbackToDefaultCheck->isChecked();
    m_showMissingKeys = m_showMissingKeysCheck->isChecked();

    m_settingsChanged = false;
}

bool LocalizationSettingsDialog::validateSettings() {
    if (m_supportedLocales.empty()) {
        QMessageBox::warning(this, "Validation Error",
            "At least one locale must be supported.");
        return false;
    }

    return true;
}

void LocalizationSettingsDialog::showAddLocaleDialog() {
    LocaleEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Lupine::Locale newLocale = dialog.getLocale();

        // Check if locale already exists
        auto it = std::find_if(m_supportedLocales.begin(), m_supportedLocales.end(),
            [&newLocale](const Lupine::Locale& locale) {
                return locale.GetIdentifier() == newLocale.GetIdentifier();
            });

        if (it != m_supportedLocales.end()) {
            QMessageBox::warning(this, "Duplicate Locale",
                "A locale with this identifier already exists.");
            return;
        }

        m_supportedLocales.push_back(newLocale);
        refreshLocaleList();
        refreshDefaultLocaleCombo();
        m_settingsChanged = true;
    }
}

void LocalizationSettingsDialog::showEditLocaleDialog(const Lupine::Locale& locale) {
    LocaleEditDialog dialog(this, locale);
    if (dialog.exec() == QDialog::Accepted) {
        Lupine::Locale editedLocale = dialog.getLocale();

        // Find and update locale
        auto it = std::find_if(m_supportedLocales.begin(), m_supportedLocales.end(),
            [&locale](const Lupine::Locale& l) {
                return l.GetIdentifier() == locale.GetIdentifier();
            });

        if (it != m_supportedLocales.end()) {
            *it = editedLocale;
            refreshLocaleList();
            refreshDefaultLocaleCombo();
            m_settingsChanged = true;
        }
    }
}

void LocalizationSettingsDialog::closeEvent(QCloseEvent* event) {
    if (m_settingsChanged) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save them?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            applySettings();
            event->accept();
        } else if (ret == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

// LocaleEditDialog implementation
LocaleEditDialog::LocaleEditDialog(QWidget* parent, const Lupine::Locale& locale)
    : QDialog(parent)
    , m_locale(locale)
    , m_autoGenerateDisplayName(true) {

    setWindowTitle(locale.GetIdentifier().empty() ? "Add Locale" : "Edit Locale");
    setModal(true);
    resize(400, 200);

    setupUI();

    // Populate fields if editing existing locale
    if (!locale.GetIdentifier().empty()) {
        m_languageCodeEdit->setText(QString::fromStdString(locale.language_code));
        m_regionCodeEdit->setText(QString::fromStdString(locale.region_code));
        m_displayNameEdit->setText(QString::fromStdString(locale.display_name));
        m_autoGenerateDisplayName = locale.display_name.empty() ||
                                   locale.display_name == locale.GetIdentifier();
        m_autoGenerateDisplayNameCheck->setChecked(m_autoGenerateDisplayName);
    }
}

void LocaleEditDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    // Form layout
    QWidget* formWidget = new QWidget();
    m_formLayout = new QGridLayout(formWidget);

    // Language code
    m_languageCodeLabel = new QLabel("Language Code:");
    m_languageCodeEdit = new QLineEdit();
    m_languageCodeEdit->setPlaceholderText("e.g., en, es, fr");
    m_languageCodeEdit->setMaxLength(3);

    // Set validator for language code (2-3 lowercase letters)
    QRegularExpression langRegex("^[a-z]{2,3}$");
    m_languageCodeEdit->setValidator(new QRegularExpressionValidator(langRegex, this));

    m_formLayout->addWidget(m_languageCodeLabel, 0, 0);
    m_formLayout->addWidget(m_languageCodeEdit, 0, 1);

    // Region code
    m_regionCodeLabel = new QLabel("Region Code (Optional):");
    m_regionCodeEdit = new QLineEdit();
    m_regionCodeEdit->setPlaceholderText("e.g., US, ES, FR");
    m_regionCodeEdit->setMaxLength(3);

    // Set validator for region code (2-3 uppercase letters)
    QRegularExpression regionRegex("^[A-Z]{2,3}$");
    m_regionCodeEdit->setValidator(new QRegularExpressionValidator(regionRegex, this));

    m_formLayout->addWidget(m_regionCodeLabel, 1, 0);
    m_formLayout->addWidget(m_regionCodeEdit, 1, 1);

    // Display name
    m_displayNameLabel = new QLabel("Display Name:");
    m_displayNameEdit = new QLineEdit();
    m_displayNameEdit->setPlaceholderText("e.g., English (United States)");

    m_formLayout->addWidget(m_displayNameLabel, 2, 0);
    m_formLayout->addWidget(m_displayNameEdit, 2, 1);

    // Auto-generate display name checkbox
    m_autoGenerateDisplayNameCheck = new QCheckBox("Auto-generate display name");
    m_autoGenerateDisplayNameCheck->setChecked(true);
    m_formLayout->addWidget(m_autoGenerateDisplayNameCheck, 3, 0, 1, 2);

    m_mainLayout->addWidget(formWidget);

    // Dialog buttons
    m_buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");

    m_okButton->setDefault(true);

    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);

    m_mainLayout->addLayout(m_buttonLayout);

    // Connect signals
    connect(m_languageCodeEdit, &QLineEdit::textChanged, this, &LocaleEditDialog::onLanguageCodeChanged);
    connect(m_regionCodeEdit, &QLineEdit::textChanged, this, &LocaleEditDialog::onRegionCodeChanged);
    connect(m_displayNameEdit, &QLineEdit::textChanged, this, &LocaleEditDialog::onDisplayNameChanged);
    connect(m_autoGenerateDisplayNameCheck, &QCheckBox::toggled, [this](bool checked) {
        m_autoGenerateDisplayName = checked;
        m_displayNameEdit->setEnabled(!checked);
        if (checked) {
            updateDisplayName();
        }
    });
    connect(m_okButton, &QPushButton::clicked, this, &LocaleEditDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LocaleEditDialog::onCancelClicked);

    // Initial state
    m_displayNameEdit->setEnabled(!m_autoGenerateDisplayName);
    updateDisplayName();
}

void LocaleEditDialog::onLanguageCodeChanged() {
    if (m_autoGenerateDisplayName) {
        updateDisplayName();
    }
}

void LocaleEditDialog::onRegionCodeChanged() {
    if (m_autoGenerateDisplayName) {
        updateDisplayName();
    }
}

void LocaleEditDialog::onDisplayNameChanged() {
    // If user manually changes display name, disable auto-generation
    if (!m_autoGenerateDisplayName) {
        return;
    }

    // Check if the current text matches what would be auto-generated
    QString autoGenerated;
    QString langCode = m_languageCodeEdit->text().toLower();
    QString regionCode = m_regionCodeEdit->text().toUpper();

    if (!langCode.isEmpty()) {
        if (!regionCode.isEmpty()) {
            autoGenerated = QString("%1 (%2)").arg(langCode, regionCode);
        } else {
            autoGenerated = langCode;
        }
    }

    if (m_displayNameEdit->text() != autoGenerated) {
        m_autoGenerateDisplayName = false;
        m_autoGenerateDisplayNameCheck->setChecked(false);
        m_displayNameEdit->setEnabled(true);
    }
}

void LocaleEditDialog::updateDisplayName() {
    if (!m_autoGenerateDisplayName) {
        return;
    }

    QString langCode = m_languageCodeEdit->text().toLower();
    QString regionCode = m_regionCodeEdit->text().toUpper();

    QString displayName;
    if (!langCode.isEmpty()) {
        if (!regionCode.isEmpty()) {
            displayName = QString("%1 (%2)").arg(langCode, regionCode);
        } else {
            displayName = langCode;
        }
    }

    m_displayNameEdit->setText(displayName);
}

bool LocaleEditDialog::validateInput() {
    QString langCode = m_languageCodeEdit->text().toLower();

    if (langCode.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Language code is required.");
        m_languageCodeEdit->setFocus();
        return false;
    }

    if (langCode.length() < 2) {
        QMessageBox::warning(this, "Validation Error", "Language code must be at least 2 characters.");
        m_languageCodeEdit->setFocus();
        return false;
    }

    QString displayName = m_displayNameEdit->text();
    if (displayName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Display name is required.");
        m_displayNameEdit->setFocus();
        return false;
    }

    return true;
}

void LocaleEditDialog::onOkClicked() {
    if (validateInput()) {
        m_locale.language_code = m_languageCodeEdit->text().toLower().toStdString();
        m_locale.region_code = m_regionCodeEdit->text().toUpper().toStdString();
        m_locale.display_name = m_displayNameEdit->text().toStdString();
        accept();
    }
}

void LocaleEditDialog::onCancelClicked() {
    reject();
}

Lupine::Locale LocaleEditDialog::getLocale() const {
    return m_locale;
}

#include "moc_LocalizationSettingsDialog.cpp"
