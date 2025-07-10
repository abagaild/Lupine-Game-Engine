#include "IconManager.h"
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>
#include <QImageReader>
#include <QBuffer>
#include <QDir>
#include <QThread>
#include <QDebug>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <exception>

IconManager& IconManager::Instance() {
    static IconManager instance;
    return instance;
}

bool IconManager::IsInitialized() const {
    // Check if QApplication is available for GUI operations
    return QApplication::instance() != nullptr;
}

QIcon IconManager::GetComponentIcon(const std::string& componentName, const std::string& category) {
    // Add safety check for initialization
    if (!IsInitialized()) {
        qWarning() << "IconManager: GetComponentIcon called before proper initialization";
        return QIcon(); // Return empty icon as fallback
    }

    std::string cacheKey = "comp_" + componentName + "_" + category;

    auto it = m_iconCache.find(cacheKey);
    if (it != m_iconCache.end()) {
        return it->second;
    }

    QIcon icon = LoadIconWithFallback(componentName, category, true);
    m_iconCache[cacheKey] = icon;
    return icon;
}

QIcon IconManager::GetNodeIcon(const std::string& nodeName, const std::string& category) {
    // Add safety check for initialization
    if (!IsInitialized()) {
        qWarning() << "IconManager: GetNodeIcon called before proper initialization";
        return QIcon(); // Return empty icon as fallback
    }

    std::string cacheKey = "node_" + nodeName + "_" + category;

    auto it = m_iconCache.find(cacheKey);
    if (it != m_iconCache.end()) {
        return it->second;
    }

    QIcon icon = LoadIconWithFallback(nodeName, category, false);
    m_iconCache[cacheKey] = icon;
    return icon;
}

QIcon IconManager::GetFileIcon(const QString& filePath) {
    // Add safety check for initialization
    if (!IsInitialized()) {
        qWarning() << "IconManager: GetFileIcon called before proper initialization";
        return QIcon(); // Return empty icon as fallback
    }

    // Validate input
    if (filePath.isEmpty()) {
        qDebug() << "IconManager: GetFileIcon called with empty file path";
        return QIcon();
    }

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString extension = fileInfo.suffix().toLower();

    std::string cacheKey = "file_" + extension.toStdString();

    auto it = m_iconCache.find(cacheKey);
    if (it != m_iconCache.end()) {
        return it->second;
    }

    QIcon icon;

    // 1. Try specific file icon (e.g., "texture.png")
    QString specificPath = m_iconBasePath + fileName;
    if (QFileInfo::exists(specificPath)) {
        icon = QIcon(specificPath);
    }
    // 2. Try extension-based icon (e.g., "png.png")
    else if (!extension.isEmpty()) {
        QString extensionPath = m_iconBasePath + extension + ".png";
        if (QFileInfo::exists(extensionPath)) {
            icon = QIcon(extensionPath);
        }
    }

    // 3. Fall back to file type emoji
    if (icon.isNull()) {
        icon = CreateFileTypeEmojiIcon(filePath);
    }

    m_iconCache[cacheKey] = icon;
    return icon;
}

void IconManager::SetIconBasePath(const QString& basePath) {
    m_iconBasePath = basePath;
    if (!m_iconBasePath.endsWith("/")) {
        m_iconBasePath += "/";
    }
    ClearCache();
}

void IconManager::ClearCache() {
    m_iconCache.clear();
}

QIcon IconManager::LoadIconWithFallback(const std::string& name, const std::string& category, bool isComponent) {
    // 1. Try specific icon (e.g., "AnimatedSprite2D.png")
    QString specificPath = m_iconBasePath + QString::fromStdString(name) + ".png";
    if (QFileInfo::exists(specificPath)) {
        return QIcon(specificPath);
    }
    
    // 2. Try category icon (e.g., "2D.png")
    if (!category.empty()) {
        QString categoryPath = m_iconBasePath + QString::fromStdString(category) + ".png";
        if (QFileInfo::exists(categoryPath)) {
            return QIcon(categoryPath);
        }
    }
    
    // 3. Fall back to emoji/symbol
    return CreateEmojiIcon(name, category, isComponent);
}

