#include "AssetDropHandler.h"
#include "panels/SceneViewPanel.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/core/Component.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/SkinnedMesh.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/RigidBody3D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Sprite3D.h"
#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/components/AnimatedSprite3D.h"
#include <QFileInfo>
#include <QMessageBox>

AssetDropHandler::AssetDropHandler(QObject* parent)
    : QObject(parent)
    , m_optionsMenu(new QMenu())
    , m_currentScene(nullptr)
    , m_sceneViewPanel(nullptr)
{
    setupCreationOptions();
}

void AssetDropHandler::setSceneViewPanel(SceneViewPanel* sceneViewPanel) {
    m_sceneViewPanel = sceneViewPanel;
}

bool AssetDropHandler::handleFileDrop(const QStringList& filePaths, 
                                     const QPoint& dropPosition, 
                                     bool showOptions, 
                                     Lupine::Scene* scene) {
    if (filePaths.isEmpty() || !scene) {
        return false;
    }

    m_currentScene = scene;
    m_currentPosition = dropPosition;

    bool success = true;
    for (const QString& filePath : filePaths) {
        QList<AssetCreationOption> options = getCreationOptions(filePath);
        if (options.isEmpty()) {
            continue; // Skip unsupported file types
        }

        if (showOptions && options.size() > 1) {
            // Show options popup
            m_currentFilePath = filePath;
            m_currentOptions = options;
            
            m_optionsMenu->clear();
            for (const AssetCreationOption& option : options) {
                QAction* action = m_optionsMenu->addAction(option.name);
                action->setData(QVariant::fromValue(static_cast<const void*>(&option)));
                action->setToolTip(option.description);
                if (option.isDefault) {
                    QFont font = action->font();
                    font.setBold(true);
                    action->setFont(font);
                }
                connect(action, &QAction::triggered, this, &AssetDropHandler::onOptionSelected);
            }
            
            // Show menu at drop position (convert to global coordinates)
            QPoint globalPos = dropPosition;
            if (m_sceneViewPanel) {
                globalPos = m_sceneViewPanel->mapToGlobal(dropPosition);
            }
            m_optionsMenu->exec(globalPos);
        } else {
            // Use default option
            AssetCreationOption defaultOption;
            for (const AssetCreationOption& option : options) {
                if (option.isDefault) {
                    defaultOption = option;
                    break;
                }
            }
            
            if (!defaultOption.name.isEmpty()) {
                auto node = createNodeForAsset(filePath, defaultOption, dropPosition);
                if (node) {
                    Lupine::Node* nodePtr = node.get();
                    // Add node to the scene's root node
                    if (scene->GetRootNode()) {
                        scene->GetRootNode()->AddChild(std::move(node));
                    } else {
                        // If no root node, create one and add our node to it
                        scene->CreateRootNode("Root");
                        scene->GetRootNode()->AddChild(std::move(node));
                    }
                    emit nodeCreatedFromAsset(nodePtr, filePath);
                } else {
                    success = false;
                }
            }
        }
    }

    return success;
}

QList<AssetCreationOption> AssetDropHandler::getCreationOptions(const QString& filePath) const {
    if (is3DModelFile(filePath)) {
        return get3DModelOptions();
    } else if (isImageFile(filePath)) {
        return getImageOptions();
    } else if (isSpriteAnimationFile(filePath)) {
        return getSpriteAnimationOptions();
    } else if (isTilemapFile(filePath)) {
        return getTilemapOptions();
    }
    
    return QList<AssetCreationOption>();
}

void AssetDropHandler::onOptionSelected() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action || m_currentFilePath.isEmpty()) {
        return;
    }
    
    const AssetCreationOption* option = static_cast<const AssetCreationOption*>(action->data().value<const void*>());
    if (option && m_currentScene) {
        auto node = createNodeForAsset(m_currentFilePath, *option, m_currentPosition);
        if (node) {
            Lupine::Node* nodePtr = node.get();
            // Add node to the scene's root node
            if (m_currentScene->GetRootNode()) {
                m_currentScene->GetRootNode()->AddChild(std::move(node));
            } else {
                // If no root node, create one and add our node to it
                m_currentScene->CreateRootNode("Root");
                m_currentScene->GetRootNode()->AddChild(std::move(node));
            }
            emit nodeCreatedFromAsset(nodePtr, m_currentFilePath);
        }
    }
    
    // Clear current state
    m_currentFilePath.clear();
    m_currentOptions.clear();
    m_currentScene = nullptr;
}

