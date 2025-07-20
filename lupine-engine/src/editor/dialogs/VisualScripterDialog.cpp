#include "VisualScripterDialog.h"
#include "GraphCanvasWidget.h"
#include "lupine/visualscripting/VScriptGraph.h"
#include "lupine/visualscripting/VScriptNode.h"
#include "lupine/visualscripting/VScriptConnection.h"
#include "lupine/visualscripting/VScriptNodeTypes.h"
#include "lupine/visualscripting/CodeGenerator.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtGui/QKeySequence>
#include <QtGui/QClipboard>
#include <QtGui/QPainter>
#include <QtGui/QDrag>
#include <QtCore/QMimeData>
#include <QtCore/QStandardPaths>
#include <iostream>
#include <QtCore/QDir>

VisualScripterDialog::VisualScripterDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_paletteWidget(nullptr)
    , m_canvasWidget(nullptr)
    , m_inspectorWidget(nullptr)
    , m_outlineWidget(nullptr)
    , m_codePreviewWidget(nullptr)
    , m_graph(std::make_unique<Lupine::VScriptGraph>())
    , m_modified(false)
    , m_codeUpdateTimer(new QTimer(this))
{
    setWindowTitle("Visual Scripter");
    setMinimumSize(1400, 900);
    resize(1600, 1000);

    setupUI();
    setupConnections();
    updateWindowTitle();
    updateActions();

    // Setup code update timer
    m_codeUpdateTimer->setSingleShot(true);
    m_codeUpdateTimer->setInterval(500); // 500ms delay
    connect(m_codeUpdateTimer, &QTimer::timeout, this, &VisualScripterDialog::updateCodePreview);
}

VisualScripterDialog::~VisualScripterDialog() {
}

void VisualScripterDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();
    setupMainLayout();
}

void VisualScripterDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New", this, &VisualScripterDialog::onNewScript, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open...", this, &VisualScripterDialog::onOpenScript, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save", this, &VisualScripterDialog::onSaveScript, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save &As...", this, &VisualScripterDialog::onSaveScriptAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_exportAction = fileMenu->addAction("&Export to Python...", this, &VisualScripterDialog::onExportToPython);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &VisualScripterDialog::onExit, QKeySequence("Ctrl+Q"));

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    m_undoAction = editMenu->addAction("&Undo", this, &VisualScripterDialog::onUndo, QKeySequence::Undo);
    m_redoAction = editMenu->addAction("&Redo", this, &VisualScripterDialog::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    m_cutAction = editMenu->addAction("Cu&t", this, &VisualScripterDialog::onCut, QKeySequence::Cut);
    m_copyAction = editMenu->addAction("&Copy", this, &VisualScripterDialog::onCopy, QKeySequence::Copy);
    m_pasteAction = editMenu->addAction("&Paste", this, &VisualScripterDialog::onPaste, QKeySequence::Paste);
    m_deleteAction = editMenu->addAction("&Delete", this, &VisualScripterDialog::onDelete, QKeySequence::Delete);
    editMenu->addSeparator();
    m_selectAllAction = editMenu->addAction("Select &All", this, &VisualScripterDialog::onSelectAll, QKeySequence::SelectAll);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_zoomInAction = viewMenu->addAction("Zoom &In", this, &VisualScripterDialog::onZoomIn, QKeySequence::ZoomIn);
    m_zoomOutAction = viewMenu->addAction("Zoom &Out", this, &VisualScripterDialog::onZoomOut, QKeySequence::ZoomOut);
    m_zoomResetAction = viewMenu->addAction("&Reset Zoom", this, &VisualScripterDialog::onZoomReset, QKeySequence("Ctrl+0"));
    m_fitToWindowAction = viewMenu->addAction("&Fit to Window", this, &VisualScripterDialog::onFitToWindow, QKeySequence("Ctrl+F"));
    viewMenu->addSeparator();
    m_toggleGridAction = viewMenu->addAction("Show &Grid", this, &VisualScripterDialog::onToggleGrid);
    m_toggleGridAction->setCheckable(true);
    m_toggleGridAction->setChecked(true);
    m_toggleSnapAction = viewMenu->addAction("&Snap to Grid", this, &VisualScripterDialog::onToggleSnap);
    m_toggleSnapAction->setCheckable(true);
    m_toggleSnapAction->setChecked(true);
}

void VisualScripterDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_mainLayout->addWidget(m_toolBar);

    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_undoAction);
    m_toolBar->addAction(m_redoAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_zoomInAction);
    m_toolBar->addAction(m_zoomOutAction);
    m_toolBar->addAction(m_zoomResetAction);
}

void VisualScripterDialog::setupMainLayout() {
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    // Left panel (Palette)
    m_paletteWidget = new NodePaletteWidget(this);
    m_paletteWidget->setMinimumWidth(250);
    m_paletteWidget->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_paletteWidget);

    // Center panel (Canvas)
    m_canvasWidget = new GraphCanvasWidget(this);
    m_mainSplitter->addWidget(m_canvasWidget);

    // Right panel (Inspector and other panels)
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->addWidget(m_rightSplitter);

    // Inspector
    m_inspectorWidget = new NodeInspectorWidget(this);
    m_inspectorWidget->setMinimumWidth(250);
    m_inspectorWidget->setMaximumWidth(350);
    m_rightSplitter->addWidget(m_inspectorWidget);

    // Bottom right panels
    m_leftSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->addWidget(m_leftSplitter);

    // Outline
    m_outlineWidget = new NodeOutlineWidget(this);
    m_leftSplitter->addWidget(m_outlineWidget);

    // Code Preview
    m_codePreviewWidget = new CodePreviewWidget(this);
    m_leftSplitter->addWidget(m_codePreviewWidget);

    // Set splitter proportions
    m_mainSplitter->setSizes({250, 800, 350});
    m_rightSplitter->setSizes({300, 400});
    m_leftSplitter->setSizes({200, 200});
}

void VisualScripterDialog::setupConnections() {
    // Palette connections
    connect(m_paletteWidget, &NodePaletteWidget::nodeRequested,
            this, &VisualScripterDialog::onNodeAdded);

    // Canvas connections
    connect(m_canvasWidget, &GraphCanvasWidget::nodeSelected,
            this, &VisualScripterDialog::onNodeSelected);
    connect(m_canvasWidget, &GraphCanvasWidget::nodeDeselected,
            this, &VisualScripterDialog::onNodeDeselected);
    connect(m_canvasWidget, &GraphCanvasWidget::nodeDropped,
            this, &VisualScripterDialog::onNodeAdded);
    connect(m_canvasWidget, &GraphCanvasWidget::nodeDeleted,
            this, &VisualScripterDialog::onNodeDeleted);
    connect(m_canvasWidget, &GraphCanvasWidget::connectionCreated,
            this, &VisualScripterDialog::onConnectionAdded);
    connect(m_canvasWidget, &GraphCanvasWidget::graphModified,
            this, &VisualScripterDialog::onGraphModified);

    // Inspector connections
    connect(m_inspectorWidget, &NodeInspectorWidget::nodePropertyChanged,
            this, &VisualScripterDialog::onGraphModified);

    // Outline connections
    connect(m_outlineWidget, &NodeOutlineWidget::nodeSelected,
            m_canvasWidget, &GraphCanvasWidget::selectNode);
}

void VisualScripterDialog::onNewScript() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    clearGraph();
    m_currentFilePath.clear();
    setModified(false);
    updateWindowTitle();
    updateCodePreview();
    updateOutline();
}

void VisualScripterDialog::onOpenScript() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(this,
        "Open Visual Script", QString(), "Visual Script Files (*.vscript)");

    if (!filepath.isEmpty()) {
        OpenScript(filepath);
    }
}

void VisualScripterDialog::onSaveScript() {
    if (m_currentFilePath.isEmpty()) {
        onSaveScriptAs();
    } else {
        SaveScript(m_currentFilePath);
    }
}

