#include "MenuBuilderDialog.h"
#include "MenuSceneView.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Button.h"
#include "lupine/components/Panel.h"
#include "lupine/components/Label.h"
#include "lupine/components/TextureRectangle.h"
#include "lupine/components/ColorRectangle.h"
#include <QApplication>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MenuBuilderDialog::MenuBuilderDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_palette(nullptr)
    , m_sceneView(nullptr)
    , m_rightTabs(nullptr)
    , m_inspector(nullptr)
    , m_eventBinding(nullptr)
    , m_menuScene(nullptr)
    , m_modified(false)
    , m_selectedElement(nullptr)
    , m_gridVisible(true)
    , m_snapToGrid(true)
    , m_gridSize(20.0f)
    , m_canvasSize(1920.0f, 1080.0f)
{
    setWindowTitle("Menu Builder");
    setMinimumSize(1400, 900);
    resize(1600, 1000);
    
    // Create new menu scene
    m_menuScene = std::make_unique<Lupine::Scene>("Menu");
    m_menuScene->CreateRootNode<Lupine::Control>("MenuRoot");
    
    setupUI();
    setupConnections();
    updateWindowTitle();
    updateActions();
}

MenuBuilderDialog::~MenuBuilderDialog()
{
}

void MenuBuilderDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    setupMenuBar();
    setupToolBar();
    setupMainLayout();
    setupStatusBar();
}

void MenuBuilderDialog::setupMenuBar()
{
    m_menuBar = new QMenuBar(this);
    m_menuBar->setMaximumHeight(50);
    m_mainLayout->addWidget(m_menuBar);
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Menu", this, &MenuBuilderDialog::onNewMenu, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open Menu...", this, &MenuBuilderDialog::onOpenMenu, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save Menu", this, &MenuBuilderDialog::onSaveMenu, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save Menu &As...", this, &MenuBuilderDialog::onSaveMenuAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_exportAction = fileMenu->addAction("&Export Menu...", this, &MenuBuilderDialog::onExportMenu);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &QDialog::close, QKeySequence("Ctrl+Q"));
    
    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    m_undoAction = editMenu->addAction("&Undo", this, &MenuBuilderDialog::onUndo, QKeySequence::Undo);
    m_redoAction = editMenu->addAction("&Redo", this, &MenuBuilderDialog::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    m_cutAction = editMenu->addAction("Cu&t", this, &MenuBuilderDialog::onCut, QKeySequence::Cut);
    m_copyAction = editMenu->addAction("&Copy", this, &MenuBuilderDialog::onCopy, QKeySequence::Copy);
    m_pasteAction = editMenu->addAction("&Paste", this, &MenuBuilderDialog::onPaste, QKeySequence::Paste);
    m_deleteAction = editMenu->addAction("&Delete", this, &MenuBuilderDialog::onDelete, QKeySequence::Delete);
    editMenu->addSeparator();
    m_duplicateAction = editMenu->addAction("D&uplicate", this, &MenuBuilderDialog::onDuplicate, QKeySequence("Ctrl+D"));
    
    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_resetViewAction = viewMenu->addAction("&Reset View", this, &MenuBuilderDialog::onResetView, QKeySequence("Ctrl+0"));
    m_fitToViewAction = viewMenu->addAction("&Fit to View", this, &MenuBuilderDialog::onFitToView, QKeySequence("Ctrl+F"));
    viewMenu->addSeparator();
    m_zoomInAction = viewMenu->addAction("Zoom &In", this, &MenuBuilderDialog::onZoomIn, QKeySequence::ZoomIn);
    m_zoomOutAction = viewMenu->addAction("Zoom &Out", this, &MenuBuilderDialog::onZoomOut, QKeySequence::ZoomOut);
    viewMenu->addSeparator();
    m_toggleGridAction = viewMenu->addAction("Toggle &Grid", this, &MenuBuilderDialog::onToggleGrid, QKeySequence("Ctrl+G"));
    m_toggleSnapAction = viewMenu->addAction("Toggle &Snap", this, &MenuBuilderDialog::onToggleSnap, QKeySequence("Ctrl+Shift+S"));
    
    // Set checkable actions
    m_toggleGridAction->setCheckable(true);
    m_toggleGridAction->setChecked(m_gridVisible);
    m_toggleSnapAction->setCheckable(true);
    m_toggleSnapAction->setChecked(m_snapToGrid);
}

void MenuBuilderDialog::setupToolBar()
{
    m_toolBar = new QToolBar(this);
    m_toolBar->setMaximumHeight(50);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_mainLayout->addWidget(m_toolBar);
    
    // Add common actions to toolbar
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_undoAction);
    m_toolBar->addAction(m_redoAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_cutAction);
    m_toolBar->addAction(m_copyAction);
    m_toolBar->addAction(m_pasteAction);
    m_toolBar->addAction(m_deleteAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_resetViewAction);
    m_toolBar->addAction(m_fitToViewAction);
    m_toolBar->addAction(m_toggleGridAction);
}

void MenuBuilderDialog::setupMainLayout()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
    
    setupPalette();
    setupSceneView();
    setupInspectorTabs();
    
    // Set splitter proportions
    m_mainSplitter->setSizes({300, 800, 400});
    m_mainSplitter->setStretchFactor(0, 0); // Palette fixed
    m_mainSplitter->setStretchFactor(1, 1); // Scene view stretches
    m_mainSplitter->setStretchFactor(2, 0); // Inspector fixed
}

void MenuBuilderDialog::setupPalette()
{
    m_palette = new ComponentPalette(this);
    m_mainSplitter->addWidget(m_palette);
}

void MenuBuilderDialog::setupSceneView()
{
    m_sceneView = new MenuSceneView(this);
    m_sceneView->setScene(m_menuScene.get());
    m_sceneView->setCanvasSize(m_canvasSize);
    m_sceneView->setGridVisible(m_gridVisible);
    m_sceneView->setSnapToGrid(m_snapToGrid);
    m_sceneView->setGridSize(m_gridSize);

    m_mainSplitter->addWidget(m_sceneView);
}

void MenuBuilderDialog::setupInspectorTabs()
{
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->addWidget(m_rightSplitter);
    
    m_rightTabs = new QTabWidget(this);
    m_rightSplitter->addWidget(m_rightTabs);
    
    // Inspector tab
    m_inspector = new MenuInspector(this);
    m_rightTabs->addTab(m_inspector, "Inspector");
    
    // Event binding tab
    m_eventBinding = new EventBindingPanel(this);
    m_rightTabs->addTab(m_eventBinding, "Events");
}

void MenuBuilderDialog::setupStatusBar()
{
    QWidget* statusWidget = new QWidget(this);
    statusWidget->setMaximumHeight(25);
    statusWidget->setStyleSheet("background-color: #3c3c3c; border-top: 1px solid #555;");
    
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(10, 2, 10, 2);
    
    QLabel* statusLabel = new QLabel("Ready", statusWidget);
    statusLabel->setStyleSheet("color: #ccc;");
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    m_mainLayout->addWidget(statusWidget);
}

