#include "MainWindow.h"
#include "panels/SceneTreePanel.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/FileBrowserPanel.h"
#include "panels/SceneViewPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/ScriptEditorPanel.h"
#include "panels/ConsolePanel.h"
#include "lupine/core/CrashHandler.h"
#include "EditorUndoSystem.h"
#include "EditorClipboard.h"
#include <QKeyEvent>
#include "panels/ActionMappingPanel.h"
#include "dialogs/ProjectSettingsDialog.h"
#include "dialogs/TweenAnimatorDialog.h"
#include "dialogs/SpriteAnimatorDialog.h"
#include "dialogs/StateAnimatorDialog.h"
#include "dialogs/TilesetEditorDialog.h"
#include "dialogs/Tileset3DEditorDialog.h"
#include "dialogs/TilemapPainterDialog.h"
#include "dialogs/GlobalsManagerDialog.h"
#include "dialogs/ExportDialog.h"
#include "dialogs/PixelPainterDialog.h"
#include "dialogs/ScribblerDialog.h"
#include "dialogs/VoxelBlockerDialog.h"
#include "dialogs/TilemapBuilder3DDialog.h"
#include "dialogs/Tilemap25DPainterDialog.h"
#include "dialogs/VisualScripterDialog.h"
#include "dialogs/NotepadDialog.h"
#include "dialogs/TodoListDialog.h"
#include "dialogs/MilestoneTrackerDialog.h"
#include "dialogs/FeatureBugTrackerDialog.h"
#include "dialogs/AssetProgressTrackerDialog.h"
#include "dialogs/MenuBuilderDialog.h"
#include "dialogs/LocalizationSettingsDialog.h"
#include "dialogs/LocalizationTablesDialog.h"
#include "lupine/editor/dialogs/ScriptableObjectsDialog.h"
#include "dialogs/AudioMixerDialog.h"
#include "dialogs/TerrainPainterDialog.h"
#include "widgets/GameRunnerToolbar.h"
#include "rendering/GizmoRenderer.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Engine.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/core/ComponentRegistration.h"
#include "lupine/core/GlobalsManager.h"
#include "lupine/scriptableobjects/ScriptableObjectManager.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/input/InputManager.h"
#include "lupine/localization/LocalizationManager.h"
#include "lupine/input/ActionMap.h"
#include "lupine/audio/AudioManager.h"
#include "lupine/physics/PhysicsManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <exception>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_actionMappingPanel(nullptr)
    , m_projectSettingsDialog(nullptr)
    , m_tweenAnimatorDialog(nullptr)
    , m_spriteAnimatorDialog(nullptr)
    , m_stateAnimatorDialog(nullptr)
    , m_tilesetEditorDialog(nullptr)
    , m_tileset3DEditorDialog(nullptr)
    , m_tilemapPainterDialog(nullptr)
    , m_globalsManagerDialog(nullptr)
    , m_pixelPainterDialog(nullptr)
    , m_scribblerDialog(nullptr)
    , m_voxelBlockerDialog(nullptr)
    , m_tilemapBuilder3DDialog(nullptr)
    , m_tilemap25DPainterDialog(nullptr)
    , m_visualScripterDialog(nullptr)
    , m_notepadDialog(nullptr)
    , m_todoListDialog(nullptr)
    , m_milestoneTrackerDialog(nullptr)
    , m_featureBugTrackerDialog(nullptr)
    , m_assetProgressTrackerDialog(nullptr)
    , m_menuBuilderDialog(nullptr)
    , m_terrainPainterDialog(nullptr)
    , m_localizationSettingsDialog(nullptr)
    , m_scriptableObjectsDialog(nullptr)
    , m_audioMixerDialog(nullptr)
    , m_localizationTablesDialog(nullptr)
    , m_undoSystem(nullptr)
    , m_clipboard(nullptr)
    , m_currentProject(nullptr)
    , m_currentScene(nullptr)
    , m_engine(nullptr)
    , m_isSceneModified(false)
    , m_isPlaying(false)
    , m_runtimeProcess(nullptr)
{
    LUPINE_SAFE_EXECUTE({
        // Initialize component registry early
        LUPINE_LOG_STARTUP("MainWindow: Initializing component registry");
        Lupine::InitializeComponentRegistry();

        LUPINE_LOG_STARTUP("MainWindow: Setting up UI");
        setupUI();
        LUPINE_LOG_STARTUP("MainWindow: Setting up menu bar");
        setupMenuBar();
        LUPINE_LOG_STARTUP("MainWindow: Setting up tool bars");
        setupToolBars();
        LUPINE_LOG_STARTUP("MainWindow: Setting up file bar height");
        setupFileBarHeight();
        LUPINE_LOG_STARTUP("MainWindow: Setting up dock widgets");
        setupDockWidgets();
        LUPINE_LOG_STARTUP("MainWindow: Setting up status bar");
        setupStatusBar();
        LUPINE_LOG_STARTUP("MainWindow: Setting up connections");
        setupConnections();

        LUPINE_LOG_STARTUP("MainWindow: Initializing editor systems");
        initializeEditorSystems();

        // Initialize the engine for editor use (without SDL window)
        LUPINE_LOG_STARTUP("MainWindow: Initializing engine");
        initializeEngine();

        LUPINE_LOG_STARTUP("MainWindow: Updating window title");
        updateWindowTitle();
        LUPINE_LOG_STARTUP("MainWindow: Updating actions");
        updateActions();

        LUPINE_LOG_STARTUP("MainWindow: Constructor completed successfully");

    }, "Critical error during MainWindow initialization");
}

MainWindow::~MainWindow()
{
    // Clean up editor systems first to avoid Qt assertion
    if (m_undoSystem) {
        m_undoSystem->setScene(nullptr);
        m_undoSystem->clear();
    }
    if (m_clipboard) {
        m_clipboard->setScene(nullptr);
        m_clipboard->clear();
    }

    // Clean up runtime process
    if (m_runtimeProcess) {
        if (m_runtimeProcess->state() != QProcess::NotRunning) {
            m_runtimeProcess->kill();
            m_runtimeProcess->waitForFinished(3000);
        }
        delete m_runtimeProcess;
    }

    // Ensure all managers are properly shut down
    Lupine::PhysicsManager::Shutdown();
    Lupine::AudioManager::Shutdown();
    Lupine::InputManager::Shutdown();
    Lupine::ResourceManager::Shutdown();
}

