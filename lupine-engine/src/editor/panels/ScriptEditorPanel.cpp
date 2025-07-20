#include "ScriptEditorPanel.h"
#include <QApplication>
#include <QStyle>
#include <QHeaderView>
#include <QMimeData>
#include <QUrl>

// ScriptFileItem Implementation
ScriptFileItem::ScriptFileItem(QTreeWidget* parent, const QString& name, const QString& filePath, const QString& language)
    : QTreeWidgetItem(parent), m_filePath(filePath), m_language(language), m_hasUnsavedChanges(false)
{
    setText(0, name);
    setToolTip(0, filePath);

    // Set icon based on language
    if (language == "Python") {
        setIcon(0, QIcon("icons/python.png"));
    } else if (language == "Lua") {
        setIcon(0, QIcon("icons/lua.png"));
    } else if (language == "C++") {
        setIcon(0, QIcon("icons/cpp.png"));
    } else if (language == "Markdown") {
        setIcon(0, QIcon("icons/markdown.png"));
    } else {
        setIcon(0, QIcon("icons/script.png"));
    }
}

ScriptFileItem::ScriptFileItem(QTreeWidgetItem* parent, const QString& name, const QString& filePath, const QString& language)
    : QTreeWidgetItem(parent), m_filePath(filePath), m_language(language), m_hasUnsavedChanges(false)
{
    setText(0, name);
    setToolTip(0, filePath);

    // Set icon based on language
    if (language == "Python") {
        setIcon(0, QIcon("icons/python.png"));
    } else if (language == "Lua") {
        setIcon(0, QIcon("icons/lua.png"));
    } else if (language == "C++") {
        setIcon(0, QIcon("icons/cpp.png"));
    } else if (language == "Markdown") {
        setIcon(0, QIcon("icons/markdown.png"));
    } else {
        setIcon(0, QIcon("icons/script.png"));
    }
}

void ScriptFileItem::setUnsavedChanges(bool hasChanges)
{
    m_hasUnsavedChanges = hasChanges;
    QString name = text(0);
    if (hasChanges && !name.endsWith("*")) {
        setText(0, name + "*");
    } else if (!hasChanges && name.endsWith("*")) {
        setText(0, name.left(name.length() - 1));
    }
}

// ScriptEditorTab Implementation
ScriptEditorTab::ScriptEditorTab(const QString& filePath, const QString& language, QWidget* parent)
    : QWidget(parent), m_filePath(filePath), m_language(language), m_hasUnsavedChanges(false)
    , m_pythonHighlighter(nullptr), m_luaHighlighter(nullptr)
    , m_cppHighlighter(nullptr), m_markdownHighlighter(nullptr)
{
    setupUI();

    // Load file content if it exists
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            setContent(in.readAll());
            m_hasUnsavedChanges = false;
        }
    }
}

ScriptEditorTab::~ScriptEditorTab()
{
    // Cleanup syntax highlighters
    delete m_pythonHighlighter;
    delete m_luaHighlighter;
    delete m_cppHighlighter;
    delete m_markdownHighlighter;
}

void ScriptEditorTab::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // Create text editor with modern styling
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 11));
    m_textEdit->setTabStopDistance(40); // 4 spaces
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Modern dark theme styling
    m_textEdit->setStyleSheet(
        "QPlainTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    border: 1px solid #3c3c3c;"
        "    selection-background-color: #264f78;"
        "    selection-color: #ffffff;"
        "}"
        "QPlainTextEdit:focus {"
        "    border: 1px solid #007acc;"
        "}"
    );

    m_layout->addWidget(m_textEdit);

    // Create syntax highlighters
    m_pythonHighlighter = new PythonSyntaxHighlighter();
    m_luaHighlighter = new LuaSyntaxHighlighter();
    m_cppHighlighter = new CppSyntaxHighlighter();
    m_markdownHighlighter = new MarkdownSyntaxHighlighter();

    // Set initial language
    setLanguage(m_language);

    connect(m_textEdit, &QPlainTextEdit::textChanged, this, &ScriptEditorTab::onTextChanged);
}

