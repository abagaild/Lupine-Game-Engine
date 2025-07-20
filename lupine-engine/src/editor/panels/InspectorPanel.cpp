#include "InspectorPanel.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/core/Scene.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Label.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/core/CrashHandler.h"
#include <iostream>
#include "../widgets/PropertyEditorWidget.h"
#include "../dialogs/AddComponentDialog.h"
#include "../MainWindow.h"
#include "../EditorUndoSystem.h"
#include <QGroupBox>
#include <QFrame>
#include <QPushButton>
#include <QMessageBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QCheckBox>
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

InspectorPanel::InspectorPanel(QWidget *parent)
    : QWidget(parent), m_selectedNode(nullptr), m_scene(nullptr), m_mainWindow(nullptr), m_isRebuilding(false)
{
    setupUI();
}

InspectorPanel::~InspectorPanel() {}

void InspectorPanel::clearLayout(QLayout* layout)
{
    qDebug() << "InspectorPanel::clearLayout - Starting, layout:" << layout;
    if (!layout) {
        qDebug() << "InspectorPanel::clearLayout - Layout is null, returning";
        return;
    }

    qDebug() << "InspectorPanel::clearLayout - Layout has" << layout->count() << "items";
    QLayoutItem* item;
    int itemIndex = 0;
    while ((item = layout->takeAt(0)) != nullptr) {
        qDebug() << "InspectorPanel::clearLayout - Processing item" << itemIndex++;

        // Store references before any deletion
        QWidget* widget = item->widget();
        QLayout* nestedLayout = item->layout();

        if (widget) {
            qDebug() << "InspectorPanel::clearLayout - Item is a widget, disconnecting signals";
            // Disconnect all signals from the widget to prevent crashes from dangling pointers
            widget->disconnect();
            qDebug() << "InspectorPanel::clearLayout - Signals disconnected, deleting widget";
            delete widget;
            qDebug() << "InspectorPanel::clearLayout - Widget deleted";
        } else if (nestedLayout) {
            qDebug() << "InspectorPanel::clearLayout - Item is a layout, clearing recursively";
            clearLayout(nestedLayout);
            qDebug() << "InspectorPanel::clearLayout - Recursive clear completed";
            // Don't delete the nested layout directly - let Qt handle it through the item
        }

        qDebug() << "InspectorPanel::clearLayout - Deleting layout item";
        try {
            delete item;
            qDebug() << "InspectorPanel::clearLayout - Layout item deleted successfully";
        } catch (...) {
            qDebug() << "InspectorPanel::clearLayout - Exception during layout item deletion";
        }
    }
    qDebug() << "InspectorPanel::clearLayout - Completed successfully";
}

void InspectorPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(4, 4, 4, 4);
    m_contentLayout->setSpacing(6);

    m_noSelectionLabel = new QLabel("No node selected");
    m_noSelectionLabel->setAlignment(Qt::AlignCenter);
    m_noSelectionLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
    m_contentLayout->addWidget(m_noSelectionLabel);

    // Node properties group
    m_nodePropertiesGroup = new QGroupBox("Node Properties");
    m_nodePropertiesGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px 0 4px; }"
    );
    m_nodePropertiesLayout = new QVBoxLayout(m_nodePropertiesGroup);
    m_nodePropertiesLayout->setContentsMargins(8, 12, 8, 8);
    m_nodePropertiesLayout->setSpacing(6);
    m_nodePropertiesGroup->setVisible(false);
    m_contentLayout->addWidget(m_nodePropertiesGroup);

    // Components group
    m_componentsGroup = new QGroupBox("Components");
    m_componentsGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px 0 4px; }"
    );
    m_componentsLayout = new QVBoxLayout(m_componentsGroup);
    m_componentsLayout->setContentsMargins(8, 12, 8, 8);
    m_componentsLayout->setSpacing(6);

    // Add component button
    m_addComponentButton = new QPushButton("Add Component");
    m_addComponentButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_addComponentButton->setStyleSheet(
        "QPushButton { padding: 6px 12px; border: 1px solid #666; border-radius: 3px; background: #444; }"
        "QPushButton:hover { background: #555; }"
        "QPushButton:pressed { background: #333; }"
    );
    m_componentsLayout->addWidget(m_addComponentButton);

    m_componentsGroup->setVisible(false);
    m_contentLayout->addWidget(m_componentsGroup);

    m_contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    m_scrollArea->setWidgetResizable(true);
    m_layout->addWidget(m_scrollArea);

    // Connect signals
    connect(m_addComponentButton, &QPushButton::clicked, this, &InspectorPanel::onAddComponentClicked);
}