void VisualScripterDialog::onSaveScriptAs() {
    QString filepath = QFileDialog::getSaveFileName(this,
        "Save Visual Script", QString(), "Visual Script Files (*.vscript)");

    if (!filepath.isEmpty()) {
        SaveScript(filepath);
    }
}

void VisualScripterDialog::onExportToPython() {
    QString filepath = QFileDialog::getSaveFileName(this,
        "Export to Python", QString(), "Python Files (*.py)");

    if (!filepath.isEmpty()) {
        ExportToPython(filepath);
    }
}

void VisualScripterDialog::onExit() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }
    accept();
}

// Placeholder implementations for other slots
void VisualScripterDialog::onUndo() { /* TODO */ }
void VisualScripterDialog::onRedo() { /* TODO */ }
void VisualScripterDialog::onCut() { /* TODO */ }
void VisualScripterDialog::onCopy() { /* TODO */ }
void VisualScripterDialog::onPaste() { /* TODO */ }
void VisualScripterDialog::onDelete() { /* TODO */ }
void VisualScripterDialog::onSelectAll() { /* TODO */ }

void VisualScripterDialog::onZoomIn() { if (m_canvasWidget) m_canvasWidget->zoomIn(); }
void VisualScripterDialog::onZoomOut() { if (m_canvasWidget) m_canvasWidget->zoomOut(); }
void VisualScripterDialog::onZoomReset() { if (m_canvasWidget) m_canvasWidget->resetZoom(); }
void VisualScripterDialog::onFitToWindow() { if (m_canvasWidget) m_canvasWidget->fitToWindow(); }
void VisualScripterDialog::onToggleGrid() { if (m_canvasWidget) m_canvasWidget->setGridVisible(m_toggleGridAction->isChecked()); }
void VisualScripterDialog::onToggleSnap() { if (m_canvasWidget) m_canvasWidget->setSnapToGrid(m_toggleSnapAction->isChecked()); }

void VisualScripterDialog::onNodeSelected(Lupine::VScriptNode* node) {
    m_inspectorWidget->SetNode(node);
}

void VisualScripterDialog::onNodeDeselected() {
    m_inspectorWidget->SetNode(nullptr);
}

void VisualScripterDialog::onNodeAdded(const QString& node_type, const QPointF& position) {
    if (!m_graph) {
        return;
    }

    // Generate unique node ID
    static int nodeCounter = 1;
    std::string nodeId = "node_" + std::to_string(nodeCounter++);

    // Create node using factory
    auto node = Lupine::VScriptNodeFactory::CreateNode(node_type.toStdString(), nodeId);
    if (node) {
        node->SetPosition(position.x(), position.y());
        m_graph->AddNode(std::move(node));

        // Refresh canvas
        if (m_canvasWidget) {
            m_canvasWidget->setGraph(m_graph.get());
        }

        onGraphModified();
    }
}

void VisualScripterDialog::onNodeDeleted(const QString& node_id) {
    // TODO: Remove node from graph
    onGraphModified();
}

void VisualScripterDialog::onConnectionAdded(const QString& from_node, const QString& from_pin,
                                           const QString& to_node, const QString& to_pin) {
    if (!m_graph) {
        return;
    }

    // Create connection
    auto connection = std::make_unique<Lupine::VScriptConnection>(
        from_node.toStdString(), from_pin.toStdString(),
        to_node.toStdString(), to_pin.toStdString()
    );

    if (m_graph->AddConnection(std::move(connection))) {
        // Refresh canvas to show the new connection
        if (m_canvasWidget) {
            m_canvasWidget->setGraph(m_graph.get());
        }
        onGraphModified();
    }
}

void VisualScripterDialog::onConnectionDeleted(const QString& from_node, const QString& from_pin,
                                             const QString& to_node, const QString& to_pin) {
    // TODO: Remove connection from graph
    onGraphModified();
}

void VisualScripterDialog::onGraphModified() {
    setModified(true);
    m_codeUpdateTimer->start(); // Delayed update
    updateOutline();
}