QString ScriptEditorTab::getContent() const
{
    return m_textEdit->toPlainText();
}

void ScriptEditorTab::setContent(const QString& content)
{
    m_textEdit->setPlainText(content);
    m_hasUnsavedChanges = false;
}

void ScriptEditorTab::setLanguage(const QString& language)
{
    m_language = language;
    updateHighlighter();
}

void ScriptEditorTab::updateHighlighter()
{
    // Disable all highlighters first
    m_pythonHighlighter->setDocument(nullptr);
    m_luaHighlighter->setDocument(nullptr);
    m_cppHighlighter->setDocument(nullptr);
    m_markdownHighlighter->setDocument(nullptr);

    // Enable the selected highlighter
    if (m_language == "Python") {
        m_pythonHighlighter->setDocument(m_textEdit->document());
    } else if (m_language == "Lua") {
        m_luaHighlighter->setDocument(m_textEdit->document());
    } else if (m_language == "C++") {
        m_cppHighlighter->setDocument(m_textEdit->document());
    } else if (m_language == "Markdown") {
        m_markdownHighlighter->setDocument(m_textEdit->document());
    }
}

bool ScriptEditorTab::saveFile()
{
    if (m_filePath.isEmpty()) {
        return saveFileAs();
    }

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << getContent();
        m_hasUnsavedChanges = false;
        return true;
    }

    QMessageBox::warning(this, "Save Error",
                        QString("Could not save file: %1").arg(m_filePath));
    return false;
}

bool ScriptEditorTab::saveFileAs()
{
    QString filter;
    if (m_language == "Python") {
        filter = "Python Files (*.py);;All Files (*.*)";
    } else if (m_language == "Lua") {
        filter = "Lua Files (*.lua);;All Files (*.*)";
    } else if (m_language == "C++") {
        filter = "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*.*)";
    } else if (m_language == "Markdown") {
        filter = "Markdown Files (*.md *.markdown);;All Files (*.*)";
    } else {
        filter = "All Files (*.*)";
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Script", m_filePath, filter);
    if (!fileName.isEmpty()) {
        QString oldPath = m_filePath;
        m_filePath = fileName;

        if (saveFile()) {
            emit filePathChanged(oldPath, m_filePath);
            return true;
        }
    }
    return false;
}

void ScriptEditorTab::onTextChanged()
{
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit contentChanged();
    }
}

// ScriptEditorPanel Implementation
ScriptEditorPanel::ScriptEditorPanel(QWidget *parent) : QWidget(parent)
{
    setupUI();
    updateToolbarState();
}

ScriptEditorPanel::~ScriptEditorPanel()
{
    // Cleanup is handled by Qt's parent-child relationship
}

void ScriptEditorPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(4);

    setupToolbar();

    // Create splitter for tree and tabs
    m_splitter = new QSplitter(Qt::Horizontal, this);

    setupScriptTree();
    setupTabWidget();

    // Add widgets to splitter
    m_splitter->addWidget(m_treeGroup);
    m_splitter->addWidget(m_tabWidget);
    m_splitter->setSizes({250, 750});

    m_mainLayout->addWidget(m_splitter);
}