void AssetDropHandler::setupCreationOptions() {
    // Options are created dynamically based on file type
}

QList<AssetCreationOption> AssetDropHandler::get3DModelOptions() const {
    QList<AssetCreationOption> options;
    
    // Default: Static Mesh Component on 3D Node
    AssetCreationOption staticMesh;
    staticMesh.name = "Static Mesh Component on 3D Node";
    staticMesh.nodeType = "Node3D";
    staticMesh.components = {"StaticMesh"};
    staticMesh.description = "Creates a 3D node with a static mesh component for non-animated 3D models";
    staticMesh.isDefault = true;
    options.append(staticMesh);
    
    // Skinned Mesh Component on 3D Node
    AssetCreationOption skinnedMesh;
    skinnedMesh.name = "Skinned Mesh Component on 3D Node";
    skinnedMesh.nodeType = "Node3D";
    skinnedMesh.components = {"SkinnedMesh"};
    skinnedMesh.description = "Creates a 3D node with a skinned mesh component for animated 3D models";
    skinnedMesh.isDefault = false;
    options.append(skinnedMesh);
    
    // Collision Mesh Component on 3D Node
    AssetCreationOption collisionMesh;
    collisionMesh.name = "Collision Mesh Component on 3D Node";
    collisionMesh.nodeType = "Node3D";
    collisionMesh.components = {"CollisionMesh3D"};
    collisionMesh.description = "Creates a 3D node with collision mesh for physics interactions";
    collisionMesh.isDefault = false;
    options.append(collisionMesh);

    // Static Mesh with Collision (Rigid Body)
    AssetCreationOption staticMeshRigid;
    staticMeshRigid.name = "Static Mesh with Collision (Rigid Body)";
    staticMeshRigid.nodeType = "Node3D";
    staticMeshRigid.components = {"StaticMesh", "CollisionMesh3D", "RigidBody3D"};
    staticMeshRigid.description = "Creates a 3D node with static mesh, collision, and rigid body for physics simulation";
    staticMeshRigid.isDefault = false;
    options.append(staticMeshRigid);

    // Static Mesh with Collision (Kinematic Body)
    AssetCreationOption staticMeshKinematic;
    staticMeshKinematic.name = "Static Mesh with Collision (Kinematic Body)";
    staticMeshKinematic.nodeType = "Node3D";
    staticMeshKinematic.components = {"StaticMesh", "CollisionMesh3D", "KinematicBody3D"};
    staticMeshKinematic.description = "Creates a 3D node with static mesh, collision, and kinematic body for controlled movement";
    staticMeshKinematic.isDefault = false;
    options.append(staticMeshKinematic);

    return options;
}

QList<AssetCreationOption> AssetDropHandler::getImageOptions() const {
    QList<AssetCreationOption> options;

    // Determine default based on current rendering context
    Lupine::RenderingContext context = Lupine::Renderer::GetRenderingContext();
    bool prefer3D = (context == Lupine::RenderingContext::Editor3D);

    // Sprite2D Component on 2D Node
    AssetCreationOption sprite2D;
    sprite2D.name = "Sprite2D Component on 2D Node";
    sprite2D.nodeType = "Node2D";
    sprite2D.components = {"Sprite2D"};
    sprite2D.description = "Creates a 2D node with a sprite component for 2D images";
    sprite2D.isDefault = !prefer3D; // Default in 2D mode
    options.append(sprite2D);

    // Sprite3D Component on 3D Node (Billboard)
    AssetCreationOption sprite3D;
    sprite3D.name = "Sprite3D Component on 3D Node";
    sprite3D.nodeType = "Node3D";
    sprite3D.components = {"Sprite3D"};
    sprite3D.description = "Creates a 3D billboard sprite that always faces the camera";
    sprite3D.isDefault = prefer3D; // Default in 3D mode
    options.append(sprite3D);

    // UI Texture Rectangle
    AssetCreationOption textureRect;
    textureRect.name = "Texture Rectangle (UI)";
    textureRect.nodeType = "Control";
    textureRect.components = {"TextureRectangle"};
    textureRect.description = "Creates a UI texture rectangle for interface elements";
    textureRect.isDefault = false;
    options.append(textureRect);

    return options;
}

