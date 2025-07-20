#include "AssetPreviewModel.h"
#include "IconManager.h"
#include <QFileInfo>
#include <QApplication>
#include <QThreadPool>
#include <QRunnable>
#include <QMutexLocker>
#include <QDebug>
#include <stdexcept>
#include <exception>

// Preview generation runnable
class PreviewRunnable : public QRunnable
{
public:
    PreviewRunnable(const QString& filePath, const QSize& size, PreviewWorker* worker)
        : m_filePath(filePath), m_size(size), m_worker(worker)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        if (m_worker) {
            QMetaObject::invokeMethod(m_worker, "generatePreview", 
                                    Qt::QueuedConnection,
                                    Q_ARG(QString, m_filePath),
                                    Q_ARG(QSize, m_size));
        }
    }

private:
    QString m_filePath;
    QSize m_size;
    PreviewWorker* m_worker;
};

AssetPreviewModel::AssetPreviewModel(QObject* parent)
    : QFileSystemModel(parent)
    , m_previewSize(64, 64)
    , m_previewsEnabled(false) // Disabled by default for safety
    , m_lazyLoadingEnabled(true)
    , m_modelPreviewsEnabled(false) // Disabled by default for safety
    , m_maxConcurrentPreviews(3) // Limit concurrent preview generation
    , m_previewGenerationDelay(500) // 500ms delay before starting preview generation
    , m_activePreviews(0)
    , m_iconManager(nullptr) // Initialize as null, will be set when safe
{
    // Validate IconManager is available before using it
    if (IconManager::Instance().IsInitialized()) {
        m_iconManager = &IconManager::Instance();
    } else {
        qWarning() << "AssetPreviewModel: IconManager not properly initialized, previews will be disabled";
    }

    // Set up preview timer for batched updates - start disabled for safety
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(100); // 100ms delay for batching
    m_previewTimer->stop(); // Ensure timer is stopped initially

    // Set up queue processing timer for lazy loading - start disabled for safety
    m_queueProcessTimer = new QTimer(this);
    m_queueProcessTimer->setSingleShot(false);
    m_queueProcessTimer->setInterval(200); // Process queue every 200ms
    connect(m_queueProcessTimer, &QTimer::timeout, this, &AssetPreviewModel::processPreviewQueue);
    // Don't start the timer immediately - wait for safe initialization
}

AssetPreviewModel::~AssetPreviewModel()
{
    // Stop all timers first to prevent new operations
    if (m_previewTimer) {
        m_previewTimer->stop();
        disconnect(m_previewTimer, nullptr, this, nullptr);
    }

    if (m_queueProcessTimer) {
        m_queueProcessTimer->stop();
        disconnect(m_queueProcessTimer, nullptr, this, nullptr);
    }

    // Clear all caches and queues
    clearPreviewCache();

    // Wait for any running background tasks to complete
    // This prevents the "QWaitCondition: Destroyed while threads are still waiting" error
    QThreadPool::globalInstance()->waitForDone(5000); // Wait up to 5 seconds

    qDebug() << "AssetPreviewModel: Destructor completed safely";
}

QVariant AssetPreviewModel::data(const QModelIndex& index, int role) const
{
    // Add comprehensive safety checks
    if (!index.isValid()) {
        return QFileSystemModel::data(index, role);
    }

    // Additional safety check - ensure we're not being destroyed
    if (!m_iconManager || !QApplication::instance()) {
        return QFileSystemModel::data(index, role);
    }

    if (role == Qt::DecorationRole && m_previewsEnabled && m_iconManager) {
        QString filePath;

        // Safely get file path with error handling
        try {
            filePath = this->filePath(index);

            // Additional validation - check if the file path is valid and not empty
            if (filePath.isEmpty()) {
                return QFileSystemModel::data(index, role);
            }

            // Quick check if file exists before proceeding
            QFileInfo quickCheck(filePath);
            if (!quickCheck.exists()) {
                return QFileSystemModel::data(index, role);
            }

        } catch (const std::exception& e) {
            qDebug() << "AssetPreviewModel::data - Exception getting file path:" << e.what();
            return QFileSystemModel::data(index, role);
        } catch (...) {
            qDebug() << "AssetPreviewModel::data - Unknown exception getting file path";
            return QFileSystemModel::data(index, role);
        }

        if (!filePath.isEmpty() && shouldGeneratePreview(filePath)) {
            QMutexLocker locker(&m_cacheMutex);

            // Check if we have a cached preview
            auto it = m_previewCache.find(filePath);
            if (it != m_previewCache.end()) {
                return it.value();
            }

            // For lazy loading, only queue the preview request
            if (m_lazyLoadingEnabled) {
                // Don't generate immediately, just queue it
                if (!m_previewRequests.contains(filePath)) {
                    m_previewRequests[filePath] = true;

                    QMutexLocker queueLocker(&m_queueMutex);
                    if (!m_previewQueue.contains(filePath)) {
                        m_previewQueue.append(filePath);
                    }
                }
            } else {
                // Immediate generation (old behavior)
                if (!m_previewRequests.contains(filePath)) {
                    m_previewRequests[filePath] = true;
                    generatePreviewAsync(filePath);
                }
            }

            // Return default icon while preview is being generated
            return m_iconManager->GetFileIcon(filePath);
        }
    }

    // Fall back to default behavior
    return QFileSystemModel::data(index, role);
}