void InspectorPanel::setSelectedNode(Lupine::Node* node)
{
    LUPINE_AUTO_TRACK_FUNCTION();

    // Add safety check to prevent crashes from invalid nodes
    if (node) {
        qDebug() << "InspectorPanel::setSelectedNode - Validating node:" << QString::fromStdString(node->GetName());
        if (!isNodeValid(node)) {
            qDebug() << "InspectorPanel::setSelectedNode - Node validation failed, setting to nullptr";
            node = nullptr;
        } else {
            qDebug() << "InspectorPanel::setSelectedNode - Node validation passed";
        }
    }

    qDebug() << "InspectorPanel::setSelectedNode - Checking if same node";

    // Safely check if we're setting the same node
    try {
        if (m_selectedNode == node) {
            qDebug() << "InspectorPanel::setSelectedNode - Same node, skipping rebuild";
            return;
        }
    } catch (...) {
        qDebug() << "InspectorPanel::setSelectedNode - Exception during node comparison, continuing";
        // If comparison fails, assume they're different and continue
    }

    qDebug() << "InspectorPanel::setSelectedNode - Checking rebuilding flag";

    // Prevent re-entrant calls during rebuilding
    if (m_isRebuilding) {
        qDebug() << "InspectorPanel::setSelectedNode - Already rebuilding, ignoring call";
        return;
    }

    // Set rebuilding flag to prevent lambda callbacks during UI rebuild
    qDebug() << "InspectorPanel::setSelectedNode - Setting rebuilding flag";
    m_isRebuilding = true;
    qDebug() << "InspectorPanel::setSelectedNode - Rebuilding flag set";

    try {
        qDebug() << "InspectorPanel::setSelectedNode - Entering try block";
        // Clear any existing connections and UI before setting new node
        qDebug() << "InspectorPanel::setSelectedNode - Setting selected node to nullptr";
        m_selectedNode = nullptr;  // Set to null first to prevent any callbacks during cleanup
        qDebug() << "InspectorPanel::setSelectedNode - Calling clearInspector";
        clearInspector(node != nullptr);  // Pass whether we will have a node selected
        qDebug() << "InspectorPanel::setSelectedNode - clearInspector completed";

        // Now set the new selected node
        m_selectedNode = node;

        if (node) {
            populateNodeProperties(node);

            // Show components
            m_componentsGroup->setVisible(true);
            qDebug() << "InspectorPanel::setSelectedNode - Getting components for node";
            const auto& components = node->GetAllComponents();
            qDebug() << "InspectorPanel::setSelectedNode - Found" << components.size() << "components";
            for (const auto& component : components) {
                if (component) {
                    qDebug() << "InspectorPanel::setSelectedNode - Creating widget for component:" << QString::fromStdString(component->GetTypeName());
                    createComponentWidget(component.get());
                    qDebug() << "InspectorPanel::setSelectedNode - Component widget created successfully";
                }
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in InspectorPanel::setSelectedNode:" << e.what();
        // If any exception occurs during population, clear the selection
        m_selectedNode = nullptr;
        clearInspector(false);
    } catch (...) {
        qWarning() << "Unknown exception in InspectorPanel::setSelectedNode";
        // If any exception occurs during population, clear the selection
        m_selectedNode = nullptr;
        clearInspector(false);
    }

    // Clear rebuilding flag
    m_isRebuilding = false;
    qDebug() << "InspectorPanel::setSelectedNode - Completed";
}

void InspectorPanel::setScene(Lupine::Scene* scene)
{
    m_scene = scene;
}

void InspectorPanel::setMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void InspectorPanel::clearInspector(bool willHaveNode)
{
    qDebug() << "InspectorPanel::clearInspector - Starting, willHaveNode:" << willHaveNode;

    // Clear property editors tracking
    qDebug() << "InspectorPanel::clearInspector - Clearing property editors tracking";
    m_componentPropertyEditors.clear();
    qDebug() << "InspectorPanel::clearInspector - Property editors cleared";

    // Clear node properties layout - recursively delete all items
    qDebug() << "InspectorPanel::clearInspector - Clearing node properties layout";
    clearLayout(m_nodePropertiesLayout);
    qDebug() << "InspectorPanel::clearInspector - Node properties layout cleared";

    // Clear components layout (except the add button)
    qDebug() << "InspectorPanel::clearInspector - Clearing components layout";
    while (m_componentsLayout->count() > 1) {
        qDebug() << "InspectorPanel::clearInspector - Components layout count:" << m_componentsLayout->count();
        QLayoutItem* item = m_componentsLayout->takeAt(0);
        if (item) {
            qDebug() << "InspectorPanel::clearInspector - Processing layout item";
            if (item->widget()) {
                qDebug() << "InspectorPanel::clearInspector - Deleting widget";
                delete item->widget();
                qDebug() << "InspectorPanel::clearInspector - Widget deleted";
            } else if (item->layout()) {
                qDebug() << "InspectorPanel::clearInspector - Clearing nested layout";
                clearLayout(item->layout());
                qDebug() << "InspectorPanel::clearInspector - Deleting nested layout";
                delete item->layout();
                qDebug() << "InspectorPanel::clearInspector - Nested layout deleted";
            }
            qDebug() << "InspectorPanel::clearInspector - Deleting layout item";
            delete item;
            qDebug() << "InspectorPanel::clearInspector - Layout item deleted";
        }
    }
    qDebug() << "InspectorPanel::clearInspector - Components layout cleared";

    qDebug() << "InspectorPanel::clearInspector - Setting visibility states";
    if (!willHaveNode) {
        m_noSelectionLabel->setVisible(true);
        m_nodePropertiesGroup->setVisible(false);
        m_componentsGroup->setVisible(false);
    } else {
        m_noSelectionLabel->setVisible(false);
        m_nodePropertiesGroup->setVisible(true);
        m_componentsGroup->setVisible(true);
    }
    qDebug() << "InspectorPanel::clearInspector - Completed successfully";
}

void InspectorPanel::populateNodeProperties(Lupine::Node* node)
{
    // Node name editor
    QLabel* nameLabel = new QLabel("Name:");
    QLineEdit* nameEdit = new QLineEdit(QString::fromStdString(node->GetName()));

    // Connect name editor to update node name
    connect(nameEdit, &QLineEdit::editingFinished, [this, node, nameEdit]() {
        // Validate node is still valid and we're not rebuilding UI
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            QString newName = nameEdit->text().trimmed();
            if (!newName.isEmpty() && newName.toStdString() != node->GetName()) {
                QString oldName = QString::fromStdString(node->GetName());

                // Record undo action before changing the name
                if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                    m_mainWindow->getUndoSystem()->recordNodeRenamed(node, oldName, newName);
                }

                node->SetName(newName.toStdString());
                emit nodeNameChanged(node, newName);
            }
        }
    });

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    m_nodePropertiesLayout->addLayout(nameLayout);

    // Node type (read-only)
    QLabel* typeLabel = new QLabel("Type:");
    QLabel* typeValue = new QLabel(QString::fromStdString(node->GetTypeName()));
    typeValue->setStyleSheet("color: gray;");

    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(typeValue);
    typeLayout->addStretch();
    m_nodePropertiesLayout->addLayout(typeLayout);

    // Node visibility checkbox
    QCheckBox* visibilityCheckBox = new QCheckBox("Visible");
    visibilityCheckBox->setChecked(node->IsVisible());

    // Connect visibility checkbox to update node visibility
    connect(visibilityCheckBox, &QCheckBox::toggled, [this, node](bool checked) {
        // Validate node is still valid and we're not rebuilding UI
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetVisible(checked);
        }
    });

    m_nodePropertiesLayout->addWidget(visibilityCheckBox);

    // Add transform properties based on node type
    addTransformProperties(node);
}