QList<AssetCreationOption> AssetDropHandler::getSpriteAnimationOptions() const {
    QList<AssetCreationOption> options;
    
    // Default: AnimatedSprite2D Component on 2D Node
    AssetCreationOption animSprite2D;
    animSprite2D.name = "AnimatedSprite2D Component on 2D Node";
    animSprite2D.nodeType = "Node2D";
    animSprite2D.components = {"AnimatedSprite2D"};
    animSprite2D.description = "Creates a 2D node with animated sprite component";
    animSprite2D.isDefault = true;
    options.append(animSprite2D);
    
    // AnimatedSprite3D Component on 3D Node
    AssetCreationOption animSprite3D;
    animSprite3D.name = "AnimatedSprite3D Component on 3D Node";
    animSprite3D.nodeType = "Node3D";
    animSprite3D.components = {"AnimatedSprite3D"};
    animSprite3D.description = "Creates a 3D billboard animated sprite";
    animSprite3D.isDefault = false;
    options.append(animSprite3D);
    
    return options;
}

QList<AssetCreationOption> AssetDropHandler::getTilemapOptions() const {
    QList<AssetCreationOption> options;
    
    // Default: Tilemap2D Component on 2D Node
    AssetCreationOption tilemap2D;
    tilemap2D.name = "Tilemap2D Component on 2D Node";
    tilemap2D.nodeType = "Node2D";
    tilemap2D.components = {"Tilemap2D"};
    tilemap2D.description = "Creates a 2D tilemap for level design";
    tilemap2D.isDefault = true;
    options.append(tilemap2D);
    
    // Tilemap25D Component on 2D Node
    AssetCreationOption tilemap25D;
    tilemap25D.name = "Tilemap25D Component on 2D Node";
    tilemap25D.nodeType = "Node2D";
    tilemap25D.components = {"Tilemap25D"};
    tilemap25D.description = "Creates a 2.5D isometric tilemap";
    tilemap25D.isDefault = false;
    options.append(tilemap25D);
    
    // Tilemap3D Component on 3D Node
    AssetCreationOption tilemap3D;
    tilemap3D.name = "Tilemap3D Component on 3D Node";
    tilemap3D.nodeType = "Node3D";
    tilemap3D.components = {"Tilemap3D"};
    tilemap3D.description = "Creates a 3D voxel-style tilemap";
    tilemap3D.isDefault = false;
    options.append(tilemap3D);
    
    return options;
}

