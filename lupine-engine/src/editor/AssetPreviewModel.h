#pragma once

#include <QFileSystemModel>
#include <QIcon>
#include <QSize>
#include <QHash>
#include <QTimer>
#include <QThread>
#include <QMutex>

class IconManager;

/**
 * @brief Custom file system model that provides preview icons for assets
 */
class AssetPreviewModel : public QFileSystemModel
{
    Q_OBJECT

public:
    explicit AssetPreviewModel(QObject* parent = nullptr);
    ~AssetPreviewModel();

    // Override to provide custom icons
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    
    // Set the icon size for previews
    void setPreviewSize(const QSize& size);
    QSize getPreviewSize() const { return m_previewSize; }
    
    // Enable/disable preview generation
    void setPreviewsEnabled(bool enabled);
    bool arePreviewsEnabled() const { return m_previewsEnabled; }

    // Lazy loading controls
    void setLazyLoadingEnabled(bool enabled);
    bool isLazyLoadingEnabled() const { return m_lazyLoadingEnabled; }

    void setMaxConcurrentPreviews(int maxConcurrent);
    int getMaxConcurrentPreviews() const { return m_maxConcurrentPreviews; }

    void setPreviewGenerationDelay(int delayMs);
    int getPreviewGenerationDelay() const { return m_previewGenerationDelay; }

    // Clear preview cache
    void clearPreviewCache();

    // Request preview for specific items (used by view for visible items)
    void requestPreview(const QString& filePath);
    void requestPreviewsForRange(const QModelIndexList& indexes);

    // Safety controls for preventing crashes
    void setModelPreviewsEnabled(bool enabled);
    bool areModelPreviewsEnabled() const { return m_modelPreviewsEnabled; }

    // Safe initialization method to enable previews after application is ready
    void enablePreviewsWhenSafe();

    // Start preview processing timer (call this after initialization is complete)
    void startPreviewProcessing();

signals:
    void previewGenerated(const QString& filePath);

private slots:
    void onPreviewReady(const QString& filePath, const QIcon& icon);

private:
    void generatePreviewAsync(const QString& filePath) const;
    bool shouldGeneratePreview(const QString& filePath) const;
    bool canGeneratePreviewNow() const;
    void processPreviewQueue();
    void schedulePreviewGeneration();
    
    QSize m_previewSize;
    bool m_previewsEnabled;
    bool m_lazyLoadingEnabled;
    bool m_modelPreviewsEnabled;
    int m_maxConcurrentPreviews;
    int m_previewGenerationDelay;

    mutable QHash<QString, QIcon> m_previewCache;
    mutable QHash<QString, bool> m_previewRequests; // Track pending requests
    mutable QStringList m_previewQueue; // Queue of files waiting for preview generation
    mutable int m_activePreviews; // Count of currently generating previews

    IconManager* m_iconManager;

    // Thread-safe preview generation
    mutable QMutex m_cacheMutex;
    mutable QMutex m_queueMutex;
    QTimer* m_previewTimer;
    QTimer* m_queueProcessTimer;
};

/**
 * @brief Worker class for generating previews in background thread
 */
class PreviewWorker : public QObject
{
    Q_OBJECT

public:
    explicit PreviewWorker(QObject* parent = nullptr);

public slots:
    void generatePreview(const QString& filePath, const QSize& size);

signals:
    void previewReady(const QString& filePath, const QIcon& icon);

private:
    IconManager* m_iconManager;
};