void VisualScripterDialog::updateCodePreview() {
    if (m_graph && m_codePreviewWidget) {
        Lupine::CodeGenerator generator;
        QString code = QString::fromStdString(generator.GenerateCode(*m_graph));
        m_codePreviewWidget->UpdateCode(code);
    }
}

void VisualScripterDialog::updateOutline() {
    if (m_outlineWidget) {
        m_outlineWidget->UpdateOutline(m_graph.get());
    }
}

void VisualScripterDialog::updateWindowTitle() {
    QString title = "Visual Scripter";
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

void VisualScripterDialog::updateActions() {
    bool hasGraph = (m_graph != nullptr);
    m_saveAction->setEnabled(hasGraph && m_modified);
    m_saveAsAction->setEnabled(hasGraph);
    m_exportAction->setEnabled(hasGraph);
}

bool VisualScripterDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool VisualScripterDialog::promptSaveChanges() {
    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<VisualScripterDialog*>(this),
        "Unsaved Changes",
        "The visual script has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    if (result == QMessageBox::Save) {
        onSaveScript();
        return !m_modified; // Return true if save was successful
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void VisualScripterDialog::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
        updateActions();
    }
}

void VisualScripterDialog::clearGraph() {
    if (m_graph) {
        m_graph->Clear();
    }
    if (m_canvasWidget) {
        m_canvasWidget->setGraph(m_graph.get());
    }
}

void VisualScripterDialog::OpenScript(const QString& filepath) {
    if (m_graph && m_graph->LoadFromFile(filepath.toStdString())) {
        m_currentFilePath = filepath;
        setModified(false);
        
        if (m_canvasWidget) {
            m_canvasWidget->setGraph(m_graph.get());
        }
        
        updateWindowTitle();
        updateCodePreview();
        updateOutline();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load visual script file.");
    }
}

bool VisualScripterDialog::SaveScript(const QString& filepath) {
    if (m_graph && m_graph->SaveToFile(filepath.toStdString())) {
        m_currentFilePath = filepath;
        setModified(false);
        updateWindowTitle();
        return true;
    } else {
        QMessageBox::warning(this, "Error", "Failed to save visual script file.");
        return false;
    }
}

bool VisualScripterDialog::ExportToPython(const QString& filepath) {
    if (m_codePreviewWidget) {
        return m_codePreviewWidget->ExportCode(filepath);
    }
    return false;
}

// NodeListWidget Implementation
NodeListWidget::NodeListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
}

void NodeListWidget::startDrag(Qt::DropActions supportedActions) {
    QListWidgetItem* item = currentItem();
    if (!item) {
        return;
    }

    QMimeData* mimeData = new QMimeData();
    QString nodeType = item->data(Qt::UserRole).toString();
    if (nodeType.isEmpty()) {
        nodeType = item->text();
    }
    mimeData->setText(nodeType);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Create a simple drag pixmap
    QPixmap pixmap(100, 30);
    pixmap.fill(Qt::lightGray);
    QPainter painter(&pixmap);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, item->text());
    drag->setPixmap(pixmap);

    drag->exec(supportedActions);
}

QMimeData* NodeListWidget::mimeData(const QList<QListWidgetItem*> items) const {
    QMimeData* mimeData = new QMimeData();
    if (!items.isEmpty()) {
        QString nodeType = items.first()->data(Qt::UserRole).toString();
        if (nodeType.isEmpty()) {
            nodeType = items.first()->text();
        }
        mimeData->setText(nodeType);
    }
    return mimeData;
}

// NodePaletteWidget Implementation
NodePaletteWidget::NodePaletteWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_categoryCombo(nullptr)
    , m_nodeList(nullptr)
    , m_searchEdit(nullptr)
{
    setupUI();
    populateCategories();
}

void NodePaletteWidget::setupUI() {
    m_layout = new QVBoxLayout(this);

    // Search box
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search nodes...");
    m_layout->addWidget(m_searchEdit);

    // Category combo
    m_categoryCombo = new QComboBox(this);
    m_layout->addWidget(m_categoryCombo);

    // Node list
    m_nodeList = new NodeListWidget(this);
    m_layout->addWidget(m_nodeList);

    // Connections
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NodePaletteWidget::onCategoryChanged);
    connect(m_nodeList, &QListWidget::itemDoubleClicked,
            this, &NodePaletteWidget::onNodeDoubleClicked);
}

