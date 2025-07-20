#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QSplitter>
#include <QTextEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>

#include "lupine/input/ActionMap.h"

namespace Lupine {

/**
 * @brief Dialog for capturing input bindings
 */
class InputCaptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit InputCaptureDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Get the captured binding
     * @return Captured action binding
     */
    ActionBinding GetCapturedBinding() const { return m_capturedBinding; }
    
    /**
     * @brief Check if a binding was captured
     * @return True if binding was captured
     */
    bool HasCapturedBinding() const { return m_hasCapture; }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onCancelClicked();

private:
    void setupUI();
    void captureBinding(const ActionBinding& binding);
    int QtKeyToSDLKey(int qtKey);

    QVBoxLayout* m_layout;
    QLabel* m_instructionLabel;
    QLabel* m_capturedLabel;
    QDialogButtonBox* m_buttonBox;

    ActionBinding m_capturedBinding;
    bool m_hasCapture;
};

/**
 * @brief Panel for configuring action mappings
 */
class ActionMappingPanel : public QWidget {
    Q_OBJECT

public:
    explicit ActionMappingPanel(QWidget* parent = nullptr);
    
    /**
     * @brief Set the action map to edit
     * @param actionMap Action map to edit
     */
    void SetActionMap(ActionMap* actionMap);
    
    /**
     * @brief Get the current action map
     * @return Current action map
     */
    ActionMap* GetActionMap() const { return m_actionMap; }

signals:
    /**
     * @brief Emitted when the action map is modified
     */
    void actionMapModified();

private slots:
    void onActionSelectionChanged();
    void onAddActionClicked();
    void onRemoveActionClicked();
    void onAddBindingClicked();
    void onRemoveBindingClicked();
    void onLoadActionMapClicked();
    void onSaveActionMapClicked();
    void onResetToDefaultClicked();
    void onActionNameChanged();
    void onActionDescriptionChanged();

private:
    void setupUI();
    void refreshActionList();
    void refreshBindingList();
    void updateActionDetails();
    void clearActionDetails();
    
    /**
     * @brief Get human-readable name for a binding
     * @param binding Action binding
     * @return Human-readable name
     */
    QString getBindingDisplayName(const ActionBinding& binding) const;
    
    /**
     * @brief Check for binding conflicts
     * @param binding Binding to check
     * @param excludeAction Action to exclude from conflict check
     * @return Conflicting action name or empty string
     */
    QString checkBindingConflict(const ActionBinding& binding, const QString& excludeAction = "") const;
    
    /**
     * @brief Show binding conflict warning
     * @param binding Conflicting binding
     * @param conflictingAction Action that has the conflicting binding
     * @return True if user wants to proceed anyway
     */
    bool showBindingConflictWarning(const ActionBinding& binding, const QString& conflictingAction) const;
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QSplitter* m_splitter;
    
    // Left side - Action list
    QWidget* m_actionListWidget;
    QVBoxLayout* m_actionListLayout;
    QTreeWidget* m_actionTree;
    QHBoxLayout* m_actionButtonLayout;
    QPushButton* m_addActionButton;
    QPushButton* m_removeActionButton;
    
    // Right side - Action details
    QWidget* m_actionDetailsWidget;
    QVBoxLayout* m_actionDetailsLayout;
    
    // Action properties
    QGroupBox* m_actionPropertiesGroup;
    QVBoxLayout* m_actionPropertiesLayout;
    QLineEdit* m_actionNameEdit;
    QTextEdit* m_actionDescriptionEdit;
    
    // Bindings
    QGroupBox* m_bindingsGroup;
    QVBoxLayout* m_bindingsLayout;
    QTreeWidget* m_bindingTree;
    QHBoxLayout* m_bindingButtonLayout;
    QPushButton* m_addBindingButton;
    QPushButton* m_removeBindingButton;
    
    // File operations
    QHBoxLayout* m_fileButtonLayout;
    QPushButton* m_loadButton;
    QPushButton* m_saveButton;
    QPushButton* m_resetButton;
    
    // Data
    ActionMap* m_actionMap;
    QString m_currentActionName;
};

} // namespace Lupine
