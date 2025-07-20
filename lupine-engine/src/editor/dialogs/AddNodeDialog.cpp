#include "AddNodeDialog.h"
#include "../IconManager.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/components/AnimatedSprite3D.h"
#include "lupine/components/Label.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/components/Button.h"
#include "lupine/components/Panel.h"
#include "lupine/components/TextureRectangle.h"
#include "lupine/components/ColorRectangle.h"
#include "lupine/components/NinePatchPanel.h"
#include "lupine/components/ProgressBar.h"
#include "lupine/components/Sprite3D.h"
#include "lupine/components/OmniLight.h"
#include "lupine/components/DirectionalLight.h"
#include "lupine/components/SpotLight.h"
#include "lupine/components/AudioSource.h"
#include "lupine/components/Tilemap2D.h"
#include "lupine/components/Tilemap25D.h"
#include "lupine/components/Tilemap3D.h"
#include "lupine/components/Camera2D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/components/PlayerController2D.h"
#include "lupine/components/PlayerController3D.h"
#include "lupine/components/PlatformerController2D.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/components/CollisionPolygon2D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/Area2D.h"
#include "lupine/components/Area3D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/PlayerController25D.h"
#include "lupine/components/AnimationPlayer.h"
#include "lupine/core/Component.h"
#include <QApplication>
#include <QHeaderView>

AddNodeDialog::AddNodeDialog(QWidget* parent)
    : QDialog(parent)
    , m_selectedNodeType(nullptr)
    , m_loadingTimer(new QTimer(this))
{
    setWindowTitle("Add Node");
    setModal(true);
    resize(600, 400);

    initializeNodeTypes();
    setupUI();

    // Connect loading timer
    connect(m_loadingTimer, &QTimer::timeout, this, &AddNodeDialog::onLoadingTimer);

    // Don't populate nodes in constructor - do it when shown
}

const NodeTypeInfo* AddNodeDialog::getSelectedNodeType() const {
    return m_selectedNodeType;
}

void AddNodeDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Load nodes asynchronously when dialog is first shown
    if (!m_nodesLoaded && !m_isLoading) {
        loadNodesAsync();
    }
}

void AddNodeDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // Search layout
    m_searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search nodes...");
    m_searchLayout->addWidget(new QLabel("Search:"));
    m_searchLayout->addWidget(m_searchEdit);
    m_mainLayout->addLayout(m_searchLayout);
    
    // Splitter for tree and description
    m_splitter = new QSplitter(Qt::Horizontal);
    
    // Node tree
    m_nodeTree = new QTreeWidget();
    m_nodeTree->setHeaderLabel("Node Types");
    m_nodeTree->setRootIsDecorated(true);
    m_nodeTree->setAlternatingRowColors(true);
    m_nodeTree->header()->setStretchLastSection(true);
    m_splitter->addWidget(m_nodeTree);
    
    // Description panel
    QWidget* descriptionWidget = new QWidget();
    QVBoxLayout* descLayout = new QVBoxLayout(descriptionWidget);
    m_descriptionLabel = new QLabel("Description:");
    m_descriptionText = new QTextEdit();
    m_descriptionText->setReadOnly(true);
    m_descriptionText->setMaximumHeight(150);

    // Loading progress
    m_loadingProgress = new QProgressBar();
    m_loadingProgress->setVisible(false);
    m_loadingLabel = new QLabel("Loading nodes...");
    m_loadingLabel->setVisible(false);

    descLayout->addWidget(m_descriptionLabel);
    descLayout->addWidget(m_descriptionText);
    descLayout->addWidget(m_loadingLabel);
    descLayout->addWidget(m_loadingProgress);
    descLayout->addStretch();
    m_splitter->addWidget(descriptionWidget);
    
    m_splitter->setSizes({400, 200});
    m_mainLayout->addWidget(m_splitter);
    
    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    m_addButton = new QPushButton("Add");
    m_addButton->setEnabled(false);
    m_cancelButton = new QPushButton("Cancel");
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_nodeTree, &QTreeWidget::itemSelectionChanged, 
            this, &AddNodeDialog::onNodeSelectionChanged);
    connect(m_nodeTree, &QTreeWidget::itemDoubleClicked, 
            this, &AddNodeDialog::onNodeDoubleClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, 
            this, &AddNodeDialog::onSearchTextChanged);
    connect(m_addButton, &QPushButton::clicked, 
            this, &AddNodeDialog::onAddButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, 
            this, &AddNodeDialog::onCancelButtonClicked);
}

