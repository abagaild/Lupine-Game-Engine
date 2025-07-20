#include "lupine/editor/TileBuilder.h"
#include <QFileSystemWatcher>
#include <QDebug>

namespace Lupine {

TileTextureWatcher::TileTextureWatcher(QObject* parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this)) {
    
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &TileTextureWatcher::OnFileChanged);
}

TileTextureWatcher::~TileTextureWatcher() {
    ClearFiles();
}

void TileTextureWatcher::AddFile(const QString& file_path) {
    if (!m_watched_files.contains(file_path)) {
        m_watched_files.append(file_path);
        m_watcher->addPath(file_path);
        qDebug() << "TileTextureWatcher: Added file to watch list:" << file_path;
    }
}

void TileTextureWatcher::RemoveFile(const QString& file_path) {
    if (m_watched_files.contains(file_path)) {
        m_watched_files.removeAll(file_path);
        m_watcher->removePath(file_path);
        qDebug() << "TileTextureWatcher: Removed file from watch list:" << file_path;
    }
}

void TileTextureWatcher::ClearFiles() {
    if (!m_watched_files.isEmpty()) {
        m_watcher->removePaths(m_watched_files);
        m_watched_files.clear();
        qDebug() << "TileTextureWatcher: Cleared all watched files";
    }
}

void TileTextureWatcher::OnFileChanged(const QString& path) {
    qDebug() << "TileTextureWatcher: File changed:" << path;
    
    // Re-add the file to the watcher (some editors remove and recreate files)
    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }
    
    emit FileChanged(path);
}

} // namespace Lupine
