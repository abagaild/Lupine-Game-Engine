#include "AssetTagManager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDebug>

// Default tag colors
const QHash<QString, QColor> AssetTagManager::s_defaultTagColors = {
    {"2D", QColor(100, 200, 100)},
    {"3D", QColor(100, 150, 255)},
    {"Audio", QColor(255, 200, 100)},
    {"Script", QColor(200, 100, 255)},
    {"Scene", QColor(255, 150, 150)},
    {"Material", QColor(150, 255, 200)},
    {"Animation", QColor(255, 255, 100)},
    {"UI", QColor(200, 200, 200)},
    {"Texture", QColor(255, 180, 120)},
    {"Model", QColor(120, 180, 255)},
    {"Important", QColor(255, 100, 100)},
    {"WIP", QColor(255, 200, 50)},
    {"Complete", QColor(100, 255, 100)},
    {"Deprecated", QColor(150, 150, 150)}
};

QJsonObject AssetTag::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["color"] = color.name();
    obj["description"] = description;
    return obj;
}

AssetTag AssetTag::fromJson(const QJsonObject& json) {
    AssetTag tag;
    tag.name = json["name"].toString();
    tag.color = QColor(json["color"].toString());
    tag.description = json["description"].toString();
    return tag;
}

AssetTagManager::AssetTagManager(QObject* parent)
    : QObject(parent)
{
}

AssetTagManager::~AssetTagManager()
{
    if (!m_projectPath.isEmpty()) {
        saveToFile();
    }
}

void AssetTagManager::setProjectPath(const QString& projectPath)
{
    if (m_projectPath != projectPath) {
        // Save current project tags if any
        if (!m_projectPath.isEmpty()) {
            saveToFile();
        }
        
        m_projectPath = projectPath;
        
        // Clear current data
        m_tags.clear();
        m_assetTags.clear();
        m_tagAssets.clear();
        
        // Load new project tags
        if (!projectPath.isEmpty()) {
            loadFromFile();
            if (m_tags.isEmpty()) {
                createDefaultTags();
            }
        }
        
        emit tagsChanged();
    }
}

void AssetTagManager::addTag(const AssetTag& tag)
{
    if (!tag.name.isEmpty() && !m_tags.contains(tag.name)) {
        m_tags[tag.name] = tag;
        emit tagAdded(tag);
        emit tagsChanged();
    }
}

void AssetTagManager::removeTag(const QString& tagName)
{
    if (m_tags.contains(tagName)) {
        // Remove tag from all assets
        auto assetsWithTag = m_tagAssets.value(tagName);
        for (const QString& assetPath : assetsWithTag) {
            m_assetTags[assetPath].remove(tagName);
            if (m_assetTags[assetPath].isEmpty()) {
                m_assetTags.remove(assetPath);
            }
        }
        
        m_tags.remove(tagName);
        m_tagAssets.remove(tagName);
        
        emit tagRemoved(tagName);
        emit tagsChanged();
    }
}

void AssetTagManager::updateTag(const QString& oldName, const AssetTag& newTag)
{
    if (m_tags.contains(oldName) && !newTag.name.isEmpty()) {
        // Update tag info
        m_tags.remove(oldName);
        m_tags[newTag.name] = newTag;
        
        // Update asset mappings if name changed
        if (oldName != newTag.name) {
            auto assetsWithTag = m_tagAssets.value(oldName);
            m_tagAssets.remove(oldName);
            m_tagAssets[newTag.name] = assetsWithTag;
            
            // Update reverse mapping
            for (const QString& assetPath : assetsWithTag) {
                m_assetTags[assetPath].remove(oldName);
                m_assetTags[assetPath].insert(newTag.name);
            }
        }
        
        emit tagUpdated(oldName, newTag);
        emit tagsChanged();
    }
}

QList<AssetTag> AssetTagManager::getAllTags() const
{
    return m_tags.values();
}

AssetTag AssetTagManager::getTag(const QString& tagName) const
{
    return m_tags.value(tagName);
}

bool AssetTagManager::hasTag(const QString& tagName) const
{
    return m_tags.contains(tagName);
}

void AssetTagManager::tagAsset(const QString& assetPath, const QString& tagName)
{
    if (m_tags.contains(tagName)) {
        QString relativePath = makeRelativePath(assetPath);
        m_assetTags[relativePath].insert(tagName);
        m_tagAssets[tagName].insert(relativePath);
        
        emit assetTagged(assetPath, tagName);
    }
}

void AssetTagManager::untagAsset(const QString& assetPath, const QString& tagName)
{
    QString relativePath = makeRelativePath(assetPath);
    if (m_assetTags.contains(relativePath)) {
        m_assetTags[relativePath].remove(tagName);
        if (m_assetTags[relativePath].isEmpty()) {
            m_assetTags.remove(relativePath);
        }
    }
    
    if (m_tagAssets.contains(tagName)) {
        m_tagAssets[tagName].remove(relativePath);
        if (m_tagAssets[tagName].isEmpty()) {
            m_tagAssets.remove(tagName);
        }
    }
    
    emit assetUntagged(assetPath, tagName);
}

void AssetTagManager::setAssetTags(const QString& assetPath, const QStringList& tags)
{
    QString relativePath = makeRelativePath(assetPath);
    
    // Remove old tags
    QSet<QString> oldTags = m_assetTags.value(relativePath);
    for (const QString& oldTag : oldTags) {
        untagAsset(assetPath, oldTag);
    }
    
    // Add new tags
    for (const QString& tag : tags) {
        tagAsset(assetPath, tag);
    }
}