void AssetPreviewModel::setPreviewSize(const QSize& size)
{
    if (m_previewSize != size) {
        m_previewSize = size;
        clearPreviewCache();
    }
}

void AssetPreviewModel::setPreviewsEnabled(bool enabled)
{
    if (m_previewsEnabled != enabled) {
        m_previewsEnabled = enabled;
        if (!enabled) {
            clearPreviewCache();
        }
    }
}

void AssetPreviewModel::setLazyLoadingEnabled(bool enabled)
{
    m_lazyLoadingEnabled = enabled;
}

void AssetPreviewModel::setMaxConcurrentPreviews(int maxConcurrent)
{
    m_maxConcurrentPreviews = qMax(1, maxConcurrent);
}

void AssetPreviewModel::setPreviewGenerationDelay(int delayMs)
{
    m_previewGenerationDelay = qMax(0, delayMs);
}

void AssetPreviewModel::clearPreviewCache()
{
    // Stop timers first to prevent new operations
    if (m_previewTimer && m_previewTimer->isActive()) {
        m_previewTimer->stop();
    }

    if (m_queueProcessTimer && m_queueProcessTimer->isActive()) {
        m_queueProcessTimer->stop();
    }

    // Clear all caches and queues
    {
        QMutexLocker locker(&m_cacheMutex);
        m_previewCache.clear();
        m_previewRequests.clear();
        m_activePreviews = 0;
    }

    {
        QMutexLocker queueLocker(&m_queueMutex);
        m_previewQueue.clear();
    }

    qDebug() << "AssetPreviewModel: Preview cache cleared";
}

void AssetPreviewModel::requestPreview(const QString& filePath)
{
    if (!m_previewsEnabled || !m_iconManager || !shouldGeneratePreview(filePath)) {
        return;
    }

    QMutexLocker locker(&m_cacheMutex);
    if (m_previewCache.contains(filePath) || m_previewRequests.contains(filePath)) {
        return; // Already cached or requested
    }

    m_previewRequests[filePath] = true;

    if (m_lazyLoadingEnabled) {
        QMutexLocker queueLocker(&m_queueMutex);
        if (!m_previewQueue.contains(filePath)) {
            m_previewQueue.prepend(filePath); // Add to front for priority
        }
    } else {
        generatePreviewAsync(filePath);
    }
}

void AssetPreviewModel::requestPreviewsForRange(const QModelIndexList& indexes)
{
    // Safety check - ensure we're not shutting down
    if (!QApplication::instance() || QApplication::instance()->closingDown()) {
        return;
    }

    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        QString filePath;
        try {
            filePath = this->filePath(index);
        } catch (const std::exception& e) {
            qDebug() << "AssetPreviewModel::requestPreviewsForRange - Exception getting file path:" << e.what();
            continue;
        } catch (...) {
            qDebug() << "AssetPreviewModel::requestPreviewsForRange - Unknown exception getting file path";
            continue;
        }

        if (!filePath.isEmpty()) {
            requestPreview(filePath);
        }
    }
}