void MainWindow::setupUI()
{
    LUPINE_SAFE_EXECUTE({
        // Central widget - Scene View
        m_sceneViewPanel = new SceneViewPanel();
        if (!m_sceneViewPanel) {
            LUPINE_LOG_CRITICAL("Failed to create SceneViewPanel");
            return;
        }
        setCentralWidget(m_sceneViewPanel);

        // Create panels with validation
        m_sceneTreePanel = new SceneTreePanel();
        if (!m_sceneTreePanel) {
            LUPINE_LOG_CRITICAL("Failed to create SceneTreePanel");
            return;
        }
        m_sceneTreePanel->setMainWindow(this);

        m_assetBrowserPanel = new AssetBrowserPanel();
        if (!m_assetBrowserPanel) {
            LUPINE_LOG_CRITICAL("Failed to create AssetBrowserPanel");
            return;
        }

        m_fileBrowserPanel = new FileBrowserPanel();
        if (!m_fileBrowserPanel) {
            LUPINE_LOG_CRITICAL("Failed to create FileBrowserPanel");
            return;
        }

        m_inspectorPanel = new InspectorPanel();
        if (!m_inspectorPanel) {
            LUPINE_LOG_CRITICAL("Failed to create InspectorPanel");
            return;
        }
        m_inspectorPanel->setMainWindow(this);

        m_scriptEditorPanel = new ScriptEditorPanel();
        if (!m_scriptEditorPanel) {
            LUPINE_LOG_CRITICAL("Failed to create ScriptEditorPanel");
            return;
        }

        m_consolePanel = new ConsolePanel();
        if (!m_consolePanel) {
            LUPINE_LOG_CRITICAL("Failed to create ConsolePanel");
            return;
        }

    }, "Failed to setup MainWindow UI");
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    m_newSceneAction = fileMenu->addAction("&New Scene", this, &MainWindow::onNewScene, QKeySequence::New);
    m_openSceneAction = fileMenu->addAction("&Open Scene...", this, &MainWindow::onOpenScene, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveSceneAction = fileMenu->addAction("&Save Scene", this, &MainWindow::onSaveScene, QKeySequence::Save);
    m_saveSceneAsAction = fileMenu->addAction("Save Scene &As...", this, &MainWindow::onSaveSceneAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_projectSettingsAction = fileMenu->addAction("&Project Settings...", this, &MainWindow::onProjectSettings);
    m_exportProjectAction = fileMenu->addAction("&Export Project...", this, &MainWindow::onExportProject);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &MainWindow::onExit, QKeySequence::Quit);
    
    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    m_undoAction = editMenu->addAction("&Undo", this, &MainWindow::onUndo, QKeySequence::Undo);
    m_redoAction = editMenu->addAction("&Redo", this, &MainWindow::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    m_cutAction = editMenu->addAction("Cu&t", this, &MainWindow::onCut, QKeySequence::Cut);
    m_copyAction = editMenu->addAction("&Copy", this, &MainWindow::onCopy, QKeySequence::Copy);
    m_pasteAction = editMenu->addAction("&Paste", this, &MainWindow::onPaste, QKeySequence::Paste);
    m_deleteAction = editMenu->addAction("&Delete", this, &MainWindow::onDelete, QKeySequence::Delete);
    editMenu->addSeparator();
    m_duplicateAction = editMenu->addAction("D&uplicate", this, &MainWindow::onDuplicate, QKeySequence("Ctrl+D"));
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");

    // View mode submenu
    QMenu* viewModeMenu = viewMenu->addMenu("View &Mode");
    m_viewMode2DAction = viewModeMenu->addAction("&2D View", this, &MainWindow::onViewMode2D, QKeySequence("2"));
    m_viewMode3DAction = viewModeMenu->addAction("&3D View", this, &MainWindow::onViewMode3D, QKeySequence("3"));

    // Create action group for view modes
    m_viewModeGroup = new QActionGroup(this);
    m_viewModeGroup->addAction(m_viewMode2DAction);
    m_viewModeGroup->addAction(m_viewMode3DAction);
    m_viewMode2DAction->setCheckable(true);
    m_viewMode3DAction->setCheckable(true);
    m_viewMode3DAction->setChecked(true); // Default to 3D

    viewMenu->addSeparator();

    // Grid options
    m_toggleGridAction = viewMenu->addAction("Show &Grid", this, &MainWindow::onToggleGrid, QKeySequence("G"));
    m_toggleGridAction->setCheckable(true);
    m_toggleGridAction->setChecked(true);

    viewMenu->addSeparator();

    // Gizmo submenu
    QMenu* gizmoMenu = viewMenu->addMenu("&Gizmos");
    m_moveGizmoAction = gizmoMenu->addAction("&Move", this, &MainWindow::onMoveGizmo, QKeySequence("Q"));
    m_rotateGizmoAction = gizmoMenu->addAction("&Rotate", this, &MainWindow::onRotateGizmo, QKeySequence("W"));
    m_scaleGizmoAction = gizmoMenu->addAction("&Scale", this, &MainWindow::onScaleGizmo, QKeySequence("E"));

    // Create action group for gizmos
    m_gizmoGroup = new QActionGroup(this);
    m_gizmoGroup->addAction(m_moveGizmoAction);
    m_gizmoGroup->addAction(m_rotateGizmoAction);
    m_gizmoGroup->addAction(m_scaleGizmoAction);
    m_moveGizmoAction->setCheckable(true);
    m_rotateGizmoAction->setCheckable(true);
    m_scaleGizmoAction->setCheckable(true);
    m_moveGizmoAction->setChecked(true); // Default to move gizmo // Grid visible by default

    viewMenu->addSeparator();
    m_toggleSceneTreeAction = viewMenu->addAction("Scene &Tree", this, &MainWindow::onToggleSceneTree);
    m_toggleAssetBrowserAction = viewMenu->addAction("&Asset Browser", this, &MainWindow::onToggleAssetBrowser);
    m_toggleInspectorAction = viewMenu->addAction("&Inspector", this, &MainWindow::onToggleInspector);
    m_toggleScriptEditorAction = viewMenu->addAction("&Script Editor", this, &MainWindow::onToggleScriptEditor);
    m_toggleConsoleAction = viewMenu->addAction("&Console", this, &MainWindow::onToggleConsole);
    viewMenu->addSeparator();
    m_enableModelPreviewsAction = viewMenu->addAction("Enable &Model Previews", this, &MainWindow::onToggleModelPreviews);
    m_enableModelPreviewsAction->setCheckable(true);
    m_enableModelPreviewsAction->setChecked(false); // Disabled by default for safety
    m_enableModelPreviewsAction->setToolTip("Enable 3D model preview generation in the asset browser (may impact performance)");
    viewMenu->addSeparator();
    m_resetLayoutAction = viewMenu->addAction("&Reset Layout", this, &MainWindow::onResetLayout);
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    m_playAction = toolsMenu->addAction("&Play Scene", this, &MainWindow::onPlayScene, QKeySequence("F5"));
    m_pauseAction = toolsMenu->addAction("&Pause Scene", this, &MainWindow::onPauseScene, QKeySequence("F6"));
    m_stopAction = toolsMenu->addAction("&Stop Scene", this, &MainWindow::onStopScene, QKeySequence("F7"));
    toolsMenu->addSeparator();

    // Animators submenu
    QMenu* animatorsMenu = toolsMenu->addMenu("&Animators");
    m_tweenAnimatorAction = animatorsMenu->addAction("&Tween Animator...", this, &MainWindow::onTweenAnimator);
    m_spriteAnimatorAction = animatorsMenu->addAction("&Sprite Animator...", this, &MainWindow::onSpriteAnimator);
    m_stateAnimatorAction = animatorsMenu->addAction("S&tate Animator...", this, &MainWindow::onStateAnimator);

    // Tiles submenu
    QMenu* tilesMenu = toolsMenu->addMenu("&Tiles");
    m_tilesetEditorAction = tilesMenu->addAction("&Tileset 2D Editor...", this, &MainWindow::onTilesetEditor);
    m_tileset3DEditorAction = tilesMenu->addAction("Tileset &3D Editor...", this, &MainWindow::onTileset3DEditor);
    tilesMenu->addSeparator();
    m_tilemapPainterAction = tilesMenu->addAction("Tilemap &Painter...", this, &MainWindow::onTilemapPainter);
    m_tilemapBuilder3DAction = tilesMenu->addAction("3D &Tilemap Builder...", this, &MainWindow::onTilemapBuilder3D);
    m_tilemap25DPainterAction = tilesMenu->addAction("2.5D Tilemap &Painter...", this, &MainWindow::onTilemap25DPainter);

    // Art Tools submenu
    QMenu* artToolsMenu = toolsMenu->addMenu("&Art Tools");
    m_pixelPainterAction = artToolsMenu->addAction("&Pixel Painter...", this, &MainWindow::onPixelPainter);
    m_scribblerAction = artToolsMenu->addAction("&Scribbler...", this, &MainWindow::onScribbler);
    m_voxelBlockerAction = artToolsMenu->addAction("&Voxel Blocker...", this, &MainWindow::onVoxelBlocker);

    toolsMenu->addSeparator();
    m_visualScripterAction = toolsMenu->addAction("&Visual Scripter...", this, &MainWindow::onVisualScripter);
    toolsMenu->addSeparator();
    m_globalsManagerAction = toolsMenu->addAction("&Globals Manager...", this, &MainWindow::onGlobalsManager);
    m_inputMapperAction = toolsMenu->addAction("&Input Mapper...", this, &MainWindow::onInputMapper);
    m_scriptableObjectsAction = toolsMenu->addAction("&Scriptable Objects...", this, &MainWindow::onScriptableObjects);

    // Builders submenu
    QMenu* buildersMenu = toolsMenu->addMenu("&Builders");
    m_menuBuilderAction = buildersMenu->addAction("&Menu Builder...", this, &MainWindow::onMenuBuilder);
    m_terrainPainterAction = buildersMenu->addAction("&Terrain Painter...", this, &MainWindow::onTerrainPainter);

    // Localization submenu
    QMenu* localizationMenu = toolsMenu->addMenu("&Localization");
    m_localizationSettingsAction = localizationMenu->addAction("&Localization Settings...", this, &MainWindow::onLocalizationSettings);
    m_localizationTablesAction = localizationMenu->addAction("&Localization Tables...", this, &MainWindow::onLocalizationTables);

    // Audio submenu
    QMenu* audioMenu = toolsMenu->addMenu("&Audio");
    m_audioMixerAction = audioMenu->addAction("Audio &Mixer...", this, &MainWindow::onAudioMixer);

    // Production submenu
    QMenu* productionMenu = toolsMenu->addMenu("&Production");
    m_notepadAction = productionMenu->addAction("&Notepad...", this, &MainWindow::onNotepad);
    m_todoListAction = productionMenu->addAction("&Todo Lists...", this, &MainWindow::onTodoList);
    m_milestoneTrackerAction = productionMenu->addAction("&Milestone Tracker...", this, &MainWindow::onMilestoneTracker);
    m_featureBugTrackerAction = productionMenu->addAction("&Feature/Bug Tracker...", this, &MainWindow::onFeatureBugTracker);
    m_assetProgressTrackerAction = productionMenu->addAction("&Asset Progress Tracker...", this, &MainWindow::onAssetProgressTracker);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    m_documentationAction = helpMenu->addAction("&Documentation", this, &MainWindow::onDocumentation);
    helpMenu->addSeparator();
    m_aboutAction = helpMenu->addAction("&About Lupine", this, &MainWindow::onAbout);
    
    // Set checkable actions
    m_toggleSceneTreeAction->setCheckable(true);
    m_toggleAssetBrowserAction->setCheckable(true);
    m_toggleInspectorAction->setCheckable(true);
    m_toggleScriptEditorAction->setCheckable(true);
    m_toggleConsoleAction->setCheckable(true);

    // Add edit actions to main window for global shortcuts
    addAction(m_undoAction);
    addAction(m_redoAction);
    addAction(m_cutAction);
    addAction(m_copyAction);
    addAction(m_pasteAction);
    addAction(m_deleteAction);
    addAction(m_duplicateAction);

    // Set shortcut context to make them work globally
    m_undoAction->setShortcutContext(Qt::WindowShortcut);
    m_redoAction->setShortcutContext(Qt::WindowShortcut);
    m_cutAction->setShortcutContext(Qt::WindowShortcut);
    m_copyAction->setShortcutContext(Qt::WindowShortcut);
    m_pasteAction->setShortcutContext(Qt::WindowShortcut);
    m_deleteAction->setShortcutContext(Qt::WindowShortcut);
    m_duplicateAction->setShortcutContext(Qt::WindowShortcut);
}

void MainWindow::setupToolBars()
{
    // Game runner toolbar
    m_gameRunnerToolbar = new GameRunnerToolbar();
    addToolBar(m_gameRunnerToolbar);

    // View mode toolbar
    QToolBar* viewModeToolbar = addToolBar("View Mode");
    viewModeToolbar->addAction(m_viewMode2DAction);
    viewModeToolbar->addAction(m_viewMode3DAction);
    viewModeToolbar->addSeparator();
    viewModeToolbar->addAction(m_toggleGridAction);

    // Gizmo toolbar
    QToolBar* gizmoToolbar = addToolBar("Gizmos");
    gizmoToolbar->addAction(m_moveGizmoAction);
    gizmoToolbar->addAction(m_rotateGizmoAction);
    gizmoToolbar->addAction(m_scaleGizmoAction);
}

void MainWindow::setupFileBarHeight()
{
    // Set fixed height for menu bar and toolbars (total ~50px)
    if (menuBar()) {
        menuBar()->setFixedHeight(24); // Menu bar: 24px
    }

    // Set toolbar heights to total ~26px for all toolbars
    QList<QToolBar*> toolbars = findChildren<QToolBar*>();
    for (QToolBar* toolbar : toolbars) {
        toolbar->setFixedHeight(26);
        toolbar->setIconSize(QSize(16, 16)); // Smaller icons for compact layout
    }
}

void MainWindow::setupDockWidgets()
{
    // Scene Tree (left)
    m_sceneTreeDock = new QDockWidget("Scene Tree", this);
    m_sceneTreeDock->setWidget(m_sceneTreePanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_sceneTreeDock);
    
    // Asset Browser (left, tabbed with scene tree)
    m_assetBrowserDock = new QDockWidget("Asset Browser", this);
    m_assetBrowserDock->setWidget(m_assetBrowserPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_assetBrowserDock);
    tabifyDockWidget(m_sceneTreeDock, m_assetBrowserDock);

    // File Browser (left, tabbed with scene tree and asset browser)
    m_fileBrowserDock = new QDockWidget("File Browser", this);
    m_fileBrowserDock->setWidget(m_fileBrowserPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_fileBrowserDock);
    tabifyDockWidget(m_assetBrowserDock, m_fileBrowserDock);
    
    // Inspector (right)
    m_inspectorDock = new QDockWidget("Inspector", this);
    m_inspectorDock->setWidget(m_inspectorPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);
    
    // Script Editor (right, tabbed with inspector)
    m_scriptEditorDock = new QDockWidget("Script Editor", this);
    m_scriptEditorDock->setWidget(m_scriptEditorPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_scriptEditorDock);
    tabifyDockWidget(m_inspectorDock, m_scriptEditorDock);
    
    // Console (bottom)
    m_consoleDock = new QDockWidget("Console", this);
    m_consoleDock->setWidget(m_consolePanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_consoleDock);
    
    // Make scene tree visible by default
    m_sceneTreeDock->raise();
    m_inspectorDock->raise();
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections()
{
    // Panel connections
    connect(m_sceneTreePanel, &SceneTreePanel::nodeSelected,
            m_inspectorPanel, &InspectorPanel::setSelectedNode);
    connect(m_sceneTreePanel, &SceneTreePanel::nodeSelected,
            this, &MainWindow::onSceneNodeSelected);
    connect(m_sceneTreePanel, &SceneTreePanel::nodeDeleted,
            this, &MainWindow::onSceneNodeDeleted);
    connect(m_sceneTreePanel, &SceneTreePanel::nodeDuplicated,
            this, &MainWindow::onSceneNodeDuplicated);

    // Inspector to scene tree connections
    connect(m_inspectorPanel, &InspectorPanel::nodeNameChanged,
            m_sceneTreePanel, &SceneTreePanel::updateNodeName);

    // Scene view selection connections
    connect(m_sceneViewPanel, &SceneViewPanel::nodeSelected,
            m_inspectorPanel, &InspectorPanel::setSelectedNode);
    connect(m_sceneViewPanel, &SceneViewPanel::nodeSelected,
            m_sceneTreePanel, &SceneTreePanel::selectNode);
    connect(m_sceneViewPanel, &SceneViewPanel::nodeSelected,
            this, &MainWindow::onSceneNodeSelected);

    // Scene tree selection connections (use silent method to prevent circular signals)
    connect(m_sceneTreePanel, &SceneTreePanel::nodeSelected,
            [this](Lupine::Node* node) {
                // Update scene view selection silently to prevent circular signals
                m_sceneViewPanel->setSelectedNodeSilently(node);
            });

    // Scene view node creation connections
    connect(m_sceneViewPanel, &SceneViewPanel::nodeCreated,
            [this](Lupine::Node* node) {
                // Refresh the scene tree to show the new node
                m_sceneTreePanel->refreshTree();
                // Select the newly created node
                m_sceneTreePanel->selectNode(node);
                // Update inspector
                m_inspectorPanel->setSelectedNode(node);
                // Force scene view to refresh its rendering completely
                m_sceneViewPanel->forceSceneRefresh();
                // Mark scene as modified
                m_isSceneModified = true;
                updateWindowTitle();
            });
    
    connect(m_assetBrowserPanel, &AssetBrowserPanel::assetSelected,
            this, &MainWindow::onAssetSelected);

    // Connect placement mode from asset browser to scene view
    m_sceneViewPanel->setPlacementMode(m_assetBrowserPanel->getPlacementMode());
    
    // Game runner connections
    connect(m_gameRunnerToolbar, &GameRunnerToolbar::playSceneRequested,
            this, &MainWindow::onPlayScene);
    connect(m_gameRunnerToolbar, &GameRunnerToolbar::playGameRequested,
            this, &MainWindow::onPlayGame);
    connect(m_gameRunnerToolbar, &GameRunnerToolbar::pauseRequested,
            this, &MainWindow::onPauseScene);
    connect(m_gameRunnerToolbar, &GameRunnerToolbar::stopRequested,
            this, &MainWindow::onStopScene);
    
    // Dock widget visibility connections
    connect(m_sceneTreeDock, &QDockWidget::visibilityChanged, 
            m_toggleSceneTreeAction, &QAction::setChecked);
    connect(m_assetBrowserDock, &QDockWidget::visibilityChanged, 
            m_toggleAssetBrowserAction, &QAction::setChecked);
    connect(m_inspectorDock, &QDockWidget::visibilityChanged, 
            m_toggleInspectorAction, &QAction::setChecked);
    connect(m_scriptEditorDock, &QDockWidget::visibilityChanged, 
            m_toggleScriptEditorAction, &QAction::setChecked);
    connect(m_consoleDock, &QDockWidget::visibilityChanged,
            m_toggleConsoleAction, &QAction::setChecked);
}

void MainWindow::initializeEditorSystems()
{
    // Initialize undo system
    m_undoSystem = new EditorUndoSystem(this);

    // Initialize clipboard
    m_clipboard = new EditorClipboard(this);

    // Connect undo system signals to update menu actions
    connect(m_undoSystem, &EditorUndoSystem::undoRedoStateChanged,
            this, &MainWindow::updateActions);

    // Connect clipboard signals
    connect(m_clipboard, &EditorClipboard::clipboardChanged,
            this, &MainWindow::updateActions);
}

bool MainWindow::openProject(const QString& projectPath)
{
    try {
        qDebug() << "MainWindow::openProject - Starting to load project:" << projectPath;

        // Validate that all required panels are initialized
        if (!m_assetBrowserPanel || !m_fileBrowserPanel || !m_sceneTreePanel ||
            !m_sceneViewPanel || !m_inspectorPanel || !m_consolePanel) {
            qCritical() << "MainWindow::openProject - One or more panels are not initialized";
            QMessageBox::critical(this, "Error", "Editor panels are not properly initialized. Please restart the application.");
            return false;
        }

        m_currentProject = std::make_unique<Lupine::Project>();
        if (!m_currentProject->LoadFromFile(projectPath.toStdString())) {
            QMessageBox::critical(this, "Error", "Failed to load project: " + projectPath);
            m_currentProject.reset();
            return false;
        }

        qDebug() << "MainWindow::openProject - Project loaded successfully";
        m_currentProjectPath = projectPath;

        // Set up asset browser with safety checks
        qDebug() << "MainWindow::openProject - Setting up asset browser";
        QFileInfo projectFile(projectPath);
        QString projectDir = projectFile.absolutePath();

        if (!m_assetBrowserPanel) {
            qCritical() << "MainWindow::openProject - Asset browser panel is null";
            throw std::runtime_error("Asset browser panel is not initialized");
        }

        LUPINE_SAFE_EXECUTE({
            m_assetBrowserPanel->setProjectPath(projectDir);
            qDebug() << "MainWindow::openProject - Asset browser set up successfully";
        }, "Failed to initialize asset browser");

        // Add a small delay to allow asset browser to fully initialize
        QApplication::processEvents();
        QThread::msleep(100);

        // Model previews are disabled by default for safety
        // They can be manually enabled through the View menu if needed
        qDebug() << "MainWindow::openProject - Model previews disabled by default for safety";
        // Uncomment the following lines to enable model previews after startup:
        /*
        QTimer::singleShot(5000, this, [this]() {
            qDebug() << "MainWindow::openProject - Enabling model previews after startup delay";
            if (m_assetBrowserPanel) {
                m_assetBrowserPanel->enableModelPreviews(true);
            }
        });
        */

        // Set up file browser to show project directory with safety checks
        qDebug() << "MainWindow::openProject - Setting up file browser";

        if (!m_fileBrowserPanel) {
            qCritical() << "MainWindow::openProject - File browser panel is null";
            throw std::runtime_error("File browser panel is not initialized");
        }

        LUPINE_SAFE_EXECUTE({
            m_fileBrowserPanel->setRootPath(projectDir);
            qDebug() << "MainWindow::openProject - File browser set up successfully";
        }, "Failed to initialize file browser");

        // Add a small delay to allow file browser to fully initialize
        QApplication::processEvents();
        QThread::msleep(100);

        // Load main scene if available
        qDebug() << "MainWindow::openProject - Checking main scene";
        if (!m_currentProject->GetMainScene().empty()) {
            QString scenePath = projectFile.absolutePath() + "/" + QString::fromStdString(m_currentProject->GetMainScene());
            QFileInfo sceneFile(scenePath);
            if (sceneFile.exists()) {
                try {
                    m_currentScene = std::make_unique<Lupine::Scene>();
                    if (m_currentScene->LoadFromFile(scenePath.toStdString())) {
                        m_currentScenePath = scenePath;

                        // Debug: Check if scene loaded correctly
                        auto* rootNode = m_currentScene->GetRootNode();
                        if (rootNode) {
                            m_consolePanel->addMessage(QString("Scene loaded with root node: %1, children: %2")
                                .arg(QString::fromStdString(rootNode->GetName()))
                                .arg(rootNode->GetChildren().size()));
                        } else {
                            m_consolePanel->addMessage("Warning: Scene loaded but no root node found");
                        }

                        // Set scene on panels with safety checks
                        if (m_sceneTreePanel) {
                            m_sceneTreePanel->setScene(m_currentScene.get());
                        } else {
                            qWarning() << "MainWindow::openProject - Scene tree panel is null";
                        }

                        if (m_sceneViewPanel) {
                            m_sceneViewPanel->setScene(m_currentScene.get());
                        } else {
                            qWarning() << "MainWindow::openProject - Scene view panel is null";
                        }

                        if (m_inspectorPanel) {
                            m_inspectorPanel->setScene(m_currentScene.get());
                        } else {
                            qWarning() << "MainWindow::openProject - Inspector panel is null";
                        }

                        // Initialize editor systems with loaded scene
                        if (m_undoSystem) {
                            m_undoSystem->setScene(m_currentScene.get());
                            // Set undo depth from project settings
                            int undoDepth = m_currentProject->GetSettingValue<int>("editor/undo_depth", 25);
                            m_undoSystem->setMaxUndoSteps(undoDepth);
                        }
                        if (m_clipboard) {
                            m_clipboard->setScene(m_currentScene.get());
                        }
                    } else {
                        m_consolePanel->addMessage("Failed to load main scene: " + scenePath);
                        // Continue without scene - don't fail the entire project load
                    }
                } catch (const std::exception& e) {
                    m_consolePanel->addMessage(QString("Exception loading main scene: %1").arg(e.what()));
                    // Continue without scene - don't fail the entire project load
                }
            } else {
                m_consolePanel->addMessage("Main scene file not found: " + scenePath);
                // Continue without scene - don't fail the entire project load
            }
        } else {
            m_consolePanel->addMessage("No main scene specified in project");
        }

        qDebug() << "MainWindow::openProject - Updating window title and actions";
        updateWindowTitle();
        updateActions();
        qDebug() << "MainWindow::openProject - Window title and actions updated";

        m_consolePanel->addMessage("Project loaded: " + projectPath);
        qDebug() << "MainWindow::openProject - Project loading completed successfully";
        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to open project: %1").arg(e.what()));
        m_currentProject.reset();
        return false;
    }
}

void MainWindow::closeProject()
{
    m_currentProject.reset();
    m_currentScene.reset();
    m_currentProjectPath.clear();
    m_currentScenePath.clear();

    m_sceneTreePanel->setScene(nullptr);
    m_sceneViewPanel->setScene(nullptr);
    m_inspectorPanel->setSelectedNode(nullptr);

    updateWindowTitle();
    updateActions();
}

void MainWindow::updateWindowTitle()
{
    QString title = "Lupine Editor";

    if (m_currentProject) {
        title += " - " + QString::fromStdString(m_currentProject->GetName());

        if (m_currentScene) {
            title += " [" + QString::fromStdString(m_currentScene->GetName()) + "]";

            if (m_isSceneModified) {
                title += "*";
            }
        }
    }

    setWindowTitle(title);
}

void MainWindow::updateActions()
{
    bool hasProject = (m_currentProject != nullptr);
    bool hasScene = (m_currentScene != nullptr);

    m_saveSceneAction->setEnabled(hasScene);
    m_saveSceneAsAction->setEnabled(hasScene);
    m_projectSettingsAction->setEnabled(hasProject);

    m_playAction->setEnabled(hasScene && !m_isPlaying);
    m_pauseAction->setEnabled(m_isPlaying);
    m_stopAction->setEnabled(m_isPlaying);

    // Update game runner toolbar
    if (m_gameRunnerToolbar) {
        m_gameRunnerToolbar->setPlayingState(m_isPlaying);
    }

    // Update edit menu actions
    bool canUndo = m_undoSystem && m_undoSystem->canUndo();
    bool canRedo = m_undoSystem && m_undoSystem->canRedo();
    bool hasClipboard = m_clipboard && m_clipboard->hasData();
    bool hasSelectedNode = hasScene && m_sceneTreePanel && m_sceneTreePanel->getSelectedNode() &&
                          m_sceneTreePanel->getSelectedNode() != m_currentScene->GetRootNode();



    m_undoAction->setEnabled(canUndo);
    m_redoAction->setEnabled(canRedo);
    m_cutAction->setEnabled(hasSelectedNode);
    m_copyAction->setEnabled(hasSelectedNode);
    m_pasteAction->setEnabled(hasClipboard && hasScene);
    m_deleteAction->setEnabled(hasSelectedNode);
    m_duplicateAction->setEnabled(hasSelectedNode);

    // Update action text with descriptions
    if (canUndo && m_undoSystem) {
        QString undoDesc = m_undoSystem->getUndoDescription();
        m_undoAction->setText(QString("&Undo %1").arg(undoDesc.isEmpty() ? "" : undoDesc));
    } else {
        m_undoAction->setText("&Undo");
    }

    if (canRedo && m_undoSystem) {
        QString redoDesc = m_undoSystem->getRedoDescription();
        m_redoAction->setText(QString("&Redo %1").arg(redoDesc.isEmpty() ? "" : redoDesc));
    } else {
        m_redoAction->setText("&Redo");
    }

    // Update dock widget visibility checkboxes
    m_toggleSceneTreeAction->setChecked(m_sceneTreeDock->isVisible());
    m_toggleAssetBrowserAction->setChecked(m_assetBrowserDock->isVisible());
    m_toggleInspectorAction->setChecked(m_inspectorDock->isVisible());
    m_toggleScriptEditorAction->setChecked(m_scriptEditorDock->isVisible());
    m_toggleConsoleAction->setChecked(m_consoleDock->isVisible());
}

// File menu slots
void MainWindow::onNewScene()
{
    if (!m_currentProject) {
        QMessageBox::information(this, "No Project", "Please open a project first.");
        return;
    }

    m_currentScene = std::make_unique<Lupine::Scene>("New Scene");
    m_currentScene->CreateRootNode<Lupine::Node>("Root");
    m_currentScenePath.clear();
    m_isSceneModified = true;

    m_sceneTreePanel->setScene(m_currentScene.get());
    m_sceneViewPanel->setScene(m_currentScene.get());
    m_inspectorPanel->setScene(m_currentScene.get());

    // Initialize editor systems with new scene
    if (m_undoSystem) {
        m_undoSystem->setScene(m_currentScene.get());
        // Set undo depth from project settings
        int undoDepth = m_currentProject->GetSettingValue<int>("editor/undo_depth", 25);
        m_undoSystem->setMaxUndoSteps(undoDepth);
    }
    if (m_clipboard) {
        m_clipboard->setScene(m_currentScene.get());
    }

    updateWindowTitle();
    updateActions();
}

void MainWindow::onOpenScene()
{
    if (!m_currentProject) {
        QMessageBox::information(this, "No Project", "Please open a project first.");
        return;
    }

    QString scenePath = QFileDialog::getOpenFileName(
        this,
        "Open Scene",
        QFileInfo(m_currentProjectPath).absolutePath(),
        "Scene Files (*.scene)"
    );

    if (!scenePath.isEmpty()) {
        try {
            auto scene = std::make_unique<Lupine::Scene>();
            if (scene->LoadFromFile(scenePath.toStdString())) {
                m_currentScene = std::move(scene);
                m_currentScenePath = scenePath;
                m_isSceneModified = false;

                // Debug: Check if scene loaded correctly
                auto* rootNode = m_currentScene->GetRootNode();
                if (rootNode) {
                    m_consolePanel->addMessage(QString("Scene loaded with root node: %1, children: %2")
                        .arg(QString::fromStdString(rootNode->GetName()))
                        .arg(rootNode->GetChildren().size()));
                } else {
                    m_consolePanel->addMessage("Warning: Scene loaded but no root node found");
                }

                qDebug() << "MainWindow::openProject - Setting scene on panels";
                m_sceneTreePanel->setScene(m_currentScene.get());
                qDebug() << "MainWindow::openProject - Scene tree panel set";
                m_sceneViewPanel->setScene(m_currentScene.get());
                qDebug() << "MainWindow::openProject - Scene view panel set";
                m_inspectorPanel->setScene(m_currentScene.get());
                qDebug() << "MainWindow::openProject - Inspector panel set";

                // Initialize editor systems with loaded scene
                if (m_undoSystem) {
                    m_undoSystem->setScene(m_currentScene.get());
                    // Set undo depth from project settings
                    int undoDepth = m_currentProject->GetSettingValue<int>("editor/undo_depth", 25);
                    m_undoSystem->setMaxUndoSteps(undoDepth);
                }
                if (m_clipboard) {
                    m_clipboard->setScene(m_currentScene.get());
                }

                updateWindowTitle();
                updateActions();

                m_consolePanel->addMessage("Scene loaded: " + scenePath);
            } else {
                QMessageBox::critical(this, "Error", "Failed to load scene: " + scenePath);
            }
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", QString("Exception loading scene: %1").arg(e.what()));
        }
    }
}

void MainWindow::onSaveScene()
{
    if (!m_currentScene) return;

    if (m_currentScenePath.isEmpty()) {
        onSaveSceneAs();
        return;
    }

    if (m_currentScene->SaveToFile(m_currentScenePath.toStdString())) {
        m_isSceneModified = false;
        updateWindowTitle();
        m_consolePanel->addMessage("Scene saved: " + m_currentScenePath);
    } else {
        QMessageBox::critical(this, "Error", "Failed to save scene.");
    }
}

void MainWindow::onSaveSceneAs()
{
    if (!m_currentScene) return;

    QString scenePath = QFileDialog::getSaveFileName(
        this,
        "Save Scene As",
        QFileInfo(m_currentProjectPath).absolutePath() + "/" + QString::fromStdString(m_currentScene->GetName()) + ".scene",
        "Scene Files (*.scene)"
    );

    if (!scenePath.isEmpty()) {
        if (m_currentScene->SaveToFile(scenePath.toStdString())) {
            m_currentScenePath = scenePath;
            m_isSceneModified = false;
            updateWindowTitle();
            m_consolePanel->addMessage("Scene saved: " + scenePath);
        } else {
            QMessageBox::critical(this, "Error", "Failed to save scene.");
        }
    }
}

void MainWindow::onProjectSettings()
{
    if (!m_currentProject) {
        QMessageBox::warning(this, "No Project", "Please open a project first.");
        return;
    }

    ProjectSettingsDialog dialog(m_currentProject.get(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_consolePanel->addMessage("Project settings updated.");
    }
}

void MainWindow::onExportProject()
{
    if (!m_currentProject) {
        QMessageBox::warning(this, "No Project", "Please open a project first.");
        return;
    }

    // Save current scene if modified
    if (m_isSceneModified && m_currentScene) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Save Scene",
            "The current scene has unsaved changes. Save before exporting?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Cancel) {
            return;
        } else if (reply == QMessageBox::Yes) {
            onSaveScene();
        }
    }

    // Open export dialog
    Lupine::ExportDialog dialog(m_currentProject.get(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_consolePanel->addMessage("Project export completed successfully.");
    }
}

void MainWindow::onExit()
{
    close();
}

// Edit menu slots


void MainWindow::onUndo()
{
    qDebug() << "onUndo called - this should appear in console";
    statusBar()->showMessage("Undo called!");
    if (m_undoSystem && m_undoSystem->canUndo()) {
        qDebug() << "Undo system exists and can undo, calling undo()";
        m_undoSystem->undo();

        // Refresh UI after undo
        if (m_sceneTreePanel) {
            m_sceneTreePanel->refreshTree();
        }
        if (m_sceneViewPanel) {
            m_sceneViewPanel->forceSceneRefresh();
        }

        m_isSceneModified = true;
        updateWindowTitle();
        statusBar()->showMessage("Undo performed");
    } else {
        qDebug() << "Cannot undo - undo system:" << (m_undoSystem ? "exists" : "null")
                 << "canUndo:" << (m_undoSystem ? m_undoSystem->canUndo() : false);
        statusBar()->showMessage("Cannot undo");
    }
}

void MainWindow::onRedo()
{
    qDebug() << "onRedo called";
    statusBar()->showMessage("Redo called!");
    if (m_undoSystem && m_undoSystem->canRedo()) {
        qDebug() << "Redo system exists and can redo, calling redo()";
        m_undoSystem->redo();

        // Refresh UI after redo
        if (m_sceneTreePanel) {
            m_sceneTreePanel->refreshTree();
        }
        if (m_sceneViewPanel) {
            m_sceneViewPanel->forceSceneRefresh();
        }

        m_isSceneModified = true;
        updateWindowTitle();
        statusBar()->showMessage("Redo performed");
    } else {
        qDebug() << "Cannot redo";
        statusBar()->showMessage("Cannot redo");
    }
}

void MainWindow::onCut()
{
    // For now, default to scene tree operations
    // TODO: Add proper focus detection for other panels
    if (m_sceneTreePanel && m_currentScene) {
        Lupine::Node* selectedNode = m_sceneTreePanel->getSelectedNode();
        if (selectedNode && selectedNode != m_currentScene->GetRootNode()) {
            if (m_clipboard) {
                m_clipboard->cutNode(selectedNode, "Cut Node");
                statusBar()->showMessage("Node cut to clipboard");
            }
        }
    }
}

void MainWindow::onCopy()
{
    // For now, default to scene tree operations
    // TODO: Add proper focus detection for other panels
    if (m_sceneTreePanel && m_currentScene) {
        Lupine::Node* selectedNode = m_sceneTreePanel->getSelectedNode();
        if (selectedNode && selectedNode != m_currentScene->GetRootNode()) {
            if (m_clipboard) {
                m_clipboard->copyNode(selectedNode, "Copy Node");
                statusBar()->showMessage("Node copied to clipboard");
            }
        }
    }
}

void MainWindow::onPaste()
{
    qDebug() << "onPaste called";
    statusBar()->showMessage("Paste called!");
    // For now, default to scene tree operations
    // TODO: Add proper focus detection for other panels
    if (m_clipboard && m_clipboard->hasData() && m_currentScene && m_sceneTreePanel) {
        Lupine::Node* selectedNode = m_sceneTreePanel->getSelectedNode();
        Lupine::Node* targetParent = selectedNode ? selectedNode : m_currentScene->GetRootNode();

        auto pastedNode = m_clipboard->pasteNode(targetParent);
        if (pastedNode) {
            // Record undo action
            if (m_undoSystem) {
                m_undoSystem->recordNodeCreated(pastedNode.get(), "Paste Node");
            }

            // Add to parent
            targetParent->AddChild(std::move(pastedNode));

            // Refresh UI
            m_sceneTreePanel->refreshTree();
            if (m_sceneViewPanel) {
                m_sceneViewPanel->forceSceneRefresh();
            }

            m_isSceneModified = true;
            updateWindowTitle();
            statusBar()->showMessage("Node pasted from clipboard");
        } else {
            statusBar()->showMessage("Failed to paste from clipboard");
        }
    } else {
        statusBar()->showMessage("Cannot paste - no clipboard data or scene");
    }
}

void MainWindow::onDelete()
{
    // For now, default to scene tree operations
    // TODO: Add proper focus detection for other panels
    if (m_sceneTreePanel && m_currentScene) {
        Lupine::Node* selectedNode = m_sceneTreePanel->getSelectedNode();
        if (selectedNode && selectedNode != m_currentScene->GetRootNode()) {
            // Validate the node before proceeding
            if (!selectedNode->IsValidNode()) {
                return; // Node is already invalid, nothing to delete
            }

            // Record undo action before deletion
            if (m_undoSystem) {
                try {
                    m_undoSystem->recordNodeDeleted(selectedNode, "Delete Node");
                } catch (...) {
                    // If undo recording fails, continue with deletion but log it
                    // This prevents the deletion from being blocked by undo system issues
                }
            }

            // Delete the node safely
            try {
                if (selectedNode->GetParent() && selectedNode->GetParent()->IsValidNode()) {
                    selectedNode->GetParent()->RemoveChild(selectedNode->GetUUID());
                }
            } catch (...) {
                // If deletion fails, the node might already be deleted
                // Continue with UI refresh to ensure consistency
            }

            // Refresh UI
            m_sceneTreePanel->refreshTree();
            if (m_sceneViewPanel) {
                m_sceneViewPanel->forceSceneRefresh();
            }
            if (m_inspectorPanel) {
                m_inspectorPanel->setSelectedNode(nullptr);
            }

            m_isSceneModified = true;
            updateWindowTitle();
            statusBar()->showMessage("Node deleted");
        }
    }
}

void MainWindow::onDuplicate()
{
    // For now, default to scene tree operations
    // TODO: Add proper focus detection for other panels
    if (m_sceneTreePanel && m_currentScene) {
        // Duplicate from scene tree - delegate to scene tree panel
        m_sceneTreePanel->onDuplicateNode();
        statusBar()->showMessage("Node duplicated");
    }
}

// View menu slots
void MainWindow::onToggleSceneTree()
{
    m_sceneTreeDock->setVisible(!m_sceneTreeDock->isVisible());
}

void MainWindow::onToggleAssetBrowser()
{
    m_assetBrowserDock->setVisible(!m_assetBrowserDock->isVisible());
}

void MainWindow::onToggleInspector()
{
    m_inspectorDock->setVisible(!m_inspectorDock->isVisible());
}

void MainWindow::onToggleScriptEditor()
{
    m_scriptEditorDock->setVisible(!m_scriptEditorDock->isVisible());
}

void MainWindow::onToggleConsole()
{
    m_consoleDock->setVisible(!m_consoleDock->isVisible());
}

void MainWindow::onToggleModelPreviews()
{
    bool enabled = m_enableModelPreviewsAction->isChecked();
    qDebug() << "MainWindow: Toggling model previews to:" << enabled;

    if (m_assetBrowserPanel) {
        m_assetBrowserPanel->enableModelPreviews(enabled);

        if (enabled) {
            // Show a warning about potential performance impact
            QMessageBox::information(this, "Model Previews Enabled",
                "3D model previews have been enabled. This may impact performance when browsing folders with many model files.\n\n"
                "You can disable this feature at any time through the View menu.");
        }
    }
}

void MainWindow::onResetLayout()
{
    // Reset to default layout
    m_sceneTreeDock->setVisible(true);
    m_assetBrowserDock->setVisible(true);
    m_inspectorDock->setVisible(true);
    m_scriptEditorDock->setVisible(true);
    m_consoleDock->setVisible(true);

    // Reset dock positions
    addDockWidget(Qt::LeftDockWidgetArea, m_sceneTreeDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_assetBrowserDock);
    addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);
    addDockWidget(Qt::RightDockWidgetArea, m_scriptEditorDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_consoleDock);

    tabifyDockWidget(m_sceneTreeDock, m_assetBrowserDock);
    tabifyDockWidget(m_inspectorDock, m_scriptEditorDock);

    m_sceneTreeDock->raise();
    m_inspectorDock->raise();
}

void MainWindow::onViewMode2D()
{
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setViewMode(SceneViewPanel::ViewMode::Mode2D);
    }
}

void MainWindow::onViewMode3D()
{
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setViewMode(SceneViewPanel::ViewMode::Mode3D);
    }
}

