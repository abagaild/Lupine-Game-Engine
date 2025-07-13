#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtCore/QTimer>
#include <memory>

namespace Lupine {
    class VScriptGraph;
    class VScriptNode;
    class VScriptConnection;
}

// Forward declarations
class NodePaletteWidget;
class GraphCanvasWidget;
class NodeInspectorWidget;
class NodeOutlineWidget;
class CodePreviewWidget;

/**
 * @brief Main dialog for the Visual Scripter tool
 * 
 * Provides a complete visual scripting environment with:
 * - Node palette for dragging nodes onto the canvas
 * - Graph canvas for visual node editing
 * - Inspector for editing node properties
 * - Outline for navigating large graphs
 * - Live code preview showing generated Python
 */
class VisualScripterDialog : public QDialog {
    Q_OBJECT

public:
    explicit VisualScripterDialog(QWidget* parent = nullptr);
    ~VisualScripterDialog();

    /**
     * @brief Open a visual script file
     * @param filepath Path to .vscript file
     */
    void OpenScript(const QString& filepath);

    /**
     * @brief Save the current visual script
     * @param filepath Path to save to (empty = use current path)
     */
    bool SaveScript(const QString& filepath = QString());

    /**
     * @brief Export the visual script to Python
     * @param filepath Path to save Python file
     */
    bool ExportToPython(const QString& filepath);

private slots:
    // File menu actions
    void onNewScript();
    void onOpenScript();
    void onSaveScript();
    void onSaveScriptAs();
    void onExportToPython();
    void onExit();

    // Edit menu actions
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onSelectAll();

    // View menu actions
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onFitToWindow();
    void onToggleGrid();
    void onToggleSnap();

    // Graph editing
    void onNodeSelected(Lupine::VScriptNode* node);
    void onNodeDeselected();
    void onNodeAdded(const QString& node_type, const QPointF& position);
    void onNodeDeleted(const QString& node_id);
    void onConnectionAdded(const QString& from_node, const QString& from_pin,
                          const QString& to_node, const QString& to_pin);
    void onConnectionDeleted(const QString& from_node, const QString& from_pin,
                            const QString& to_node, const QString& to_pin);
    void onGraphModified();

    // UI updates
    void updateCodePreview();
    void updateOutline();
    void updateWindowTitle();
    void updateActions();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainLayout();
    void setupConnections();

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    void clearGraph();

    // UI Components
    QVBoxLayout* m_mainLayout;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    QSplitter* m_rightSplitter;

    // Panels
    NodePaletteWidget* m_paletteWidget;
    GraphCanvasWidget* m_canvasWidget;
    NodeInspectorWidget* m_inspectorWidget;
    NodeOutlineWidget* m_outlineWidget;
    CodePreviewWidget* m_codePreviewWidget;

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
    QAction* m_selectAllAction;

    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomResetAction;
    QAction* m_fitToWindowAction;
    QAction* m_toggleGridAction;
    QAction* m_toggleSnapAction;

    // Data
    std::unique_ptr<Lupine::VScriptGraph> m_graph;
    QString m_currentFilePath;
    bool m_modified;

    // Update timer for code preview
    QTimer* m_codeUpdateTimer;
};

/**
 * @brief Custom list widget that supports drag operations for nodes
 */
class NodeListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit NodeListWidget(QWidget* parent = nullptr);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const;
};

/**
 * @brief Widget for displaying and selecting nodes from the palette
 */
class NodePaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit NodePaletteWidget(QWidget* parent = nullptr);

signals:
    void nodeRequested(const QString& node_type, const QPointF& position);

private slots:
    void onCategoryChanged();
    void onNodeDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void populateCategories();
    void populateNodes(const QString& category);

    QVBoxLayout* m_layout;
    QComboBox* m_categoryCombo;
    NodeListWidget* m_nodeList;
    QLineEdit* m_searchEdit;
};

/**
 * @brief Widget for displaying node properties in the inspector
 */
class NodeInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit NodeInspectorWidget(QWidget* parent = nullptr);

    /**
     * @brief Set the node to inspect
     * @param node Node to show properties for (nullptr to clear)
     */
    void SetNode(Lupine::VScriptNode* node);

signals:
    void nodePropertyChanged(const QString& node_id, const QString& property, const QString& value);

private slots:
    void onPropertyChanged();

private:
    void setupUI();
    void clearProperties();
    void populateProperties(Lupine::VScriptNode* node);

    QVBoxLayout* m_layout;
    QGroupBox* m_propertiesGroup;
    QFormLayout* m_propertiesLayout;
    QLabel* m_nodeNameLabel;
    QLabel* m_nodeTypeLabel;

    Lupine::VScriptNode* m_currentNode;
    std::vector<QWidget*> m_propertyWidgets;
};

/**
 * @brief Widget for showing an outline/overview of the graph
 */
class NodeOutlineWidget : public QWidget {
    Q_OBJECT

public:
    explicit NodeOutlineWidget(QWidget* parent = nullptr);

    /**
     * @brief Update the outline with the current graph
     * @param graph Graph to show outline for
     */
    void UpdateOutline(Lupine::VScriptGraph* graph);

signals:
    void nodeSelected(const QString& node_id);

private slots:
    void onNodeClicked(QTreeWidgetItem* item, int column);

private:
    void setupUI();

    QVBoxLayout* m_layout;
    QTreeWidget* m_treeWidget;
    QLineEdit* m_searchEdit;
};

/**
 * @brief Widget for showing live Python code preview
 */
class CodePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit CodePreviewWidget(QWidget* parent = nullptr);

    /**
     * @brief Update the code preview with generated Python
     * @param code Generated Python code
     */
    void UpdateCode(const QString& code);

    /**
     * @brief Export the current code to a file
     * @param filepath Path to save the Python file
     */
    bool ExportCode(const QString& filepath) const;

private slots:
    void onCopyCode();
    void onExportCode();

private:
    void setupUI();

    QVBoxLayout* m_layout;
    QPlainTextEdit* m_codeEdit;
    QPushButton* m_copyButton;
    QPushButton* m_exportButton;
};
