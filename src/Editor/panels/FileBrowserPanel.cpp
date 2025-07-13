#include "FileBrowserPanel.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QMimeData>
#include <QDrag>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

FileBrowserPanel::FileBrowserPanel(QWidget* parent)
    : QWidget(parent)
    , m_fileModel(new QFileSystemModel(this))
{
    setupUI();
    setupContextMenu();
    
    // Configure file model
    m_fileModel->setNameFilters(QStringList() << "*.*");
    m_fileModel->setNameFilterDisables(false);

    m_treeView->setModel(m_fileModel);
    // Don't set root index until project is loaded
    
    // Hide size, type, and date columns - only show name
    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);
    
    // Enable drag and drop
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop);
    
    // Connect signals
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileBrowserPanel::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileBrowserPanel::onCustomContextMenuRequested);
    connect(m_refreshButton, &QPushButton::clicked, this, &FileBrowserPanel::onRefreshAction);
    connect(m_upButton, &QPushButton::clicked, this, [this]() {
        QDir currentDir(m_rootPath);
        if (currentDir.cdUp()) {
            setRootPath(currentDir.absolutePath());
        }
    });
}

void FileBrowserPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(4);
    
    // Toolbar
    m_toolbarLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit();
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText("Project path...");
    
    m_upButton = new QPushButton("↑");
    m_upButton->setMaximumWidth(30);
    m_upButton->setToolTip("Go up one directory");
    
    m_refreshButton = new QPushButton("⟳");
    m_refreshButton->setMaximumWidth(30);
    m_refreshButton->setToolTip("Refresh");
    
    m_toolbarLayout->addWidget(m_pathEdit);
    m_toolbarLayout->addWidget(m_upButton);
    m_toolbarLayout->addWidget(m_refreshButton);
    m_mainLayout->addLayout(m_toolbarLayout);
    
    // File tree
    m_treeView = new QTreeView();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->header()->setStretchLastSection(true);
    m_mainLayout->addWidget(m_treeView);
}

void FileBrowserPanel::setupContextMenu() {
    m_contextMenu = new QMenu(this);
    
    m_newSceneAction = new QAction("New Scene", this);
    m_newScriptAction = new QAction("New Script", this);
    m_newFolderAction = new QAction("New Folder", this);
    m_deleteAction = new QAction("Delete", this);
    m_renameAction = new QAction("Rename", this);
    m_refreshAction = new QAction("Refresh", this);
    
    m_contextMenu->addAction(m_newSceneAction);
    m_contextMenu->addAction(m_newScriptAction);
    m_contextMenu->addAction(m_newFolderAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_renameAction);
    m_contextMenu->addAction(m_deleteAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_refreshAction);
    
    connect(m_newSceneAction, &QAction::triggered, this, &FileBrowserPanel::onNewSceneAction);
    connect(m_newScriptAction, &QAction::triggered, this, &FileBrowserPanel::onNewScriptAction);
    connect(m_newFolderAction, &QAction::triggered, this, &FileBrowserPanel::onNewFolderAction);
    connect(m_deleteAction, &QAction::triggered, this, &FileBrowserPanel::onDeleteAction);
    connect(m_renameAction, &QAction::triggered, this, &FileBrowserPanel::onRenameAction);
    connect(m_refreshAction, &QAction::triggered, this, &FileBrowserPanel::onRefreshAction);
}

void FileBrowserPanel::setRootPath(const QString& path) {
    try {
        qDebug() << "FileBrowserPanel::setRootPath - Setting path:" << path;

        // Validate input
        if (path.isEmpty()) {
            qWarning() << "FileBrowserPanel::setRootPath - Empty path provided";
            return;
        }

        // Validate that the path exists
        QDir dir(path);
        if (!dir.exists()) {
            qWarning() << "FileBrowserPanel::setRootPath - Path does not exist:" << path;
            return;
        }

        // Validate that models are initialized
        if (!m_fileModel) {
            qCritical() << "FileBrowserPanel::setRootPath - File model is null";
            return;
        }

        if (!m_treeView) {
            qCritical() << "FileBrowserPanel::setRootPath - Tree view is null";
            return;
        }

        m_rootPath = path;
        m_pathEdit->setText(path);

        // Set root path with error checking
        qDebug() << "FileBrowserPanel::setRootPath - Setting root path on model";
        QModelIndex rootIndex = m_fileModel->setRootPath(path);

        if (!rootIndex.isValid()) {
            qWarning() << "FileBrowserPanel::setRootPath - Invalid root index returned for path:" << path;
            return;
        }

        qDebug() << "FileBrowserPanel::setRootPath - Setting root index on tree view";
        m_treeView->setRootIndex(rootIndex);

        qDebug() << "FileBrowserPanel::setRootPath - Expanding to depth 0";
        m_treeView->expandToDepth(0);

        qDebug() << "FileBrowserPanel::setRootPath - Successfully set root path:" << path;

    } catch (const std::exception& e) {
        qCritical() << "FileBrowserPanel::setRootPath - Exception:" << e.what();
    } catch (...) {
        qCritical() << "FileBrowserPanel::setRootPath - Unknown exception occurred";
    }
}