void MainWindow::onToggleGrid()
{
    if (m_sceneViewPanel) {
        bool gridVisible = m_toggleGridAction->isChecked();
        m_sceneViewPanel->setGridVisible(gridVisible);
    }
}

void MainWindow::onMoveGizmo()
{
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setActiveGizmo(GizmoType::Move);
    }
}

void MainWindow::onRotateGizmo()
{
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setActiveGizmo(GizmoType::Rotate);
    }
}

void MainWindow::onScaleGizmo()
{
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setActiveGizmo(GizmoType::Scale);
    }
}

// Tools menu slots
void MainWindow::onPlayScene()
{
    if (!m_currentScene) {
        m_consolePanel->addMessage("No scene loaded to play.");
        return;
    }

    // Save the current scene to a temporary file for the runtime
    QString tempScenePath = QDir::temp().filePath("lupine_editor_temp_scene.scene");
    if (!m_currentScene->SaveToFile(tempScenePath.toStdString())) {
        m_consolePanel->addMessage("Failed to save scene for runtime.");
        return;
    }

    // Stop any existing runtime process
    if (m_runtimeProcess && m_runtimeProcess->state() != QProcess::NotRunning) {
        m_runtimeProcess->kill();
        m_runtimeProcess->waitForFinished(3000);
    }

    // Create runtime process if it doesn't exist
    if (!m_runtimeProcess) {
        m_runtimeProcess = new QProcess(this);
        connect(m_runtimeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &MainWindow::onRuntimeProcessFinished);
        connect(m_runtimeProcess, &QProcess::errorOccurred,
                this, &MainWindow::onRuntimeProcessError);
    }

    // Find the runtime executable
    QString runtimePath = QApplication::applicationDirPath() + "/lupine-runtime.exe";
    if (!QFile::exists(runtimePath)) {
        // Try relative path for development builds (Debug)
        runtimePath = QApplication::applicationDirPath() + "/../bin/Debug/lupine-runtime.exe";
        if (!QFile::exists(runtimePath)) {
            // Try relative path for development builds (Release)
            runtimePath = QApplication::applicationDirPath() + "/../bin/Release/lupine-runtime.exe";
            if (!QFile::exists(runtimePath)) {
                m_consolePanel->addMessage("Runtime executable not found. Tried paths: " +
                    QApplication::applicationDirPath() + "/lupine-runtime.exe, " +
                    QApplication::applicationDirPath() + "/../bin/Debug/lupine-runtime.exe, " +
                    QApplication::applicationDirPath() + "/../bin/Release/lupine-runtime.exe");
                return;
            }
        }
    }

    // Start the runtime with the scene and project settings
    QStringList arguments;
    if (m_currentProject) {
        // Pass project file to get proper settings, then override with specific scene
        arguments << "--project" << m_currentProjectPath;
        arguments << "--scene" << tempScenePath;
    } else {
        // Fallback to scene-only mode if no project is loaded
        arguments << "--scene" << tempScenePath;
    }
    arguments << "--title" << "Lupine Runtime - " + QString::fromStdString(m_currentScene->GetName());

    m_runtimeProcess->start(runtimePath, arguments);

    if (m_runtimeProcess->waitForStarted(5000)) {
        m_isPlaying = true;
        updateActions();
        m_consolePanel->addMessage("Runtime started successfully.");
    } else {
        m_consolePanel->addMessage("Failed to start runtime: " + m_runtimeProcess->errorString());
    }
}