void ScriptEditorPanel::setupToolbar()
{
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(4);

    // Create toolbar buttons with modern styling
    m_newButton = new QToolButton();
    m_newButton->setIcon(QIcon("icons/new.png"));
    m_newButton->setToolTip("New Script (Ctrl+N)");
    m_newButton->setAutoRaise(true);

    m_openButton = new QToolButton();
    m_openButton->setIcon(QIcon("icons/open.png"));
    m_openButton->setToolTip("Open Script (Ctrl+O)");
    m_openButton->setAutoRaise(true);

    m_saveButton = new QToolButton();
    m_saveButton->setIcon(QIcon("icons/save.png"));
    m_saveButton->setToolTip("Save Script (Ctrl+S)");
    m_saveButton->setAutoRaise(true);

    m_saveAsButton = new QToolButton();
    m_saveAsButton->setIcon(QIcon("icons/save_as.png"));
    m_saveAsButton->setToolTip("Save Script As (Ctrl+Shift+S)");
    m_saveAsButton->setAutoRaise(true);

    m_closeButton = new QToolButton();
    m_closeButton->setIcon(QIcon("icons/close.png"));
    m_closeButton->setToolTip("Close Script (Ctrl+W)");
    m_closeButton->setAutoRaise(true);

    // Language selection
    m_languageLabel = new QLabel("Language:");
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({"Python", "Lua", "C++", "Markdown"});
    m_languageCombo->setMinimumWidth(100);

    // Add widgets to toolbar
    m_toolbarLayout->addWidget(m_newButton);
    m_toolbarLayout->addWidget(m_openButton);
    m_toolbarLayout->addWidget(m_saveButton);
    m_toolbarLayout->addWidget(m_saveAsButton);
    m_toolbarLayout->addWidget(m_closeButton);
    m_toolbarLayout->addSpacing(20);
    m_toolbarLayout->addWidget(m_languageLabel);
    m_toolbarLayout->addWidget(m_languageCombo);
    m_toolbarLayout->addStretch();

    m_mainLayout->addLayout(m_toolbarLayout);

    // Connect signals
    connect(m_newButton, &QToolButton::clicked, this, &ScriptEditorPanel::onNewScript);
    connect(m_openButton, &QToolButton::clicked, this, &ScriptEditorPanel::onOpenScript);
    connect(m_saveButton, &QToolButton::clicked, this, &ScriptEditorPanel::onSaveScript);
    connect(m_saveAsButton, &QToolButton::clicked, this, &ScriptEditorPanel::onSaveScriptAs);
    connect(m_closeButton, &QToolButton::clicked, this, &ScriptEditorPanel::onCloseScript);
    connect(m_languageCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &ScriptEditorPanel::onLanguageChanged);
}

void ScriptEditorPanel::setupScriptTree()
{
    m_treeGroup = new QGroupBox("Project Scripts");
    QVBoxLayout* treeLayout = new QVBoxLayout(m_treeGroup);

    m_scriptTree = new QTreeWidget();
    m_scriptTree->setHeaderLabel("Scripts");
    m_scriptTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_scriptTree->setDragDropMode(QAbstractItemView::NoDragDrop);

    // Modern styling
    m_scriptTree->setStyleSheet(
        "QTreeWidget {"
        "    background-color: #2d2d30;"
        "    color: #cccccc;"
        "    border: 1px solid #3c3c3c;"
        "    selection-background-color: #094771;"
        "}"
        "QTreeWidget::item {"
        "    padding: 4px;"
        "    border: none;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #3e3e40;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #094771;"
        "}"
    );

    treeLayout->addWidget(m_scriptTree);

    connect(m_scriptTree, &QTreeWidget::itemClicked,
            this, &ScriptEditorPanel::onScriptTreeItemClicked);
    connect(m_scriptTree, &QTreeWidget::customContextMenuRequested,
            this, &ScriptEditorPanel::onScriptTreeContextMenu);
}

void ScriptEditorPanel::setupTabWidget()
{
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    // Modern tab styling
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #3c3c3c;"
        "    background-color: #1e1e1e;"
        "}"
        "QTabBar::tab {"
        "    background-color: #2d2d30;"
        "    color: #cccccc;"
        "    padding: 8px 16px;"
        "    margin-right: 2px;"
        "    border: 1px solid #3c3c3c;"
        "    border-bottom: none;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #1e1e1e;"
        "    color: #ffffff;"
        "}"
        "QTabBar::tab:hover {"
        "    background-color: #3e3e40;"
        "}"
    );

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &ScriptEditorPanel::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &ScriptEditorPanel::onTabCloseRequested);
}