void InspectorPanel::addTransformProperties(Lupine::Node* node)
{
    if (!node) return;

    qDebug() << "InspectorPanel::addTransformProperties - Node type:" << QString::fromStdString(node->GetTypeName());

    // Check node type and add appropriate transform properties
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        qDebug() << "InspectorPanel::addTransformProperties - Adding Node2D properties";
        addNode2DTransformProperties(node2d);
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        qDebug() << "InspectorPanel::addTransformProperties - Adding Node3D properties";
        addNode3DTransformProperties(node3d);
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        qDebug() << "InspectorPanel::addTransformProperties - Adding Control properties";
        addControlTransformProperties(control);
    } else {
        qDebug() << "InspectorPanel::addTransformProperties - No transform properties for this node type";
    }
}

void InspectorPanel::addNode2DTransformProperties(Lupine::Node2D* node)
{
    if (!node) return;

    qDebug() << "InspectorPanel::addNode2DTransformProperties - Starting";

    // Add a separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_nodePropertiesLayout->addWidget(separator);

    // Transform section label
    QLabel* transformLabel = new QLabel("Transform");
    QFont boldFont = transformLabel->font();
    boldFont.setBold(true);
    transformLabel->setFont(boldFont);
    m_nodePropertiesLayout->addWidget(transformLabel);

    // Position (Vec2)
    glm::vec2 position = node->GetPosition();

    QLabel* positionLabel = new QLabel("Position");
    positionLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(positionLabel);

    QHBoxLayout* posLayout = new QHBoxLayout();
    posLayout->setContentsMargins(0, 0, 0, 0);
    posLayout->setSpacing(4);

    QLabel* xLabel = new QLabel("X:");
    xLabel->setMinimumWidth(15);
    xLabel->setMaximumWidth(15);
    xLabel->setStyleSheet("color: #f88;");
    posLayout->addWidget(xLabel);

    QDoubleSpinBox* posX = new QDoubleSpinBox();
    posX->setRange(-99999.0, 99999.0);
    posX->setValue(position.x);
    posX->setDecimals(3);
    posX->setMinimumWidth(60);
    posX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posX);

    QLabel* yLabel = new QLabel("Y:");
    yLabel->setMinimumWidth(15);
    yLabel->setMaximumWidth(15);
    yLabel->setStyleSheet("color: #8f8;");
    posLayout->addWidget(yLabel);

    QDoubleSpinBox* posY = new QDoubleSpinBox();
    posY->setRange(-99999.0, 99999.0);
    posY->setValue(position.y);
    posY->setDecimals(3);
    posY->setMinimumWidth(60);
    posY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posY);

    m_nodePropertiesLayout->addLayout(posLayout);

    // Rotation (float, in degrees for UI)
    float rotation = glm::degrees(node->GetRotation());

    QLabel* rotationLabel = new QLabel("Rotation");
    rotationLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(rotationLabel);

    QHBoxLayout* rotLayout = new QHBoxLayout();
    rotLayout->setContentsMargins(0, 0, 0, 0);
    rotLayout->setSpacing(4);

    QDoubleSpinBox* rotSpinBox = new QDoubleSpinBox();
    rotSpinBox->setRange(-360.0, 360.0);
    rotSpinBox->setValue(rotation);
    rotSpinBox->setDecimals(3);
    rotSpinBox->setSuffix("°");
    rotSpinBox->setMinimumWidth(80);
    rotSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rotLayout->addWidget(rotSpinBox);

    m_nodePropertiesLayout->addLayout(rotLayout);

    // Scale (Vec2)
    glm::vec2 scale = node->GetScale();

    QLabel* scaleLabel = new QLabel("Scale");
    scaleLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(scaleLabel);

    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(4);

    QLabel* scaleXLabel = new QLabel("X:");
    scaleXLabel->setMinimumWidth(15);
    scaleXLabel->setMaximumWidth(15);
    scaleXLabel->setStyleSheet("color: #f88;");
    scaleLayout->addWidget(scaleXLabel);

    QDoubleSpinBox* scaleX = new QDoubleSpinBox();
    scaleX->setRange(0.001, 99999.0);
    scaleX->setValue(scale.x);
    scaleX->setDecimals(3);
    scaleX->setMinimumWidth(60);
    scaleX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scaleLayout->addWidget(scaleX);

    QLabel* scaleYLabel = new QLabel("Y:");
    scaleYLabel->setMinimumWidth(15);
    scaleYLabel->setMaximumWidth(15);
    scaleYLabel->setStyleSheet("color: #8f8;");
    scaleLayout->addWidget(scaleYLabel);

    QDoubleSpinBox* scaleY = new QDoubleSpinBox();
    scaleY->setRange(0.001, 99999.0);
    scaleY->setValue(scale.y);
    scaleY->setDecimals(3);
    scaleY->setMinimumWidth(60);
    scaleY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scaleLayout->addWidget(scaleY);

    scaleLayout->addStretch();

    m_nodePropertiesLayout->addLayout(scaleLayout);

    // Connect signals to update the node when values change
    connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posY](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            glm::vec2 oldPos = node->GetPosition();
            glm::vec2 newPos = glm::vec2(x, posY->value());

            // Record undo action before changing
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeTransformChanged(
                    node,
                    glm::vec3(oldPos.x, oldPos.y, 0.0f), glm::vec3(newPos.x, newPos.y, 0.0f),
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(1.0f), glm::vec3(1.0f),
                    "Change Position"
                );
            }

            node->SetPosition(newPos);
        }
    });
    connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posX](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            glm::vec2 oldPos = node->GetPosition();
            glm::vec2 newPos = glm::vec2(posX->value(), y);

            // Record undo action before changing
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeTransformChanged(
                    node,
                    glm::vec3(oldPos.x, oldPos.y, 0.0f), glm::vec3(newPos.x, newPos.y, 0.0f),
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(1.0f), glm::vec3(1.0f),
                    "Change Position"
                );
            }

            node->SetPosition(newPos);
        }
    });
    connect(rotSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node](double degrees) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            float oldRot = node->GetRotation();
            float newRot = glm::radians(static_cast<float>(degrees));

            // Record undo action before changing
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeTransformChanged(
                    node,
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, oldRot), glm::vec3(0.0f, 0.0f, newRot),
                    glm::vec3(1.0f), glm::vec3(1.0f),
                    "Change Rotation"
                );
            }

            node->SetRotation(newRot);
        }
    });
    connect(scaleX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, scaleY](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            glm::vec2 oldScale = node->GetScale();
            glm::vec2 newScale = glm::vec2(x, scaleY->value());

            // Record undo action before changing
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeTransformChanged(
                    node,
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(oldScale.x, oldScale.y, 1.0f), glm::vec3(newScale.x, newScale.y, 1.0f),
                    "Change Scale"
                );
            }

            node->SetScale(newScale);
        }
    });
    connect(scaleY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, scaleX](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            glm::vec2 oldScale = node->GetScale();
            glm::vec2 newScale = glm::vec2(scaleX->value(), y);

            // Record undo action before changing
            if (m_mainWindow && m_mainWindow->getUndoSystem()) {
                m_mainWindow->getUndoSystem()->recordNodeTransformChanged(
                    node,
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f), glm::vec3(0.0f),
                    glm::vec3(oldScale.x, oldScale.y, 1.0f), glm::vec3(newScale.x, newScale.y, 1.0f),
                    "Change Scale"
                );
            }

            node->SetScale(newScale);
        }
    });
}