void NodePaletteWidget::populateCategories() {
    m_categoryCombo->addItem("All", -1);
    m_categoryCombo->addItem("Events", static_cast<int>(Lupine::VScriptNodeCategory::Event));
    m_categoryCombo->addItem("Flow Control", static_cast<int>(Lupine::VScriptNodeCategory::FlowControl));
    m_categoryCombo->addItem("Variables", static_cast<int>(Lupine::VScriptNodeCategory::Variable));
    m_categoryCombo->addItem("Math", static_cast<int>(Lupine::VScriptNodeCategory::Math));
    m_categoryCombo->addItem("Logic", static_cast<int>(Lupine::VScriptNodeCategory::Logic));
    m_categoryCombo->addItem("Functions", static_cast<int>(Lupine::VScriptNodeCategory::Function));
    m_categoryCombo->addItem("Custom", static_cast<int>(Lupine::VScriptNodeCategory::Custom));

    onCategoryChanged();
}

void NodePaletteWidget::populateNodes(const QString& category) {
    m_nodeList->clear();

    std::vector<std::string> nodeTypes;

    if (category == "All") {
        nodeTypes = Lupine::VScriptNodeFactory::GetAvailableNodeTypes();
    } else {
        // Map category names to enum values
        Lupine::VScriptNodeCategory cat = Lupine::VScriptNodeCategory::Event;
        if (category == "Events") cat = Lupine::VScriptNodeCategory::Event;
        else if (category == "Flow Control") cat = Lupine::VScriptNodeCategory::FlowControl;
        else if (category == "Variables") cat = Lupine::VScriptNodeCategory::Variable;
        else if (category == "Math") cat = Lupine::VScriptNodeCategory::Math;
        else if (category == "Logic") cat = Lupine::VScriptNodeCategory::Logic;
        else if (category == "Functions") cat = Lupine::VScriptNodeCategory::Function;
        else if (category == "Custom") cat = Lupine::VScriptNodeCategory::Custom;

        nodeTypes = Lupine::VScriptNodeFactory::GetNodeTypesByCategory(cat);
    }

    // Add nodes to list
    for (const auto& nodeType : nodeTypes) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(nodeType));
        item->setData(Qt::UserRole, QString::fromStdString(nodeType)); // Store node type
        m_nodeList->addItem(item);
    }
}

void NodePaletteWidget::onCategoryChanged() {
    QString category = m_categoryCombo->currentText();
    populateNodes(category);
}

void NodePaletteWidget::onNodeDoubleClicked(QListWidgetItem* item) {
    if (item) {
        // Get the node type from the item data
        QString nodeType = item->data(Qt::UserRole).toString();
        if (nodeType.isEmpty()) {
            nodeType = item->text(); // Fallback to text
        }

        // Use a default position in the center of the canvas
        emit nodeRequested(nodeType, QPointF(200, 200));
    }
}

// NodeInspectorWidget Implementation
NodeInspectorWidget::NodeInspectorWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_propertiesGroup(nullptr)
    , m_propertiesLayout(nullptr)
    , m_nodeNameLabel(nullptr)
    , m_nodeTypeLabel(nullptr)
    , m_currentNode(nullptr)
{
    setupUI();
}

void NodeInspectorWidget::setupUI() {
    m_layout = new QVBoxLayout(this);

    // Node info
    QGroupBox* infoGroup = new QGroupBox("Node Information", this);
    QFormLayout* infoLayout = new QFormLayout(infoGroup);

    m_nodeNameLabel = new QLabel("None", this);
    m_nodeTypeLabel = new QLabel("None", this);

    infoLayout->addRow("Name:", m_nodeNameLabel);
    infoLayout->addRow("Type:", m_nodeTypeLabel);

    m_layout->addWidget(infoGroup);

    // Properties
    m_propertiesGroup = new QGroupBox("Properties", this);
    m_propertiesLayout = new QFormLayout(m_propertiesGroup);
    m_layout->addWidget(m_propertiesGroup);

    // Stretch
    m_layout->addStretch();
}

