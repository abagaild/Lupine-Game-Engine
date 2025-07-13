#include "NotepadDialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QStandardPaths>

// NoteTab Implementation
NoteTab::NoteTab(const QString& filePath, QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_textEdit(nullptr)
    , m_filePath(filePath)
    , m_currentLanguage("None")
    , m_hasUnsavedChanges(false)
    , m_pythonHighlighter(nullptr)
    , m_luaHighlighter(nullptr)
    , m_markdownHighlighter(nullptr)
{
    setupUI();
    
    if (!filePath.isEmpty()) {
        loadFile(filePath);
    } else {
        newFile();
    }
}

NoteTab::~NoteTab() {
    // Cleanup handled automatically by Qt
}

void NoteTab::setupUI() {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 10));
    m_layout->addWidget(m_textEdit);
    
    // Create syntax highlighters
    m_pythonHighlighter = new PythonSyntaxHighlighter(m_textEdit->document());
    m_luaHighlighter = new LuaSyntaxHighlighter(m_textEdit->document());
    m_cppHighlighter = new CppSyntaxHighlighter(m_textEdit->document());
    m_markdownHighlighter = new MarkdownSyntaxHighlighter(m_textEdit->document());

    // Initially disable all highlighters
    m_pythonHighlighter->setDocument(nullptr);
    m_luaHighlighter->setDocument(nullptr);
    m_cppHighlighter->setDocument(nullptr);
    m_markdownHighlighter->setDocument(nullptr);
    
    connect(m_textEdit, &QTextEdit::textChanged, this, &NoteTab::onTextChanged);
}

bool NoteTab::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    m_textEdit->setPlainText(content);
    
    m_filePath = filePath;
    m_hasUnsavedChanges = false;
    
    // Auto-detect language from file extension
    QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension == "py") {
        setSyntaxHighlighting("Python");
    } else if (extension == "lua") {
        setSyntaxHighlighting("Lua");
    } else if (extension == "md" || extension == "markdown") {
        setSyntaxHighlighting("Markdown");
    }
    
    emit filePathChanged(filePath);
    return true;
}

bool NoteTab::saveFile() {
    if (m_filePath.isEmpty()) {
        return saveFileAs();
    }
    
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    
    m_hasUnsavedChanges = false;
    return true;
}

bool NoteTab::saveFileAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Note As", "", 
        "Text Files (*.txt);;Python Files (*.py);;Lua Files (*.lua);;Markdown Files (*.md);;All Files (*.*)");
    
    if (filePath.isEmpty()) {
        return false;
    }
    
    m_filePath = filePath;
    emit filePathChanged(filePath);
    return saveFile();
}

void NoteTab::newFile() {
    m_textEdit->clear();
    m_filePath.clear();
    m_hasUnsavedChanges = false;
    setSyntaxHighlighting("None");
}

QString NoteTab::getContent() const {
    return m_textEdit->toPlainText();
}

void NoteTab::setContent(const QString& content) {
    m_textEdit->setPlainText(content);
    m_hasUnsavedChanges = false;
}

bool NoteTab::hasUnsavedChanges() const {
    return m_hasUnsavedChanges;
}

QString NoteTab::getFileName() const {
    if (m_filePath.isEmpty()) {
        return "Untitled";
    }
    return QFileInfo(m_filePath).fileName();
}

void NoteTab::setSyntaxHighlighting(const QString& language) {
    m_currentLanguage = language;
    updateHighlighter();
}

void NoteTab::onTextChanged() {
    m_hasUnsavedChanges = true;
    emit contentChanged();
}