bool AssetDropHandler::isImageFile(const QString& filePath) const {
    QStringList imageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tiff", ".gif"};
    for (const QString& ext : imageExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetDropHandler::is3DModelFile(const QString& filePath) const {
    QStringList modelExtensions = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend"};
    for (const QString& ext : modelExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetDropHandler::isSpriteAnimationFile(const QString& filePath) const {
    return filePath.endsWith(".anim", Qt::CaseInsensitive) ||
           filePath.endsWith(".spriteanim", Qt::CaseInsensitive) ||
           filePath.contains("_anim", Qt::CaseInsensitive);
}

bool AssetDropHandler::isTilemapFile(const QString& filePath) const {
    return filePath.endsWith(".tilemap", Qt::CaseInsensitive) ||
           filePath.endsWith(".tmx", Qt::CaseInsensitive) ||
           filePath.endsWith(".tsx", Qt::CaseInsensitive);
}

std::unique_ptr<Lupine::Node> AssetDropHandler::createNodeForAsset(const QString& filePath,
                                                                  const AssetCreationOption& option,
                                                                  const QPoint& position) const {
    if (is3DModelFile(filePath)) {
        return create3DModelNode(filePath, option, position);
    } else if (isImageFile(filePath)) {
        return createImageNode(filePath, option, position);
    } else if (isSpriteAnimationFile(filePath)) {
        return createSpriteAnimationNode(filePath, option, position);
    } else if (isTilemapFile(filePath)) {
        return createTilemapNode(filePath, option, position);
    }

    return nullptr;
}

std::unique_ptr<Lupine::Node> AssetDropHandler::create3DModelNode(const QString& filePath,
                                                                 const AssetCreationOption& option,
                                                                 const QPoint& position) const {
    // Create the appropriate node type
    std::unique_ptr<Lupine::Node> node;
    if (option.nodeType == "Node3D") {
        node = std::make_unique<Lupine::Node3D>();
    } else {
        return nullptr;
    }

    // Set node name based on file
    QFileInfo fileInfo(filePath);
    node->SetName(fileInfo.baseName().toStdString());

    // Set position (convert from screen coordinates to world coordinates)
    glm::vec3 worldPos = screenToWorldPosition(position);
    if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node.get())) {
        node3d->SetPosition(worldPos);
    }

    // Add components
    for (const QString& componentName : option.components) {
        // Create and add the component using the component registry
        auto& registry = Lupine::ComponentRegistry::Instance();
        auto component = registry.CreateComponent(componentName.toStdString());
        if (component) {
            // Set asset path property for mesh components
            if (componentName == "StaticMesh") {
                auto* staticMesh = dynamic_cast<Lupine::StaticMesh*>(component.get());
                if (staticMesh) {
                    staticMesh->SetMeshPath(filePath.toStdString());
                    // Force component to load the mesh immediately
                    staticMesh->OnReady();
                }
            } else if (componentName == "SkinnedMesh") {
                auto* skinnedMesh = dynamic_cast<Lupine::SkinnedMesh*>(component.get());
                if (skinnedMesh) {
                    skinnedMesh->SetMeshPath(filePath.toStdString());
                    // Force component to load the mesh immediately
                    skinnedMesh->OnReady();
                }
            } else if (componentName == "CollisionMesh3D") {
                auto* collisionMesh = dynamic_cast<Lupine::CollisionMesh3D*>(component.get());
                if (collisionMesh) {
                    collisionMesh->SetMeshPath(filePath.toStdString());
                    // Force component to load the mesh immediately
                    collisionMesh->OnReady();
                }
            } else if (componentName == "RigidBody3D") {
                auto* rigidBody = dynamic_cast<Lupine::RigidBody3D*>(component.get());
                if (rigidBody) {
                    // Set up rigid body for mesh collision
                    rigidBody->SetCollisionShape(Lupine::CollisionShapeType::Mesh, glm::vec3(1.0f));
                    rigidBody->SetBodyType(Lupine::PhysicsBodyType::Dynamic);
                }
            } else if (componentName == "KinematicBody3D") {
                auto* kinematicBody = dynamic_cast<Lupine::KinematicBody3D*>(component.get());
                if (kinematicBody) {
                    // Set up kinematic body for mesh collision
                    kinematicBody->SetShapeType(Lupine::CollisionShapeType::Mesh);
                    kinematicBody->SetSize(glm::vec3(1.0f));
                }
            }
            node->AddComponent(std::move(component));
        }
    }

    return node;
}

std::unique_ptr<Lupine::Node> AssetDropHandler::createImageNode(const QString& filePath,
                                                               const AssetCreationOption& option,
                                                               const QPoint& position) const {
    // Create the appropriate node type
    std::unique_ptr<Lupine::Node> node;
    if (option.nodeType == "Node2D") {
        node = std::make_unique<Lupine::Node2D>();
    } else if (option.nodeType == "Node3D") {
        node = std::make_unique<Lupine::Node3D>();
    } else {
        // For Control nodes, would need to create appropriate UI node
        return nullptr;
    }

    // Set node name based on file
    QFileInfo fileInfo(filePath);
    node->SetName(fileInfo.baseName().toStdString());

    // Set position (convert from screen coordinates to world coordinates)
    glm::vec3 worldPos = screenToWorldPosition(position);
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node.get())) {
        node2d->SetPosition(glm::vec2(worldPos.x, worldPos.y));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node.get())) {
        node3d->SetPosition(worldPos);
    }

    // Add components and set asset path
    for (const QString& componentName : option.components) {
        auto& registry = Lupine::ComponentRegistry::Instance();
        auto component = registry.CreateComponent(componentName.toStdString());
        if (component) {
            // Set texture path for sprite components
            if (componentName == "Sprite2D") {
                auto* sprite2D = dynamic_cast<Lupine::Sprite2D*>(component.get());
                if (sprite2D) {
                    sprite2D->SetTexturePath(filePath.toStdString());
                }
            } else if (componentName == "Sprite3D") {
                auto* sprite3D = dynamic_cast<Lupine::Sprite3D*>(component.get());
                if (sprite3D) {
                    sprite3D->SetTexturePath(filePath.toStdString());
                }
            }
            node->AddComponent(std::move(component));
        }
    }

    return node;
}