QString FileBrowserPanel::getSelectedFilePath() const {
    QModelIndexList selected = m_treeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return QString();
    }
    
    return m_fileModel->filePath(selected.first());
}

void FileBrowserPanel::refresh() {
    if (!m_rootPath.isEmpty()) {
        m_fileModel->setRootPath("");
        QModelIndex rootIndex = m_fileModel->setRootPath(m_rootPath);
        m_treeView->setRootIndex(rootIndex);
    }
}

void FileBrowserPanel::onItemDoubleClicked(const QModelIndex& index) {
    QString filePath = m_fileModel->filePath(index);
    QFileInfo fileInfo(filePath);
    
    if (fileInfo.isFile()) {
        if (isSceneFile(filePath) || isScriptFile(filePath)) {
            emit fileOpenRequested(filePath);
        } else {
            // Try to open with default application
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    }
}

void FileBrowserPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    m_contextMenuIndex = index;
    
    // Enable/disable actions based on selection
    bool hasSelection = index.isValid();
    bool isDirectory = hasSelection && m_fileModel->isDir(index);
    
    m_deleteAction->setEnabled(hasSelection);
    m_renameAction->setEnabled(hasSelection);
    
    // Show context menu
    m_contextMenu->exec(m_treeView->mapToGlobal(pos));
}

void FileBrowserPanel::onNewSceneAction() {
    QString dirPath = m_rootPath;
    if (m_contextMenuIndex.isValid()) {
        QString selectedPath = m_fileModel->filePath(m_contextMenuIndex);
        QFileInfo info(selectedPath);
        dirPath = info.isDir() ? selectedPath : info.dir().absolutePath();
    }
    
    createNewScene(dirPath);
}

void FileBrowserPanel::onNewScriptAction() {
    QString dirPath = m_rootPath;
    if (m_contextMenuIndex.isValid()) {
        QString selectedPath = m_fileModel->filePath(m_contextMenuIndex);
        QFileInfo info(selectedPath);
        dirPath = info.isDir() ? selectedPath : info.dir().absolutePath();
    }
    
    createNewScript(dirPath);
}

void FileBrowserPanel::onNewFolderAction() {
    QString dirPath = m_rootPath;
    if (m_contextMenuIndex.isValid()) {
        QString selectedPath = m_fileModel->filePath(m_contextMenuIndex);
        QFileInfo info(selectedPath);
        dirPath = info.isDir() ? selectedPath : info.dir().absolutePath();
    }
    
    createNewFolder(dirPath);
}

void FileBrowserPanel::onDeleteAction() {
    if (m_contextMenuIndex.isValid()) {
        deleteSelectedItem();
    }
}

void FileBrowserPanel::onRenameAction() {
    if (m_contextMenuIndex.isValid()) {
        renameSelectedItem();
    }
}

void FileBrowserPanel::onRefreshAction() {
    refresh();
}

void FileBrowserPanel::createNewScene(const QString& directoryPath) {
    bool ok;
    QString name = QInputDialog::getText(this, "New Scene", "Scene name:", QLineEdit::Normal, "NewScene", &ok);
    if (ok && !name.isEmpty()) {
        QString fileName = name + ".lupscene";
        QString filePath = QDir(directoryPath).absoluteFilePath(fileName);

        // Create empty scene file
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "{\n";
            out << "  \"scene\": {\n";
            out << "    \"name\": \"" << name << "\",\n";
            out << "    \"nodes\": []\n";
            out << "  }\n";
            out << "}\n";
            file.close();

            emit newSceneRequested(filePath);
            refresh();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create scene file: " + filePath);
        }
    }
}