void AssetPreviewModel::onPreviewReady(const QString& filePath, const QIcon& icon)
{
    // Safety check - ensure we're not shutting down
    if (!QApplication::instance() || QApplication::instance()->closingDown()) {
        return;
    }

    {
        QMutexLocker locker(&m_cacheMutex);
        m_previewCache[filePath] = icon;
        m_previewRequests.remove(filePath);
        m_activePreviews = qMax(0, m_activePreviews - 1);
    }

    // Find the model index for this file and emit dataChanged
    try {
        QModelIndex index = this->index(filePath);
        if (index.isValid()) {
            emit dataChanged(index, index, {Qt::DecorationRole});
        }
    } catch (const std::exception& e) {
        qDebug() << "AssetPreviewModel::onPreviewReady - Exception finding index for:" << filePath << "Error:" << e.what();
    } catch (...) {
        qDebug() << "AssetPreviewModel::onPreviewReady - Unknown exception finding index for:" << filePath;
    }

    emit previewGenerated(filePath);
}

void AssetPreviewModel::processPreviewQueue()
{
    // COMPLETELY DISABLE preview queue processing to prevent crashes
    qDebug() << "AssetPreviewModel::processPreviewQueue - Processing disabled to prevent crashes";
    if (m_queueProcessTimer && m_queueProcessTimer->isActive()) {
        m_queueProcessTimer->stop();
    }
    return;

    // Add comprehensive safety checks
    if (!m_lazyLoadingEnabled || !canGeneratePreviewNow()) {
        return;
    }

    // Check if we're being destroyed or shutting down
    if (!parent() || !QApplication::instance() || QApplication::instance()->closingDown()) {
        qDebug() << "AssetPreviewModel::processPreviewQueue - Application shutting down, stopping timer";
        if (m_queueProcessTimer) {
            m_queueProcessTimer->stop();
        }
        return;
    }

    // Check if we have a valid root path
    if (rootPath().isEmpty()) {
        qDebug() << "AssetPreviewModel::processPreviewQueue - No root path set, stopping timer";
        if (m_queueProcessTimer) {
            m_queueProcessTimer->stop();
        }
        return;
    }

    // Ensure we have a valid IconManager
    if (!m_iconManager || !m_iconManager->IsInitialized()) {
        qDebug() << "AssetPreviewModel::processPreviewQueue - IconManager not available, stopping timer";
        if (m_queueProcessTimer) {
            m_queueProcessTimer->stop();
        }
        return;
    }

    // Ensure QApplication is still valid
    if (!QApplication::instance()) {
        qDebug() << "AssetPreviewModel::processPreviewQueue - QApplication not available, stopping timer";
        if (m_queueProcessTimer) {
            m_queueProcessTimer->stop();
        }
        return;
    }

    QString filePath;
    {
        QMutexLocker queueLocker(&m_queueMutex);
        if (m_previewQueue.isEmpty()) {
            return;
        }
        filePath = m_previewQueue.takeFirst();
    }

    // Check if still needed (might have been removed from view)
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_previewCache.contains(filePath)) {
            return; // Already generated
        }
        m_activePreviews++;
    }

    generatePreviewAsync(filePath);
}

bool AssetPreviewModel::canGeneratePreviewNow() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_activePreviews < m_maxConcurrentPreviews;
}