QIcon IconManager::CreateEmojiIcon(const std::string& name, const std::string& category, bool isComponent) {
    // Add safety check for initialization
    if (!IsInitialized()) {
        qWarning() << "IconManager: CreateEmojiIcon called before proper initialization";
        return QIcon(); // Return empty icon as fallback
    }

    QPixmap pixmap(24, 24);
    if (pixmap.isNull()) {
        qDebug() << "IconManager: Failed to create pixmap for CreateEmojiIcon";
        return QIcon();
    }

    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    if (!painter.isActive()) {
        qDebug() << "IconManager: Failed to create painter for CreateEmojiIcon";
        return QIcon();
    }

    painter.setRenderHint(QPainter::Antialiasing);

    // Get category color
    QColor bgColor = GetCategoryColor(category);
    QColor textColor = bgColor.lightness() > 128 ? Qt::black : Qt::white;

    // Draw background circle
    painter.setBrush(QBrush(bgColor));
    painter.setPen(QPen(bgColor.darker(120), 1));
    painter.drawEllipse(1, 1, 22, 22);

    // Get emoji/symbol
    QString emoji = GetEmojiForName(name, isComponent);

    // Draw emoji/symbol
    painter.setPen(textColor);
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(emoji);
    int x = (24 - textRect.width()) / 2;
    int y = (24 + textRect.height()) / 2 - 2;
    
    painter.drawText(x, y, emoji);
    
    return QIcon(pixmap);
}

QColor IconManager::GetCategoryColor(const std::string& category) {
    if (category == "2D") return QColor("#4CAF50");      // Green
    if (category == "3D") return QColor("#2196F3");      // Blue
    if (category == "UI") return QColor("#FF9800");      // Orange
    if (category == "Physics") return QColor("#F44336"); // Red
    if (category == "Audio") return QColor("#9C27B0");   // Purple
    if (category == "Scripting") return QColor("#607D8B"); // Blue Grey
    if (category == "Rendering") return QColor("#00BCD4"); // Cyan
    if (category == "Animation") return QColor("#FFEB3B"); // Yellow
    if (category == "Input") return QColor("#795548");    // Brown
    if (category == "Network") return QColor("#E91E63");  // Pink
    if (category == "Core") return QColor("#9E9E9E");     // Grey
    
    // Default color
    return QColor("#757575"); // Medium Grey
}

QString IconManager::GetEmojiForName(const std::string& name, bool isComponent) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // Component-specific emojis
    if (isComponent) {
        if (lowerName.find("sprite") != std::string::npos) return "🖼️";
        if (lowerName.find("label") != std::string::npos || lowerName.find("text") != std::string::npos) return "📝";
        if (lowerName.find("button") != std::string::npos) return "🔘";
        if (lowerName.find("camera") != std::string::npos) return "📷";
        if (lowerName.find("light") != std::string::npos) return "💡";
        if (lowerName.find("audio") != std::string::npos || lowerName.find("sound") != std::string::npos) return "🔊";
        if (lowerName.find("physics") != std::string::npos || lowerName.find("body") != std::string::npos) return "⚡";
        if (lowerName.find("collision") != std::string::npos) return "💥";
        if (lowerName.find("script") != std::string::npos) return "📜";
        if (lowerName.find("animator") != std::string::npos || lowerName.find("animation") != std::string::npos) return "🎬";
        if (lowerName.find("mesh") != std::string::npos) return "🔷";
        if (lowerName.find("transform") != std::string::npos) return "📐";
        if (lowerName.find("tilemap") != std::string::npos) return "🗂️";
        if (lowerName.find("particle") != std::string::npos) return "✨";
        if (lowerName.find("player") != std::string::npos) return "🎮";
        if (lowerName.find("controller") != std::string::npos) return "🕹️";
        if (lowerName.find("area") != std::string::npos) return "📍";
        if (lowerName.find("timer") != std::string::npos) return "⏰";
        if (lowerName.find("progress") != std::string::npos) return "📊";
        if (lowerName.find("panel") != std::string::npos) return "🗃️";
        if (lowerName.find("rectangle") != std::string::npos) return "▭";
        
        return "🔧"; // Default component emoji
    }
    
    // Node-specific emojis
    if (lowerName.find("node2d") != std::string::npos) return "2️⃣";
    if (lowerName.find("node3d") != std::string::npos) return "3️⃣";
    if (lowerName.find("control") != std::string::npos) return "🎛️";
    if (lowerName.find("scene") != std::string::npos) return "🎭";
    if (lowerName.find("root") != std::string::npos) return "🌳";
    if (lowerName.find("group") != std::string::npos) return "📁";
    
    return "📦"; // Default node emoji
}