void FileBrowserPanel::createNewScript(const QString& directoryPath) {
    QStringList scriptTypes = {"Python Script (.py)", "Lua Script (.lua)"};
    bool ok;
    QString scriptType = QInputDialog::getItem(this, "New Script", "Script type:", scriptTypes, 0, false, &ok);
    if (!ok) return;

    QString name = QInputDialog::getText(this, "New Script", "Script name:", QLineEdit::Normal, "NewScript", &ok);
    if (ok && !name.isEmpty()) {
        QString extension = scriptType.contains("Python") ? ".py" : ".lua";
        QString fileName = name + extension;
        QString filePath = QDir(directoryPath).absoluteFilePath(fileName);

        // Create script file with template
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (extension == ".py") {
                out << "# " << name << " - Python Script\n";
                out << "# Generated by Lupine Engine\n\n";
                out << "class " << name << ":\n";
                out << "    def __init__(self):\n";
                out << "        pass\n\n";
                out << "    def ready(self):\n";
                out << "        pass\n\n";
                out << "    def update(self, delta_time):\n";
                out << "        pass\n";
            } else {
                out << "-- " << name << " - Lua Script\n";
                out << "-- Generated by Lupine Engine\n\n";
                out << "local " << name << " = {}\n\n";
                out << "function " << name << ":ready()\n";
                out << "    -- Called when the script is ready\n";
                out << "end\n\n";
                out << "function " << name << ":update(delta_time)\n";
                out << "    -- Called every frame\n";
                out << "end\n\n";
                out << "return " << name << "\n";
            }
            file.close();

            emit newScriptRequested(filePath);
            refresh();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create script file: " + filePath);
        }
    }
}

void FileBrowserPanel::createNewFolder(const QString& directoryPath) {
    bool ok;
    QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "NewFolder", &ok);
    if (ok && !name.isEmpty()) {
        QString folderPath = QDir(directoryPath).absoluteFilePath(name);
        QDir dir;
        if (dir.mkpath(folderPath)) {
            refresh();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create folder: " + folderPath);
        }
    }
}

void FileBrowserPanel::deleteSelectedItem() {
    QString filePath = m_fileModel->filePath(m_contextMenuIndex);
    QFileInfo fileInfo(filePath);

    QString message = fileInfo.isDir() ?
        "Are you sure you want to delete the folder '" + fileInfo.fileName() + "' and all its contents?" :
        "Are you sure you want to delete the file '" + fileInfo.fileName() + "'?";

    int ret = QMessageBox::question(this, "Delete", message,
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (fileInfo.isDir()) {
            QDir dir(filePath);
            if (dir.removeRecursively()) {
                refresh();
            } else {
                QMessageBox::warning(this, "Error", "Failed to delete folder: " + filePath);
            }
        } else {
            QFile file(filePath);
            if (file.remove()) {
                refresh();
            } else {
                QMessageBox::warning(this, "Error", "Failed to delete file: " + filePath);
            }
        }
    }
}

void FileBrowserPanel::renameSelectedItem() {
    QString filePath = m_fileModel->filePath(m_contextMenuIndex);
    QFileInfo fileInfo(filePath);

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, fileInfo.baseName(), &ok);
    if (ok && !newName.isEmpty() && newName != fileInfo.baseName()) {
        QString newFilePath = fileInfo.dir().absoluteFilePath(newName + (fileInfo.isFile() ? "." + fileInfo.suffix() : ""));

        QFile file(filePath);
        if (file.rename(newFilePath)) {
            refresh();
        } else {
            QMessageBox::warning(this, "Error", "Failed to rename: " + filePath);
        }
    }
}

bool FileBrowserPanel::isSceneFile(const QString& filePath) const {
    return filePath.endsWith(".lupscene", Qt::CaseInsensitive);
}

bool FileBrowserPanel::isScriptFile(const QString& filePath) const {
    return filePath.endsWith(".py", Qt::CaseInsensitive) ||
           filePath.endsWith(".lua", Qt::CaseInsensitive);
}

bool FileBrowserPanel::isImageFile(const QString& filePath) const {
    QStringList imageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tiff", ".gif"};
    for (const QString& ext : imageExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool FileBrowserPanel::is3DModelFile(const QString& filePath) const {
    QStringList modelExtensions = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend"};
    for (const QString& ext : modelExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool FileBrowserPanel::isSpriteAnimationFile(const QString& filePath) const {
    // Assuming sprite animation files have specific extensions or naming conventions
    return filePath.endsWith(".anim", Qt::CaseInsensitive) ||
           filePath.endsWith(".spriteanim", Qt::CaseInsensitive) ||
           filePath.contains("_anim", Qt::CaseInsensitive);
}

bool FileBrowserPanel::isTilemapFile(const QString& filePath) const {
    return filePath.endsWith(".tilemap", Qt::CaseInsensitive) ||
           filePath.endsWith(".tmx", Qt::CaseInsensitive) ||
           filePath.endsWith(".tsx", Qt::CaseInsensitive);
}

QString FileBrowserPanel::getFileIcon(const QString& filePath) const {
    QFileInfo fileInfo(filePath);

    if (fileInfo.isDir()) {
        return "📁";
    }

    if (isSceneFile(filePath)) return "🎭";
    if (isScriptFile(filePath)) return "📜";
    if (isImageFile(filePath)) return "🖼️";
    if (is3DModelFile(filePath)) return "🔷";
    if (isSpriteAnimationFile(filePath)) return "🎬";
    if (isTilemapFile(filePath)) return "🗂️";

    // Default file icon
    return "📄";
}