void NodeInspectorWidget::SetNode(Lupine::VScriptNode* node) {
    m_currentNode = node;
    clearProperties();

    if (node) {
        m_nodeNameLabel->setText(QString::fromStdString(node->GetDisplayName()));
        m_nodeTypeLabel->setText(QString::fromStdString(node->GetType()));
        populateProperties(node);
    } else {
        m_nodeNameLabel->setText("None");
        m_nodeTypeLabel->setText("None");
    }
}

void NodeInspectorWidget::clearProperties() {
    // Clear existing property widgets
    for (QWidget* widget : m_propertyWidgets) {
        widget->deleteLater();
    }
    m_propertyWidgets.clear();

    // Clear layout
    QLayoutItem* item;
    while ((item = m_propertiesLayout->takeAt(0)) != nullptr) {
        delete item;
    }
}

void NodeInspectorWidget::populateProperties(Lupine::VScriptNode* node) {
    if (!node) return;

    const auto& properties = node->GetProperties();
    for (const auto& [key, value] : properties) {
        QLabel* label = new QLabel(QString::fromStdString(key), this);
        QWidget* widget = nullptr;

        // Create appropriate widget based on property type
        if (key == "operator") {
            // Dropdown for comparison operators
            QComboBox* combo = new QComboBox(this);
            combo->addItems({"==", "!=", "<", ">", "<=", ">="});
            combo->setCurrentText(QString::fromStdString(value));
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, &NodeInspectorWidget::onPropertyChanged);
            widget = combo;
        } else if (key == "variable_type") {
            // Dropdown for variable types
            QComboBox* combo = new QComboBox(this);
            combo->addItems({"Any", "String", "Integer", "Float", "Boolean", "Vector2", "Vector3", "Object"});
            combo->setCurrentText(QString::fromStdString(value));
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, &NodeInspectorWidget::onPropertyChanged);
            widget = combo;
        } else if (key == "type") {
            // Dropdown for number types
            QComboBox* combo = new QComboBox(this);
            combo->addItems({"float", "int"});
            combo->setCurrentText(QString::fromStdString(value));
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, &NodeInspectorWidget::onPropertyChanged);
            widget = combo;
        } else if (key == "value" && node->GetType() == "LiteralBool") {
            // Checkbox for boolean values
            QCheckBox* checkbox = new QCheckBox(this);
            checkbox->setChecked(value == "True");
            connect(checkbox, &QCheckBox::toggled, this, &NodeInspectorWidget::onPropertyChanged);
            widget = checkbox;
        } else if (key == "code") {
            // Multi-line text edit for code
            QTextEdit* textEdit = new QTextEdit(this);
            textEdit->setPlainText(QString::fromStdString(value));
            textEdit->setMaximumHeight(100);
            connect(textEdit, &QTextEdit::textChanged, this, &NodeInspectorWidget::onPropertyChanged);
            widget = textEdit;
        } else {
            // Default: line edit
            QLineEdit* edit = new QLineEdit(QString::fromStdString(value), this);
            connect(edit, &QLineEdit::textChanged, this, &NodeInspectorWidget::onPropertyChanged);
            widget = edit;
        }

        if (widget) {
            m_propertiesLayout->addRow(label, widget);
            m_propertyWidgets.push_back(label);
            m_propertyWidgets.push_back(widget);
        }
    }
}