void MainWindow::onPlayGame()
{
    if (!m_currentProject) {
        m_consolePanel->addMessage("No project loaded to play.");
        return;
    }

    // Get the main scene from the project
    std::string mainSceneName = m_currentProject->GetMainScene();
    if (mainSceneName.empty()) {
        m_consolePanel->addMessage("No main scene set in project.");
        return;
    }

    // Construct the full path to the main scene
    QFileInfo projectFile(m_currentProjectPath);
    QString mainScenePath = projectFile.absolutePath() + "/" + QString::fromStdString(mainSceneName);

    if (!QFile::exists(mainScenePath)) {
        m_consolePanel->addMessage("Main scene file not found: " + mainScenePath);
        return;
    }

    // Stop any existing runtime process
    if (m_runtimeProcess && m_runtimeProcess->state() != QProcess::NotRunning) {
        m_runtimeProcess->kill();
        m_runtimeProcess->waitForFinished(3000);
    }

    // Create runtime process if it doesn't exist
    if (!m_runtimeProcess) {
        m_runtimeProcess = new QProcess(this);
        connect(m_runtimeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &MainWindow::onRuntimeProcessFinished);
        connect(m_runtimeProcess, &QProcess::errorOccurred,
                this, &MainWindow::onRuntimeProcessError);
    }

    // Find the runtime executable
    QString runtimePath = QApplication::applicationDirPath() + "/lupine-runtime.exe";
    if (!QFile::exists(runtimePath)) {
        // Try relative path for development builds (Debug)
        runtimePath = QApplication::applicationDirPath() + "/../bin/Debug/lupine-runtime.exe";
        if (!QFile::exists(runtimePath)) {
            // Try relative path for development builds (Release)
            runtimePath = QApplication::applicationDirPath() + "/../bin/Release/lupine-runtime.exe";
            if (!QFile::exists(runtimePath)) {
                m_consolePanel->addMessage("Runtime executable not found. Tried paths: " +
                    QApplication::applicationDirPath() + "/lupine-runtime.exe, " +
                    QApplication::applicationDirPath() + "/../bin/Debug/lupine-runtime.exe, " +
                    QApplication::applicationDirPath() + "/../bin/Release/lupine-runtime.exe");
                return;
            }
        }
    }

    // Start the runtime with the project file (which includes main scene and settings)
    QStringList arguments;
    arguments << "--project" << m_currentProjectPath;
    arguments << "--title" << "Lupine Runtime - " + QString::fromStdString(m_currentProject->GetName());

    m_runtimeProcess->start(runtimePath, arguments);

    if (m_runtimeProcess->waitForStarted(5000)) {
        m_isPlaying = true;
        updateActions();
        m_consolePanel->addMessage("Game started successfully.");
    } else {
        m_consolePanel->addMessage("Failed to start game: " + m_runtimeProcess->errorString());
    }
}