void NoteTab::updateHighlighter() {
    // Disable all highlighters first
    m_pythonHighlighter->setDocument(nullptr);
    m_luaHighlighter->setDocument(nullptr);
    m_cppHighlighter->setDocument(nullptr);
    m_markdownHighlighter->setDocument(nullptr);

    // Enable the selected highlighter
    if (m_currentLanguage == "Python") {
        m_pythonHighlighter->setDocument(m_textEdit->document());
    } else if (m_currentLanguage == "Lua") {
        m_luaHighlighter->setDocument(m_textEdit->document());
    } else if (m_currentLanguage == "C++") {
        m_cppHighlighter->setDocument(m_textEdit->document());
    } else if (m_currentLanguage == "Markdown") {
        m_markdownHighlighter->setDocument(m_textEdit->document());
    }
}
// NotepadDialog Implementation
NotepadDialog::NotepadDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_tabWidget(nullptr)
    , m_statusLabel(nullptr)
    , m_syntaxHighlightingCheck(nullptr)
    , m_languageCombo(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeSpinBox(nullptr)
    , m_syntaxHighlightingEnabled(true)
    , m_defaultLanguage("None")
    , m_settings(nullptr)
{
    setWindowTitle("Lupine Notepad");
    setMinimumSize(800, 600);
    resize(1200, 800);

    m_settings = new QSettings("LupineEngine", "Notepad", this);

    setupUI();
    loadSettings();

    // Create initial tab
    newNote();
}

NotepadDialog::~NotepadDialog() {
    saveSettings();
}

void NotepadDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();

    // Main tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_mainLayout->addWidget(m_tabWidget);

    setupStatusBar();

    // Connect signals
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &NotepadDialog::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &NotepadDialog::onTabCloseRequested);
}

void NotepadDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New", this, &NotepadDialog::onNewNote, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &NotepadDialog::onOpenNote, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &NotepadDialog::onSaveNote, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &NotepadDialog::onSaveNoteAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_closeAction = fileMenu->addAction("&Close Tab", this, &NotepadDialog::onCloseNote, QKeySequence::Close);
    m_closeAllAction = fileMenu->addAction("Close &All", this, &NotepadDialog::onCloseAllNotes);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close, QKeySequence("Alt+F4"));

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_syntaxHighlightingCheck = new QCheckBox("Syntax Highlighting");
    m_syntaxHighlightingCheck->setChecked(m_syntaxHighlightingEnabled);
    QWidgetAction* syntaxAction = new QWidgetAction(this);
    syntaxAction->setDefaultWidget(m_syntaxHighlightingCheck);
    viewMenu->addAction(syntaxAction);

    connect(m_syntaxHighlightingCheck, &QCheckBox::toggled, this, &NotepadDialog::onSyntaxHighlightingToggled);
}

void NotepadDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(50);
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Language selection
    m_toolBar->addWidget(new QLabel("Language:"));
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({"None", "Python", "Lua", "C++", "Markdown"});
    m_languageCombo->setCurrentText(m_defaultLanguage);
    m_toolBar->addWidget(m_languageCombo);
    m_toolBar->addSeparator();

    // Font controls
    m_toolBar->addWidget(new QLabel("Font:"));
    m_fontCombo = new QFontComboBox();
    m_fontCombo->setCurrentFont(m_defaultFont);
    m_toolBar->addWidget(m_fontCombo);

    m_fontSizeSpinBox = new QSpinBox();
    m_fontSizeSpinBox->setRange(6, 72);
    m_fontSizeSpinBox->setValue(m_defaultFont.pointSize());
    m_toolBar->addWidget(m_fontSizeSpinBox);

    connect(m_languageCombo, &QComboBox::currentTextChanged, this, &NotepadDialog::onLanguageChanged);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &NotepadDialog::onFontChanged);
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &NotepadDialog::onFontSizeChanged);
}

void NotepadDialog::setupStatusBar() {
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Ready");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();

    QWidget* statusWidget = new QWidget();
    statusWidget->setLayout(statusLayout);
    statusWidget->setMaximumHeight(25);
    m_mainLayout->addWidget(statusWidget);
}

