#pragma once

#include <QIcon>
#include <QString>
#include <QPixmap>
#include <unordered_map>
#include <string>

/**
 * @brief Centralized icon management for components and nodes
 * 
 * Implements a fallback system:
 * 1. Try specific icon (e.g., "AnimatedSprite2D.png")
 * 2. Try category icon (e.g., "2D.png")
 * 3. Fall back to emoji/symbol
 */
class IconManager
{
public:
    static IconManager& Instance();

    /**
     * @brief Check if IconManager is properly initialized
     * @return true if QApplication is available and IconManager can perform GUI operations
     */
    bool IsInitialized() const;
    
    /**
     * @brief Get icon for a component
     * @param componentName Name of the component (e.g., "AnimatedSprite2D")
     * @param category Category of the component (e.g., "2D")
     * @return QIcon with appropriate fallback
     */
    QIcon GetComponentIcon(const std::string& componentName, const std::string& category = "");
    
    /**
     * @brief Get icon for a node
     * @param nodeName Name of the node type (e.g., "Node2D")
     * @param category Category of the node (e.g., "2D")
     * @return QIcon with appropriate fallback
     */
    QIcon GetNodeIcon(const std::string& nodeName, const std::string& category = "");

    /**
     * @brief Get icon for a file type
     * @param filePath Path to the file
     * @return QIcon with appropriate fallback based on file type
     */
    QIcon GetFileIcon(const QString& filePath);

    /**
     * @brief Get preview icon for an image file
     * @param filePath Path to the image file
     * @param size Size of the preview icon
     * @return QIcon with image preview or fallback
     */
    QIcon GetImagePreview(const QString& filePath, const QSize& size = QSize(64, 64));

    /**
     * @brief Get preview icon for a 3D model file
     * @param filePath Path to the model file
     * @param size Size of the preview icon
     * @return QIcon with model preview or fallback
     */
    QIcon GetModelPreview(const QString& filePath, const QSize& size = QSize(64, 64));

    /**
     * @brief Get thread-safe preview icon for a 3D model file (fallback only)
     * @param filePath Path to the model file
     * @param size Size of the preview icon
     * @return QIcon with safe fallback representation
     */
    QIcon GetSafeModelPreview(const QString& filePath, const QSize& size = QSize(64, 64));
    
    /**
     * @brief Set the base path for icon resources
     * @param basePath Path to the icons directory
     */
    void SetIconBasePath(const QString& basePath);
    
    /**
     * @brief Clear the icon cache
     */
    void ClearCache();

private:
    IconManager() = default;
    
    /**
     * @brief Try to load icon with fallback system
     * @param name Primary name to try
     * @param category Category fallback
     * @param isComponent Whether this is for a component (affects emoji fallback)
     * @return QIcon with appropriate fallback
     */
    QIcon LoadIconWithFallback(const std::string& name, const std::string& category, bool isComponent);
    
    /**
     * @brief Create emoji/symbol icon as final fallback
     * @param name Name to generate emoji for
     * @param category Category for color coding
     * @param isComponent Whether this is for a component
     * @return QIcon with emoji/symbol
     */
    QIcon CreateEmojiIcon(const std::string& name, const std::string& category, bool isComponent);

    /**
     * @brief Create file type emoji icon
     * @param filePath Path to the file
     * @return QIcon with file type emoji/symbol
     */
    QIcon CreateFileTypeEmojiIcon(const QString& filePath);
    
    /**
     * @brief Get color for category
     * @param category Category name
     * @return QColor for the category
     */
    QColor GetCategoryColor(const std::string& category);
    
    /**
     * @brief Get emoji/symbol for name
     * @param name Component/node name
     * @param isComponent Whether this is for a component
     * @return Unicode emoji or symbol
     */
    QString GetEmojiForName(const std::string& name, bool isComponent);

    /**
     * @brief Get emoji/symbol for file type
     * @param filePath Path to the file
     * @return Unicode emoji or symbol based on file type
     */
    QString GetFileTypeEmoji(const QString& filePath);

    /**
     * @brief Get file type category for color coding
     * @param filePath Path to the file
     * @return Category string for color coding
     */
    QString GetFileTypeCategory(const QString& filePath);

    /**
     * @brief Generate image thumbnail
     * @param filePath Path to the image file
     * @param size Size of the thumbnail
     * @return QPixmap with image thumbnail
     */
    QPixmap GenerateImageThumbnail(const QString& filePath, const QSize& size);

    /**
     * @brief Generate 3D model thumbnail
     * @param filePath Path to the model file
     * @param size Size of the thumbnail
     * @return QPixmap with model thumbnail
     */
    QPixmap GenerateModelThumbnail(const QString& filePath, const QSize& size);

    /**
     * @brief Create a simple wireframe representation of a model
     * @param filePath Path to the model file
     * @param size Size of the thumbnail
     * @return QPixmap with wireframe representation
     */
    QPixmap CreateModelWireframe(const QString& filePath, const QSize& size);

    // File type detection helpers
    bool IsImageFile(const QString& filePath) const;
    bool Is3DModelFile(const QString& filePath) const;
    bool IsScriptFile(const QString& filePath) const;
    bool IsSceneFile(const QString& filePath) const;
    bool IsAudioFile(const QString& filePath) const;
    bool IsAnimationFile(const QString& filePath) const;
    bool IsTilemapFile(const QString& filePath) const;
    bool IsVideoFile(const QString& filePath) const;
    bool IsTextFile(const QString& filePath) const;
    
    QString m_iconBasePath = ":/icons/";
    std::unordered_map<std::string, QIcon> m_iconCache;
    std::unordered_map<std::string, QPixmap> m_previewCache;
};