QIcon IconManager::CreateFileTypeEmojiIcon(const QString& filePath) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get file type category for color
    QString category = GetFileTypeCategory(filePath);
    QColor bgColor = GetCategoryColor(category.toStdString());
    QColor textColor = bgColor.lightness() > 128 ? Qt::black : Qt::white;

    // Draw background circle
    painter.setBrush(QBrush(bgColor));
    painter.setPen(QPen(bgColor.darker(120), 1));
    painter.drawEllipse(1, 1, 22, 22);

    // Get file type emoji
    QString emoji = GetFileTypeEmoji(filePath);

    // Draw emoji
    painter.setPen(textColor);
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(emoji);
    int x = (24 - textRect.width()) / 2;
    int y = (24 + textRect.height()) / 2 - 2;

    painter.drawText(x, y, emoji);

    return QIcon(pixmap);
}

QString IconManager::GetFileTypeEmoji(const QString& filePath) {
    if (IsImageFile(filePath)) return "🖼️";
    if (Is3DModelFile(filePath)) return "🔷";
    if (IsScriptFile(filePath)) return "📜";
    if (IsSceneFile(filePath)) return "🎭";
    if (IsAudioFile(filePath)) return "🔊";
    if (IsAnimationFile(filePath)) return "🎬";
    if (IsTilemapFile(filePath)) return "🗂️";
    if (IsVideoFile(filePath)) return "🎥";
    if (IsTextFile(filePath)) return "📝";

    QFileInfo fileInfo(filePath);
    if (fileInfo.isDir()) return "📁";

    return "📄"; // Default file emoji
}

QString IconManager::GetFileTypeCategory(const QString& filePath) {
    if (IsImageFile(filePath) || IsAnimationFile(filePath) || IsTilemapFile(filePath)) {
        return "2D";
    }
    if (Is3DModelFile(filePath)) {
        return "3D";
    }
    if (IsScriptFile(filePath) || IsTextFile(filePath)) {
        return "Scripting";
    }
    if (IsAudioFile(filePath)) {
        return "Audio";
    }
    if (IsVideoFile(filePath)) {
        return "Rendering";
    }
    if (IsSceneFile(filePath)) {
        return "Core";
    }

    return "Core"; // Default category
}