void MainWindow::onPauseScene()
{
    if (!m_isPlaying) return;

    // Note: The current runtime doesn't support pausing, so we'll just log it
    m_consolePanel->addMessage("Pause requested (runtime doesn't support pausing yet).");
}

void MainWindow::onStopScene()
{
    if (!m_isPlaying) return;

    // Stop the runtime process
    if (m_runtimeProcess && m_runtimeProcess->state() != QProcess::NotRunning) {
        m_runtimeProcess->terminate();
        if (!m_runtimeProcess->waitForFinished(3000)) {
            m_runtimeProcess->kill();
            m_runtimeProcess->waitForFinished(1000);
        }
    }

    m_isPlaying = false;
    updateActions();
    m_consolePanel->addMessage("Runtime stopped.");
}

void MainWindow::onGlobalsManager()
{
    if (!m_globalsManagerDialog) {
        m_globalsManagerDialog = new GlobalsManagerDialog(m_currentProject.get(), this);
    }

    m_globalsManagerDialog->show();
    m_globalsManagerDialog->raise();
    m_globalsManagerDialog->activateWindow();
}

void MainWindow::onInputMapper()
{
    // Create and show the Input Mapper dialog
    QDialog* inputMapperDialog = new QDialog(this);
    inputMapperDialog->setWindowTitle("Input Mapper");
    inputMapperDialog->setModal(true);
    inputMapperDialog->resize(800, 600);

    // Create the action mapping panel
    Lupine::ActionMappingPanel* actionMappingPanel = new Lupine::ActionMappingPanel(inputMapperDialog);

    // Load action map from current project or create default
    static Lupine::ActionMap s_globalActionMap;
    if (m_currentProject) {
        // Try to load action map from project
        auto actionMapSetting = m_currentProject->GetSetting("input/action_map");
        if (actionMapSetting && std::holds_alternative<std::string>(*actionMapSetting)) {
            std::string actionMapJson = std::get<std::string>(*actionMapSetting);
            try {
                auto json = nlohmann::json::parse(actionMapJson);
                s_globalActionMap.LoadFromJson(json);
            } catch (const std::exception& e) {
                std::cerr << "Error loading action map from project: " << e.what() << std::endl;
                s_globalActionMap = Lupine::ActionMap::CreateDefault();
            }
        } else {
            s_globalActionMap = Lupine::ActionMap::CreateDefault();
        }
    } else {
        s_globalActionMap = Lupine::ActionMap::CreateDefault();
    }
    actionMappingPanel->SetActionMap(&s_globalActionMap);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(inputMapperDialog);
    layout->addWidget(actionMappingPanel);

    // Add OK/Cancel buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");
    QPushButton* applyButton = new QPushButton("Apply");

    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    layout->addLayout(buttonLayout);

    // Connect buttons
    connect(okButton, &QPushButton::clicked, inputMapperDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, inputMapperDialog, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, [&]() {
        // Apply the action map to the input manager
        s_globalActionMap.ApplyToInputManager();

        // Save to project if we have one
        if (m_currentProject) {
            try {
                nlohmann::json actionMapJson = s_globalActionMap.ToJson();
                std::string actionMapString = actionMapJson.dump();
                m_currentProject->SetSetting("input/action_map", actionMapString);
                m_consolePanel->addMessage("Input mappings saved to project.");
            } catch (const std::exception& e) {
                m_consolePanel->addMessage("Error saving input mappings: " + QString::fromStdString(e.what()));
            }
        }
    });

    // Show the dialog
    if (inputMapperDialog->exec() == QDialog::Accepted) {
        // Apply the action map to the input manager
        s_globalActionMap.ApplyToInputManager();

        // Save to project if we have one
        if (m_currentProject) {
            try {
                nlohmann::json actionMapJson = s_globalActionMap.ToJson();
                std::string actionMapString = actionMapJson.dump();
                m_currentProject->SetSetting("input/action_map", actionMapString);
                m_consolePanel->addMessage("Input mappings applied and saved to project.");
            } catch (const std::exception& e) {
                m_consolePanel->addMessage("Error saving input mappings: " + QString::fromStdString(e.what()));
            }
        } else {
            m_consolePanel->addMessage("Input mappings applied.");
        }
    }

    inputMapperDialog->deleteLater();
}