// Public methods
void ScriptEditorPanel::setProjectPath(const QString& projectPath)
{
    m_projectPath = projectPath;
    m_scriptsPath = QDir(projectPath).absoluteFilePath("scripts");

    // Create scripts directory if it doesn't exist
    QDir scriptsDir(m_scriptsPath);
    if (!scriptsDir.exists()) {
        scriptsDir.mkpath(".");
    }

    refreshScriptTree();
}

void ScriptEditorPanel::refreshScriptTree()
{
    m_scriptTree->clear();

    if (m_scriptsPath.isEmpty() || !QDir(m_scriptsPath).exists()) {
        return;
    }

    QDir scriptsDir(m_scriptsPath);
    QStringList filters;
    filters << "*.py" << "*.lua" << "*.cpp" << "*.h" << "*.hpp" << "*.md";

    QFileInfoList files = scriptsDir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo& fileInfo : files) {
        QString language = detectLanguageFromExtension(fileInfo.absoluteFilePath());
        new ScriptFileItem(m_scriptTree, fileInfo.baseName(),
                          fileInfo.absoluteFilePath(), language);
    }

    m_scriptTree->expandAll();
}

void ScriptEditorPanel::newScript()
{
    onNewScript();
}

void ScriptEditorPanel::openScript()
{
    onOpenScript();
}

void ScriptEditorPanel::saveCurrentScript()
{
    onSaveScript();
}

void ScriptEditorPanel::saveCurrentScriptAs()
{
    onSaveScriptAs();
}

void ScriptEditorPanel::closeCurrentScript()
{
    onCloseScript();
}

void ScriptEditorPanel::closeAllScripts()
{
    onCloseAllScripts();
}

// Private slots
void ScriptEditorPanel::onNewScript()
{
    QString language = m_languageCombo->currentText();
    createNewScriptFile(language);
}

void ScriptEditorPanel::onOpenScript()
{
    QString filter = "Script Files (*.py *.lua *.cpp *.h *.hpp *.md);;All Files (*.*)";
    QString fileName = QFileDialog::getOpenFileName(this, "Open Script", m_scriptsPath, filter);

    if (!fileName.isEmpty()) {
        // Check if file is already open
        ScriptEditorTab* existingTab = findTabByPath(fileName);
        if (existingTab) {
            // Switch to existing tab
            for (int i = 0; i < m_tabWidget->count(); ++i) {
                if (m_tabWidget->widget(i) == existingTab) {
                    m_tabWidget->setCurrentIndex(i);
                    return;
                }
            }
        }

        // Open new tab
        QString language = detectLanguageFromExtension(fileName);
        ScriptEditorTab* tab = new ScriptEditorTab(fileName, language);

        connect(tab, &ScriptEditorTab::contentChanged, this, &ScriptEditorPanel::onContentChanged);
        connect(tab, &ScriptEditorTab::filePathChanged, this, &ScriptEditorPanel::onFilePathChanged);

        QFileInfo fileInfo(fileName);
        int index = m_tabWidget->addTab(tab, fileInfo.baseName());
        m_tabWidget->setCurrentIndex(index);

        addScriptToTree(fileName);
        updateToolbarState();
    }
}

void ScriptEditorPanel::onSaveScript()
{
    ScriptEditorTab* tab = getCurrentTab();
    if (tab) {
        if (tab->saveFile()) {
            // Update tree item
            QString filePath = tab->getFilePath();
            for (int i = 0; i < m_scriptTree->topLevelItemCount(); ++i) {
                ScriptFileItem* item = static_cast<ScriptFileItem*>(m_scriptTree->topLevelItem(i));
                if (item && item->getFilePath() == filePath) {
                    item->setUnsavedChanges(false);
                    break;
                }
            }

            // Update tab title
            int currentIndex = m_tabWidget->currentIndex();
            if (currentIndex >= 0) {
                QFileInfo fileInfo(filePath);
                m_tabWidget->setTabText(currentIndex, fileInfo.baseName());
            }
        }
    }
}