void MenuBuilderDialog::setupConnections()
{
    // Connect palette signals
    connect(m_palette, &ComponentPalette::componentRequested,
            this, &MenuBuilderDialog::onElementAdded);

    // Connect scene view signals
    connect(m_sceneView, &MenuSceneView::nodeSelected,
            this, &MenuBuilderDialog::onElementSelected);
    connect(m_sceneView, &MenuSceneView::nodeDeselected,
            this, &MenuBuilderDialog::onElementDeselected);
    connect(m_sceneView, &MenuSceneView::nodeAdded,
            this, &MenuBuilderDialog::onElementAdded);
    connect(m_sceneView, &MenuSceneView::nodeMoved,
            this, &MenuBuilderDialog::onElementMoved);
    connect(m_sceneView, &MenuSceneView::nodeResized,
            this, &MenuBuilderDialog::onElementResized);

    // Connect inspector signals
    connect(m_inspector, &MenuInspector::propertyChanged,
            this, &MenuBuilderDialog::onPropertyChanged);

    // Connect event binding signals
    connect(m_eventBinding, &EventBindingPanel::eventBindingChanged,
            this, &MenuBuilderDialog::onEventBindingChanged);
}

void MenuBuilderDialog::updateWindowTitle()
{
    QString title = "Menu Builder";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    } else {
        title += " - Untitled";
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void MenuBuilderDialog::updateActions()
{
    bool hasSelection = (m_selectedElement != nullptr);
    bool hasClipboard = !m_clipboardData.isEmpty();
    
    m_cutAction->setEnabled(hasSelection);
    m_copyAction->setEnabled(hasSelection);
    m_pasteAction->setEnabled(hasClipboard);
    m_deleteAction->setEnabled(hasSelection);
    m_duplicateAction->setEnabled(hasSelection);
    
    // TODO: Implement undo/redo system
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
}

void MenuBuilderDialog::closeEvent(QCloseEvent* event)
{
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        event->ignore();
        return;
    }
    event->accept();
}

bool MenuBuilderDialog::hasUnsavedChanges() const
{
    return m_modified;
}

bool MenuBuilderDialog::promptSaveChanges()
{
    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<MenuBuilderDialog*>(this),
        "Unsaved Changes",
        "The menu has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );
    
    if (result == QMessageBox::Save) {
        onSaveMenu();
        return !m_modified; // Return true if save succeeded
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void MenuBuilderDialog::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

// File operation slots (basic implementations)
void MenuBuilderDialog::onNewMenu()
{
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }
    
    newMenu();
}

void MenuBuilderDialog::onOpenMenu()
{
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }
    
    openMenu();
}

void MenuBuilderDialog::onSaveMenu()
{
    saveMenu();
}

void MenuBuilderDialog::onSaveMenuAs()
{
    saveMenuAs();
}

void MenuBuilderDialog::onExportMenu()
{
    exportMenu();
}

// Placeholder implementations for file operations
void MenuBuilderDialog::newMenu()
{
    m_menuScene = std::make_unique<Lupine::Scene>("Menu");
    m_menuScene->CreateRootNode<Lupine::Control>("MenuRoot");
    m_currentFilePath.clear();
    m_selectedElement = nullptr;

    // Update scene view with new scene
    if (m_sceneView) {
        m_sceneView->setScene(m_menuScene.get());
        m_sceneView->setSelectedNode(nullptr);
        m_sceneView->resetCamera();
    }

    // Clear inspector and event binding panels
    if (m_inspector) {
        m_inspector->clearSelection();
    }
    if (m_eventBinding) {
        m_eventBinding->clearSelection();
    }

    setModified(false);
    updateWindowTitle();
    updateActions();
}

void MenuBuilderDialog::openMenu()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Menu", "", "Menu Files (*.menu);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (loadFromFile(filePath)) {
            m_currentFilePath = filePath;
            setModified(false);
            updateWindowTitle();
        }
    }
}

void MenuBuilderDialog::saveMenu()
{
    if (m_currentFilePath.isEmpty()) {
        saveMenuAs();
    } else {
        saveToFile(m_currentFilePath);
    }
}

void MenuBuilderDialog::saveMenuAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Menu As", "", "Menu Files (*.menu);;JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        if (saveToFile(filePath)) {
            m_currentFilePath = filePath;
            setModified(false);
            updateWindowTitle();
        }
    }
}

void MenuBuilderDialog::exportMenu()
{
    QMessageBox::information(this, "Export Menu", "Export functionality will be implemented in a future update.");
}

bool MenuBuilderDialog::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading: " + file.errorString());
        return false;
    }

    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "Error", "JSON parse error: " + error.errorString());
        return false;
    }

    QJsonObject root = doc.object();

    // Load canvas settings
    if (root.contains("canvas")) {
        QJsonObject canvas = root["canvas"].toObject();
        m_canvasSize = QSizeF(canvas["width"].toDouble(1920.0), canvas["height"].toDouble(1080.0));
        if (m_sceneView) {
            m_sceneView->setCanvasSize(m_canvasSize);
        }
    }

    // Load grid settings
    if (root.contains("grid")) {
        QJsonObject grid = root["grid"].toObject();
        m_gridVisible = grid["visible"].toBool(true);
        m_gridSize = grid["size"].toDouble(20.0);
        m_snapToGrid = grid["snap"].toBool(true);

        if (m_sceneView) {
            m_sceneView->setGridVisible(m_gridVisible);
            m_sceneView->setGridSize(m_gridSize);
            m_sceneView->setSnapToGrid(m_snapToGrid);
        }
    }

    // Create new scene
    m_menuScene = std::make_unique<Lupine::Scene>("Menu");
    auto* root_node = m_menuScene->CreateRootNode<Lupine::Control>("MenuRoot");

    // Load nodes
    if (root.contains("nodes") && root_node) {
        QJsonArray nodes = root["nodes"].toArray();
        loadNodesFromJson(nodes, root_node);
    }

    if (m_sceneView) {
        m_sceneView->setScene(m_menuScene.get());
    }

    return true;
}

bool MenuBuilderDialog::saveToFile(const QString& filePath)
{
    QJsonObject root;

    // Save canvas settings
    QJsonObject canvas;
    canvas["width"] = m_canvasSize.width();
    canvas["height"] = m_canvasSize.height();
    root["canvas"] = canvas;

    // Save grid settings
    QJsonObject grid;
    grid["visible"] = m_gridVisible;
    grid["size"] = m_gridSize;
    grid["snap"] = m_snapToGrid;
    root["grid"] = grid;

    // Save nodes
    if (m_menuScene && m_menuScene->GetRootNode()) {
        QJsonArray nodes;
        saveNodesToJson(m_menuScene->GetRootNode(), nodes);
        root["nodes"] = nodes;
    }

    // Save metadata
    QJsonObject metadata;
    metadata["version"] = "1.0";
    metadata["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["tool"] = "Lupine Menu Builder";
    root["metadata"] = metadata;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing: " + file.errorString());
        return false;
    }

    file.write(doc.toJson());
    return true;
}