void InspectorPanel::addNode3DTransformProperties(Lupine::Node3D* node)
{
    if (!node) return;

    qDebug() << "InspectorPanel::addNode3DTransformProperties - Starting";

    // Add a separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_nodePropertiesLayout->addWidget(separator);

    // Transform section label
    QLabel* transformLabel = new QLabel("Transform");
    QFont boldFont = transformLabel->font();
    boldFont.setBold(true);
    transformLabel->setFont(boldFont);
    m_nodePropertiesLayout->addWidget(transformLabel);

    // Position (Vec3)
    glm::vec3 position = node->GetPosition();

    QLabel* positionLabel = new QLabel("Position");
    positionLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(positionLabel);

    QHBoxLayout* posLayout = new QHBoxLayout();
    posLayout->setContentsMargins(0, 0, 0, 0);
    posLayout->setSpacing(4);

    QLabel* xLabel = new QLabel("X:");
    xLabel->setMinimumWidth(15);
    xLabel->setMaximumWidth(15);
    xLabel->setStyleSheet("color: #f88;");
    posLayout->addWidget(xLabel);

    QDoubleSpinBox* posX = new QDoubleSpinBox();
    posX->setRange(-99999.0, 99999.0);
    posX->setValue(position.x);
    posX->setDecimals(3);
    posX->setMinimumWidth(50);
    posX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posX);

    QLabel* yLabel = new QLabel("Y:");
    yLabel->setMinimumWidth(15);
    yLabel->setMaximumWidth(15);
    yLabel->setStyleSheet("color: #8f8;");
    posLayout->addWidget(yLabel);

    QDoubleSpinBox* posY = new QDoubleSpinBox();
    posY->setRange(-99999.0, 99999.0);
    posY->setValue(position.y);
    posY->setDecimals(3);
    posY->setMinimumWidth(50);
    posY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posY);

    QLabel* zLabel = new QLabel("Z:");
    zLabel->setMinimumWidth(15);
    zLabel->setMaximumWidth(15);
    zLabel->setStyleSheet("color: #88f;");
    posLayout->addWidget(zLabel);

    QDoubleSpinBox* posZ = new QDoubleSpinBox();
    posZ->setRange(-99999.0, 99999.0);
    posZ->setValue(position.z);
    posZ->setDecimals(3);
    posZ->setMinimumWidth(50);
    posZ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posZ);

    m_nodePropertiesLayout->addLayout(posLayout);

    // Rotation (Quaternion, displayed as Euler angles in degrees)
    glm::quat rotation = node->GetRotation();
    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation));

    QLabel* rotationLabel = new QLabel("Rotation");
    rotationLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(rotationLabel);

    QHBoxLayout* rotLayout = new QHBoxLayout();
    rotLayout->setContentsMargins(0, 0, 0, 0);
    rotLayout->setSpacing(4);

    QLabel* rotXLabel = new QLabel("X:");
    rotXLabel->setMinimumWidth(15);
    rotXLabel->setMaximumWidth(15);
    rotXLabel->setStyleSheet("color: #f88;");
    rotLayout->addWidget(rotXLabel);

    QDoubleSpinBox* rotX = new QDoubleSpinBox();
    rotX->setRange(-360.0, 360.0);
    rotX->setValue(eulerAngles.x);
    rotX->setDecimals(3);
    rotX->setSuffix("°");
    rotX->setMinimumWidth(50);
    rotX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rotLayout->addWidget(rotX);

    QLabel* rotYLabel = new QLabel("Y:");
    rotYLabel->setMinimumWidth(15);
    rotYLabel->setMaximumWidth(15);
    rotYLabel->setStyleSheet("color: #8f8;");
    rotLayout->addWidget(rotYLabel);

    QDoubleSpinBox* rotY = new QDoubleSpinBox();
    rotY->setRange(-360.0, 360.0);
    rotY->setValue(eulerAngles.y);
    rotY->setDecimals(3);
    rotY->setSuffix("°");
    rotY->setMinimumWidth(50);
    rotY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rotLayout->addWidget(rotY);

    QLabel* rotZLabel = new QLabel("Z:");
    rotZLabel->setMinimumWidth(15);
    rotZLabel->setMaximumWidth(15);
    rotZLabel->setStyleSheet("color: #88f;");
    rotLayout->addWidget(rotZLabel);

    QDoubleSpinBox* rotZ = new QDoubleSpinBox();
    rotZ->setRange(-360.0, 360.0);
    rotZ->setValue(eulerAngles.z);
    rotZ->setDecimals(3);
    rotZ->setSuffix("°");
    rotZ->setMinimumWidth(50);
    rotZ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rotLayout->addWidget(rotZ);

    m_nodePropertiesLayout->addLayout(rotLayout);

    // Scale (Vec3)
    glm::vec3 scale = node->GetScale();

    QLabel* scaleLabel = new QLabel("Scale");
    scaleLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(scaleLabel);

    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(4);

    QLabel* scaleXLabel = new QLabel("X:");
    scaleXLabel->setMinimumWidth(15);
    scaleXLabel->setMaximumWidth(15);
    scaleXLabel->setStyleSheet("color: #f88;");
    scaleLayout->addWidget(scaleXLabel);

    QDoubleSpinBox* scaleX = new QDoubleSpinBox();
    scaleX->setRange(0.001, 99999.0);
    scaleX->setValue(scale.x);
    scaleX->setDecimals(3);
    scaleX->setMinimumWidth(50);
    scaleX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scaleLayout->addWidget(scaleX);

    QLabel* scaleYLabel = new QLabel("Y:");
    scaleYLabel->setMinimumWidth(15);
    scaleYLabel->setMaximumWidth(15);
    scaleYLabel->setStyleSheet("color: #8f8;");
    scaleLayout->addWidget(scaleYLabel);

    QDoubleSpinBox* scaleY = new QDoubleSpinBox();
    scaleY->setRange(0.001, 99999.0);
    scaleY->setValue(scale.y);
    scaleY->setDecimals(3);
    scaleY->setMinimumWidth(50);
    scaleY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scaleLayout->addWidget(scaleY);

    QLabel* scaleZLabel = new QLabel("Z:");
    scaleZLabel->setMinimumWidth(15);
    scaleZLabel->setMaximumWidth(15);
    scaleZLabel->setStyleSheet("color: #88f;");
    scaleLayout->addWidget(scaleZLabel);

    QDoubleSpinBox* scaleZ = new QDoubleSpinBox();
    scaleZ->setRange(0.001, 99999.0);
    scaleZ->setValue(scale.z);
    scaleZ->setDecimals(3);
    scaleZ->setMinimumWidth(50);
    scaleZ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scaleLayout->addWidget(scaleZ);

    scaleLayout->addStretch();

    m_nodePropertiesLayout->addLayout(scaleLayout);

    // Connect signals to update the node when values change
    connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posY, posZ](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetPosition(glm::vec3(x, posY->value(), posZ->value()));
        }
    });
    connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posX, posZ](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetPosition(glm::vec3(posX->value(), y, posZ->value()));
        }
    });
    connect(posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posX, posY](double z) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetPosition(glm::vec3(posX->value(), posY->value(), z));
        }
    });

    auto updateRotation = [this, node, rotX, rotY, rotZ]() {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            glm::vec3 euler = glm::radians(glm::vec3(rotX->value(), rotY->value(), rotZ->value()));
            node->SetRotation(glm::quat(euler));
        }
    };
    connect(rotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateRotation);
    connect(rotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateRotation);
    connect(rotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateRotation);

    connect(scaleX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, scaleY, scaleZ](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetScale(glm::vec3(x, scaleY->value(), scaleZ->value()));
        }
    });
    connect(scaleY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, scaleX, scaleZ](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetScale(glm::vec3(scaleX->value(), y, scaleZ->value()));
        }
    });
    connect(scaleZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, scaleX, scaleY](double z) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetScale(glm::vec3(scaleX->value(), scaleY->value(), z));
        }
    });
}

