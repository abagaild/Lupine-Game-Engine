#pragma once

#include <QListView>
#include <QTimer>
#include <QScrollBar>

class AssetPreviewModel;

/**
 * @brief Enhanced list view that supports lazy loading of previews
 * 
 * This view automatically requests previews for visible items and implements
 * viewport-based lazy loading to improve performance with large asset libraries.
 */
class AssetListView : public QListView
{
    Q_OBJECT

public:
    explicit AssetListView(QWidget* parent = nullptr);
    ~AssetListView();

    // Set the preview model for lazy loading
    void setPreviewModel(AssetPreviewModel* previewModel);
    
    // Enable/disable viewport-based preview loading
    void setViewportPreviewsEnabled(bool enabled);
    bool areViewportPreviewsEnabled() const { return m_viewportPreviewsEnabled; }
    
    // Set delay before requesting previews for visible items
    void setPreviewRequestDelay(int delayMs);
    int getPreviewRequestDelay() const { return m_previewRequestDelay; }

signals:
    void assetDragStarted(const QStringList& filePaths);

protected:
    // Override to handle viewport changes
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // Drag and drop support
    void startDrag(Qt::DropActions supportedActions) override;
    QMimeData* createMimeData(const QModelIndexList& indexes) const;

private slots:
    void requestVisiblePreviews();
    void onVerticalScrollChanged(int value);

private:
    void schedulePreviewRequest();
    QModelIndexList getVisibleIndexes() const;
    void connectScrollBars();
    
    AssetPreviewModel* m_previewModel;
    bool m_viewportPreviewsEnabled;
    int m_previewRequestDelay;
    
    QTimer* m_previewRequestTimer;
    bool m_previewRequestPending;
};