// Placeholder implementations for other slots
void MenuBuilderDialog::onUndo() { /* TODO */ }
void MenuBuilderDialog::onRedo() { /* TODO */ }
void MenuBuilderDialog::onCut() { /* TODO */ }
void MenuBuilderDialog::onCopy() { /* TODO */ }
void MenuBuilderDialog::onPaste() { /* TODO */ }
void MenuBuilderDialog::onDelete() { /* TODO */ }
void MenuBuilderDialog::onDuplicate() { /* TODO */ }

void MenuBuilderDialog::onResetView()
{
    if (m_sceneView) {
        m_sceneView->resetCamera();
    }
}

void MenuBuilderDialog::onFitToView()
{
    if (m_sceneView) {
        m_sceneView->fitToView();
    }
}

void MenuBuilderDialog::onZoomIn()
{
    if (m_sceneView) {
        m_sceneView->zoomIn();
    }
}

void MenuBuilderDialog::onZoomOut()
{
    if (m_sceneView) {
        m_sceneView->zoomOut();
    }
}

void MenuBuilderDialog::onToggleGrid()
{
    m_gridVisible = !m_gridVisible;
    m_toggleGridAction->setChecked(m_gridVisible);
    if (m_sceneView) {
        m_sceneView->setGridVisible(m_gridVisible);
    }
}

void MenuBuilderDialog::onToggleSnap()
{
    m_snapToGrid = !m_snapToGrid;
    m_toggleSnapAction->setChecked(m_snapToGrid);
    if (m_sceneView) {
        m_sceneView->setSnapToGrid(m_snapToGrid);
    }
}
void MenuBuilderDialog::onElementSelected(Lupine::Node* node)
{
    m_selectedElement = node;
    updateActions();

    // Update inspector and event binding panels
    if (m_inspector) {
        m_inspector->setSelectedNode(node);
    }
    if (m_eventBinding) {
        m_eventBinding->setSelectedNode(node);
    }
    if (m_sceneView) {
        m_sceneView->setSelectedNode(node);
    }
}

void MenuBuilderDialog::onElementDeselected()
{
    m_selectedElement = nullptr;
    updateActions();

    // Clear inspector and event binding panels
    if (m_inspector) {
        m_inspector->clearSelection();
    }
    if (m_eventBinding) {
        m_eventBinding->clearSelection();
    }
    if (m_sceneView) {
        m_sceneView->setSelectedNode(nullptr);
    }
}
void MenuBuilderDialog::onElementAdded(const QString& componentType, const QPointF& position)
{
    if (!m_menuScene) return;

    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Handle prefabs and templates
    if (componentType == "Main Menu") {
        createMainMenuPrefab(position);
    } else if (componentType == "Settings Panel") {
        createSettingsPanelPrefab(position);
    } else if (componentType == "Dialog Box") {
        createDialogBoxPrefab(position);
    } else if (componentType == "Button Group") {
        createButtonGroupTemplate(position);
    } else if (componentType == "Form Layout") {
        createFormLayoutTemplate(position);
    } else if (componentType == "Button + Label") {
        createButtonLabelTemplate(position);
    } else if (componentType == "Panel + Background") {
        createPanelBackgroundTemplate(position);
    } else {
        // Create single component
        createSingleComponent(componentType, position);
    }

    setModified(true);
}

void MenuBuilderDialog::createSingleComponent(const QString& componentType, const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create node name
    QString nodeName = componentType + QString::number(root->GetChildCount() + 1);
    auto* newNode = root->CreateChild<Lupine::Control>(nodeName.toStdString());
    if (!newNode) return;

    // Set position
    newNode->SetPosition(glm::vec2(position.x(), position.y()));

    // Add appropriate component based on type with better defaults
    if (componentType == "Button") {
        newNode->SetSize(glm::vec2(120.0f, 40.0f));
        auto* button = newNode->AddComponent<Lupine::Button>();
        if (button) {
            button->SetText("Button");
        }
    } else if (componentType == "Panel") {
        newNode->SetSize(glm::vec2(200.0f, 150.0f));
        auto* panel = newNode->AddComponent<Lupine::Panel>();
        if (panel) {
            panel->SetBackgroundColor(glm::vec4(0.2f, 0.2f, 0.2f, 0.9f));
        }
    } else if (componentType == "Label") {
        newNode->SetSize(glm::vec2(100.0f, 30.0f));
        auto* label = newNode->AddComponent<Lupine::Label>();
        if (label) {
            label->SetText("Label");
        }
    } else if (componentType == "TextureRectangle") {
        newNode->SetSize(glm::vec2(100.0f, 100.0f));
        auto* texRect = newNode->AddComponent<Lupine::TextureRectangle>();
        Q_UNUSED(texRect);
    } else if (componentType == "ColorRectangle") {
        newNode->SetSize(glm::vec2(100.0f, 100.0f));
        auto* colorRect = newNode->AddComponent<Lupine::ColorRectangle>();
        if (colorRect) {
            colorRect->SetColor(glm::vec4(0.5f, 0.5f, 0.8f, 1.0f));
        }
    } else if (componentType == "NinePatchPanel") {
        newNode->SetSize(glm::vec2(150.0f, 100.0f));
        // Use Panel as placeholder for NinePatchPanel
        auto* panel = newNode->AddComponent<Lupine::Panel>();
        if (panel) {
            panel->SetBackgroundColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }
    } else if (componentType == "ProgressBar") {
        newNode->SetSize(glm::vec2(200.0f, 20.0f));
        // Use ColorRectangle as placeholder for ProgressBar
        auto* progressRect = newNode->AddComponent<Lupine::ColorRectangle>();
        if (progressRect) {
            progressRect->SetColor(glm::vec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green progress bar
        }
    } else {
        // Default Control node
        newNode->SetSize(glm::vec2(100.0f, 50.0f));
    }

    // Select the new node
    onElementSelected(newNode);
}
void MenuBuilderDialog::onElementMoved(Lupine::Node* node, const QPointF& newPosition) { Q_UNUSED(node); Q_UNUSED(newPosition); setModified(true); }
void MenuBuilderDialog::onElementResized(Lupine::Node* node, const QSizeF& newSize) { Q_UNUSED(node); Q_UNUSED(newSize); setModified(true); }
void MenuBuilderDialog::onPropertyChanged(const QString& propertyName, const QVariant& value) { Q_UNUSED(propertyName); Q_UNUSED(value); setModified(true); }
void MenuBuilderDialog::onEventBindingChanged(const QString& eventName, const QString& action) { Q_UNUSED(eventName); Q_UNUSED(action); setModified(true); }

// ComponentPalette Implementation
ComponentPalette::ComponentPalette(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_tabWidget(nullptr)
    , m_prefabsList(nullptr)
    , m_templatesList(nullptr)
    , m_allNodesList(nullptr)
    , m_dragItem(nullptr)
{
    setupUI();
}

void ComponentPalette::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);

    // Title
    QLabel* title = new QLabel("Component Palette", this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; color: #fff; padding: 5px;");
    m_layout->addWidget(title);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    m_layout->addWidget(m_tabWidget);

    setupPrefabsTab();
    setupTemplatesTab();
    setupAllNodesTab();

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ComponentPalette::onTabChanged);
}