void InspectorPanel::addControlTransformProperties(Lupine::Control* node)
{
    if (!node) return;

    qDebug() << "InspectorPanel::addControlTransformProperties - Starting";

    // Add a separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_nodePropertiesLayout->addWidget(separator);

    // Transform section label
    QLabel* transformLabel = new QLabel("Transform");
    QFont boldFont = transformLabel->font();
    boldFont.setBold(true);
    transformLabel->setFont(boldFont);
    m_nodePropertiesLayout->addWidget(transformLabel);

    // Position (Vec2)
    glm::vec2 position = node->GetPosition();

    QLabel* positionLabel = new QLabel("Position");
    positionLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(positionLabel);

    QHBoxLayout* posLayout = new QHBoxLayout();
    posLayout->setContentsMargins(0, 0, 0, 0);
    posLayout->setSpacing(4);

    QLabel* xLabel = new QLabel("X:");
    xLabel->setMinimumWidth(15);
    xLabel->setMaximumWidth(15);
    xLabel->setStyleSheet("color: #f88;");
    posLayout->addWidget(xLabel);

    QDoubleSpinBox* posX = new QDoubleSpinBox();
    posX->setRange(-99999.0, 99999.0);
    posX->setValue(position.x);
    posX->setDecimals(3);
    posX->setMinimumWidth(60);
    posX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posX);

    QLabel* yLabel = new QLabel("Y:");
    yLabel->setMinimumWidth(15);
    yLabel->setMaximumWidth(15);
    yLabel->setStyleSheet("color: #8f8;");
    posLayout->addWidget(yLabel);

    QDoubleSpinBox* posY = new QDoubleSpinBox();
    posY->setRange(-99999.0, 99999.0);
    posY->setValue(position.y);
    posY->setDecimals(3);
    posY->setMinimumWidth(60);
    posY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    posLayout->addWidget(posY);

    m_nodePropertiesLayout->addLayout(posLayout);

    // Size (Vec2)
    glm::vec2 size = node->GetSize();

    QLabel* sizeLabel = new QLabel("Size");
    sizeLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(sizeLabel);

    QHBoxLayout* sizeLayout = new QHBoxLayout();
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    sizeLayout->setSpacing(4);

    QLabel* wLabel = new QLabel("W:");
    wLabel->setMinimumWidth(15);
    wLabel->setMaximumWidth(15);
    wLabel->setStyleSheet("color: #f88;");
    sizeLayout->addWidget(wLabel);

    QDoubleSpinBox* sizeX = new QDoubleSpinBox();
    sizeX->setRange(0.0, 99999.0);
    sizeX->setValue(size.x);
    sizeX->setDecimals(3);
    sizeX->setMinimumWidth(60);
    sizeX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizeLayout->addWidget(sizeX);

    QLabel* hLabel = new QLabel("H:");
    hLabel->setMinimumWidth(15);
    hLabel->setMaximumWidth(15);
    hLabel->setStyleSheet("color: #8f8;");
    sizeLayout->addWidget(hLabel);

    QDoubleSpinBox* sizeY = new QDoubleSpinBox();
    sizeY->setRange(0.0, 99999.0);
    sizeY->setValue(size.y);
    sizeY->setDecimals(3);
    sizeY->setMinimumWidth(60);
    sizeY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizeLayout->addWidget(sizeY);

    sizeLayout->addStretch();

    m_nodePropertiesLayout->addLayout(sizeLayout);

    // World Space toggle
    QLabel* worldSpaceLabel = new QLabel("World Space");
    worldSpaceLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    m_nodePropertiesLayout->addWidget(worldSpaceLabel);

    QHBoxLayout* worldSpaceLayout = new QHBoxLayout();
    worldSpaceLayout->setContentsMargins(0, 0, 0, 0);
    worldSpaceLayout->setSpacing(8);

    QCheckBox* worldSpaceCheckBox = new QCheckBox("Follow 2D Camera");
    worldSpaceCheckBox->setChecked(node->GetWorldSpace());
    worldSpaceCheckBox->setToolTip("When enabled, Control follows 2D camera transforms. When disabled, renders in screen space (UI overlay).");
    worldSpaceCheckBox->setStyleSheet("color: #ddd;");
    worldSpaceLayout->addWidget(worldSpaceCheckBox);
    worldSpaceLayout->addStretch();

    m_nodePropertiesLayout->addLayout(worldSpaceLayout);

    // Connect signals to update the node when values change
    connect(posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posY](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetPosition(glm::vec2(x, posY->value()));
        }
    });
    connect(posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, posX](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetPosition(glm::vec2(posX->value(), y));
        }
    });
    connect(sizeX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, sizeY](double x) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetSize(glm::vec2(x, sizeY->value()));
        }
    });
    connect(sizeY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, node, sizeX](double y) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetSize(glm::vec2(sizeX->value(), y));
        }
    });
    connect(worldSpaceCheckBox, &QCheckBox::toggled, [this, node](bool checked) {
        if (!m_isRebuilding && node && m_selectedNode == node && isNodeValid(node)) {
            node->SetWorldSpace(checked);
        }
    });
}

