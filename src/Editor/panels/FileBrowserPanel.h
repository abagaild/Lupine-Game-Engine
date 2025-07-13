#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QModelIndex>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QString>

/**
 * @brief File browser panel for project files and folders
 * 
 * Features:
 * - Browse project files and folders
 * - Double-click to open scenes/scripts
 * - Right-click context menu to create new files
 * - Drag and drop support for asset creation
 */
class FileBrowserPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileBrowserPanel(QWidget* parent = nullptr);
    ~FileBrowserPanel() = default;

    /**
     * @brief Set the root directory for the file browser
     * @param path Path to the project root directory
     */
    void setRootPath(const QString& path);

    /**
     * @brief Get the currently selected file path
     * @return Selected file path or empty string if none selected
     */
    QString getSelectedFilePath() const;

    /**
     * @brief Refresh the file browser
     */
    void refresh();

signals:
    /**
     * @brief Emitted when a file should be opened
     * @param filePath Path to the file to open
     */
    void fileOpenRequested(const QString& filePath);

    /**
     * @brief Emitted when a new scene should be created
     * @param directoryPath Directory where the scene should be created
     */
    void newSceneRequested(const QString& directoryPath);

    /**
     * @brief Emitted when a new script should be created
     * @param directoryPath Directory where the script should be created
     */
    void newScriptRequested(const QString& directoryPath);

    /**
     * @brief Emitted when files are dragged from the browser
     * @param filePaths List of file paths being dragged
     */
    void filesDragStarted(const QStringList& filePaths);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onNewSceneAction();
    void onNewScriptAction();
    void onNewFolderAction();
    void onDeleteAction();
    void onRenameAction();
    void onRefreshAction();

private:
    void setupUI();
    void setupContextMenu();
    void createNewScene(const QString& directoryPath);
    void createNewScript(const QString& directoryPath);
    void createNewFolder(const QString& directoryPath);
    void deleteSelectedItem();
    void renameSelectedItem();
    bool isSceneFile(const QString& filePath) const;
    bool isScriptFile(const QString& filePath) const;
    bool isImageFile(const QString& filePath) const;
    bool is3DModelFile(const QString& filePath) const;
    bool isSpriteAnimationFile(const QString& filePath) const;
    bool isTilemapFile(const QString& filePath) const;
    QString getFileIcon(const QString& filePath) const;

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QLineEdit* m_pathEdit;
    QPushButton* m_refreshButton;
    QPushButton* m_upButton;
    QTreeView* m_treeView;
    QFileSystemModel* m_fileModel;

    // Context menu
    QMenu* m_contextMenu;
    QAction* m_newSceneAction;
    QAction* m_newScriptAction;
    QAction* m_newFolderAction;
    QAction* m_deleteAction;
    QAction* m_renameAction;
    QAction* m_refreshAction;

    // Data
    QString m_rootPath;
    QModelIndex m_contextMenuIndex;
};