void ComponentPalette::setupPrefabsTab()
{
    m_prefabsList = new QListWidget();
    m_prefabsList->setDragDropMode(QAbstractItemView::DragOnly);
    m_prefabsList->setDefaultDropAction(Qt::CopyAction);

    // Add common UI prefabs
    QStringList prefabs = {
        "Main Menu",
        "Settings Panel",
        "Dialog Box",
        "Button Group",
        "Form Layout"
    };

    populateComponentList(m_prefabsList, prefabs);
    connect(m_prefabsList, &QListWidget::itemDoubleClicked, this, &ComponentPalette::onItemDoubleClicked);

    m_tabWidget->addTab(m_prefabsList, "Prefabs");
}

void ComponentPalette::setupTemplatesTab()
{
    m_templatesList = new QListWidget();
    m_templatesList->setDragDropMode(QAbstractItemView::DragOnly);
    m_templatesList->setDefaultDropAction(Qt::CopyAction);

    // Add UI template nodes
    QStringList templates = {
        "Button + Label",
        "Panel + Background",
        "Image + Text",
        "Slider + Value",
        "Checkbox + Label",
        "Input Field",
        "Dropdown Menu"
    };

    populateComponentList(m_templatesList, templates);
    connect(m_templatesList, &QListWidget::itemDoubleClicked, this, &ComponentPalette::onItemDoubleClicked);

    m_tabWidget->addTab(m_templatesList, "Templates");
}

void ComponentPalette::setupAllNodesTab()
{
    m_allNodesList = new QListWidget();
    m_allNodesList->setDragDropMode(QAbstractItemView::DragOnly);
    m_allNodesList->setDefaultDropAction(Qt::CopyAction);

    // Add all available UI components
    QStringList allNodes = {
        "Control",
        "Button",
        "Panel",
        "Label",
        "TextureRectangle",
        "ColorRectangle",
        "NinePatchPanel",
        "ProgressBar"
    };

    populateComponentList(m_allNodesList, allNodes);
    connect(m_allNodesList, &QListWidget::itemDoubleClicked, this, &ComponentPalette::onItemDoubleClicked);

    m_tabWidget->addTab(m_allNodesList, "All Nodes");
}

void ComponentPalette::populateComponentList(QListWidget* list, const QStringList& components)
{
    for (const QString& component : components) {
        QListWidgetItem* item = new QListWidgetItem(component, list);
        item->setData(Qt::UserRole, component);
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled);

        // Set icon based on component type
        if (component == "Button") {
            item->setIcon(QIcon(":/icons/button.png"));
        } else if (component == "Panel") {
            item->setIcon(QIcon(":/icons/panel.png"));
        } else if (component == "Label") {
            item->setIcon(QIcon(":/icons/label.png"));
        } else {
            item->setIcon(QIcon(":/icons/component.png"));
        }
    }

    // Enable drag and drop for the list
    list->setDragDropMode(QAbstractItemView::DragOnly);
    list->setDefaultDropAction(Qt::CopyAction);
}

void ComponentPalette::onTabChanged(int index)
{
    Q_UNUSED(index);
    // Handle tab change if needed
}

void ComponentPalette::onItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        QString componentType = item->data(Qt::UserRole).toString();
        emit componentRequested(componentType, QPointF(0, 0));
    }
}

void ComponentPalette::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();

        // Find which list widget contains the mouse position
        QListWidget* currentList = nullptr;
        if (m_tabWidget->currentIndex() == 0) {
            currentList = m_prefabsList;
        } else if (m_tabWidget->currentIndex() == 1) {
            currentList = m_templatesList;
        } else if (m_tabWidget->currentIndex() == 2) {
            currentList = m_allNodesList;
        }

        if (currentList) {
            QPoint listPos = currentList->mapFromParent(event->pos());
            m_dragItem = currentList->itemAt(listPos);
        }
    }
    QWidget::mousePressEvent(event);
}

void ComponentPalette::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (m_dragItem) {
        startDrag(m_dragItem);
    }

    QWidget::mouseMoveEvent(event);
}

void ComponentPalette::startDrag(QListWidgetItem* item)
{
    if (!item) return;

    QString componentType = item->data(Qt::UserRole).toString();

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData();
    mimeData->setText(componentType);
    drag->setMimeData(mimeData);

    // Create drag pixmap
    QPixmap pixmap(100, 30);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(QColor(100, 100, 200, 180)));
    painter.setPen(QPen(QColor(50, 50, 150), 2));
    painter.drawRoundedRect(pixmap.rect().adjusted(1, 1, -1, -1), 5, 5);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, componentType);
    painter.end();

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(50, 15));

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    Q_UNUSED(dropAction);
}

// JSON serialization helper methods
void MenuBuilderDialog::saveNodesToJson(Lupine::Node* node, QJsonArray& array)
{
    if (!node) return;

    // Save this node
    QJsonObject nodeObj = saveNodeToJson(node);
    array.append(nodeObj);

    // Save children
    for (size_t i = 0; i < node->GetChildCount(); ++i) {
        auto* child = node->GetChild(i);
        if (child) {
            saveNodesToJson(child, array);
        }
    }
}

void MenuBuilderDialog::loadNodesFromJson(const QJsonArray& array, Lupine::Node* parent)
{
    if (!parent) return;

    for (const QJsonValue& value : array) {
        if (value.isObject()) {
            QJsonObject nodeObj = value.toObject();
            loadNodeFromJson(nodeObj, parent);
        }
    }
}

QJsonObject MenuBuilderDialog::saveNodeToJson(Lupine::Node* node)
{
    QJsonObject obj;

    if (!node) return obj;

    // Basic node properties
    obj["name"] = QString::fromStdString(node->GetName());
    obj["uuid"] = QString::fromStdString(node->GetUUID().ToString());
    obj["type"] = "Control"; // For now, assume all nodes are Control nodes

    // Control-specific properties
    if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        QJsonObject transform;
        glm::vec2 pos = control->GetPosition();
        glm::vec2 size = control->GetSize();

        transform["position"] = QJsonArray{pos.x, pos.y};
        transform["size"] = QJsonArray{size.x, size.y};
        obj["transform"] = transform;
    }

    // Save components
    QJsonArray components;
    // TODO: Iterate through components and save their properties
    // For now, we'll save basic component types

    // Check for Button component
    if (node->GetComponent<Lupine::Button>()) {
        QJsonObject buttonComp;
        buttonComp["type"] = "Button";
        auto* button = node->GetComponent<Lupine::Button>();
        buttonComp["text"] = QString::fromStdString(button->GetText());
        components.append(buttonComp);
    }

    // Check for Panel component
    if (node->GetComponent<Lupine::Panel>()) {
        QJsonObject panelComp;
        panelComp["type"] = "Panel";
        auto* panel = node->GetComponent<Lupine::Panel>();
        glm::vec4 color = panel->GetBackgroundColor();
        panelComp["backgroundColor"] = QJsonArray{color.r, color.g, color.b, color.a};
        components.append(panelComp);
    }

    // Check for Label component
    if (node->GetComponent<Lupine::Label>()) {
        QJsonObject labelComp;
        labelComp["type"] = "Label";
        auto* label = node->GetComponent<Lupine::Label>();
        labelComp["text"] = QString::fromStdString(label->GetText());
        components.append(labelComp);
    }

    // Check for ColorRectangle component
    if (node->GetComponent<Lupine::ColorRectangle>()) {
        QJsonObject colorRectComp;
        colorRectComp["type"] = "ColorRectangle";
        auto* colorRect = node->GetComponent<Lupine::ColorRectangle>();
        glm::vec4 color = colorRect->GetColor();
        colorRectComp["color"] = QJsonArray{color.r, color.g, color.b, color.a};
        components.append(colorRectComp);
    }

    // Check for TextureRectangle component
    if (node->GetComponent<Lupine::TextureRectangle>()) {
        QJsonObject texRectComp;
        texRectComp["type"] = "TextureRectangle";
        auto* texRect = node->GetComponent<Lupine::TextureRectangle>();
        texRectComp["texturePath"] = QString::fromStdString(texRect->GetTexturePath());
        components.append(texRectComp);
    }

    obj["components"] = components;

    return obj;
}