void AddNodeDialog::initializeNodeTypes() {
    m_nodeTypes.clear();
    
    // Basic node types
    m_nodeTypes.push_back(NodeTypeInfo("Node", "Basic",
        "Base node class. All other nodes inherit from this.", "Node"));
    m_nodeTypes.push_back(NodeTypeInfo("Node2D", "2D",
        "2D node with position, rotation, and scale properties.", "Node2D"));
    m_nodeTypes.push_back(NodeTypeInfo("Node3D", "3D",
        "3D node with position, rotation, and scale properties.", "Node3D"));
    m_nodeTypes.push_back(NodeTypeInfo("Control", "UI",
        "UI control node for screen-space elements with anchoring and margins.", "Control"));

    // 2D Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Sprite Node", "2D Templates",
        "Node2D with a Sprite2D component for displaying 2D textures.", "Node2D", true, QStringList{"Sprite2D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Animated Sprite 2D Node", "2D Templates",
        "Node2D with an AnimatedSprite2D component for displaying animated 2D sprites.", "Node2D", true, QStringList{"AnimatedSprite2D"}));

    // UI Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Label Node", "UI Templates",
        "Control node with a Label component for displaying text.", "Control", true, QStringList{"Label"}));
    m_nodeTypes.push_back(NodeTypeInfo("Button Node", "UI Templates",
        "Control node with a Button component for interactive buttons.", "Control", true, QStringList{"Button"}));
    m_nodeTypes.push_back(NodeTypeInfo("Panel Node", "UI Templates",
        "Control node with a Panel component for background containers.", "Control", true, QStringList{"Panel"}));
    m_nodeTypes.push_back(NodeTypeInfo("Texture Rectangle Node", "UI Templates",
        "Control node with a TextureRectangle component for displaying textures.", "Control", true, QStringList{"TextureRectangle"}));
    m_nodeTypes.push_back(NodeTypeInfo("Color Rectangle Node", "UI Templates",
        "Control node with a ColorRectangle component for solid color rectangles.", "Control", true, QStringList{"ColorRectangle"}));
    m_nodeTypes.push_back(NodeTypeInfo("Nine Patch Panel Node", "UI Templates",
        "Control node with a NinePatchPanel component for scalable UI panels.", "Control", true, QStringList{"NinePatchPanel"}));
    m_nodeTypes.push_back(NodeTypeInfo("Progress Bar Node", "UI Templates",
        "Control node with a ProgressBar component for displaying progress.", "Control", true, QStringList{"ProgressBar"}));

    // 3D Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Mesh Node", "3D Templates",
        "Node3D with a PrimitiveMesh component for displaying 3D shapes.", "Node3D", true, QStringList{"PrimitiveMesh"}));
    m_nodeTypes.push_back(NodeTypeInfo("Sprite3D Node", "3D Templates",
        "Node3D with a Sprite3D component for displaying 2D textures in 3D space.", "Node3D", true, QStringList{"Sprite3D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Animated Sprite 3D Node", "3D Templates",
        "Node3D with an AnimatedSprite3D component for displaying animated 2D sprites in 3D space.", "Node3D", true, QStringList{"AnimatedSprite3D"}));

    // Lighting Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Omni Light Node", "Lighting",
        "Node3D with an OmniLight component for omnidirectional point lighting.", "Node3D", true, QStringList{"OmniLight"}));
    m_nodeTypes.push_back(NodeTypeInfo("Directional Light Node", "Lighting",
        "Node3D with a DirectionalLight component for uniform directional lighting.", "Node3D", true, QStringList{"DirectionalLight"}));
    m_nodeTypes.push_back(NodeTypeInfo("Spot Light Node", "Lighting",
        "Node3D with a SpotLight component for cone-shaped lighting.", "Node3D", true, QStringList{"SpotLight"}));
    m_nodeTypes.push_back(NodeTypeInfo("Skybox Node", "Lighting",
        "Node3D with a Skybox3D component for 3D background rendering with solid color, panoramic textures, and procedural sky.", "Node3D", true, QStringList{"Skybox3D"}));

    // Camera Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Camera2D Node", "Camera",
        "Node2D with a Camera2D component for 2D camera control and view projection.", "Node2D", true, QStringList{"Camera2D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Camera3D Node", "Camera",
        "Node3D with a Camera3D component for 3D camera control and view projection.", "Node3D", true, QStringList{"Camera3D"}));

    // Player Controller Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Basic 2D Player Controller", "Player Controllers",
        "Node2D with AnimatedSprite2D, PlayerController2D, KinematicBody2D, and CollisionShape2D components for basic 2D player movement with sprite animations.", "Node2D", true, QStringList{"AnimatedSprite2D", "PlayerController2D", "KinematicBody2D", "CollisionShape2D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Platformer 2D Player Controller", "Player Controllers",
        "Node2D with AnimatedSprite2D, PlatformerController2D, KinematicBody2D, and CollisionShape2D components for 2D platformer movement with gravity, jumping, and sprite animations.", "Node2D", true, QStringList{"AnimatedSprite2D", "PlatformerController2D", "KinematicBody2D", "CollisionShape2D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Basic 3D Player Controller", "Player Controllers",
        "Node3D with PlayerController3D and KinematicBody3D components for basic 3D player movement with WASD and jump.", "Node3D", true, QStringList{"PlayerController3D", "KinematicBody3D"}));
    m_nodeTypes.push_back(NodeTypeInfo("3D Player with Static Mesh", "Player Controllers",
        "Node3D with StaticMesh, PlayerController3D, and KinematicBody3D components for 3D player with mesh rendering.", "Node3D", true, QStringList{"StaticMesh", "PlayerController3D", "KinematicBody3D"}));
    m_nodeTypes.push_back(NodeTypeInfo("3D Player with Sprite3D", "Player Controllers",
        "Node3D with Sprite3D, PlayerController3D, and KinematicBody3D components for 3D player with sprite rendering.", "Node3D", true, QStringList{"Sprite3D", "PlayerController3D", "KinematicBody3D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Player Controller 2.5D", "Player Controllers",
        "Node3D with AnimatedSprite3D, PlayerController2.5D, and KinematicBody3D components for 2.5D player with 3D movement and sprite sheet animations.", "Node3D", true, QStringList{"AnimatedSprite3D", "PlayerController25D", "KinematicBody3D"}));


    // Audio Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Audio Source Node", "Audio",
        "Node with an AudioSource component for playing sound effects and music.", "Node", true, QStringList{"AudioSource"}));
    m_nodeTypes.push_back(NodeTypeInfo("2D Audio Source Node", "Audio",
        "Node2D with an AudioSource component for 2D positional audio.", "Node2D", true, QStringList{"AudioSource"}));
    m_nodeTypes.push_back(NodeTypeInfo("3D Audio Source Node", "Audio",
        "Node3D with an AudioSource component for 3D spatial audio.", "Node3D", true, QStringList{"AudioSource"}));

    // Tilemap Template nodes
    m_nodeTypes.push_back(NodeTypeInfo("Tilemap2D Node", "Tilemap",
        "Node2D with a Tilemap2D component for sprite-based tile rendering.", "Node2D", true, QStringList{"Tilemap2D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Tilemap2.5D Node", "Tilemap",
        "Node2D with a Tilemap25D component for sprite3D-based tile rendering.", "Node2D", true, QStringList{"Tilemap25D"}));
    m_nodeTypes.push_back(NodeTypeInfo("Tilemap3D Node", "Tilemap",
        "Node3D with a Tilemap3D component for mesh-based tile rendering with frustum culling.", "Node3D", true, QStringList{"Tilemap3D"}));
}

