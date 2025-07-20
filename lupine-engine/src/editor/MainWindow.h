#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QActionGroup>
#include <QProcess>
#include <memory>

// Forward declarations
class SceneTreePanel;
class AssetBrowserPanel;
class FileBrowserPanel;
class SceneViewPanel;
class InspectorPanel;
class ScriptEditorPanel;
class ConsolePanel;
class ActionMappingPanel;
class ProjectSettingsDialog;
class TweenAnimatorDialog;
class SpriteAnimatorDialog;
class StateAnimatorDialog;
class TilesetEditorDialog;
class Tileset3DEditorDialog;
class TilemapPainterDialog;
class GlobalsManagerDialog;
class GameRunnerToolbar;
class PixelPainterDialog;
class ScribblerDialog;
class VoxelBlockerDialog;
class TilemapBuilder3DDialog;
class Tilemap25DPainterDialog;
class VisualScripterDialog;
class NotepadDialog;
class TodoListDialog;
class MilestoneTrackerDialog;
class FeatureBugTrackerDialog;
class AssetProgressTrackerDialog;
class TerrainPainterDialog;
class MenuBuilderDialog;
class ScriptableObjectsDialog;
class AudioMixerDialog;

class LocalizationSettingsDialog;
class LocalizationTablesDialog;
class EditorUndoSystem;
class EditorClipboard;