Lupine::Node* MenuBuilderDialog::loadNodeFromJson(const QJsonObject& obj, Lupine::Node* parent)
{
    if (!parent) return nullptr;

    QString name = obj["name"].toString();
    QString type = obj["type"].toString();

    // Create node based on type
    Lupine::Node* newNode = nullptr;
    if (type == "Control") {
        newNode = parent->CreateChild<Lupine::Control>(name.toStdString());
    }

    if (!newNode) return nullptr;

    // Load transform
    if (obj.contains("transform")) {
        QJsonObject transform = obj["transform"].toObject();

        if (auto* control = dynamic_cast<Lupine::Control*>(newNode)) {
            if (transform.contains("position")) {
                QJsonArray pos = transform["position"].toArray();
                control->SetPosition(glm::vec2(pos[0].toDouble(), pos[1].toDouble()));
            }

            if (transform.contains("size")) {
                QJsonArray size = transform["size"].toArray();
                control->SetSize(glm::vec2(size[0].toDouble(), size[1].toDouble()));
            }
        }
    }

    // Load components
    if (obj.contains("components")) {
        QJsonArray components = obj["components"].toArray();

        for (const QJsonValue& compValue : components) {
            QJsonObject compObj = compValue.toObject();
            QString compType = compObj["type"].toString();

            if (compType == "Button") {
                auto* button = newNode->AddComponent<Lupine::Button>();
                if (button && compObj.contains("text")) {
                    button->SetText(compObj["text"].toString().toStdString());
                }
            } else if (compType == "Panel") {
                auto* panel = newNode->AddComponent<Lupine::Panel>();
                if (panel && compObj.contains("backgroundColor")) {
                    QJsonArray color = compObj["backgroundColor"].toArray();
                    panel->SetBackgroundColor(glm::vec4(color[0].toDouble(), color[1].toDouble(),
                                                       color[2].toDouble(), color[3].toDouble()));
                }
            } else if (compType == "Label") {
                auto* label = newNode->AddComponent<Lupine::Label>();
                if (label && compObj.contains("text")) {
                    label->SetText(compObj["text"].toString().toStdString());
                }
            } else if (compType == "ColorRectangle") {
                auto* colorRect = newNode->AddComponent<Lupine::ColorRectangle>();
                if (colorRect && compObj.contains("color")) {
                    QJsonArray color = compObj["color"].toArray();
                    colorRect->SetColor(glm::vec4(color[0].toDouble(), color[1].toDouble(),
                                                 color[2].toDouble(), color[3].toDouble()));
                }
            } else if (compType == "TextureRectangle") {
                auto* texRect = newNode->AddComponent<Lupine::TextureRectangle>();
                if (texRect && compObj.contains("texturePath")) {
                    texRect->SetTexturePath(compObj["texturePath"].toString().toStdString());
                }
            }
        }
    }

    return newNode;
}

// MenuInspector Implementation
MenuInspector::MenuInspector(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_scrollArea(nullptr)
    , m_propertiesWidget(nullptr)
    , m_propertiesLayout(nullptr)
    , m_selectedNode(nullptr)
{
    setupUI();
}

void MenuInspector::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);

    // Title
    QLabel* title = new QLabel("Inspector", this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; color: #fff; padding: 5px;");
    m_layout->addWidget(title);

    // Scroll area for properties
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_propertiesWidget = new QWidget();
    m_propertiesLayout = new QVBoxLayout(m_propertiesWidget);
    m_propertiesLayout->setContentsMargins(5, 5, 5, 5);
    m_propertiesLayout->addStretch();

    m_scrollArea->setWidget(m_propertiesWidget);
    m_layout->addWidget(m_scrollArea);

    // No selection message
    QLabel* noSelectionLabel = new QLabel("No element selected", m_propertiesWidget);
    noSelectionLabel->setAlignment(Qt::AlignCenter);
    noSelectionLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
    m_propertiesLayout->insertWidget(0, noSelectionLabel);
}

void MenuInspector::setSelectedNode(Lupine::Node* node)
{
    m_selectedNode = node;
    updateProperties();
}

void MenuInspector::clearSelection()
{
    m_selectedNode = nullptr;
    updateProperties();
}