void NotepadDialog::newNote() {
    NoteTab* tab = new NoteTab();
    int index = m_tabWidget->addTab(tab, "Untitled");
    m_tabWidget->setCurrentIndex(index);

    connect(tab, &NoteTab::contentChanged, this, &NotepadDialog::onContentChanged);
    connect(tab, &NoteTab::filePathChanged, this, [this, tab](const QString&) {
        int index = m_tabWidget->indexOf(tab);
        if (index >= 0) {
            updateTabTitle(index);
        }
    });

    updateWindowTitle();
}

void NotepadDialog::openNote() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open Note", "",
        "Text Files (*.txt);;Python Files (*.py);;Lua Files (*.lua);;Markdown Files (*.md);;All Files (*.*)");

    if (!filePath.isEmpty()) {
        NoteTab* tab = new NoteTab(filePath);
        int index = m_tabWidget->addTab(tab, tab->getFileName());
        m_tabWidget->setCurrentIndex(index);

        connect(tab, &NoteTab::contentChanged, this, &NotepadDialog::onContentChanged);
        connect(tab, &NoteTab::filePathChanged, this, [this, tab](const QString&) {
            int index = m_tabWidget->indexOf(tab);
            if (index >= 0) {
                updateTabTitle(index);
            }
        });

        updateWindowTitle();
    }
}

void NotepadDialog::saveCurrentNote() {
    NoteTab* tab = getCurrentTab();
    if (tab) {
        if (tab->saveFile()) {
            updateTabTitle(m_tabWidget->currentIndex());
            m_statusLabel->setText("File saved successfully");
        } else {
            m_statusLabel->setText("Failed to save file");
        }
    }
}

void NotepadDialog::saveCurrentNoteAs() {
    NoteTab* tab = getCurrentTab();
    if (tab) {
        if (tab->saveFileAs()) {
            updateTabTitle(m_tabWidget->currentIndex());
            m_statusLabel->setText("File saved successfully");
        } else {
            m_statusLabel->setText("Failed to save file");
        }
    }
}

void NotepadDialog::closeCurrentNote() {
    int index = m_tabWidget->currentIndex();
    if (index >= 0) {
        onTabCloseRequested(index);
    }
}

void NotepadDialog::closeAllNotes() {
    while (m_tabWidget->count() > 0) {
        if (!onTabCloseRequested(0)) {
            break; // User cancelled
        }
    }
}

void NotepadDialog::switchToTab(int index) {
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

NoteTab* NotepadDialog::getCurrentTab() {
    return qobject_cast<NoteTab*>(m_tabWidget->currentWidget());
}

int NotepadDialog::getTabCount() const {
    return m_tabWidget->count();
}

void NotepadDialog::closeEvent(QCloseEvent* event) {
    // Check for unsaved changes in all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(i));
        if (tab && tab->hasUnsavedChanges()) {
            if (!promptSaveChanges(tab)) {
                event->ignore();
                return;
            }
        }
    }

    saveSettings();
    event->accept();
}

// Slot implementations
void NotepadDialog::onNewNote() {
    newNote();
}

void NotepadDialog::onOpenNote() {
    openNote();
}

void NotepadDialog::onSaveNote() {
    saveCurrentNote();
}

void NotepadDialog::onSaveNoteAs() {
    saveCurrentNoteAs();
}

void NotepadDialog::onCloseNote() {
    closeCurrentNote();
}

void NotepadDialog::onCloseAllNotes() {
    closeAllNotes();
}

void NotepadDialog::onTabChanged(int index) {
    updateWindowTitle();

    NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(index));
    if (tab) {
        // Update language combo to match current tab
        m_languageCombo->setCurrentText(tab->getCurrentLanguage());
    }
}

bool NotepadDialog::onTabCloseRequested(int index) {
    NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(index));
    if (tab && tab->hasUnsavedChanges()) {
        if (!promptSaveChanges(tab)) {
            return false; // User cancelled
        }
    }

    m_tabWidget->removeTab(index);

    // Create new tab if all tabs are closed
    if (m_tabWidget->count() == 0) {
        newNote();
    }

    return true;
}