void AssetPreviewModel::generatePreviewAsync(const QString& filePath) const
{
    // Add safety guards to prevent crashes
    try {
        if (filePath.isEmpty()) {
            qDebug() << "AssetPreviewModel: Empty file path provided for preview generation";
            return;
        }

        // Validate IconManager is available
        if (!m_iconManager) {
            qDebug() << "AssetPreviewModel: IconManager not available for preview generation:" << filePath;
            const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, QIcon());
            return;
        }

        // Check if QApplication is still valid
        if (!QApplication::instance()) {
            qDebug() << "AssetPreviewModel: QApplication instance is null, cannot generate preview";
            const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
            return;
        }

        // Validate file exists before starting background processing
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "AssetPreviewModel: File does not exist or is not readable for preview:" << filePath;
            // Still emit a result with fallback icon
            const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
            return;
        }

        // Check file size to prevent loading extremely large files
        if (fileInfo.size() > 100 * 1024 * 1024) { // 100MB limit
            qDebug() << "AssetPreviewModel: File too large for preview generation:" << filePath << "Size:" << fileInfo.size();
            const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
            return;
        }

        // Additional check for model files if model previews are disabled
        QString extension = fileInfo.suffix().toLower();
        QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
        if (modelExtensions.contains(extension)) {
            if (!m_modelPreviewsEnabled) {
                qDebug() << "AssetPreviewModel: Model previews disabled, using default icon for:" << filePath;
                const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
                return;
            }

            // Additional safety check for model files - ensure we're not in a critical startup phase
            if (!QApplication::instance() || !QApplication::instance()->thread()) {
                qDebug() << "AssetPreviewModel: Application not ready for model preview generation:" << filePath;
                const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
                return;
            }
        }

        // Create a worker and run it in thread pool
        PreviewWorker* worker = new PreviewWorker();

        // Ensure worker is deleted when done
        worker->setParent(nullptr);
        connect(worker, &PreviewWorker::previewReady,
                this, &AssetPreviewModel::onPreviewReady,
                Qt::QueuedConnection);
        connect(worker, &PreviewWorker::previewReady,
                worker, &QObject::deleteLater,
                Qt::QueuedConnection);

        // Use thread pool for background processing
        PreviewRunnable* runnable = new PreviewRunnable(filePath, m_previewSize, worker);
        QThreadPool::globalInstance()->start(runnable);

    } catch (const std::exception& e) {
        qDebug() << "AssetPreviewModel: Exception in generatePreviewAsync:" << e.what() << "for file:" << filePath;
        const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
    } catch (...) {
        qDebug() << "AssetPreviewModel: Unknown exception in generatePreviewAsync for file:" << filePath;
        const_cast<AssetPreviewModel*>(this)->onPreviewReady(filePath, m_iconManager->GetFileIcon(filePath));
    }
}

void AssetPreviewModel::setModelPreviewsEnabled(bool enabled)
{
    qDebug() << "AssetPreviewModel: Setting model previews enabled to:" << enabled;

    // Add safety check for main thread
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        qWarning() << "AssetPreviewModel: setModelPreviewsEnabled called from background thread";
        return;
    }

    m_modelPreviewsEnabled = enabled;
    if (!enabled) {
        // Clear any cached model previews when disabled
        QMutexLocker locker(&m_cacheMutex);
        auto it = m_previewCache.begin();
        while (it != m_previewCache.end()) {
            QFileInfo fileInfo(it.key());
            QString extension = fileInfo.suffix().toLower();
            QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
            if (modelExtensions.contains(extension)) {
                it = m_previewCache.erase(it);
            } else {
                ++it;
            }
        }

        // Also clear any pending model preview requests
        QMutexLocker queueLocker(&m_queueMutex);
        auto queueIt = m_previewQueue.begin();
        while (queueIt != m_previewQueue.end()) {
            QFileInfo fileInfo(*queueIt);
            QString extension = fileInfo.suffix().toLower();
            QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
            if (modelExtensions.contains(extension)) {
                queueIt = m_previewQueue.erase(queueIt);
            } else {
                ++queueIt;
            }
        }
    }
}

void AssetPreviewModel::enablePreviewsWhenSafe()
{
    // Only enable previews if the application is properly initialized
    if (!QApplication::instance()) {
        qWarning() << "AssetPreviewModel: Cannot enable previews - QApplication not available";
        return;
    }

    if (!m_iconManager || !m_iconManager->IsInitialized()) {
        qWarning() << "AssetPreviewModel: Cannot enable previews - IconManager not properly initialized";
        return;
    }

    // Validate that the model has a valid root path
    if (rootPath().isEmpty()) {
        qWarning() << "AssetPreviewModel: Cannot enable previews - No root path set";
        return;
    }

    // Enable basic previews (images are generally safe)
    setPreviewsEnabled(true);

    // DO NOT start any timers during initial setup to prevent crashes
    // The timer will be started manually later when it's safe
    qDebug() << "AssetPreviewModel: Timers disabled during initialization to prevent crashes";

    // Model previews remain disabled by default for safety
    // They can be enabled separately if needed
    qDebug() << "AssetPreviewModel: Basic previews enabled safely";
}