void InspectorPanel::populateComponentProperties(Lupine::Component* component)
{
    // This method is now replaced by createComponentWidget
    // Keeping for compatibility but it's no longer used
}

void InspectorPanel::createComponentWidget(Lupine::Component* component)
{
    QGroupBox* componentGroup = createComponentGroupBox(component);

    // Insert before the add component button
    int insertIndex = m_componentsLayout->count() - 1;
    m_componentsLayout->insertWidget(insertIndex, componentGroup);

    // Create property editors for export variables
    std::vector<PropertyEditorWidget*> propertyEditors;

    for (const auto& [varName, exportVar] : component->GetAllExportVariables()) {
        PropertyEditorWidget* editor = nullptr;

        // Handle enum types with options
        if (exportVar.type == Lupine::ExportVariableType::Enum && !exportVar.enumOptions.empty()) {
            QStringList options;
            for (const auto& option : exportVar.enumOptions) {
                options.append(QString::fromStdString(option));
            }
            editor = CreatePropertyEditor(
                QString::fromStdString(varName),
                QString::fromStdString(exportVar.description),
                exportVar.type,
                options,
                componentGroup
            );
        } else {
            // Use regular factory function for other types
            editor = CreatePropertyEditor(
                QString::fromStdString(varName),
                QString::fromStdString(exportVar.description),
                exportVar.type,
                componentGroup
            );
        }

        if (editor) {
            editor->setValue(exportVar.value);
            editor->setDefaultValue(exportVar.defaultValue);

            // Set scene for NodeReferencePropertyEditor
            if (auto* nodeRefEditor = dynamic_cast<NodeReferencePropertyEditor*>(editor)) {
                nodeRefEditor->setScene(m_scene);
            }

            componentGroup->layout()->addWidget(editor);
            propertyEditors.push_back(editor);

            // Connect value change signal
            connect(editor, &PropertyEditorWidget::valueChanged,
                    this, &InspectorPanel::onPropertyValueChanged);

            // Connect reset signal
            connect(editor, &PropertyEditorWidget::resetRequested,
                    this, &InspectorPanel::onPropertyResetRequested);
        }
    }

    m_componentPropertyEditors[component] = propertyEditors;
}

