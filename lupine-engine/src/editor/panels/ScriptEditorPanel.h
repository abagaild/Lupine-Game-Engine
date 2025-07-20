#pragma once

#ifndef SCRIPTEDITORPANEL_H
#define SCRIPTEDITORPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QListWidget>
#include <QTabWidget>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QFont>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QAction>

#include "../SyntaxHighlighters.h"

/**
 * @brief Script file item for the script tree
 */
class ScriptFileItem : public QTreeWidgetItem
{
public:
    ScriptFileItem(QTreeWidget* parent, const QString& name, const QString& filePath, const QString& language);
    ScriptFileItem(QTreeWidgetItem* parent, const QString& name, const QString& filePath, const QString& language);

    QString getFilePath() const { return m_filePath; }
    QString getLanguage() const { return m_language; }
    void setFilePath(const QString& filePath) { m_filePath = filePath; }
    void setLanguage(const QString& language) { m_language = language; }
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    void setUnsavedChanges(bool hasChanges);

private:
    QString m_filePath;
    QString m_language;
    bool m_hasUnsavedChanges;
};

/**
 * @brief Script editor tab with syntax highlighting
 */
class ScriptEditorTab : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptEditorTab(const QString& filePath, const QString& language, QWidget* parent = nullptr);
    ~ScriptEditorTab();

    QString getFilePath() const { return m_filePath; }
    QString getLanguage() const { return m_language; }
    QString getContent() const;
    void setContent(const QString& content);
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    void setLanguage(const QString& language);
    bool saveFile();
    bool saveFileAs();

signals:
    void contentChanged();
    void filePathChanged(const QString& oldPath, const QString& newPath);

private slots:
    void onTextChanged();

private:
    void setupUI();
    void updateHighlighter();
    void updateTabTitle();

    QVBoxLayout* m_layout;
    QPlainTextEdit* m_textEdit;

    QString m_filePath;
    QString m_language;
    bool m_hasUnsavedChanges;

    // Syntax highlighters
    PythonSyntaxHighlighter* m_pythonHighlighter;
    LuaSyntaxHighlighter* m_luaHighlighter;
    CppSyntaxHighlighter* m_cppHighlighter;
    MarkdownSyntaxHighlighter* m_markdownHighlighter;
};

/**
 * @brief Modern script writer panel with CRUD operations and syntax highlighting
 */
class ScriptEditorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptEditorPanel(QWidget *parent = nullptr);
    ~ScriptEditorPanel();

    // File operations
    void newScript();
    void openScript();
    void saveCurrentScript();
    void saveCurrentScriptAs();
    void closeCurrentScript();
    void closeAllScripts();

    // Project operations
    void setProjectPath(const QString& projectPath);
    void refreshScriptTree();

private slots:
    void onNewScript();
    void onOpenScript();
    void onSaveScript();
    void onSaveScriptAs();
    void onCloseScript();
    void onCloseAllScripts();
    void onLanguageChanged(const QString& language);
    void onScriptTreeItemClicked(QTreeWidgetItem* item, int column);
    void onScriptTreeContextMenu(const QPoint& pos);
    void onTabChanged(int index);
    bool onTabCloseRequested(int index);
    void onContentChanged();
    void onFilePathChanged(const QString& oldPath, const QString& newPath);

private:
    void setupUI();
    void setupToolbar();
    void setupScriptTree();
    void setupTabWidget();
    void updateToolbarState();
    void addScriptToTree(const QString& filePath);
    void removeScriptFromTree(const QString& filePath);
    ScriptEditorTab* getCurrentTab();
    ScriptEditorTab* findTabByPath(const QString& filePath);
    QString detectLanguageFromExtension(const QString& filePath);
    QString getLanguageIcon(const QString& language);
    bool promptSaveChanges(ScriptEditorTab* tab);
    void createNewScriptFile(const QString& language);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QSplitter* m_splitter;

    // Toolbar
    QToolButton* m_newButton;
    QToolButton* m_openButton;
    QToolButton* m_saveButton;
    QToolButton* m_saveAsButton;
    QToolButton* m_closeButton;
    QComboBox* m_languageCombo;
    QLabel* m_languageLabel;

    // Script tree
    QGroupBox* m_treeGroup;
    QTreeWidget* m_scriptTree;

    // Tab widget for open scripts
    QTabWidget* m_tabWidget;

    // State
    QString m_projectPath;
    QString m_scriptsPath;
};

#endif // SCRIPTEDITORPANEL_H