void NodeInspectorWidget::onPropertyChanged() {
    if (!m_currentNode) {
        return;
    }

    QWidget* sender = qobject_cast<QWidget*>(QObject::sender());
    if (!sender) {
        return;
    }

    // Find which property was changed
    for (int i = 0; i < m_propertiesLayout->rowCount(); ++i) {
        QLayoutItem* labelItem = m_propertiesLayout->itemAt(i, QFormLayout::LabelRole);
        QLayoutItem* fieldItem = m_propertiesLayout->itemAt(i, QFormLayout::FieldRole);

        if (fieldItem && fieldItem->widget() == sender) {
            QLabel* label = qobject_cast<QLabel*>(labelItem->widget());
            if (label) {
                QString propertyName = label->text();
                QString newValue;

                // Get the new value based on widget type
                if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender)) {
                    newValue = lineEdit->text();
                } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(sender)) {
                    newValue = comboBox->currentText();
                } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender)) {
                    newValue = checkBox->isChecked() ? "True" : "False";
                } else if (QTextEdit* textEdit = qobject_cast<QTextEdit*>(sender)) {
                    newValue = textEdit->toPlainText();
                }

                // Update the node property
                m_currentNode->SetProperty(propertyName.toStdString(), newValue.toStdString());

                // Emit signal for graph modification
                emit nodePropertyChanged(QString::fromStdString(m_currentNode->GetId()),
                                       propertyName, newValue);
                break;
            }
        }
    }
}

// NodeOutlineWidget Implementation
NodeOutlineWidget::NodeOutlineWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_treeWidget(nullptr)
    , m_searchEdit(nullptr)
{
    setupUI();
}

void NodeOutlineWidget::setupUI() {
    m_layout = new QVBoxLayout(this);

    // Search box
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search nodes...");
    m_layout->addWidget(m_searchEdit);

    // Tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({"Node", "Type"});
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_layout->addWidget(m_treeWidget);

    // Connections
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &NodeOutlineWidget::onNodeClicked);
}

void NodeOutlineWidget::UpdateOutline(Lupine::VScriptGraph* graph) {
    m_treeWidget->clear();

    if (!graph) return;

    auto nodes = graph->GetNodes();
    for (auto* node : nodes) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, QString::fromStdString(node->GetDisplayName()));
        item->setText(1, QString::fromStdString(node->GetType()));
        item->setData(0, Qt::UserRole, QString::fromStdString(node->GetId()));
    }

    m_treeWidget->expandAll();
}

void NodeOutlineWidget::onNodeClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        QString nodeId = item->data(0, Qt::UserRole).toString();
        emit nodeSelected(nodeId);
    }
}

// CodePreviewWidget Implementation
CodePreviewWidget::CodePreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_codeEdit(nullptr)
    , m_copyButton(nullptr)
    , m_exportButton(nullptr)
{
    setupUI();
}

void CodePreviewWidget::setupUI() {
    m_layout = new QVBoxLayout(this);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_copyButton = new QPushButton("Copy", this);
    m_exportButton = new QPushButton("Export...", this);

    buttonLayout->addWidget(m_copyButton);
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addStretch();

    m_layout->addLayout(buttonLayout);

    // Code editor
    m_codeEdit = new QPlainTextEdit(this);
    m_codeEdit->setReadOnly(true);
    m_codeEdit->setFont(QFont("Consolas", 10));
    m_layout->addWidget(m_codeEdit);

    // Connections
    connect(m_copyButton, &QPushButton::clicked, this, &CodePreviewWidget::onCopyCode);
    connect(m_exportButton, &QPushButton::clicked, this, &CodePreviewWidget::onExportCode);
}

void CodePreviewWidget::UpdateCode(const QString& code) {
    m_codeEdit->setPlainText(code);
}

bool CodePreviewWidget::ExportCode(const QString& filepath) const {
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << m_codeEdit->toPlainText();
        return true;
    }
    return false;
}

void CodePreviewWidget::onCopyCode() {
    QApplication::clipboard()->setText(m_codeEdit->toPlainText());
}

void CodePreviewWidget::onExportCode() {
    QString filepath = QFileDialog::getSaveFileName(this,
        "Export Python Code", QString(), "Python Files (*.py)");

    if (!filepath.isEmpty()) {
        if (ExportCode(filepath)) {
            QMessageBox::information(this, "Success", "Code exported successfully.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to export code.");
        }
    }
}