void ScriptEditorPanel::onSaveScriptAs()
{
    ScriptEditorTab* tab = getCurrentTab();
    if (tab && tab->saveFileAs()) {
        // Update tree and tab
        refreshScriptTree();

        int currentIndex = m_tabWidget->currentIndex();
        if (currentIndex >= 0) {
            QFileInfo fileInfo(tab->getFilePath());
            m_tabWidget->setTabText(currentIndex, fileInfo.baseName());
        }
    }
}

void ScriptEditorPanel::onCloseScript()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0) {
        onTabCloseRequested(currentIndex);
    }
}

void ScriptEditorPanel::onCloseAllScripts()
{
    while (m_tabWidget->count() > 0) {
        if (!onTabCloseRequested(0)) {
            break; // User cancelled
        }
    }
}

void ScriptEditorPanel::onLanguageChanged(const QString& language)
{
    ScriptEditorTab* tab = getCurrentTab();
    if (tab) {
        tab->setLanguage(language);
    }
}

void ScriptEditorPanel::onScriptTreeItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)

    ScriptFileItem* scriptItem = static_cast<ScriptFileItem*>(item);
    if (!scriptItem) return;

    QString filePath = scriptItem->getFilePath();

    // Check if file is already open
    ScriptEditorTab* existingTab = findTabByPath(filePath);
    if (existingTab) {
        // Switch to existing tab
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->widget(i) == existingTab) {
                m_tabWidget->setCurrentIndex(i);
                return;
            }
        }
    }

    // Open new tab
    ScriptEditorTab* tab = new ScriptEditorTab(filePath, scriptItem->getLanguage());

    connect(tab, &ScriptEditorTab::contentChanged, this, &ScriptEditorPanel::onContentChanged);
    connect(tab, &ScriptEditorTab::filePathChanged, this, &ScriptEditorPanel::onFilePathChanged);

    QFileInfo fileInfo(filePath);
    int index = m_tabWidget->addTab(tab, fileInfo.baseName());
    m_tabWidget->setCurrentIndex(index);

    updateToolbarState();
}

void ScriptEditorPanel::onScriptTreeContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_scriptTree->itemAt(pos);

    QMenu contextMenu(this);

    if (item) {
        // Item-specific actions
        contextMenu.addAction("Open", [this, item]() {
            onScriptTreeItemClicked(item, 0);
        });
        contextMenu.addSeparator();
        contextMenu.addAction("Delete", [this, item]() {
            ScriptFileItem* scriptItem = static_cast<ScriptFileItem*>(item);
            if (scriptItem) {
                int ret = QMessageBox::question(this, "Delete Script",
                    QString("Are you sure you want to delete '%1'?").arg(scriptItem->text(0)),
                    QMessageBox::Yes | QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    QFile::remove(scriptItem->getFilePath());
                    removeScriptFromTree(scriptItem->getFilePath());
                    refreshScriptTree();
                }
            }
        });
    } else {
        // General actions
        contextMenu.addAction("New Python Script", [this]() {
            createNewScriptFile("Python");
        });
        contextMenu.addAction("New Lua Script", [this]() {
            createNewScriptFile("Lua");
        });
        contextMenu.addAction("New C++ Script", [this]() {
            createNewScriptFile("C++");
        });
        contextMenu.addAction("New Markdown File", [this]() {
            createNewScriptFile("Markdown");
        });
        contextMenu.addSeparator();
        contextMenu.addAction("Refresh", [this]() {
            refreshScriptTree();
        });
    }

    contextMenu.exec(m_scriptTree->mapToGlobal(pos));
}

void ScriptEditorPanel::onTabChanged(int index)
{
    updateToolbarState();

    if (index >= 0) {
        ScriptEditorTab* tab = static_cast<ScriptEditorTab*>(m_tabWidget->widget(index));
        if (tab) {
            // Update language combo
            m_languageCombo->setCurrentText(tab->getLanguage());
        }
    }
}