void AddNodeDialog::loadNodesAsync() {
    if (m_isLoading || m_nodesLoaded) {
        return;
    }

    m_isLoading = true;
    m_loadingIndex = 0;

    // Show loading UI and hide node tree initially
    m_loadingLabel->setText("Loading nodes...");
    m_loadingLabel->setVisible(true);
    m_loadingProgress->setVisible(true);
    m_loadingProgress->setMaximum(static_cast<int>(m_nodeTypes.size()));
    m_nodeTree->setVisible(false);  // Hide tree while loading
    m_addButton->setEnabled(false);

    // Start loading timer for progressive UI updates
    m_loadingTimer->start(10); // Update every 10ms
}

void AddNodeDialog::onLoadingTimer() {
    if (!m_isLoading || m_nodesLoaded) {
        m_loadingTimer->stop();
        return;
    }

    // Process a few nodes per timer tick to keep UI responsive
    const size_t nodesPerTick = 3;
    size_t endIndex = std::min(m_loadingIndex + nodesPerTick, m_nodeTypes.size());

    // Update progress
    m_loadingProgress->setValue(static_cast<int>(endIndex));

    m_loadingIndex = endIndex;

    // Check if we're done loading
    if (m_loadingIndex >= m_nodeTypes.size()) {
        // Finish loading
        m_loadingTimer->stop();
        m_isLoading = false;
        m_nodesLoaded = true;

        // Hide loading UI and show node tree
        m_loadingLabel->setVisible(false);
        m_loadingProgress->setVisible(false);
        m_nodeTree->setVisible(true);   // Show tree when loading complete
        m_nodeTree->setEnabled(true);

        // Populate the tree with node data
        populateNodeTree();

        // Update UI state
        updateLoadingProgress();
    }
}

