#include "AssetListView.h"
#include "AssetPreviewModel.h"
#include <QScrollBar>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>

AssetListView::AssetListView(QWidget* parent)
    : QListView(parent)
    , m_previewModel(nullptr)
    , m_viewportPreviewsEnabled(true)
    , m_previewRequestDelay(300) // 300ms delay
    , m_previewRequestPending(false)
{
    // Set up drag and drop
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);

    // Set up preview request timer
    m_previewRequestTimer = new QTimer(this);
    m_previewRequestTimer->setSingleShot(true);
    connect(m_previewRequestTimer, &QTimer::timeout, this, &AssetListView::requestVisiblePreviews);

    // Connect scroll bar signals
    connectScrollBars();
}

AssetListView::~AssetListView()
{
}

void AssetListView::setPreviewModel(AssetPreviewModel* previewModel)
{
    m_previewModel = previewModel;
}

void AssetListView::setViewportPreviewsEnabled(bool enabled)
{
    if (m_viewportPreviewsEnabled != enabled) {
        m_viewportPreviewsEnabled = enabled;
        if (enabled) {
            schedulePreviewRequest();
        } else {
            m_previewRequestTimer->stop();
        }
    }
}

void AssetListView::setPreviewRequestDelay(int delayMs)
{
    m_previewRequestDelay = qMax(0, delayMs);
    m_previewRequestTimer->setInterval(m_previewRequestDelay);
}

void AssetListView::scrollContentsBy(int dx, int dy)
{
    QListView::scrollContentsBy(dx, dy);
    
    if (m_viewportPreviewsEnabled && (dx != 0 || dy != 0)) {
        schedulePreviewRequest();
    }
}

void AssetListView::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    
    if (m_viewportPreviewsEnabled) {
        schedulePreviewRequest();
    }
}

void AssetListView::showEvent(QShowEvent* event)
{
    QListView::showEvent(event);
    
    if (m_viewportPreviewsEnabled) {
        schedulePreviewRequest();
    }
}

void AssetListView::requestVisiblePreviews()
{
    if (!m_previewModel || !m_viewportPreviewsEnabled) {
        return;
    }

    // Additional safety check - don't request previews if the model doesn't have previews enabled
    if (!m_previewModel->arePreviewsEnabled()) {
        m_previewRequestPending = false;
        return;
    }

    QModelIndexList visibleIndexes = getVisibleIndexes();
    if (!visibleIndexes.isEmpty()) {
        m_previewModel->requestPreviewsForRange(visibleIndexes);
    }

    m_previewRequestPending = false;
}

void AssetListView::onVerticalScrollChanged(int value)
{
    Q_UNUSED(value)
    if (m_viewportPreviewsEnabled) {
        schedulePreviewRequest();
    }
}

void AssetListView::schedulePreviewRequest()
{
    if (!m_previewRequestPending) {
        m_previewRequestPending = true;
        m_previewRequestTimer->start();
    }
}

QModelIndexList AssetListView::getVisibleIndexes() const
{
    QModelIndexList visibleIndexes;
    
    if (!model()) {
        return visibleIndexes;
    }
    
    // Get the visible rectangle
    QRect visibleRect = viewport()->rect();
    
    // Find all items that intersect with the visible area
    for (int row = 0; row < model()->rowCount(rootIndex()); ++row) {
        QModelIndex index = model()->index(row, 0, rootIndex());
        if (!index.isValid()) {
            continue;
        }
        
        QRect itemRect = visualRect(index);
        if (itemRect.intersects(visibleRect)) {
            visibleIndexes.append(index);
        }
        
        // Optimization: if we've gone past the visible area, we can stop
        if (itemRect.top() > visibleRect.bottom()) {
            break;
        }
    }
    
    return visibleIndexes;
}

void AssetListView::connectScrollBars()
{
    // Connect to vertical scroll bar
    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged,
                this, &AssetListView::onVerticalScrollChanged);
    }
    
    // Connect to horizontal scroll bar (less common but possible)
    if (horizontalScrollBar()) {
        connect(horizontalScrollBar(), &QScrollBar::valueChanged,
                this, &AssetListView::onVerticalScrollChanged);
    }
}

void AssetListView::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }

    QMimeData* mimeData = createMimeData(indexes);
    if (!mimeData) {
        return;
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Set drag pixmap
    QPixmap dragPixmap(64, 64);
    dragPixmap.fill(Qt::transparent);
    QPainter painter(&dragPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a simple file icon
    painter.setBrush(QBrush(QColor(100, 150, 200, 180)));
    painter.setPen(QPen(QColor(50, 100, 150), 2));
    painter.drawRoundedRect(8, 8, 48, 48, 4, 4);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(8, 8, 48, 48), Qt::AlignCenter, QString::number(indexes.size()));

    drag->setPixmap(dragPixmap);
    drag->setHotSpot(QPoint(32, 32));

    // Emit signal with file paths
    QStringList filePaths;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        if (QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(model())) {
            if (QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(proxyModel->sourceModel())) {
                QModelIndex sourceIndex = proxyModel->mapToSource(index);
                if (sourceIndex.isValid()) {
                    try {
                        QString filePath = fsModel->filePath(sourceIndex);
                        if (!filePath.isEmpty()) {
                            filePaths.append(filePath);
                        }
                    } catch (const std::exception& e) {
                        qDebug() << "AssetListView::startDrag - Exception getting file path:" << e.what();
                    } catch (...) {
                        qDebug() << "AssetListView::startDrag - Unknown exception getting file path";
                    }
                }
            }
        }
    }
    emit assetDragStarted(filePaths);

    drag->exec(supportedActions, Qt::CopyAction);
}

QMimeData* AssetListView::createMimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QList<QUrl> urls;

    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        if (QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(model())) {
            if (QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(proxyModel->sourceModel())) {
                QModelIndex sourceIndex = proxyModel->mapToSource(index);
                if (sourceIndex.isValid()) {
                    try {
                        QString filePath = fsModel->filePath(sourceIndex);
                        if (!filePath.isEmpty()) {
                            urls.append(QUrl::fromLocalFile(filePath));
                        }
                    } catch (const std::exception& e) {
                        qDebug() << "AssetListView::createMimeData - Exception getting file path:" << e.what();
                    } catch (...) {
                        qDebug() << "AssetListView::createMimeData - Unknown exception getting file path";
                    }
                }
            }
        }
    }

    mimeData->setUrls(urls);
    return mimeData;
}