QGroupBox* InspectorPanel::createComponentGroupBox(Lupine::Component* component)
{
    QGroupBox* group = new QGroupBox(QString::fromStdString(component->GetTypeName()));
    group->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px 0 4px; }"
    );

    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setContentsMargins(8, 12, 8, 8);
    layout->setSpacing(6);

    // Component header with remove button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    QLabel* componentLabel = new QLabel(QString::fromStdString(component->GetTypeName()));
    componentLabel->setStyleSheet("font-weight: bold; color: #ddd; font-size: 12px;");

    QPushButton* removeButton = new QPushButton("Remove");
    removeButton->setMaximumWidth(80);
    removeButton->setMaximumHeight(24);
    removeButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; border: 1px solid #666; border-radius: 3px; background: #444; font-size: 11px; }"
        "QPushButton:hover { background: #555; }"
        "QPushButton:pressed { background: #333; }"
    );
    removeButton->setProperty("componentPtr", QVariant::fromValue(static_cast<void*>(component)));

    headerLayout->addWidget(componentLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(removeButton);

    layout->addLayout(headerLayout);

    // Add separator line
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("QFrame { color: #555; margin: 4px 0; }");
    layout->addWidget(line);

    connect(removeButton, &QPushButton::clicked, this, &InspectorPanel::onRemoveComponentClicked);

    return group;
}