bool ScriptEditorPanel::onTabCloseRequested(int index)
{
    ScriptEditorTab* tab = static_cast<ScriptEditorTab*>(m_tabWidget->widget(index));
    if (!tab) return true;

    if (tab->hasUnsavedChanges()) {
        if (!promptSaveChanges(tab)) {
            return false; // User cancelled
        }
    }

    m_tabWidget->removeTab(index);
    tab->deleteLater();

    updateToolbarState();
    return true;
}

void ScriptEditorPanel::onContentChanged()
{
    ScriptEditorTab* tab = qobject_cast<ScriptEditorTab*>(sender());
    if (!tab) return;

    // Update tab title
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == tab) {
            QFileInfo fileInfo(tab->getFilePath());
            QString title = fileInfo.baseName();
            if (tab->hasUnsavedChanges()) {
                title += "*";
            }
            m_tabWidget->setTabText(i, title);
            break;
        }
    }

    // Update tree item
    QString filePath = tab->getFilePath();
    for (int i = 0; i < m_scriptTree->topLevelItemCount(); ++i) {
        ScriptFileItem* item = static_cast<ScriptFileItem*>(m_scriptTree->topLevelItem(i));
        if (item && item->getFilePath() == filePath) {
            item->setUnsavedChanges(tab->hasUnsavedChanges());
            break;
        }
    }
}

void ScriptEditorPanel::onFilePathChanged(const QString& oldPath, const QString& newPath)
{
    // Update tree
    removeScriptFromTree(oldPath);
    addScriptToTree(newPath);
    refreshScriptTree();
}

// Private utility methods
void ScriptEditorPanel::updateToolbarState()
{
    bool hasCurrentTab = (m_tabWidget->currentIndex() >= 0);

    m_saveButton->setEnabled(hasCurrentTab);
    m_saveAsButton->setEnabled(hasCurrentTab);
    m_closeButton->setEnabled(hasCurrentTab);
    m_languageCombo->setEnabled(hasCurrentTab);
}

void ScriptEditorPanel::addScriptToTree(const QString& filePath)
{
    // Check if already exists
    for (int i = 0; i < m_scriptTree->topLevelItemCount(); ++i) {
        ScriptFileItem* item = static_cast<ScriptFileItem*>(m_scriptTree->topLevelItem(i));
        if (item && item->getFilePath() == filePath) {
            return; // Already exists
        }
    }

    QFileInfo fileInfo(filePath);
    QString language = detectLanguageFromExtension(filePath);
    new ScriptFileItem(m_scriptTree, fileInfo.baseName(), filePath, language);
}

void ScriptEditorPanel::removeScriptFromTree(const QString& filePath)
{
    for (int i = 0; i < m_scriptTree->topLevelItemCount(); ++i) {
        ScriptFileItem* item = static_cast<ScriptFileItem*>(m_scriptTree->topLevelItem(i));
        if (item && item->getFilePath() == filePath) {
            delete item;
            break;
        }
    }
}

ScriptEditorTab* ScriptEditorPanel::getCurrentTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0) {
        return static_cast<ScriptEditorTab*>(m_tabWidget->widget(currentIndex));
    }
    return nullptr;
}

ScriptEditorTab* ScriptEditorPanel::findTabByPath(const QString& filePath)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        ScriptEditorTab* tab = static_cast<ScriptEditorTab*>(m_tabWidget->widget(i));
        if (tab && tab->getFilePath() == filePath) {
            return tab;
        }
    }
    return nullptr;
}

QString ScriptEditorPanel::detectLanguageFromExtension(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    if (extension == "py") {
        return "Python";
    } else if (extension == "lua") {
        return "Lua";
    } else if (extension == "cpp" || extension == "cc" || extension == "cxx" ||
               extension == "h" || extension == "hpp") {
        return "C++";
    } else if (extension == "md" || extension == "markdown") {
        return "Markdown";
    }

    return "Python"; // Default
}

