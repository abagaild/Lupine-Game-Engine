#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QTextEdit>
#include <QMenuBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QFont>
#include <QFontComboBox>
#include <QSpinBox>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QSettings>
#include "../SyntaxHighlighters.h"

/**
 * @brief Tab widget for individual note documents
 */
class NoteTab : public QWidget
{
    Q_OBJECT

public:
    explicit NoteTab(const QString& filePath = QString(), QWidget* parent = nullptr);
    ~NoteTab();

    // File operations
    bool loadFile(const QString& filePath);
    bool saveFile();
    bool saveFileAs();
    void newFile();

    // Content management
    QString getContent() const;
    void setContent(const QString& content);
    bool hasUnsavedChanges() const;
    QString getFilePath() const { return m_filePath; }
    QString getFileName() const;

    // Syntax highlighting
    void setSyntaxHighlighting(const QString& language);
    QString getCurrentLanguage() const { return m_currentLanguage; }

signals:
    void contentChanged();
    void filePathChanged(const QString& filePath);

private slots:
    void onTextChanged();

private:
    void setupUI();
    void updateHighlighter();

    QVBoxLayout* m_layout;
    QTextEdit* m_textEdit;
    
    QString m_filePath;
    QString m_currentLanguage;
    bool m_hasUnsavedChanges;
    
    // Syntax highlighters
    PythonSyntaxHighlighter* m_pythonHighlighter;
    LuaSyntaxHighlighter* m_luaHighlighter;
    CppSyntaxHighlighter* m_cppHighlighter;
    MarkdownSyntaxHighlighter* m_markdownHighlighter;
};

/**
 * @brief Notepad++ style text editor with multiple tabs and syntax highlighting
 */
class NotepadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotepadDialog(QWidget* parent = nullptr);
    ~NotepadDialog();

    // File operations
    void newNote();
    void openNote();
    void saveCurrentNote();
    void saveCurrentNoteAs();
    void closeCurrentNote();
    void closeAllNotes();

    // Tab management
    void switchToTab(int index);
    NoteTab* getCurrentTab();
    int getTabCount() const;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewNote();
    void onOpenNote();
    void onSaveNote();
    void onSaveNoteAs();
    void onCloseNote();
    void onCloseAllNotes();
    void onTabChanged(int index);
    bool onTabCloseRequested(int index);
    void onContentChanged();
    void onSyntaxHighlightingToggled(bool enabled);
    void onLanguageChanged(const QString& language);
    void onFontChanged(const QFont& font);
    void onFontSizeChanged(int size);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void updateWindowTitle();
    void updateTabTitle(int index);
    bool promptSaveChanges(NoteTab* tab);
    void loadSettings();
    void saveSettings();

    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QTabWidget* m_tabWidget;
    QLabel* m_statusLabel;
    
    // Settings controls
    QCheckBox* m_syntaxHighlightingCheck;
    QComboBox* m_languageCombo;
    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpinBox;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_closeAction;
    QAction* m_closeAllAction;
    QAction* m_exitAction;
    
    // Settings
    bool m_syntaxHighlightingEnabled;
    QString m_defaultLanguage;
    QFont m_defaultFont;
    
    // Persistence
    QSettings* m_settings;
    QStringList m_recentFiles;
};