void MainWindow::onPixelPainter()
{
    if (!m_pixelPainterDialog) {
        m_pixelPainterDialog = new PixelPainterDialog(this);
    }

    m_pixelPainterDialog->show();
    m_pixelPainterDialog->raise();
    m_pixelPainterDialog->activateWindow();
}

void MainWindow::onScribbler()
{
    if (!m_scribblerDialog) {
        m_scribblerDialog = new ScribblerDialog(this);
    }

    m_scribblerDialog->show();
    m_scribblerDialog->raise();
    m_scribblerDialog->activateWindow();
}

void MainWindow::onVoxelBlocker()
{
    if (!m_voxelBlockerDialog) {
        m_voxelBlockerDialog = new VoxelBlockerDialog(this);
    }

    m_voxelBlockerDialog->show();
    m_voxelBlockerDialog->raise();
    m_voxelBlockerDialog->activateWindow();
}

void MainWindow::onTilemapBuilder3D()
{
    if (!m_tilemapBuilder3DDialog) {
        m_tilemapBuilder3DDialog = new TilemapBuilder3DDialog(this);
    }

    m_tilemapBuilder3DDialog->show();
    m_tilemapBuilder3DDialog->raise();
    m_tilemapBuilder3DDialog->activateWindow();
}

void MainWindow::onTilemap25DPainter()
{
    if (!m_tilemap25DPainterDialog) {
        m_tilemap25DPainterDialog = new Tilemap25DPainterDialog(this);
    }

    m_tilemap25DPainterDialog->show();
    m_tilemap25DPainterDialog->raise();
    m_tilemap25DPainterDialog->activateWindow();
}