QStringList AssetTagManager::getAssetTags(const QString& assetPath) const
{
    QString relativePath = makeRelativePath(assetPath);
    return m_assetTags.value(relativePath).values();
}

QStringList AssetTagManager::getAssetsWithTag(const QString& tagName) const
{
    QStringList result;
    auto assets = m_tagAssets.value(tagName);
    for (const QString& relativePath : assets) {
        result.append(makeAbsolutePath(relativePath));
    }
    return result;
}

QStringList AssetTagManager::filterAssetsByTags(const QStringList& assetPaths, const QStringList& requiredTags, bool matchAll) const
{
    if (requiredTags.isEmpty()) {
        return assetPaths;
    }
    
    QStringList result;
    for (const QString& assetPath : assetPaths) {
        QStringList assetTags = getAssetTags(assetPath);
        
        if (matchAll) {
            // Asset must have ALL required tags
            bool hasAllTags = true;
            for (const QString& requiredTag : requiredTags) {
                if (!assetTags.contains(requiredTag)) {
                    hasAllTags = false;
                    break;
                }
            }
            if (hasAllTags) {
                result.append(assetPath);
            }
        } else {
            // Asset must have ANY of the required tags
            bool hasAnyTag = false;
            for (const QString& requiredTag : requiredTags) {
                if (assetTags.contains(requiredTag)) {
                    hasAnyTag = true;
                    break;
                }
            }
            if (hasAnyTag) {
                result.append(assetPath);
            }
        }
    }
    
    return result;
}

void AssetTagManager::createDefaultTags()
{
    // Asset type tags
    addTag(AssetTag("2D", s_defaultTagColors.value("2D"), "2D graphics and sprites"));
    addTag(AssetTag("3D", s_defaultTagColors.value("3D"), "3D models and meshes"));
    addTag(AssetTag("Audio", s_defaultTagColors.value("Audio"), "Sound effects and music"));
    addTag(AssetTag("Script", s_defaultTagColors.value("Script"), "Code and scripts"));
    addTag(AssetTag("Scene", s_defaultTagColors.value("Scene"), "Scene files"));
    addTag(AssetTag("Material", s_defaultTagColors.value("Material"), "Materials and shaders"));
    addTag(AssetTag("Animation", s_defaultTagColors.value("Animation"), "Animation files"));
    addTag(AssetTag("UI", s_defaultTagColors.value("UI"), "User interface elements"));
    
    // Content type tags
    addTag(AssetTag("Texture", s_defaultTagColors.value("Texture"), "Texture images"));
    addTag(AssetTag("Model", s_defaultTagColors.value("Model"), "3D model files"));
    
    // Status tags
    addTag(AssetTag("Important", s_defaultTagColors.value("Important"), "Important assets"));
    addTag(AssetTag("WIP", s_defaultTagColors.value("WIP"), "Work in progress"));
    addTag(AssetTag("Complete", s_defaultTagColors.value("Complete"), "Completed assets"));
    addTag(AssetTag("Deprecated", s_defaultTagColors.value("Deprecated"), "Deprecated assets"));
}

bool AssetTagManager::saveToFile()
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    QString filePath = getTagsFilePath();
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QJsonObject root;

    // Save tags
    QJsonArray tagsArray;
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        tagsArray.append(it.value().toJson());
    }
    root["tags"] = tagsArray;

    // Save asset tags
    QJsonObject assetTagsObj;
    for (auto it = m_assetTags.begin(); it != m_assetTags.end(); ++it) {
        QJsonArray tagArray;
        for (const QString& tag : it.value()) {
            tagArray.append(tag);
        }
        assetTagsObj[it.key()] = tagArray;
    }
    root["assetTags"] = assetTagsObj;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        return true;
    }

    return false;
}

bool AssetTagManager::loadFromFile()
{
    QString filePath = getTagsFilePath();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    // Load tags
    QJsonArray tagsArray = root["tags"].toArray();
    for (const QJsonValue& value : tagsArray) {
        AssetTag tag = AssetTag::fromJson(value.toObject());
        if (!tag.name.isEmpty()) {
            m_tags[tag.name] = tag;
        }
    }

    // Load asset tags
    QJsonObject assetTagsObj = root["assetTags"].toObject();
    for (auto it = assetTagsObj.begin(); it != assetTagsObj.end(); ++it) {
        QString assetPath = it.key();
        QJsonArray tagArray = it.value().toArray();

        QSet<QString> tags;
        for (const QJsonValue& tagValue : tagArray) {
            QString tagName = tagValue.toString();
            tags.insert(tagName);
            m_tagAssets[tagName].insert(assetPath);
        }
        m_assetTags[assetPath] = tags;
    }

    return true;
}

QString AssetTagManager::getTagsFilePath() const
{
    return QDir(m_projectPath).filePath(".lupine/asset_tags.json");
}

QString AssetTagManager::makeRelativePath(const QString& absolutePath) const
{
    if (m_projectPath.isEmpty()) {
        return absolutePath;
    }

    QDir projectDir(m_projectPath);
    return projectDir.relativeFilePath(absolutePath);
}

QString AssetTagManager::makeAbsolutePath(const QString& relativePath) const
{
    if (m_projectPath.isEmpty()) {
        return relativePath;
    }

    return QDir(m_projectPath).absoluteFilePath(relativePath);
}

int AssetTagManager::getTaggedAssetCount() const
{
    return m_assetTags.size();
}

QHash<QString, int> AssetTagManager::getTagUsageStats() const
{
    QHash<QString, int> stats;
    for (auto it = m_tagAssets.begin(); it != m_tagAssets.end(); ++it) {
        stats[it.key()] = it.value().size();
    }
    return stats;
}