void AddNodeDialog::updateLoadingProgress() {
    // Re-enable UI elements after loading
    if (m_nodesLoaded) {
        m_nodeTree->setEnabled(true);
        // Add button will be enabled when a node is selected
    }
}

void AddNodeDialog::populateNodeTree() {
    if (!m_nodesLoaded) {
        return; // Don't populate until nodes are loaded
    }

    m_nodeTree->clear();
    m_categoryItems.clear();

    // Group nodes by category
    for (const auto& nodeType : m_nodeTypes) {
        // Find or create category item
        QTreeWidgetItem* categoryItem = findOrCreateCategoryItem(nodeType.category);

        // Create node item
        QTreeWidgetItem* nodeItem = new QTreeWidgetItem(categoryItem);
        nodeItem->setText(0, nodeType.name);
        nodeItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<const void*>(&nodeType)));
        nodeItem->setToolTip(0, nodeType.description);

        // Set icon using the new icon system with fallback
        std::string category = nodeType.category.toStdString();
        QIcon icon = IconManager::Instance().GetNodeIcon(nodeType.typeName.toStdString(), category);
        nodeItem->setIcon(0, icon);

    }
    
    // Expand all categories
    m_nodeTree->expandAll();
}

QTreeWidgetItem* AddNodeDialog::findOrCreateCategoryItem(const QString& category) {
    auto it = m_categoryItems.find(category.toStdString());
    if (it != m_categoryItems.end()) {
        return it->second;
    }
    
    QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_nodeTree);
    categoryItem->setText(0, category);
    categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsSelectable);
    categoryItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    
    // Make category items bold
    QFont font = categoryItem->font(0);
    font.setBold(true);
    categoryItem->setFont(0, font);
    
    m_categoryItems[category.toStdString()] = categoryItem;
    return categoryItem;
}

void AddNodeDialog::filterNodes(const QString& filter) {
    // Simple implementation - hide/show items based on filter
    for (int i = 0; i < m_nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* categoryItem = m_nodeTree->topLevelItem(i);
        bool categoryHasVisibleChildren = false;
        
        for (int j = 0; j < categoryItem->childCount(); ++j) {
            QTreeWidgetItem* nodeItem = categoryItem->child(j);
            bool matches = filter.isEmpty() || 
                          nodeItem->text(0).contains(filter, Qt::CaseInsensitive);
            nodeItem->setHidden(!matches);
            if (matches) {
                categoryHasVisibleChildren = true;
            }
        }
        
        categoryItem->setHidden(!categoryHasVisibleChildren);
    }
}

void AddNodeDialog::onNodeSelectionChanged() {
    QTreeWidgetItem* item = m_nodeTree->currentItem();
    if (!item || !item->parent()) {
        // Category item selected or no selection
        m_selectedNodeType = nullptr;
        m_addButton->setEnabled(false);
        m_descriptionText->clear();
        return;
    }
    
    // Get node type from item data
    QVariant itemData = item->data(0, Qt::UserRole);
    if (itemData.isValid()) {
        m_selectedNodeType = static_cast<const NodeTypeInfo*>(itemData.value<const void*>());
        m_addButton->setEnabled(true);
        
        // Update description
        if (m_selectedNodeType) {
            QString description = m_selectedNodeType->description;
            if (m_selectedNodeType->isTemplate && !m_selectedNodeType->components.isEmpty()) {
                description += "\n\nComponents: " + m_selectedNodeType->components.join(", ");
            }
            m_descriptionText->setPlainText(description);
        }
    } else {
        m_selectedNodeType = nullptr;
        m_addButton->setEnabled(false);
        m_descriptionText->clear();
    }
}

void AddNodeDialog::onNodeDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    if (item && item->parent() && m_selectedNodeType) {
        accept();
    }
}

void AddNodeDialog::onSearchTextChanged(const QString& text) {
    filterNodes(text);
}

void AddNodeDialog::onAddButtonClicked() {
    if (m_selectedNodeType) {
        accept();
    }
}

void AddNodeDialog::onCancelButtonClicked() {
    reject();
}