void MainWindow::onVisualScripter()
{
    if (!m_visualScripterDialog) {
        m_visualScripterDialog = new VisualScripterDialog(this);
    }

    m_visualScripterDialog->show();
    m_visualScripterDialog->raise();
    m_visualScripterDialog->activateWindow();
}

void MainWindow::onTweenAnimator()
{
    if (!m_tweenAnimatorDialog) {
        m_tweenAnimatorDialog = new TweenAnimatorDialog(this);
        if (m_currentScene) {
            m_tweenAnimatorDialog->SetScene(m_currentScene.get());
        }
    }

    m_tweenAnimatorDialog->show();
    m_tweenAnimatorDialog->raise();
    m_tweenAnimatorDialog->activateWindow();
}


void MainWindow::onSpriteAnimator()
{
    if (!m_spriteAnimatorDialog) {
        m_spriteAnimatorDialog = new SpriteAnimatorDialog(this);
    }

    m_spriteAnimatorDialog->show();
    m_spriteAnimatorDialog->raise();
    m_spriteAnimatorDialog->activateWindow();
}

void MainWindow::onStateAnimator()
{
    if (!m_stateAnimatorDialog) {
        m_stateAnimatorDialog = new StateAnimatorDialog(this);
    }

    m_stateAnimatorDialog->show();
    m_stateAnimatorDialog->raise();
    m_stateAnimatorDialog->activateWindow();
}

