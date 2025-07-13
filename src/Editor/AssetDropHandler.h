#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <memory>
#include <glm/glm.hpp>

namespace Lupine {
    class Node;
    class Scene;
}

class SceneViewPanel;

/**
 * @brief Asset creation options for different file types
 */
struct AssetCreationOption {
    QString name;           // Display name (e.g., "Static Mesh Component on 3D Node")
    QString nodeType;       // Node type to create (e.g., "Node3D")
    QStringList components; // Components to add (e.g., "StaticMesh")
    QString description;    // Description of what this option does
    bool isDefault;         // Whether this is the default option
};

/**
 * @brief Handles drag-and-drop asset creation from file browser to scene
 * 
 * Features:
 * - Default node+component creation for each file type
 * - Alt+drop shows popup with multiple options
 * - Support for 3D files, images, sprite animations, tilemaps
 */
class AssetDropHandler : public QObject
{
    Q_OBJECT

public:
    explicit AssetDropHandler(QObject* parent = nullptr);
    ~AssetDropHandler() = default;

    /**
     * @brief Set the scene view panel for coordinate conversion
     * @param sceneViewPanel Pointer to the scene view panel
     */
    void setSceneViewPanel(SceneViewPanel* sceneViewPanel);

    /**
     * @brief Handle dropping files onto the scene
     * @param filePaths List of file paths being dropped
     * @param dropPosition Position where files were dropped (scene coordinates)
     * @param showOptions Whether to show options popup (Alt key held)
     * @param scene Target scene to add nodes to
     * @return True if files were handled successfully
     */
    bool handleFileDrop(const QStringList& filePaths, 
                       const QPoint& dropPosition, 
                       bool showOptions, 
                       Lupine::Scene* scene);

    /**
     * @brief Get available creation options for a file type
     * @param filePath Path to the file
     * @return List of available creation options
     */
    QList<AssetCreationOption> getCreationOptions(const QString& filePath) const;

    /**
     * @brief Check if file is an image file
     * @param filePath Path to check
     * @return True if image file
     */
    bool isImageFile(const QString& filePath) const;

    /**
     * @brief Check if file is a 3D model file
     * @param filePath Path to check
     * @return True if 3D model file
     */
    bool is3DModelFile(const QString& filePath) const;

    /**
     * @brief Create a node with components for the given file and option
     * @param filePath Path to the asset file
     * @param option Creation option to use
     * @param position Position for the new node
     * @return Created node or nullptr if failed
     */
    std::unique_ptr<Lupine::Node> createNodeForAsset(const QString& filePath, 
                                                     const AssetCreationOption& option,
                                                     const QPoint& position) const;

signals:
    /**
     * @brief Emitted when a node is created from an asset
     * @param node The created node
     * @param filePath Path to the source asset file
     */
    void nodeCreatedFromAsset(Lupine::Node* node, const QString& filePath);

private slots:
    void onOptionSelected();

private:
    void setupCreationOptions();
    QList<AssetCreationOption> get3DModelOptions() const;
    QList<AssetCreationOption> getImageOptions() const;
    QList<AssetCreationOption> getSpriteAnimationOptions() const;
    QList<AssetCreationOption> getTilemapOptions() const;

    bool isSpriteAnimationFile(const QString& filePath) const;
    bool isTilemapFile(const QString& filePath) const;
    
    std::unique_ptr<Lupine::Node> create3DModelNode(const QString& filePath, 
                                                   const AssetCreationOption& option,
                                                   const QPoint& position) const;
    std::unique_ptr<Lupine::Node> createImageNode(const QString& filePath, 
                                                 const AssetCreationOption& option,
                                                 const QPoint& position) const;
    std::unique_ptr<Lupine::Node> createSpriteAnimationNode(const QString& filePath, 
                                                           const AssetCreationOption& option,
                                                           const QPoint& position) const;
    std::unique_ptr<Lupine::Node> createTilemapNode(const QString& filePath,
                                                   const AssetCreationOption& option,
                                                   const QPoint& position) const;

    // Helper methods
    glm::vec3 screenToWorldPosition(const QPoint& screenPos) const;

    // Data
    QMenu* m_optionsMenu;
    QString m_currentFilePath;
    QPoint m_currentPosition;
    Lupine::Scene* m_currentScene;
    QList<AssetCreationOption> m_currentOptions;

    // Reference to scene view for coordinate conversion
    mutable class SceneViewPanel* m_sceneViewPanel;
};