void MenuInspector::updateProperties()
{
    // Clear existing property editors
    for (auto it = m_propertyEditors.begin(); it != m_propertyEditors.end(); ++it) {
        it.value()->deleteLater();
    }
    m_propertyEditors.clear();

    // Clear layout
    QLayoutItem* item;
    while ((item = m_propertiesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (!m_selectedNode) {
        QLabel* noSelectionLabel = new QLabel("No element selected", m_propertiesWidget);
        noSelectionLabel->setAlignment(Qt::AlignCenter);
        noSelectionLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
        m_propertiesLayout->addWidget(noSelectionLabel);
        m_propertiesLayout->addStretch();
        return;
    }

    // Node properties
    QGroupBox* nodeGroup = new QGroupBox("Node Properties", m_propertiesWidget);
    QFormLayout* nodeLayout = new QFormLayout(nodeGroup);

    // Name
    QLineEdit* nameEdit = new QLineEdit(QString::fromStdString(m_selectedNode->GetName()));
    nodeLayout->addRow("Name:", nameEdit);
    m_propertyEditors["name"] = nameEdit;
    connect(nameEdit, &QLineEdit::textChanged, this, &MenuInspector::onPropertyValueChanged);

    // UUID (read-only)
    QLineEdit* uuidEdit = new QLineEdit(QString::fromStdString(m_selectedNode->GetUUID().ToString()));
    uuidEdit->setReadOnly(true);
    nodeLayout->addRow("UUID:", uuidEdit);

    m_propertiesLayout->addWidget(nodeGroup);

    // Transform properties for Control nodes
    if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
        QGroupBox* transformGroup = new QGroupBox("Transform", m_propertiesWidget);
        QFormLayout* transformLayout = new QFormLayout(transformGroup);

        // Position
        QWidget* posWidget = new QWidget();
        QHBoxLayout* posLayout = new QHBoxLayout(posWidget);
        posLayout->setContentsMargins(0, 0, 0, 0);

        QDoubleSpinBox* posX = new QDoubleSpinBox();
        posX->setRange(-9999, 9999);
        posX->setValue(control->GetPosition().x);
        QDoubleSpinBox* posY = new QDoubleSpinBox();
        posY->setRange(-9999, 9999);
        posY->setValue(control->GetPosition().y);

        posLayout->addWidget(new QLabel("X:"));
        posLayout->addWidget(posX);
        posLayout->addWidget(new QLabel("Y:"));
        posLayout->addWidget(posY);

        transformLayout->addRow("Position:", posWidget);
        m_propertyEditors["position_x"] = posX;
        m_propertyEditors["position_y"] = posY;
        connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MenuInspector::onPropertyValueChanged);
        connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MenuInspector::onPropertyValueChanged);

        // Size
        QWidget* sizeWidget = new QWidget();
        QHBoxLayout* sizeLayout = new QHBoxLayout(sizeWidget);
        sizeLayout->setContentsMargins(0, 0, 0, 0);

        QDoubleSpinBox* sizeW = new QDoubleSpinBox();
        sizeW->setRange(0, 9999);
        sizeW->setValue(control->GetSize().x);
        QDoubleSpinBox* sizeH = new QDoubleSpinBox();
        sizeH->setRange(0, 9999);
        sizeH->setValue(control->GetSize().y);

        sizeLayout->addWidget(new QLabel("W:"));
        sizeLayout->addWidget(sizeW);
        sizeLayout->addWidget(new QLabel("H:"));
        sizeLayout->addWidget(sizeH);

        transformLayout->addRow("Size:", sizeWidget);
        m_propertyEditors["size_w"] = sizeW;
        m_propertyEditors["size_h"] = sizeH;
        connect(sizeW, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MenuInspector::onPropertyValueChanged);
        connect(sizeH, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MenuInspector::onPropertyValueChanged);

        m_propertiesLayout->addWidget(transformGroup);
    }

    m_propertiesLayout->addStretch();
}

void MenuInspector::onPropertyValueChanged()
{
    if (!m_selectedNode) return;

    QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
    if (!sender) return;

    // Find the property name
    QString propertyName;
    for (auto it = m_propertyEditors.begin(); it != m_propertyEditors.end(); ++it) {
        if (it.value() == sender) {
            propertyName = it.key();
            break;
        }
    }

    if (propertyName.isEmpty()) return;

    // Get the value and emit signal
    QVariant value;
    if (auto* lineEdit = qobject_cast<QLineEdit*>(sender)) {
        value = lineEdit->text();
    } else if (auto* spinBox = qobject_cast<QDoubleSpinBox*>(sender)) {
        value = spinBox->value();
    } else if (auto* checkBox = qobject_cast<QCheckBox*>(sender)) {
        value = checkBox->isChecked();
    }

    emit propertyChanged(propertyName, value);
}

void MenuInspector::createPropertyEditor(const QString& name, const QVariant& value, const QString& type)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    Q_UNUSED(type);
    // This method can be used for dynamic property creation in the future
}

// EventBindingPanel Implementation
EventBindingPanel::EventBindingPanel(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_bindingsTree(nullptr)
    , m_eventCombo(nullptr)
    , m_actionCombo(nullptr)
    , m_addButton(nullptr)
    , m_removeButton(nullptr)
    , m_selectedNode(nullptr)
{
    setupUI();
    populatePresetActions();
}

void EventBindingPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);

    // Title
    QLabel* title = new QLabel("Event Bindings", this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; color: #fff; padding: 5px;");
    m_layout->addWidget(title);

    // Event bindings tree
    m_bindingsTree = new QTreeWidget(this);
    m_bindingsTree->setHeaderLabels({"Event", "Action"});
    m_bindingsTree->setRootIsDecorated(false);
    m_bindingsTree->setAlternatingRowColors(true);
    m_layout->addWidget(m_bindingsTree);

    // Add binding controls
    QGroupBox* addGroup = new QGroupBox("Add Event Binding", this);
    QFormLayout* addLayout = new QFormLayout(addGroup);

    m_eventCombo = new QComboBox();
    m_eventCombo->addItems({"OnClick", "OnHover", "OnPress", "OnRelease", "OnFocus", "OnBlur"});
    addLayout->addRow("Event:", m_eventCombo);

    m_actionCombo = new QComboBox();
    m_actionCombo->setEditable(true);
    addLayout->addRow("Action:", m_actionCombo);

    QWidget* buttonWidget = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_addButton = new QPushButton("Add");
    m_removeButton = new QPushButton("Remove");
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();

    addLayout->addRow(buttonWidget);
    m_layout->addWidget(addGroup);

    // Connect signals
    connect(m_addButton, &QPushButton::clicked, this, &EventBindingPanel::onAddBinding);
    connect(m_removeButton, &QPushButton::clicked, this, &EventBindingPanel::onRemoveBinding);
    connect(m_bindingsTree, &QTreeWidget::itemSelectionChanged, this, &EventBindingPanel::onEventBindingChanged);

    // No selection message
    QLabel* noSelectionLabel = new QLabel("No element selected", this);
    noSelectionLabel->setAlignment(Qt::AlignCenter);
    noSelectionLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
    m_layout->addWidget(noSelectionLabel);

    updateEventBindings();
}

void EventBindingPanel::populatePresetActions()
{
    m_presetActions = {
        "ChangeScene(\"scenes/main_menu.scene\")",
        "ChangeScene(\"scenes/game.scene\")",
        "ChangeScene(\"scenes/settings.scene\")",
        "QuitGame()",
        "PlaySound(\"sounds/click.wav\")",
        "PlaySound(\"sounds/hover.wav\")",
        "ShowDialog(\"Are you sure?\")",
        "HideDialog()",
        "SetVisible(false)",
        "SetVisible(true)",
        "Animate(\"fade_in\")",
        "Animate(\"fade_out\")",
        "Animate(\"scale_up\")",
        "Animate(\"scale_down\")",
        "SetGlobalVariable(\"key\", \"value\")",
        "GetGlobalVariable(\"key\")",
        "SaveGame()",
        "LoadGame()",
        "OpenURL(\"https://example.com\")",
        "Custom Code..."
    };

    m_actionCombo->addItems(m_presetActions);
}

void EventBindingPanel::setSelectedNode(Lupine::Node* node)
{
    m_selectedNode = node;
    updateEventBindings();
}

void EventBindingPanel::clearSelection()
{
    m_selectedNode = nullptr;
    updateEventBindings();
}