std::unique_ptr<Lupine::Node> AddNodeDialog::createNode(const NodeTypeInfo& nodeType, const std::string& name) {
    std::unique_ptr<Lupine::Node> node;

    // Create the base node based on type
    if (nodeType.typeName == "Node2D") {
        node = std::make_unique<Lupine::Node2D>(name);
    } else if (nodeType.typeName == "Node3D") {
        node = std::make_unique<Lupine::Node3D>(name);
    } else if (nodeType.typeName == "Control") {
        node = std::make_unique<Lupine::Control>(name);
    } else {
        node = std::make_unique<Lupine::Node>(name);
    }

    // Add components for template nodes
    if (nodeType.isTemplate && !nodeType.components.isEmpty()) {
        for (const QString& componentName : nodeType.components) {
            std::unique_ptr<Lupine::Component> component;

            if (componentName == "Sprite2D") {
                component = std::make_unique<Lupine::Sprite2D>();
            } else if (componentName == "AnimatedSprite2D") {
                component = std::make_unique<Lupine::AnimatedSprite2D>();
            } else if (componentName == "AnimatedSprite3D") {
                component = std::make_unique<Lupine::AnimatedSprite3D>();
            } else if (componentName == "Label") {
                component = std::make_unique<Lupine::Label>();
            } else if (componentName == "PrimitiveMesh") {
                component = std::make_unique<Lupine::PrimitiveMesh>();
            } else if (componentName == "Button") {
                component = std::make_unique<Lupine::Button>();
            } else if (componentName == "Panel") {
                component = std::make_unique<Lupine::Panel>();
            } else if (componentName == "TextureRectangle") {
                component = std::make_unique<Lupine::TextureRectangle>();
            } else if (componentName == "ColorRectangle") {
                component = std::make_unique<Lupine::ColorRectangle>();
            } else if (componentName == "NinePatchPanel") {
                component = std::make_unique<Lupine::NinePatchPanel>();
            } else if (componentName == "ProgressBar") {
                component = std::make_unique<Lupine::ProgressBar>();
            } else if (componentName == "Sprite3D") {
                component = std::make_unique<Lupine::Sprite3D>();
            } else if (componentName == "OmniLight") {
                component = std::make_unique<Lupine::OmniLight>();
            } else if (componentName == "DirectionalLight") {
                component = std::make_unique<Lupine::DirectionalLight>();
            } else if (componentName == "SpotLight") {
                component = std::make_unique<Lupine::SpotLight>();
            } else if (componentName == "Camera2D") {
                component = std::make_unique<Lupine::Camera2D>();
            } else if (componentName == "Camera3D") {
                component = std::make_unique<Lupine::Camera3D>();
            } else if (componentName == "PlayerController2D") {
                component = std::make_unique<Lupine::PlayerController2D>();
            } else if (componentName == "PlayerController3D") {
                component = std::make_unique<Lupine::PlayerController3D>();
            } else if (componentName == "PlatformerController2D") {
                component = std::make_unique<Lupine::PlatformerController2D>();
            } else if (componentName == "StaticMesh") {
                component = std::make_unique<Lupine::StaticMesh>();
            } else if (componentName == "CollisionShape2D") {
                auto collisionShape = std::make_unique<Lupine::CollisionShape2D>();
                // Set default size to 100x100 to match sprite size
                collisionShape->SetSize(glm::vec2(100.0f, 100.0f));
                component = std::move(collisionShape);
            } else if (componentName == "CollisionPolygon2D") {
                component = std::make_unique<Lupine::CollisionPolygon2D>();
            } else if (componentName == "CollisionMesh3D") {
                component = std::make_unique<Lupine::CollisionMesh3D>();
            } else if (componentName == "Area2D") {
                component = std::make_unique<Lupine::Area2D>();
            } else if (componentName == "Area3D") {
                component = std::make_unique<Lupine::Area3D>();
            } else if (componentName == "KinematicBody2D") {
                component = std::make_unique<Lupine::KinematicBody2D>();
            } else if (componentName == "KinematicBody3D") {
                component = std::make_unique<Lupine::KinematicBody3D>();
            } else if (componentName == "Tilemap2D") {
                component = std::make_unique<Lupine::Tilemap2D>();
            } else if (componentName == "Tilemap25D") {
                component = std::make_unique<Lupine::Tilemap25D>();
            } else if (componentName == "Tilemap3D") {
                component = std::make_unique<Lupine::Tilemap3D>();
            } else if (componentName == "AudioSource") {
                component = std::make_unique<Lupine::AudioSource>();
            } else if (componentName == "PlayerController25D") {
                component = std::make_unique<Lupine::PlayerController25D>();
            } else if (componentName == "AnimationPlayer") {
                component = std::make_unique<Lupine::AnimationPlayer>();
            }

            if (component) {
                node->AddComponent(std::move(component));
            }
        }
    }

    return node;
}