void MainWindow::onTilesetEditor()
{
    if (!m_tilesetEditorDialog) {
        m_tilesetEditorDialog = new TilesetEditorDialog(this);
    }

    m_tilesetEditorDialog->show();
    m_tilesetEditorDialog->raise();
    m_tilesetEditorDialog->activateWindow();
}

void MainWindow::onTileset3DEditor()
{
    if (!m_tileset3DEditorDialog) {
        m_tileset3DEditorDialog = new Tileset3DEditorDialog(this);
    }

    m_tileset3DEditorDialog->show();
    m_tileset3DEditorDialog->raise();
    m_tileset3DEditorDialog->activateWindow();
}

void MainWindow::onTilemapPainter()
{
    if (!m_tilemapPainterDialog) {
        m_tilemapPainterDialog = new TilemapPainterDialog(this);
    }

    m_tilemapPainterDialog->show();
    m_tilemapPainterDialog->raise();
    m_tilemapPainterDialog->activateWindow();
}

// Help menu slots
void MainWindow::onAbout()
{
    QMessageBox::about(this, "About Lupine Editor",
        QString("Lupine Game Engine Editor\n"
                "Version %1\n\n"
                "A modern game engine with Qt-based editor.")
        .arg("1.0.0"));
}

void MainWindow::onDocumentation()
{
    QMessageBox::information(this, "Documentation", "Documentation is not yet available.");
}

// Panel interaction slots
void MainWindow::onSceneNodeSelected()
{
    try {
        // Additional handling when scene node is selected
        statusBar()->showMessage("Node selected");

        // Update actions when selection changes
        updateActions();
    } catch (...) {
        // Ignore exceptions in status bar updates to prevent crashes
        qWarning() << "Exception occurred during node selection status update";
    }
}

void MainWindow::onSceneNodeDeleted(Lupine::Node* deletedNode)
{
    // Clear inspector if the deleted node was selected
    if (m_inspectorPanel) {
        m_inspectorPanel->setSelectedNode(nullptr);
    }
    statusBar()->showMessage("Node deleted");
}

void MainWindow::onSceneNodeDuplicated(Lupine::Node* originalNode, Lupine::Node* duplicatedNode)
{
    Q_UNUSED(originalNode)

    // Update inspector to show the duplicated node
    if (m_inspectorPanel) {
        m_inspectorPanel->setSelectedNode(duplicatedNode);
    }

    // Force scene view to refresh its rendering completely
    if (m_sceneViewPanel) {
        m_sceneViewPanel->forceSceneRefresh();
    }

    // Mark scene as modified
    m_isSceneModified = true;
    updateWindowTitle();

    statusBar()->showMessage("Node duplicated");
}

void MainWindow::onAssetSelected()
{
    // Additional handling when asset is selected
    statusBar()->showMessage("Asset selected");
}

void MainWindow::onRuntimeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isPlaying = false;
    updateActions();

    if (exitStatus == QProcess::CrashExit) {
        m_consolePanel->addMessage("Runtime crashed with exit code: " + QString::number(exitCode));
    } else {
        m_consolePanel->addMessage("Runtime finished with exit code: " + QString::number(exitCode));
    }

    // Clean up temporary scene file
    QString tempScenePath = QDir::temp().filePath("lupine_editor_temp_scene.scene");
    QFile::remove(tempScenePath);
}

void MainWindow::onRuntimeProcessError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start runtime process";
            break;
        case QProcess::Crashed:
            errorString = "Runtime process crashed";
            break;
        case QProcess::Timedout:
            errorString = "Runtime process timed out";
            break;
        case QProcess::WriteError:
            errorString = "Write error to runtime process";
            break;
        case QProcess::ReadError:
            errorString = "Read error from runtime process";
            break;
        default:
            errorString = "Unknown runtime process error";
            break;
    }

    m_consolePanel->addMessage("Runtime error: " + errorString);
    m_isPlaying = false;
    updateActions();
}

void MainWindow::initializeEngine()
{
    // Create engine instance for editor use
    m_engine = std::make_unique<Lupine::Engine>();

    // Initialize the engine for editor use (without SDL window)
    // We need to initialize the core systems like PhysicsManager for the editor to work properly
    if (!Lupine::ResourceManager::Initialize()) {
        m_consolePanel->addMessage("Failed to initialize ResourceManager for editor");
        return;
    }

    if (!Lupine::InputManager::Initialize()) {
        m_consolePanel->addMessage("Failed to initialize InputManager for editor");
        return;
    }

    if (!Lupine::AudioManager::Initialize()) {
        m_consolePanel->addMessage("Failed to initialize AudioManager for editor");
        return;
    }

    if (!Lupine::PhysicsManager::Initialize()) {
        m_consolePanel->addMessage("Failed to initialize PhysicsManager for editor");
        return;
    }

    // Initialize LocalizationManager
    Lupine::LocalizationManager::Instance().Initialize();
    m_consolePanel->addMessage("LocalizationManager initialized for editor");

    if (!Lupine::GlobalsManager::Initialize()) {
        m_consolePanel->addMessage("Failed to initialize GlobalsManager for editor");
        return;
    }

    // Initialize ScriptableObjectManager - Temporarily disabled
    // Lupine::ScriptableObjectManager::Instance().Initialize();
    // m_consolePanel->addMessage("ScriptableObjectManager initialized for editor");

    // Set the engine on the scene view panel
    // The renderer will be initialized when the OpenGL context is ready
    if (m_sceneViewPanel) {
        m_sceneViewPanel->setEngine(m_engine.get());
    }

    m_consolePanel->addMessage("Engine systems initialized for editor");
}

// Production tools slots
void MainWindow::onNotepad()
{
    if (!m_notepadDialog) {
        m_notepadDialog = new NotepadDialog(this);
    }
    m_notepadDialog->show();
    m_notepadDialog->raise();
    m_notepadDialog->activateWindow();
}

void MainWindow::onTodoList()
{
    if (!m_todoListDialog) {
        m_todoListDialog = new TodoListDialog(this);
    }
    m_todoListDialog->show();
    m_todoListDialog->raise();
    m_todoListDialog->activateWindow();
}

void MainWindow::onMilestoneTracker()
{
    if (!m_milestoneTrackerDialog) {
        m_milestoneTrackerDialog = new MilestoneTrackerDialog(this);
    }
    m_milestoneTrackerDialog->show();
    m_milestoneTrackerDialog->raise();
    m_milestoneTrackerDialog->activateWindow();
}

void MainWindow::onFeatureBugTracker()
{
    if (!m_featureBugTrackerDialog) {
        m_featureBugTrackerDialog = new FeatureBugTrackerDialog(this);
    }
    m_featureBugTrackerDialog->show();
    m_featureBugTrackerDialog->raise();
    m_featureBugTrackerDialog->activateWindow();
}

void MainWindow::onAssetProgressTracker()
{
    if (!m_assetProgressTrackerDialog) {
        m_assetProgressTrackerDialog = new AssetProgressTrackerDialog(this);
    }
    m_assetProgressTrackerDialog->show();
    m_assetProgressTrackerDialog->raise();
    m_assetProgressTrackerDialog->activateWindow();
}

// Builder tools slots
void MainWindow::onMenuBuilder()
{
    if (!m_menuBuilderDialog) {
        m_menuBuilderDialog = new MenuBuilderDialog(this);
    }
    m_menuBuilderDialog->show();
    m_menuBuilderDialog->raise();
    m_menuBuilderDialog->activateWindow();
}

void MainWindow::onTerrainPainter()
{
    if (!m_terrainPainterDialog) {
        m_terrainPainterDialog = new TerrainPainterDialog(this);
    }
    m_terrainPainterDialog->show();
    m_terrainPainterDialog->raise();
    m_terrainPainterDialog->activateWindow();
}

// Localization slots
void MainWindow::onLocalizationSettings()
{
    if (!m_localizationSettingsDialog) {
        m_localizationSettingsDialog = new LocalizationSettingsDialog(this);
    }
    m_localizationSettingsDialog->show();
    m_localizationSettingsDialog->raise();
    m_localizationSettingsDialog->activateWindow();
}

void MainWindow::onLocalizationTables()
{
    if (!m_localizationTablesDialog) {
        m_localizationTablesDialog = new LocalizationTablesDialog(this);
    }
    m_localizationTablesDialog->show();
    m_localizationTablesDialog->raise();
    m_localizationTablesDialog->activateWindow();
}

void MainWindow::onScriptableObjects()
{
    if (!m_scriptableObjectsDialog) {
        m_scriptableObjectsDialog = new ScriptableObjectsDialog(this);
    }
    m_scriptableObjectsDialog->show();
    m_scriptableObjectsDialog->raise();
    m_scriptableObjectsDialog->activateWindow();
}

void MainWindow::onAudioMixer()
{
    if (!m_audioMixerDialog) {
        m_audioMixerDialog = new AudioMixerDialog(this);
    }

    m_audioMixerDialog->show();
    m_audioMixerDialog->raise();
    m_audioMixerDialog->activateWindow();
}