void EventBindingPanel::updateEventBindings()
{
    m_bindingsTree->clear();

    if (!m_selectedNode) {
        m_addButton->setEnabled(false);
        m_removeButton->setEnabled(false);
        return;
    }

    m_addButton->setEnabled(true);
    m_removeButton->setEnabled(false);

    // TODO: Load actual event bindings from the node
    // For now, show placeholder bindings
    if (m_selectedNode->GetName() == "Button") {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_bindingsTree);
        item->setText(0, "OnClick");
        item->setText(1, "ChangeScene(\"scenes/main_menu.scene\")");
        m_bindingsTree->addTopLevelItem(item);
    }
}

void EventBindingPanel::onAddBinding()
{
    if (!m_selectedNode) return;

    QString event = m_eventCombo->currentText();
    QString action = m_actionCombo->currentText();

    if (event.isEmpty() || action.isEmpty()) return;

    QTreeWidgetItem* item = new QTreeWidgetItem(m_bindingsTree);
    item->setText(0, event);
    item->setText(1, action);
    m_bindingsTree->addTopLevelItem(item);

    emit eventBindingChanged(event, action);
}

void EventBindingPanel::onRemoveBinding()
{
    QTreeWidgetItem* current = m_bindingsTree->currentItem();
    if (!current) return;

    QString event = current->text(0);
    delete current;

    emit eventBindingChanged(event, ""); // Empty action means remove
}

void EventBindingPanel::onEventBindingChanged()
{
    QTreeWidgetItem* current = m_bindingsTree->currentItem();
    m_removeButton->setEnabled(current != nullptr);
}

