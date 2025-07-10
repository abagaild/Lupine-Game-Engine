#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QSet>
#include <QColor>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * @brief Represents a tag with name and color
 */
struct AssetTag {
    QString name;
    QColor color;
    QString description;
    
    AssetTag() = default;
    AssetTag(const QString& n, const QColor& c = QColor(), const QString& desc = "")
        : name(n), color(c), description(desc) {}
    
    bool operator==(const AssetTag& other) const {
        return name == other.name;
    }
    
    QJsonObject toJson() const;
    static AssetTag fromJson(const QJsonObject& json);
};

/**
 * @brief Manages tags for assets in the project
 */
class AssetTagManager : public QObject
{
    Q_OBJECT

public:
    explicit AssetTagManager(QObject* parent = nullptr);
    ~AssetTagManager();

    // Project management
    void setProjectPath(const QString& projectPath);
    QString getProjectPath() const { return m_projectPath; }
    
    // Tag management
    void addTag(const AssetTag& tag);
    void removeTag(const QString& tagName);
    void updateTag(const QString& oldName, const AssetTag& newTag);
    QList<AssetTag> getAllTags() const;
    AssetTag getTag(const QString& tagName) const;
    bool hasTag(const QString& tagName) const;
    
    // Asset tagging
    void tagAsset(const QString& assetPath, const QString& tagName);
    void untagAsset(const QString& assetPath, const QString& tagName);
    void setAssetTags(const QString& assetPath, const QStringList& tags);
    QStringList getAssetTags(const QString& assetPath) const;
    QStringList getAssetsWithTag(const QString& tagName) const;
    
    // Filtering and searching
    QStringList filterAssetsByTags(const QStringList& assetPaths, const QStringList& requiredTags, bool matchAll = false) const;
    QStringList searchAssetsByTags(const QString& searchText) const;
    
    // Predefined tag categories
    void createDefaultTags();
    QStringList getTagCategories() const;
    QList<AssetTag> getTagsInCategory(const QString& category) const;
    
    // Persistence
    bool saveToFile();
    bool loadFromFile();
    
    // Statistics
    int getTaggedAssetCount() const;
    int getUntaggedAssetCount(const QStringList& allAssets) const;
    QHash<QString, int> getTagUsageStats() const;

signals:
    void tagAdded(const AssetTag& tag);
    void tagRemoved(const QString& tagName);
    void tagUpdated(const QString& oldName, const AssetTag& newTag);
    void assetTagged(const QString& assetPath, const QString& tagName);
    void assetUntagged(const QString& assetPath, const QString& tagName);
    void tagsChanged();

private:
    QString getTagsFilePath() const;
    QString makeRelativePath(const QString& absolutePath) const;
    QString makeAbsolutePath(const QString& relativePath) const;
    
    QString m_projectPath;
    QHash<QString, AssetTag> m_tags; // tagName -> tag
    QHash<QString, QSet<QString>> m_assetTags; // assetPath -> set of tagNames
    QHash<QString, QSet<QString>> m_tagAssets; // tagName -> set of assetPaths
    
    // Default tag categories and colors
    static const QHash<QString, QColor> s_defaultTagColors;
};