QString ScriptEditorPanel::getLanguageIcon(const QString& language)
{
    if (language == "Python") {
        return "icons/python.png";
    } else if (language == "Lua") {
        return "icons/lua.png";
    } else if (language == "C++") {
        return "icons/cpp.png";
    } else if (language == "Markdown") {
        return "icons/markdown.png";
    }
    return "icons/script.png";
}

bool ScriptEditorPanel::promptSaveChanges(ScriptEditorTab* tab)
{
    if (!tab || !tab->hasUnsavedChanges()) {
        return true;
    }

    QFileInfo fileInfo(tab->getFilePath());
    QString fileName = fileInfo.baseName();
    if (fileName.isEmpty()) {
        fileName = "Untitled";
    }

    int ret = QMessageBox::question(this, "Unsaved Changes",
        QString("'%1' has unsaved changes. Do you want to save them?").arg(fileName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        return tab->saveFile();
    } else if (ret == QMessageBox::Discard) {
        return true;
    }

    return false; // Cancel
}

void ScriptEditorPanel::createNewScriptFile(const QString& language)
{
    QString extension;
    QString templateContent;

    if (language == "Python") {
        extension = ".py";
        templateContent = "#!/usr/bin/env python3\n"
                         "# -*- coding: utf-8 -*-\n"
                         "\"\"\"\n"
                         "New Python Script\n"
                         "Generated by Lupine Engine\n"
                         "\"\"\"\n\n"
                         "class Script:\n"
                         "    def __init__(self):\n"
                         "        pass\n\n"
                         "    def ready(self):\n"
                         "        \"\"\"Called when the script is ready\"\"\"\n"
                         "        pass\n\n"
                         "    def update(self, delta_time):\n"
                         "        \"\"\"Called every frame\"\"\"\n"
                         "        pass\n";
    } else if (language == "Lua") {
        extension = ".lua";
        templateContent = "-- New Lua Script\n"
                         "-- Generated by Lupine Engine\n\n"
                         "local Script = {}\n\n"
                         "function Script:ready()\n"
                         "    -- Called when the script is ready\n"
                         "end\n\n"
                         "function Script:update(delta_time)\n"
                         "    -- Called every frame\n"
                         "end\n\n"
                         "return Script\n";
    } else if (language == "C++") {
        extension = ".cpp";
        templateContent = "#include <iostream>\n"
                         "#include \"lupine/core/Component.h\"\n\n"
                         "/**\n"
                         " * New C++ Script\n"
                         " * Generated by Lupine Engine\n"
                         " */\n"
                         "class Script : public Lupine::Component {\n"
                         "public:\n"
                         "    Script() : Component(\"Script\") {}\n\n"
                         "    void Ready() override {\n"
                         "        // Called when the script is ready\n"
                         "    }\n\n"
                         "    void Update(float delta_time) override {\n"
                         "        // Called every frame\n"
                         "    }\n"
                         "};\n";
    } else if (language == "Markdown") {
        extension = ".md";
        templateContent = "# New Markdown Document\n\n"
                         "Generated by Lupine Engine\n\n"
                         "## Overview\n\n"
                         "Write your documentation here...\n\n"
                         "## Features\n\n"
                         "- Feature 1\n"
                         "- Feature 2\n"
                         "- Feature 3\n\n"
                         "## Code Example\n\n"
                         "```python\n"
                         "# Example code\n"
                         "print(\"Hello, World!\")\n"
                         "```\n";
    }

    // Create new tab with template content
    ScriptEditorTab* tab = new ScriptEditorTab("", language);
    tab->setContent(templateContent);

    connect(tab, &ScriptEditorTab::contentChanged, this, &ScriptEditorPanel::onContentChanged);
    connect(tab, &ScriptEditorTab::filePathChanged, this, &ScriptEditorPanel::onFilePathChanged);

    int index = m_tabWidget->addTab(tab, "Untitled" + extension);
    m_tabWidget->setCurrentIndex(index);

    updateToolbarState();
}