// Prefab creation methods
void MenuBuilderDialog::createMainMenuPrefab(const QPointF& position)
{
    if (!m_menuScene) {
        qWarning() << "MenuBuilderDialog::createMainMenuPrefab: No scene available";
        return;
    }

    auto* root = m_menuScene->GetRootNode();
    if (!root) {
        qWarning() << "MenuBuilderDialog::createMainMenuPrefab: No root node available";
        return;
    }

    try {
        // Create main menu container
        auto* menuContainer = root->CreateChild<Lupine::Control>("MainMenu");
        if (!menuContainer) {
            qWarning() << "MenuBuilderDialog::createMainMenuPrefab: Failed to create menu container";
            return;
        }

        menuContainer->SetPosition(glm::vec2(position.x(), position.y()));
        menuContainer->SetSize(glm::vec2(300.0f, 400.0f));

        // Add background panel
        auto* bgPanel = menuContainer->AddComponent<Lupine::Panel>();
        if (bgPanel) {
            bgPanel->SetBackgroundColor(glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
        }

        // Create title label
        auto* titleNode = menuContainer->CreateChild<Lupine::Control>("Title");
        if (titleNode) {
            titleNode->SetPosition(glm::vec2(0.0f, -150.0f));
            titleNode->SetSize(glm::vec2(300.0f, 50.0f));
            auto* titleLabel = titleNode->AddComponent<Lupine::Label>();
            if (titleLabel) {
                titleLabel->SetText("Game Title");
            }
        }

        // Create buttons
        QStringList buttonTexts = {"Start Game", "Settings", "Credits", "Quit"};
        for (int i = 0; i < buttonTexts.size(); ++i) {
            auto* buttonNode = menuContainer->CreateChild<Lupine::Control>(buttonTexts[i].toStdString());
            if (buttonNode) {
                buttonNode->SetPosition(glm::vec2(0.0f, -50.0f + i * 60.0f));
                buttonNode->SetSize(glm::vec2(200.0f, 40.0f));
                auto* button = buttonNode->AddComponent<Lupine::Button>();
                if (button) {
                    button->SetText(buttonTexts[i].toStdString());
                }
            }
        }

        onElementSelected(menuContainer);
    }
    catch (const std::exception& e) {
        qWarning() << "MenuBuilderDialog::createMainMenuPrefab: Exception occurred:" << e.what();
    }
}

void MenuBuilderDialog::createSettingsPanelPrefab(const QPointF& position)
{
    if (!m_menuScene) {
        qWarning() << "MenuBuilderDialog::createSettingsPanelPrefab: No scene available";
        return;
    }

    auto* root = m_menuScene->GetRootNode();
    if (!root) {
        qWarning() << "MenuBuilderDialog::createSettingsPanelPrefab: No root node available";
        return;
    }

    try {
        // Create settings panel container
        auto* settingsPanel = root->CreateChild<Lupine::Control>("SettingsPanel");
        if (!settingsPanel) {
            qWarning() << "MenuBuilderDialog::createSettingsPanelPrefab: Failed to create settings panel";
            return;
        }

        settingsPanel->SetPosition(glm::vec2(position.x(), position.y()));
        settingsPanel->SetSize(glm::vec2(400.0f, 300.0f));

        // Add background
        auto* bgPanel = settingsPanel->AddComponent<Lupine::Panel>();
        if (bgPanel) {
            bgPanel->SetBackgroundColor(glm::vec4(0.15f, 0.15f, 0.15f, 0.95f));
        }

        // Title
        auto* titleNode = settingsPanel->CreateChild<Lupine::Control>("SettingsTitle");
        if (titleNode) {
            titleNode->SetPosition(glm::vec2(0.0f, -120.0f));
            titleNode->SetSize(glm::vec2(400.0f, 40.0f));
            auto* titleLabel = titleNode->AddComponent<Lupine::Label>();
            if (titleLabel) {
                titleLabel->SetText("Settings");
            }
        }

        // Volume slider area
        auto* volumeLabel = settingsPanel->CreateChild<Lupine::Control>("VolumeLabel");
        if (volumeLabel) {
            volumeLabel->SetPosition(glm::vec2(-150.0f, -50.0f));
            volumeLabel->SetSize(glm::vec2(100.0f, 30.0f));
            auto* volLabel = volumeLabel->AddComponent<Lupine::Label>();
            if (volLabel) {
                volLabel->SetText("Volume:");
            }
        }

        // Close button
        auto* closeButton = settingsPanel->CreateChild<Lupine::Control>("CloseButton");
        if (closeButton) {
            closeButton->SetPosition(glm::vec2(0.0f, 100.0f));
            closeButton->SetSize(glm::vec2(100.0f, 40.0f));
            auto* closeBtn = closeButton->AddComponent<Lupine::Button>();
            if (closeBtn) {
                closeBtn->SetText("Close");
            }
        }

        onElementSelected(settingsPanel);
    }
    catch (const std::exception& e) {
        qWarning() << "MenuBuilderDialog::createSettingsPanelPrefab: Exception occurred:" << e.what();
    }
}

void MenuBuilderDialog::createDialogBoxPrefab(const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create dialog container
    auto* dialogBox = root->CreateChild<Lupine::Control>("DialogBox");
    dialogBox->SetPosition(glm::vec2(position.x(), position.y()));
    dialogBox->SetSize(glm::vec2(350.0f, 200.0f));

    // Add background with border
    auto* bgPanel = dialogBox->AddComponent<Lupine::Panel>();
    if (bgPanel) {
        bgPanel->SetBackgroundColor(glm::vec4(0.2f, 0.2f, 0.2f, 0.95f));
    }

    // Message text
    auto* messageNode = dialogBox->CreateChild<Lupine::Control>("Message");
    messageNode->SetPosition(glm::vec2(0.0f, -30.0f));
    messageNode->SetSize(glm::vec2(300.0f, 60.0f));
    auto* messageLabel = messageNode->AddComponent<Lupine::Label>();
    if (messageLabel) {
        messageLabel->SetText("Are you sure you want to continue?");
    }

    // OK button
    auto* okButton = dialogBox->CreateChild<Lupine::Control>("OKButton");
    okButton->SetPosition(glm::vec2(-60.0f, 60.0f));
    okButton->SetSize(glm::vec2(80.0f, 35.0f));
    auto* okBtn = okButton->AddComponent<Lupine::Button>();
    if (okBtn) {
        okBtn->SetText("OK");
    }

    // Cancel button
    auto* cancelButton = dialogBox->CreateChild<Lupine::Control>("CancelButton");
    cancelButton->SetPosition(glm::vec2(60.0f, 60.0f));
    cancelButton->SetSize(glm::vec2(80.0f, 35.0f));
    auto* cancelBtn = cancelButton->AddComponent<Lupine::Button>();
    if (cancelBtn) {
        cancelBtn->SetText("Cancel");
    }

    onElementSelected(dialogBox);
}

void MenuBuilderDialog::createButtonGroupTemplate(const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create container
    auto* buttonGroup = root->CreateChild<Lupine::Control>("ButtonGroup");
    buttonGroup->SetPosition(glm::vec2(position.x(), position.y()));
    buttonGroup->SetSize(glm::vec2(250.0f, 60.0f));

    // Create three buttons side by side
    QStringList buttonTexts = {"Option A", "Option B", "Option C"};
    for (int i = 0; i < buttonTexts.size(); ++i) {
        auto* buttonNode = buttonGroup->CreateChild<Lupine::Control>(buttonTexts[i].toStdString());
        buttonNode->SetPosition(glm::vec2(-80.0f + i * 80.0f, 0.0f));
        buttonNode->SetSize(glm::vec2(70.0f, 35.0f));
        auto* button = buttonNode->AddComponent<Lupine::Button>();
        if (button) {
            button->SetText(buttonTexts[i].toStdString());
        }
    }

    onElementSelected(buttonGroup);
}

void MenuBuilderDialog::createFormLayoutTemplate(const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create form container
    auto* formLayout = root->CreateChild<Lupine::Control>("FormLayout");
    formLayout->SetPosition(glm::vec2(position.x(), position.y()));
    formLayout->SetSize(glm::vec2(300.0f, 200.0f));

    // Add background
    auto* bgPanel = formLayout->AddComponent<Lupine::Panel>();
    if (bgPanel) {
        bgPanel->SetBackgroundColor(glm::vec4(0.12f, 0.12f, 0.12f, 0.9f));
    }

    // Create form fields
    QStringList fieldLabels = {"Name:", "Email:", "Password:"};
    for (int i = 0; i < fieldLabels.size(); ++i) {
        // Label
        auto* labelNode = formLayout->CreateChild<Lupine::Control>(QString("Label%1").arg(i).toStdString());
        labelNode->SetPosition(glm::vec2(-100.0f, -60.0f + i * 50.0f));
        labelNode->SetSize(glm::vec2(80.0f, 30.0f));
        auto* label = labelNode->AddComponent<Lupine::Label>();
        if (label) {
            label->SetText(fieldLabels[i].toStdString());
        }

        // Input field (using ColorRectangle as placeholder)
        auto* inputNode = formLayout->CreateChild<Lupine::Control>(QString("Input%1").arg(i).toStdString());
        inputNode->SetPosition(glm::vec2(20.0f, -60.0f + i * 50.0f));
        inputNode->SetSize(glm::vec2(150.0f, 30.0f));
        auto* inputRect = inputNode->AddComponent<Lupine::ColorRectangle>();
        if (inputRect) {
            inputRect->SetColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }
    }

    onElementSelected(formLayout);
}

void MenuBuilderDialog::createButtonLabelTemplate(const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create container
    auto* container = root->CreateChild<Lupine::Control>("ButtonLabelTemplate");
    container->SetPosition(glm::vec2(position.x(), position.y()));
    container->SetSize(glm::vec2(200.0f, 80.0f));

    // Create button
    auto* buttonNode = container->CreateChild<Lupine::Control>("Button");
    buttonNode->SetPosition(glm::vec2(0.0f, 15.0f));
    buttonNode->SetSize(glm::vec2(120.0f, 40.0f));
    auto* button = buttonNode->AddComponent<Lupine::Button>();
    if (button) {
        button->SetText("Click Me");
    }

    // Create label below button
    auto* labelNode = container->CreateChild<Lupine::Control>("Label");
    labelNode->SetPosition(glm::vec2(0.0f, -25.0f));
    labelNode->SetSize(glm::vec2(200.0f, 25.0f));
    auto* label = labelNode->AddComponent<Lupine::Label>();
    if (label) {
        label->SetText("Button Description");
    }

    onElementSelected(container);
}

void MenuBuilderDialog::createPanelBackgroundTemplate(const QPointF& position)
{
    auto* root = m_menuScene->GetRootNode();
    if (!root) return;

    // Create main panel
    auto* mainPanel = root->CreateChild<Lupine::Control>("PanelBackground");
    mainPanel->SetPosition(glm::vec2(position.x(), position.y()));
    mainPanel->SetSize(glm::vec2(300.0f, 200.0f));

    // Add main background
    auto* bgPanel = mainPanel->AddComponent<Lupine::Panel>();
    if (bgPanel) {
        bgPanel->SetBackgroundColor(glm::vec4(0.15f, 0.15f, 0.15f, 0.9f));
    }

    // Add border panel
    auto* borderPanel = mainPanel->CreateChild<Lupine::Control>("Border");
    borderPanel->SetPosition(glm::vec2(0.0f, 0.0f));
    borderPanel->SetSize(glm::vec2(310.0f, 210.0f));
    auto* borderRect = borderPanel->AddComponent<Lupine::ColorRectangle>();
    if (borderRect) {
        borderRect->SetColor(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    // Add content area
    auto* contentPanel = mainPanel->CreateChild<Lupine::Control>("Content");
    contentPanel->SetPosition(glm::vec2(0.0f, 0.0f));
    contentPanel->SetSize(glm::vec2(280.0f, 180.0f));
    auto* contentBg = contentPanel->AddComponent<Lupine::Panel>();
    if (contentBg) {
        contentBg->SetBackgroundColor(glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));
    }

    // Add sample content label
    auto* contentLabel = contentPanel->CreateChild<Lupine::Control>("ContentLabel");
    contentLabel->SetPosition(glm::vec2(0.0f, 0.0f));
    contentLabel->SetSize(glm::vec2(280.0f, 30.0f));
    auto* label = contentLabel->AddComponent<Lupine::Label>();
    if (label) {
        label->SetText("Panel Content Area");
    }

    onElementSelected(mainPanel);
}