std::unique_ptr<Lupine::Node> AssetDropHandler::createSpriteAnimationNode(const QString& filePath,
                                                                         const AssetCreationOption& option,
                                                                         const QPoint& position) const {
    // Create the appropriate node type
    std::unique_ptr<Lupine::Node> node;
    if (option.nodeType == "Node2D") {
        node = std::make_unique<Lupine::Node2D>();
    } else if (option.nodeType == "Node3D") {
        node = std::make_unique<Lupine::Node3D>();
    } else {
        return nullptr;
    }

    // Set node name based on file
    QFileInfo fileInfo(filePath);
    node->SetName(fileInfo.baseName().toStdString());

    // Set position (convert from screen coordinates to world coordinates)
    glm::vec3 worldPos = screenToWorldPosition(position);
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node.get())) {
        node2d->SetPosition(glm::vec2(worldPos.x, worldPos.y));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node.get())) {
        node3d->SetPosition(worldPos);
    }

    // Add animated sprite components
    for (const QString& componentName : option.components) {
        auto& registry = Lupine::ComponentRegistry::Instance();
        auto component = registry.CreateComponent(componentName.toStdString());
        if (component) {
            // Set animation path for animated sprite components
            if (componentName == "AnimatedSprite2D") {
                auto* animSprite2D = dynamic_cast<Lupine::AnimatedSprite2D*>(component.get());
                if (animSprite2D) {
                    animSprite2D->SetSpriteAnimationResource(filePath.toStdString());
                }
            } else if (componentName == "AnimatedSprite3D") {
                auto* animSprite3D = dynamic_cast<Lupine::AnimatedSprite3D*>(component.get());
                if (animSprite3D) {
                    animSprite3D->SetSpriteAnimationResource(filePath.toStdString());
                }
            }
            node->AddComponent(std::move(component));
        }
    }

    return node;
}

std::unique_ptr<Lupine::Node> AssetDropHandler::createTilemapNode(const QString& filePath,
                                                                 const AssetCreationOption& option,
                                                                 const QPoint& position) const {
    // Create the appropriate node type
    std::unique_ptr<Lupine::Node> node;
    if (option.nodeType == "Node2D") {
        node = std::make_unique<Lupine::Node2D>();
    } else if (option.nodeType == "Node3D") {
        node = std::make_unique<Lupine::Node3D>();
    } else {
        return nullptr;
    }

    // Set node name based on file
    QFileInfo fileInfo(filePath);
    node->SetName(fileInfo.baseName().toStdString());

    // Set position (convert from screen coordinates to world coordinates)
    glm::vec3 worldPos = screenToWorldPosition(position);
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node.get())) {
        node2d->SetPosition(glm::vec2(worldPos.x, worldPos.y));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node.get())) {
        node3d->SetPosition(worldPos);
    }

    // Add tilemap components
    for (const QString& componentName : option.components) {
        auto& registry = Lupine::ComponentRegistry::Instance();
        auto component = registry.CreateComponent(componentName.toStdString());
        if (component) {
            // Set tilemap path for tilemap components
            // Note: Tilemap component would need to be implemented
            // This is a placeholder for when tilemap components are added
            node->AddComponent(std::move(component));
        }
    }

    return node;
}

glm::vec3 AssetDropHandler::screenToWorldPosition(const QPoint& screenPos) const {
    if (!m_sceneViewPanel) {
        // Fallback to simple screen coordinate conversion
        return glm::vec3(screenPos.x() * 0.01f, -screenPos.y() * 0.01f, 0.0f);
    }

    // Use the scene view panel's coordinate conversion for consistency
    return m_sceneViewPanel->screenToWorldPosition(screenPos);
}