namespace Lupine {
    class Project;
    class Scene;
    class Engine;
        class Node;
        }

        class MainWindow : public QMainWindow
        {
            Q_OBJECT

            public:
                explicit MainWindow(QWidget *parent = nullptr);
                    ~MainWindow();

                            bool openProject(const QString& projectPath);
                                void closeProject();

                                // Getter for asset browser panel
                                AssetBrowserPanel* getAssetBrowserPanel() const { return m_assetBrowserPanel; }

                                // Editor system accessors
                                EditorUndoSystem* getUndoSystem() const { return m_undoSystem; }
                                EditorClipboard* getClipboard() const { return m_clipboard; }

                                private slots:
                                    // File menu
                                        void onNewScene();
                                            void onOpenScene();
                                                void onSaveScene();
                                                    void onSaveSceneAs();
    void onProjectSettings();
    void onExportProject();
    void onExit();
    
    // Edit menu
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onDuplicate();
    
    // View menu
    void onToggleSceneTree();
    void onToggleAssetBrowser();
    void onToggleInspector();
    void onToggleScriptEditor();
    void onToggleConsole();
    void onToggleModelPreviews();
    void onViewMode2D();
    void onViewMode3D();
    void onToggleGrid();
    void onMoveGizmo();
    void onRotateGizmo();
    void onScaleGizmo();
    void onResetLayout();
    
    // Tools menu
    void onPlayScene();
    void onPlayGame();
    void onPauseScene();
    void onStopScene();
    void onTweenAnimator();
    void onSpriteAnimator();
    void onStateAnimator();
    void onTilesetEditor();
    void onTileset3DEditor();
    void onTilemapPainter();
    void onGlobalsManager();
    void onInputMapper();
    void onPixelPainter();
    void onScribbler();
    void onVoxelBlocker();
    void onTilemapBuilder3D();
    void onTilemap25DPainter();
    void onVisualScripter();

    // Production tools slots
    void onNotepad();
    void onTodoList();
    void onMilestoneTracker();
    void onFeatureBugTracker();
    void onAssetProgressTracker();

    // Builder tools slots
    void onMenuBuilder();
    void onTerrainPainter();

    // Localization slots
    void onLocalizationSettings();
    void onLocalizationTables();

    // Scriptable Objects slot
    void onScriptableObjects();

    // Audio tools slot
    void onAudioMixer();

    // Help menu
    void onAbout();
    void onDocumentation();
    
    // Panel interactions
    void onSceneNodeSelected();
    void onSceneNodeDeleted(Lupine::Node* deletedNode);
    void onSceneNodeDuplicated(Lupine::Node* originalNode, Lupine::Node* duplicatedNode);
    void onAssetSelected();

    // Runtime process handling
    void onRuntimeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRuntimeProcessError(QProcess::ProcessError error);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBars();
    void setupFileBarHeight();
    void setupDockWidgets();
    void setupStatusBar();
    void setupConnections();
    void initializeEditorSystems();
    void updateWindowTitle();
    void updateActions();
    void initializeEngine();
    
    // Menu actions
    QAction* m_newSceneAction;
    QAction* m_openSceneAction;
    QAction* m_saveSceneAction;
    QAction* m_saveSceneAsAction;
    QAction* m_projectSettingsAction;
    QAction* m_exportProjectAction;
    QAction* m_exitAction;
    
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_deleteAction;
    QAction* m_duplicateAction;

    QAction* m_toggleSceneTreeAction;
    QAction* m_toggleAssetBrowserAction;
    QAction* m_toggleInspectorAction;
    QAction* m_toggleScriptEditorAction;
    QAction* m_toggleConsoleAction;
    QAction* m_resetLayoutAction;
    QAction* m_enableModelPreviewsAction;
    
    QAction* m_playAction;
    QAction* m_pauseAction;
    QAction* m_stopAction;
    QAction* m_tweenAnimatorAction;
    QAction* m_spriteAnimatorAction;
    QAction* m_stateAnimatorAction;
    QAction* m_tilesetEditorAction;
    QAction* m_tileset3DEditorAction;
    QAction* m_tilemapPainterAction;
    QAction* m_globalsManagerAction;
    QAction* m_inputMapperAction;
    QAction* m_pixelPainterAction;
    QAction* m_scribblerAction;
    QAction* m_voxelBlockerAction;
    QAction* m_tilemapBuilder3DAction;
    QAction* m_tilemap25DPainterAction;
    QAction* m_visualScripterAction;

    // Production tools actions
    QAction* m_notepadAction;
    QAction* m_todoListAction;
    QAction* m_milestoneTrackerAction;
    QAction* m_featureBugTrackerAction;
    QAction* m_assetProgressTrackerAction;

    // Builder tools actions
    QAction* m_menuBuilderAction;
    QAction* m_terrainPainterAction;

    // Localization actions
    QAction* m_localizationSettingsAction;
    QAction* m_localizationTablesAction;

    // Scriptable Objects action
    QAction* m_scriptableObjectsAction;

    // Audio tools action
    QAction* m_audioMixerAction;

    QAction* m_aboutAction;
    QAction* m_documentationAction;

    // View mode actions
    QAction* m_viewMode2DAction;
    QAction* m_viewMode3DAction;
    QActionGroup* m_viewModeGroup;

    // Grid actions
    QAction* m_toggleGridAction;

    // Gizmo actions
    QAction* m_moveGizmoAction;
    QAction* m_rotateGizmoAction;
    QAction* m_scaleGizmoAction;
    QActionGroup* m_gizmoGroup;

    // Toolbars
    GameRunnerToolbar* m_gameRunnerToolbar;
    
    // Dock widgets and panels
    QDockWidget* m_sceneTreeDock;
    QDockWidget* m_assetBrowserDock;
    QDockWidget* m_fileBrowserDock;
    QDockWidget* m_inspectorDock;
    QDockWidget* m_scriptEditorDock;
    QDockWidget* m_consoleDock;
    
    SceneTreePanel* m_sceneTreePanel;
    AssetBrowserPanel* m_assetBrowserPanel;
    FileBrowserPanel* m_fileBrowserPanel;
    SceneViewPanel* m_sceneViewPanel;
    InspectorPanel* m_inspectorPanel;
    ScriptEditorPanel* m_scriptEditorPanel;
    ConsolePanel* m_consolePanel;

    // Dialogs
    ActionMappingPanel* m_actionMappingPanel;
    ProjectSettingsDialog* m_projectSettingsDialog;
    TweenAnimatorDialog* m_tweenAnimatorDialog;
    SpriteAnimatorDialog* m_spriteAnimatorDialog;
    StateAnimatorDialog* m_stateAnimatorDialog;
    TilesetEditorDialog* m_tilesetEditorDialog;
    Tileset3DEditorDialog* m_tileset3DEditorDialog;
    TilemapPainterDialog* m_tilemapPainterDialog;
    GlobalsManagerDialog* m_globalsManagerDialog;
    PixelPainterDialog* m_pixelPainterDialog;
    ScribblerDialog* m_scribblerDialog;
    VoxelBlockerDialog* m_voxelBlockerDialog;
    TilemapBuilder3DDialog* m_tilemapBuilder3DDialog;
    Tilemap25DPainterDialog* m_tilemap25DPainterDialog;
    VisualScripterDialog* m_visualScripterDialog;

    // Production tools dialogs
    NotepadDialog* m_notepadDialog;
    TodoListDialog* m_todoListDialog;
    MilestoneTrackerDialog* m_milestoneTrackerDialog;
    FeatureBugTrackerDialog* m_featureBugTrackerDialog;
    AssetProgressTrackerDialog* m_assetProgressTrackerDialog;

    // Builder tools dialogs
    MenuBuilderDialog* m_menuBuilderDialog;
    TerrainPainterDialog* m_terrainPainterDialog;

    // Localization dialogs
    LocalizationSettingsDialog* m_localizationSettingsDialog;
    LocalizationTablesDialog* m_localizationTablesDialog;

    // Scriptable Objects dialog
    ScriptableObjectsDialog* m_scriptableObjectsDialog;

    // Audio tools dialog
    AudioMixerDialog* m_audioMixerDialog;

    // Editor systems
    EditorUndoSystem* m_undoSystem;
    EditorClipboard* m_clipboard;

    // Engine data
    std::unique_ptr<Lupine::Project> m_currentProject;
    std::unique_ptr<Lupine::Scene> m_currentScene;
    std::unique_ptr<Lupine::Engine> m_engine;

    QString m_currentProjectPath;
    QString m_currentScenePath;
    bool m_isSceneModified;
    bool m_isPlaying;

    // Runtime process for play mode
    QProcess* m_runtimeProcess;
};

#endif // MAINWINDOW_H