void NotepadDialog::onContentChanged() {
    int index = m_tabWidget->currentIndex();
    if (index >= 0) {
        updateTabTitle(index);
    }
}

void NotepadDialog::onSyntaxHighlightingToggled(bool enabled) {
    m_syntaxHighlightingEnabled = enabled;

    // Apply to all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(i));
        if (tab) {
            if (enabled) {
                tab->setSyntaxHighlighting(tab->getCurrentLanguage());
            } else {
                tab->setSyntaxHighlighting("None");
            }
        }
    }
}

void NotepadDialog::onLanguageChanged(const QString& language) {
    NoteTab* tab = getCurrentTab();
    if (tab && m_syntaxHighlightingEnabled) {
        tab->setSyntaxHighlighting(language);
    }
}

void NotepadDialog::onFontChanged(const QFont& font) {
    m_defaultFont = font;
    m_defaultFont.setPointSize(m_fontSizeSpinBox->value());

    // Apply to all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(i));
        if (tab) {
            tab->findChild<QTextEdit*>()->setFont(m_defaultFont);
        }
    }
}

void NotepadDialog::onFontSizeChanged(int size) {
    m_defaultFont.setPointSize(size);

    // Apply to all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(i));
        if (tab) {
            tab->findChild<QTextEdit*>()->setFont(m_defaultFont);
        }
    }
}

void NotepadDialog::updateWindowTitle() {
    NoteTab* tab = getCurrentTab();
    if (tab) {
        QString title = "Lupine Notepad - " + tab->getFileName();
        if (tab->hasUnsavedChanges()) {
            title += "*";
        }
        setWindowTitle(title);
    } else {
        setWindowTitle("Lupine Notepad");
    }
}

void NotepadDialog::updateTabTitle(int index) {
    NoteTab* tab = qobject_cast<NoteTab*>(m_tabWidget->widget(index));
    if (tab) {
        QString title = tab->getFileName();
        if (tab->hasUnsavedChanges()) {
            title += "*";
        }
        m_tabWidget->setTabText(index, title);
    }

    updateWindowTitle();
}

bool NotepadDialog::promptSaveChanges(NoteTab* tab) {
    QMessageBox::StandardButton result = QMessageBox::question(this,
        "Unsaved Changes",
        QString("The file '%1' has unsaved changes. Do you want to save them?").arg(tab->getFileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (result == QMessageBox::Save) {
        return tab->saveFile();
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void NotepadDialog::loadSettings() {
    // Load window geometry
    restoreGeometry(m_settings->value("geometry").toByteArray());

    // Load font settings
    m_defaultFont = m_settings->value("font", QFont("Consolas", 10)).value<QFont>();

    // Load syntax highlighting setting
    m_syntaxHighlightingEnabled = m_settings->value("syntaxHighlighting", true).toBool();
    m_syntaxHighlightingCheck->setChecked(m_syntaxHighlightingEnabled);

    // Load default language
    m_defaultLanguage = m_settings->value("defaultLanguage", "None").toString();
    m_languageCombo->setCurrentText(m_defaultLanguage);

    // Load recent files
    m_recentFiles = m_settings->value("recentFiles").toStringList();

    // Update UI
    m_fontCombo->setCurrentFont(m_defaultFont);
    m_fontSizeSpinBox->setValue(m_defaultFont.pointSize());
}

void NotepadDialog::saveSettings() {
    // Save window geometry
    m_settings->setValue("geometry", saveGeometry());

    // Save font settings
    m_settings->setValue("font", m_defaultFont);

    // Save syntax highlighting setting
    m_settings->setValue("syntaxHighlighting", m_syntaxHighlightingEnabled);

    // Save default language
    m_settings->setValue("defaultLanguage", m_languageCombo->currentText());

    // Save recent files
    m_settings->setValue("recentFiles", m_recentFiles);
}

#include "NotepadDialog.moc"