void AssetPreviewModel::startPreviewProcessing()
{
    // COMPLETELY DISABLE timer starting to prevent crashes
    qDebug() << "AssetPreviewModel::startPreviewProcessing - Timer starting disabled to prevent crashes";
    return;

    // Only start if everything is properly initialized and safe
    if (!m_previewsEnabled || !m_iconManager || !QApplication::instance() || QApplication::instance()->closingDown()) {
        qDebug() << "AssetPreviewModel::startPreviewProcessing - Not safe to start processing";
        return;
    }

    if (rootPath().isEmpty()) {
        qDebug() << "AssetPreviewModel::startPreviewProcessing - No root path set";
        return;
    }

    if (m_queueProcessTimer && !m_queueProcessTimer->isActive()) {
        m_queueProcessTimer->start();
        qDebug() << "AssetPreviewModel: Preview processing timer started manually";
    }
}

bool AssetPreviewModel::shouldGeneratePreview(const QString& filePath) const
{
    // Add comprehensive safety checks before accessing file info
    if (filePath.isEmpty()) {
        return false;
    }

    // Check if application is shutting down
    if (!QApplication::instance() || QApplication::instance()->closingDown()) {
        return false;
    }

    try {
        QFileInfo fileInfo(filePath);

        // Check if file exists before checking if it's a file
        if (!fileInfo.exists()) {
            return false;
        }

        if (!fileInfo.isFile()) {
            return false;
        }

        QString extension = fileInfo.suffix().toLower();

        // Image files
        QStringList imageExtensions = {"png", "jpg", "jpeg", "bmp", "tga", "tiff", "gif", "webp"};
        if (imageExtensions.contains(extension)) {
            return true;
        }

        // 3D model files - only if model previews are enabled
        QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
        if (modelExtensions.contains(extension)) {
            return m_modelPreviewsEnabled;
        }

        return false;

    } catch (const std::exception& e) {
        qDebug() << "AssetPreviewModel::shouldGeneratePreview - Exception:" << e.what() << "for file:" << filePath;
        return false;
    } catch (...) {
        qDebug() << "AssetPreviewModel::shouldGeneratePreview - Unknown exception for file:" << filePath;
        return false;
    }
}

// PreviewWorker implementation
PreviewWorker::PreviewWorker(QObject* parent)
    : QObject(parent)
    , m_iconManager(nullptr) // Initialize as null, will be set when safe
{
    // Validate IconManager is available before using it
    if (IconManager::Instance().IsInitialized()) {
        m_iconManager = &IconManager::Instance();
    } else {
        qWarning() << "PreviewWorker: IconManager not properly initialized";
    }
}

void PreviewWorker::generatePreview(const QString& filePath, const QSize& size)
{
    // Add safety guards to prevent crashes
    try {
        // Validate IconManager is available
        if (!m_iconManager) {
            qDebug() << "PreviewWorker: IconManager not available for preview generation:" << filePath;
            emit previewReady(filePath, QIcon());
            return;
        }

        QFileInfo fileInfo(filePath);

        // Validate file exists and is readable
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "PreviewWorker: File not accessible:" << filePath;
            emit previewReady(filePath, m_iconManager->GetFileIcon(filePath));
            return;
        }

        QString extension = fileInfo.suffix().toLower();
        QIcon icon;

        // Generate appropriate preview with error handling
        QStringList imageExtensions = {"png", "jpg", "jpeg", "bmp", "tga", "tiff", "gif", "webp"};
        if (imageExtensions.contains(extension)) {
            // Image previews can be generated safely in background thread
            icon = m_iconManager->GetImagePreview(filePath, size);
        } else {
            QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
            if (modelExtensions.contains(extension)) {
                // For 3D models, use thread-safe fallback for now
                // TODO: Implement proper 3D preview generation on main thread
                qDebug() << "PreviewWorker: Using fallback for 3D model:" << filePath;
                icon = m_iconManager->GetSafeModelPreview(filePath, size);
            }
        }

        // Always emit a result, even if it's just the file icon
        if (icon.isNull()) {
            icon = m_iconManager->GetFileIcon(filePath);
        }

        emit previewReady(filePath, icon);

    } catch (const std::exception& e) {
        qDebug() << "PreviewWorker: Exception during preview generation:" << e.what() << "for file:" << filePath;
        emit previewReady(filePath, m_iconManager->GetFileIcon(filePath));
    } catch (...) {
        qDebug() << "PreviewWorker: Unknown exception during preview generation for file:" << filePath;
        emit previewReady(filePath, m_iconManager->GetFileIcon(filePath));
    }
}
