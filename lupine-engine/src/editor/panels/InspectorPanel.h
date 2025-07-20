#pragma once

#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFrame>
#include <unordered_map>
#include "lupine/core/Component.h"

namespace Lupine {
    class Node;
    class Node2D;
    class Node3D;
    class Control;
    class Scene;
}

class PropertyEditorWidget;
class AddComponentDialog;

class InspectorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit InspectorPanel(QWidget *parent = nullptr);
    ~InspectorPanel();

    void setSelectedNode(Lupine::Node* node);
    void setScene(Lupine::Scene* scene);
    void setMainWindow(class MainWindow* mainWindow);

signals:
    void nodeNameChanged(Lupine::Node* node, const QString& newName);

private slots:
    void onAddComponentClicked();
    void onRemoveComponentClicked();
    void onPropertyValueChanged(const Lupine::ExportValue& value);
    void onPropertyResetRequested();

private:
    void setupUI();
    void clearInspector(bool willHaveNode = false);
    void clearLayout(QLayout* layout);
    void populateNodeProperties(Lupine::Node* node);
    void populateComponentProperties(Lupine::Component* component);
    void createComponentWidget(Lupine::Component* component);
    QGroupBox* createComponentGroupBox(Lupine::Component* component);
    void addTransformProperties(Lupine::Node* node);
    void addNode2DTransformProperties(Lupine::Node2D* node);
    void addNode3DTransformProperties(Lupine::Node3D* node);
    void addControlTransformProperties(Lupine::Control* node);
    bool isNodeValid(Lupine::Node* node) const;
    bool isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const;
    
    QVBoxLayout* m_layout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QLabel* m_noSelectionLabel;

    // Node properties section
    QGroupBox* m_nodePropertiesGroup;
    QVBoxLayout* m_nodePropertiesLayout;

    // Components section
    QGroupBox* m_componentsGroup;
    QVBoxLayout* m_componentsLayout;
    QPushButton* m_addComponentButton;

    Lupine::Node* m_selectedNode;
    Lupine::Scene* m_scene;
    class MainWindow* m_mainWindow;

    // Property editor tracking
    std::unordered_map<Lupine::Component*, std::vector<PropertyEditorWidget*>> m_componentPropertyEditors;

    // Flag to prevent operations during UI rebuild
    bool m_isRebuilding;
};

#endif // INSPECTORPANEL_H