void InspectorPanel::onAddComponentClicked()
{
    if (!m_selectedNode) return;

    AddComponentDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString componentName = dialog.getSelectedComponentName();
        if (!componentName.isEmpty()) {
            // Create the component
            auto& registry = Lupine::ComponentRegistry::Instance();
            auto component = registry.CreateComponent(componentName.toStdString());

            if (component) {
                // Add to node
                Lupine::Component* componentPtr = component.get();
                m_selectedNode->AddComponent(std::move(component));

                // Create widget for the new component
                createComponentWidget(componentPtr);
            } else {
                QMessageBox::warning(this, "Error", "Failed to create component: " + componentName);
            }
        }
    }
}

void InspectorPanel::onRemoveComponentClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button || !m_selectedNode) return;

    void* componentPtr = button->property("componentPtr").value<void*>();
    Lupine::Component* component = static_cast<Lupine::Component*>(componentPtr);

    if (component) {
        // Remove from tracking
        m_componentPropertyEditors.erase(component);

        // Remove from node
        m_selectedNode->RemoveComponent(component->GetUUID());

        // Refresh the inspector
        setSelectedNode(m_selectedNode);
    }
}

void InspectorPanel::onPropertyValueChanged(const Lupine::ExportValue& value)
{
    PropertyEditorWidget* editor = qobject_cast<PropertyEditorWidget*>(sender());
    if (!editor) return;

    // Find which component this property belongs to
    for (const auto& [component, editors] : m_componentPropertyEditors) {
        auto it = std::find(editors.begin(), editors.end(), editor);
        if (it != editors.end()) {
            // Find the property name by looking at the editor's name label
            QLabel* nameLabel = editor->findChild<QLabel*>();
            if (nameLabel) {
                QString propertyName = nameLabel->text();

                // Get old value before changing
                const Lupine::ExportValue* oldValue = component->GetExportVariable(propertyName.toStdString());

                // Record undo action before changing
                if (oldValue && m_mainWindow && m_mainWindow->getUndoSystem() && m_selectedNode) {
                    m_mainWindow->getUndoSystem()->recordComponentPropertyChanged(
                        m_selectedNode, component, propertyName, *oldValue, value,
                        QString("Change %1").arg(propertyName)
                    );
                }

                // Update the component's export variable
                bool success = component->SetExportVariable(propertyName.toStdString(), value);

                if (success) {
                    // Trigger component update if it has an update method
                    // This ensures the component responds to property changes
                    if (auto* sprite2d = dynamic_cast<Lupine::Sprite2D*>(component)) {
                        // Sprite2D has UpdateFromExportVariables method
                        // This will be called automatically in OnUpdate
                    }
                    // Add similar handling for other component types as needed
                }
            }
            break;
        }
    }
}

void InspectorPanel::onPropertyResetRequested()
{
    PropertyEditorWidget* editor = qobject_cast<PropertyEditorWidget*>(sender());
    if (!editor) return;

    // Find which component this property belongs to
    for (const auto& [component, editors] : m_componentPropertyEditors) {
        auto it = std::find(editors.begin(), editors.end(), editor);
        if (it != editors.end()) {
            // Find the property name by looking at the editor's name label
            QLabel* nameLabel = editor->findChild<QLabel*>();
            if (nameLabel) {
                std::string propertyName = nameLabel->text().toStdString();

                // Reset the property in the component
                bool success = component->ResetExportVariable(propertyName);
                if (success) {
                    // Update the editor with the reset value
                    const Lupine::ExportValue* resetValue = component->GetExportVariable(propertyName);
                    if (resetValue) {
                        editor->setValue(*resetValue);
                    }
                }
            }
            break;
        }
    }
}

bool InspectorPanel::isNodeValid(Lupine::Node* node) const
{
    qDebug() << "InspectorPanel::isNodeValid - Checking node:" << (node ? QString::fromStdString(node->GetName()) : "nullptr");

    if (!node || !m_scene) {
        qDebug() << "InspectorPanel::isNodeValid - Node or scene is null";
        return false;
    }

    try {
        qDebug() << "InspectorPanel::isNodeValid - Getting root node";
        // Check if the node exists in the scene hierarchy using safer recursive search
        if (m_scene->GetRootNode()) {
            qDebug() << "InspectorPanel::isNodeValid - Root node exists, checking if target is root";
            // For root node, check directly
            if (node == m_scene->GetRootNode()) {
                qDebug() << "InspectorPanel::isNodeValid - Node is root, returning true";
                return true;
            }

            qDebug() << "InspectorPanel::isNodeValid - Searching hierarchy for node";
            // For other nodes, use recursive search without creating vectors
            bool result = isNodeInHierarchy(m_scene->GetRootNode(), node);
            qDebug() << "InspectorPanel::isNodeValid - Hierarchy search result:" << result;
            return result;
        }
    } catch (...) {
        qDebug() << "InspectorPanel::isNodeValid - Exception occurred during validation";
        // If any exception occurs during validation, consider the node invalid
        return false;
    }

    qDebug() << "InspectorPanel::isNodeValid - No root node, returning false";
    return false;
}

bool InspectorPanel::isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const
{
    if (!root || !target) return false;

    try {
        // Check if this is the target node
        if (root == target) {
            return true;
        }

        // Check children recursively
        for (const auto& child : root->GetChildren()) {
            if (child.get() == target || isNodeInHierarchy(child.get(), target)) {
                return true;
            }
        }
    } catch (...) {
        // If any exception occurs during traversal, return false
        return false;
    }

    return false;
}
