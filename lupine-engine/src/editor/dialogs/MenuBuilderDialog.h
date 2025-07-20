#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QScrollArea>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDrag>
#include <QPainter>
#include <QFormLayout>
#include <memory>

// Forward declarations
namespace Lupine {
    class Scene;
    class Node;
    class Component;
}

class MenuSceneView;
class ComponentPalette;
class MenuInspector;
class EventBindingPanel;

/**
 * @brief Menu Builder Dialog for creating and editing UI menus and interfaces
 * 
 * This dialog provides a drag-and-drop interface for building UI menus with:
 * - Component palette with prefabs, UI templates, and all nodes
 * - 2D scene view for visual menu layout
 * - Inspector for component property editing
 * - Event binding system for interactive elements
 */
class MenuBuilderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MenuBuilderDialog(QWidget* parent = nullptr);
    ~MenuBuilderDialog();

    // File operations
    void newMenu();
    void openMenu();
    void saveMenu();
    void saveMenuAs();
    void exportMenu();

    // Menu operations
    void addUIElement(const QString& componentType);
    void deleteSelectedElement();
    void duplicateSelectedElement();
    void copySelectedElement();
    void pasteElement();

    // View operations
    void resetView();
    void fitToView();
    void zoomIn();
    void zoomOut();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File menu actions
    void onNewMenu();
    void onOpenMenu();
    void onSaveMenu();
    void onSaveMenuAs();
    void onExportMenu();

    // Edit menu actions
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onDuplicate();

    // View menu actions
    void onResetView();
    void onFitToView();
    void onZoomIn();
    void onZoomOut();
    void onToggleGrid();
    void onToggleSnap();

    // Scene interactions
    void onElementSelected(Lupine::Node* node);
    void onElementDeselected();
    void onElementAdded(const QString& componentType, const QPointF& position);
    void onElementMoved(Lupine::Node* node, const QPointF& newPosition);
    void onElementResized(Lupine::Node* node, const QSizeF& newSize);

    // Property changes
    void onPropertyChanged(const QString& propertyName, const QVariant& value);
    void onEventBindingChanged(const QString& eventName, const QString& action);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainLayout();
    void setupPalette();
    void setupSceneView();
    void setupInspectorTabs();
    void setupStatusBar();
    void setupConnections();

    void updateWindowTitle();
    void updateActions();
    void updateStatusBar();

    // File operations
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);

    // JSON serialization helpers
    void saveNodesToJson(Lupine::Node* node, QJsonArray& array);
    void loadNodesFromJson(const QJsonArray& array, Lupine::Node* parent);
    QJsonObject saveNodeToJson(Lupine::Node* node);
    Lupine::Node* loadNodeFromJson(const QJsonObject& obj, Lupine::Node* parent);

    // Component creation helpers
    void createSingleComponent(const QString& componentType, const QPointF& position);
    void createMainMenuPrefab(const QPointF& position);
    void createSettingsPanelPrefab(const QPointF& position);
    void createDialogBoxPrefab(const QPointF& position);
    void createButtonGroupTemplate(const QPointF& position);
    void createFormLayoutTemplate(const QPointF& position);
    void createButtonLabelTemplate(const QPointF& position);
    void createPanelBackgroundTemplate(const QPointF& position);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;

    // Left panel - Component palette
    ComponentPalette* m_palette;

    // Center panel - Scene view
    MenuSceneView* m_sceneView;

    // Right panel - Inspector and event binding
    QTabWidget* m_rightTabs;
    MenuInspector* m_inspector;
    EventBindingPanel* m_eventBinding;

    // Menu actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_exitAction;

    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_deleteAction;
    QAction* m_duplicateAction;

    QAction* m_resetViewAction;
    QAction* m_fitToViewAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_toggleGridAction;
    QAction* m_toggleSnapAction;

    // Data
    std::unique_ptr<Lupine::Scene> m_menuScene;
    QString m_currentFilePath;
    bool m_modified;
    Lupine::Node* m_selectedElement;

    // Clipboard
    QString m_clipboardData;

    // Settings
    bool m_gridVisible;
    bool m_snapToGrid;
    float m_gridSize;
    QSizeF m_canvasSize;
};

/**
 * @brief Component palette widget for dragging UI elements
 */
class ComponentPalette : public QWidget
{
    Q_OBJECT

public:
    explicit ComponentPalette(QWidget* parent = nullptr);

signals:
    void componentRequested(const QString& componentType, const QPointF& position);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onTabChanged(int index);
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupPrefabsTab();
    void setupTemplatesTab();
    void setupAllNodesTab();
    void populateComponentList(QListWidget* list, const QStringList& components);
    void startDrag(QListWidgetItem* item);

    QVBoxLayout* m_layout;
    QTabWidget* m_tabWidget;
    QListWidget* m_prefabsList;
    QListWidget* m_templatesList;
    QListWidget* m_allNodesList;

    // Drag state
    QPoint m_dragStartPosition;
    QListWidgetItem* m_dragItem;
};

/**
 * @brief Menu inspector for editing component properties
 */
class MenuInspector : public QWidget
{
    Q_OBJECT

public:
    explicit MenuInspector(QWidget* parent = nullptr);

    void setSelectedNode(Lupine::Node* node);
    void clearSelection();

signals:
    void propertyChanged(const QString& propertyName, const QVariant& value);

private slots:
    void onPropertyValueChanged();

private:
    void setupUI();
    void updateProperties();
    void createPropertyEditor(const QString& name, const QVariant& value, const QString& type);

    QVBoxLayout* m_layout;
    QScrollArea* m_scrollArea;
    QWidget* m_propertiesWidget;
    QVBoxLayout* m_propertiesLayout;
    Lupine::Node* m_selectedNode;
    QMap<QString, QWidget*> m_propertyEditors;
};

/**
 * @brief Event binding panel for UI interactions
 */
class EventBindingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit EventBindingPanel(QWidget* parent = nullptr);

    void setSelectedNode(Lupine::Node* node);
    void clearSelection();

signals:
    void eventBindingChanged(const QString& eventName, const QString& action);

private slots:
    void onEventBindingChanged();
    void onAddBinding();
    void onRemoveBinding();

private:
    void setupUI();
    void updateEventBindings();
    void populatePresetActions();

    QVBoxLayout* m_layout;
    QTreeWidget* m_bindingsTree;
    QComboBox* m_eventCombo;
    QComboBox* m_actionCombo;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    Lupine::Node* m_selectedNode;
    QStringList m_availableEvents;
    QStringList m_presetActions;
};