bool IconManager::IsImageFile(const QString& filePath) const {
    QStringList imageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tiff", ".gif", ".webp"};
    for (const QString& ext : imageExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool IconManager::Is3DModelFile(const QString& filePath) const {
    QStringList modelExtensions = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend", ".ply"};
    for (const QString& ext : modelExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool IconManager::IsScriptFile(const QString& filePath) const {
    QStringList scriptExtensions = {".py", ".lua", ".js", ".cs", ".cpp", ".h", ".hpp"};
    for (const QString& ext : scriptExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool IconManager::IsSceneFile(const QString& filePath) const {
    return filePath.endsWith(".lupscene", Qt::CaseInsensitive);
}

bool IconManager::IsAudioFile(const QString& filePath) const {
    QStringList audioExtensions = {".wav", ".ogg", ".mp3", ".flac", ".aac", ".m4a", ".wma"};
    for (const QString& ext : audioExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool IconManager::IsAnimationFile(const QString& filePath) const {
    QStringList animExtensions = {".anim", ".spriteanim", ".skelanim"};
    for (const QString& ext : animExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return filePath.contains("_anim", Qt::CaseInsensitive) ||
           filePath.contains("animation", Qt::CaseInsensitive);
}

bool IconManager::IsTilemapFile(const QString& filePath) const {
    QStringList tilemapExtensions = {".tilemap", ".tmx", ".tsx"};
    for (const QString& ext : tilemapExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return filePath.contains("tilemap", Qt::CaseInsensitive) ||
           filePath.contains("tileset", Qt::CaseInsensitive);
}

bool IconManager::IsVideoFile(const QString& filePath) const {
    QStringList videoExtensions = {".mp4", ".avi", ".mov", ".wmv", ".flv", ".webm", ".mkv"};
    for (const QString& ext : videoExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool IconManager::IsTextFile(const QString& filePath) const {
    QStringList textExtensions = {".txt", ".md", ".json", ".xml", ".csv", ".log", ".ini", ".cfg"};
    for (const QString& ext : textExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QIcon IconManager::GetImagePreview(const QString& filePath, const QSize& size) {
    // Add safety guards
    try {
        if (!IsImageFile(filePath)) {
            return GetFileIcon(filePath);
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "IconManager: Image file not accessible:" << filePath;
            return GetFileIcon(filePath);
        }

        std::string cacheKey = "img_" + filePath.toStdString() + "_" +
                              std::to_string(size.width()) + "x" + std::to_string(size.height());

        auto it = m_previewCache.find(cacheKey);
        if (it != m_previewCache.end()) {
            return QIcon(it->second);
        }

        QPixmap thumbnail = GenerateImageThumbnail(filePath, size);
        if (!thumbnail.isNull()) {
            m_previewCache[cacheKey] = thumbnail;
            return QIcon(thumbnail);
        }

        return GetFileIcon(filePath);

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in GetImagePreview:" << e.what() << "for file:" << filePath;
        return GetFileIcon(filePath);
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in GetImagePreview for file:" << filePath;
        return GetFileIcon(filePath);
    }
}

QIcon IconManager::GetModelPreview(const QString& filePath, const QSize& size) {
    // WARNING: This method should only be called from the main thread due to GUI operations
    // For background thread usage, use GetSafeModelPreview instead
    try {
        if (!Is3DModelFile(filePath)) {
            return GetFileIcon(filePath);
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "IconManager: Model file not accessible:" << filePath;
            return GetFileIcon(filePath);
        }

        std::string cacheKey = "model_" + filePath.toStdString() + "_" +
                              std::to_string(size.width()) + "x" + std::to_string(size.height());

        auto it = m_previewCache.find(cacheKey);
        if (it != m_previewCache.end()) {
            return QIcon(it->second);
        }

        // Only generate thumbnail if we're on the main thread
        if (QThread::currentThread() != QApplication::instance()->thread()) {
            qDebug() << "IconManager: GetModelPreview called from background thread, using safe fallback";
            return GetSafeModelPreview(filePath, size);
        }

        QPixmap thumbnail = GenerateModelThumbnail(filePath, size);
        if (!thumbnail.isNull()) {
            m_previewCache[cacheKey] = thumbnail;
            return QIcon(thumbnail);
        }

        return GetFileIcon(filePath);

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in GetModelPreview:" << e.what() << "for file:" << filePath;
        return GetFileIcon(filePath);
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in GetModelPreview for file:" << filePath;
        return GetFileIcon(filePath);
    }
}

QIcon IconManager::GetSafeModelPreview(const QString& filePath, const QSize& size) {
    // Thread-safe fallback for 3D model previews
    // This method doesn't use any GUI operations and is safe to call from any thread
    try {
        if (!Is3DModelFile(filePath)) {
            return GetFileIcon(filePath);
        }

        // Validate file exists
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            qDebug() << "IconManager: Model file not found:" << filePath;
            return GetFileIcon(filePath);
        }

        // For now, just return the file icon with model-specific styling
        // TODO: Implement proper thread-safe 3D preview generation
        return GetFileIcon(filePath);

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in GetSafeModelPreview:" << e.what() << "for file:" << filePath;
        return GetFileIcon(filePath);
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in GetSafeModelPreview for file:" << filePath;
        return GetFileIcon(filePath);
    }
}

QPixmap IconManager::GenerateImageThumbnail(const QString& filePath, const QSize& size) {
    try {
        // Validate input parameters
        if (filePath.isEmpty() || size.isEmpty() || !size.isValid()) {
            qDebug() << "IconManager: Invalid parameters for GenerateImageThumbnail";
            return QPixmap();
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "IconManager: Image file not accessible for thumbnail:" << filePath;
            return QPixmap();
        }

        // Check file size to prevent loading extremely large images
        if (fileInfo.size() > 50 * 1024 * 1024) { // 50MB limit for images
            qDebug() << "IconManager: Image file too large for thumbnail generation:" << filePath << "Size:" << fileInfo.size();
            return QPixmap();
        }

        QImageReader reader(filePath);
        if (!reader.canRead()) {
            qDebug() << "IconManager: Cannot read image file:" << filePath << "Error:" << reader.errorString();
            return QPixmap();
        }

        // Additional validation for image format
        QString format = reader.format();
        if (format.isEmpty()) {
            qDebug() << "IconManager: Unknown image format for file:" << filePath;
            return QPixmap();
        }

        // Scale the image to fit within the size while maintaining aspect ratio
        QSize imageSize = reader.size();
        if (imageSize.isValid()) {
            // Validate image dimensions to prevent memory issues
            if (imageSize.width() > 8192 || imageSize.height() > 8192) {
                qDebug() << "IconManager: Image dimensions too large:" << filePath << "Size:" << imageSize;
                return QPixmap();
            }

            imageSize.scale(size, Qt::KeepAspectRatio);
            reader.setScaledSize(imageSize);
        } else {
            qDebug() << "IconManager: Invalid image size for file:" << filePath;
            return QPixmap();
        }

        QImage image = reader.read();
        if (image.isNull()) {
            qDebug() << "IconManager: Failed to read image:" << filePath << "Error:" << reader.errorString();
            return QPixmap();
        }

        // Validate the loaded image
        if (image.width() <= 0 || image.height() <= 0) {
            qDebug() << "IconManager: Invalid image dimensions after loading:" << filePath;
            return QPixmap();
        }

        // Create a pixmap with the exact requested size and center the image
        QPixmap pixmap(size);
        if (pixmap.isNull()) {
            qDebug() << "IconManager: Failed to create pixmap for image thumbnail:" << filePath;
            return QPixmap();
        }

        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        if (!painter.isActive()) {
            qDebug() << "IconManager: Failed to create painter for image thumbnail:" << filePath;
            return QPixmap();
        }

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Center the image
        int x = (size.width() - image.width()) / 2;
        int y = (size.height() - image.height()) / 2;

        // Validate coordinates
        if (x < 0 || y < 0 || x + image.width() > size.width() || y + image.height() > size.height()) {
            qDebug() << "IconManager: Invalid image positioning for thumbnail:" << filePath;
            // Try to draw at origin instead
            x = 0;
            y = 0;
        }

        painter.drawImage(x, y, image);

        // Add a subtle border
        painter.setPen(QPen(QColor(128, 128, 128, 100), 1));
        painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));

        return pixmap;

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in GenerateImageThumbnail:" << e.what() << "for file:" << filePath;
        return QPixmap();
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in GenerateImageThumbnail for file:" << filePath;
        return QPixmap();
    }
}

QPixmap IconManager::GenerateModelThumbnail(const QString& filePath, const QSize& size) {
    // WARNING: This method uses GUI operations and should only be called from the main thread
    try {
        // Validate input parameters
        if (filePath.isEmpty() || size.isEmpty() || !size.isValid()) {
            qDebug() << "IconManager: Invalid parameters for GenerateModelThumbnail";
            return QPixmap();
        }

        // Check if we're on the main thread
        if (QThread::currentThread() != QApplication::instance()->thread()) {
            qDebug() << "IconManager: GenerateModelThumbnail called from background thread, returning empty pixmap";
            return QPixmap();
        }

        // Additional safety check for QApplication instance
        if (!QApplication::instance()) {
            qDebug() << "IconManager: QApplication instance is null, cannot generate model thumbnail";
            return QPixmap();
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
            qDebug() << "IconManager: Model file not found or not readable for thumbnail:" << filePath;
            return QPixmap();
        }

        // Check file size to prevent loading extremely large files
        if (fileInfo.size() > 100 * 1024 * 1024) { // 100MB limit
            qDebug() << "IconManager: Model file too large for thumbnail generation:" << filePath << "Size:" << fileInfo.size();
            QIcon icon = GetFileIcon(filePath);
            return icon.pixmap(size); // Convert QIcon to QPixmap
        }

        // For now, create a wireframe representation
        // In the future, this could be enhanced with actual 3D rendering
        return CreateModelWireframe(filePath, size);

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in GenerateModelThumbnail:" << e.what() << "for file:" << filePath;
        QIcon icon = GetFileIcon(filePath);
        return icon.pixmap(size); // Convert QIcon to QPixmap
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in GenerateModelThumbnail for file:" << filePath;
        QIcon icon = GetFileIcon(filePath);
        return icon.pixmap(size); // Convert QIcon to QPixmap
    }
}

QPixmap IconManager::CreateModelWireframe(const QString& filePath, const QSize& size) {
    // WARNING: This method uses GUI operations and should only be called from the main thread
    try {
        // Validate input parameters
        if (filePath.isEmpty() || size.isEmpty() || !size.isValid()) {
            qDebug() << "IconManager: Invalid parameters for CreateModelWireframe";
            return QPixmap();
        }

        // Check if we're on the main thread
        if (QThread::currentThread() != QApplication::instance()->thread()) {
            qDebug() << "IconManager: CreateModelWireframe called from background thread, returning empty pixmap";
            return QPixmap();
        }

        // Additional safety check for QApplication instance
        if (!QApplication::instance()) {
            qDebug() << "IconManager: QApplication instance is null, cannot create wireframe";
            return QPixmap();
        }

        // Validate size constraints
        if (size.width() <= 0 || size.height() <= 0 || size.width() > 1024 || size.height() > 1024) {
            qDebug() << "IconManager: Invalid size for CreateModelWireframe:" << size;
            return QPixmap();
        }

        QPixmap pixmap(size);
        if (pixmap.isNull()) {
            qDebug() << "IconManager: Failed to create pixmap for CreateModelWireframe";
            return QPixmap();
        }

        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        if (!painter.isActive()) {
            qDebug() << "IconManager: Failed to create painter for CreateModelWireframe";
            return QPixmap();
        }

        painter.setRenderHint(QPainter::Antialiasing);

        // Get file info for different model types
        QFileInfo fileInfo(filePath);
        QString extension = fileInfo.suffix().toLower();

        // Set up colors based on model type
        QColor wireColor = QColor(100, 150, 255); // Blue for general models
        if (extension == "fbx") {
            wireColor = QColor(255, 150, 100); // Orange for FBX
        } else if (extension == "obj") {
            wireColor = QColor(150, 255, 100); // Green for OBJ
        } else if (extension == "gltf" || extension == "glb") {
            wireColor = QColor(255, 100, 150); // Pink for GLTF
        }

        painter.setPen(QPen(wireColor, 2));

        // Draw a simple 3D cube wireframe with safety checks
        int margin = qMax(1, size.width() / 8);
        int cubeSize = qMax(1, size.width() - 2 * margin);
        int depth = qMax(1, cubeSize / 3);

        // Validate dimensions
        if (margin < 0 || cubeSize <= 0 || depth <= 0) {
            qDebug() << "IconManager: Invalid wireframe dimensions for file:" << filePath;
            return QPixmap();
        }

        // Front face
        QRect frontFace(margin, margin, cubeSize, cubeSize);
        if (!frontFace.isValid() || !pixmap.rect().contains(frontFace)) {
            qDebug() << "IconManager: Invalid front face rectangle for wireframe:" << filePath;
            return QPixmap();
        }
        painter.drawRect(frontFace);

        // Back face (offset for 3D effect)
        QRect backFace(margin + depth, margin - depth, cubeSize, cubeSize);
        if (backFace.isValid() && pixmap.rect().intersects(backFace)) {
            painter.setPen(QPen(wireColor.darker(150), 1));
            painter.drawRect(backFace);

            // Connect corners for 3D effect
            painter.setPen(QPen(wireColor, 1));
            painter.drawLine(frontFace.topLeft(), backFace.topLeft());
            painter.drawLine(frontFace.topRight(), backFace.topRight());
            painter.drawLine(frontFace.bottomLeft(), backFace.bottomLeft());
            painter.drawLine(frontFace.bottomRight(), backFace.bottomRight());
        }

        // Add file extension label
        painter.setPen(QPen(Qt::white));
        QFont font = painter.font();
        font.setPointSize(8);
        font.setBold(true);
        painter.setFont(font);

        QRect textRect = pixmap.rect();
        textRect.setTop(textRect.bottom() - 15);
        painter.fillRect(textRect, QColor(0, 0, 0, 128));
        painter.drawText(textRect, Qt::AlignCenter, extension.toUpper());

        return pixmap;

    } catch (const std::exception& e) {
        qDebug() << "IconManager: Exception in CreateModelWireframe:" << e.what() << "for file:" << filePath;
        return QPixmap();
    } catch (...) {
        qDebug() << "IconManager: Unknown exception in CreateModelWireframe for file:" << filePath;
        return QPixmap();
    }
}
