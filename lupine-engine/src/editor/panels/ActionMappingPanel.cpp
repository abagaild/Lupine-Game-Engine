#include "ActionMappingPanel.h"
#include "lupine/input/InputConstants.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QApplication>

namespace Lupine {

// InputCaptureDialog implementation
InputCaptureDialog::InputCaptureDialog(QWidget* parent)
    : QDialog(parent)
    , m_hasCapture(false) {
    setupUI();
    setWindowTitle("Capture Input");
    setModal(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void InputCaptureDialog::setupUI() {
    m_layout = new QVBoxLayout(this);
    
    m_instructionLabel = new QLabel("Press any key or mouse button to capture...\n(Gamepad support coming soon)");
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_instructionLabel);
    
    m_capturedLabel = new QLabel("");
    m_capturedLabel->setAlignment(Qt::AlignCenter);
    m_capturedLabel->setStyleSheet("font-weight: bold; color: green;");
    m_capturedLabel->setVisible(false);
    m_layout->addWidget(m_capturedLabel);
    
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_layout->addWidget(m_buttonBox);
    
    resize(300, 150);
}

void InputCaptureDialog::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return; // Ignore key repeats
    }

    // Convert Qt key to SDL key
    int sdl_key = QtKeyToSDLKey(event->key());
    if (sdl_key == -1) {
        return; // Unsupported key
    }

    ActionBinding binding(InputDevice::Keyboard, sdl_key, InputActionType::Pressed);
    captureBinding(binding);
}

void InputCaptureDialog::mousePressEvent(QMouseEvent* event) {
    int button = 0;
    switch (event->button()) {
        case Qt::LeftButton: button = InputConstants::MOUSE_BUTTON_LEFT; break;
        case Qt::RightButton: button = InputConstants::MOUSE_BUTTON_RIGHT; break;
        case Qt::MiddleButton: button = InputConstants::MOUSE_BUTTON_MIDDLE; break;
        default: return; // Ignore other buttons
    }
    
    ActionBinding binding(InputDevice::Mouse, button, InputActionType::Pressed);
    captureBinding(binding);
}

void InputCaptureDialog::captureBinding(const ActionBinding& binding) {
    m_capturedBinding = binding;
    m_hasCapture = true;
    
    QString displayName;
    if (binding.device == InputDevice::Keyboard) {
        displayName = QString::fromStdString(InputConstants::GetKeyName(binding.code));
    } else if (binding.device == InputDevice::Mouse) {
        displayName = QString::fromStdString(InputConstants::GetMouseButtonName(binding.code));
    } else if (binding.device == InputDevice::Gamepad) {
        displayName = QString::fromStdString(InputConstants::GetGamepadButtonName(binding.code));
    }
    
    m_capturedLabel->setText(QString("Captured: %1").arg(displayName));
    m_capturedLabel->setVisible(true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void InputCaptureDialog::onCancelClicked() {
    reject();
}

int InputCaptureDialog::QtKeyToSDLKey(int qtKey) {
    // Convert Qt key codes to SDL key codes
    switch (qtKey) {
        // Letters
        case Qt::Key_A: return SDLK_a;
        case Qt::Key_B: return SDLK_b;
        case Qt::Key_C: return SDLK_c;
        case Qt::Key_D: return SDLK_d;
        case Qt::Key_E: return SDLK_e;
        case Qt::Key_F: return SDLK_f;
        case Qt::Key_G: return SDLK_g;
        case Qt::Key_H: return SDLK_h;
        case Qt::Key_I: return SDLK_i;
        case Qt::Key_J: return SDLK_j;
        case Qt::Key_K: return SDLK_k;
        case Qt::Key_L: return SDLK_l;
        case Qt::Key_M: return SDLK_m;
        case Qt::Key_N: return SDLK_n;
        case Qt::Key_O: return SDLK_o;
        case Qt::Key_P: return SDLK_p;
        case Qt::Key_Q: return SDLK_q;
        case Qt::Key_R: return SDLK_r;
        case Qt::Key_S: return SDLK_s;
        case Qt::Key_T: return SDLK_t;
        case Qt::Key_U: return SDLK_u;
        case Qt::Key_V: return SDLK_v;
        case Qt::Key_W: return SDLK_w;
        case Qt::Key_X: return SDLK_x;
        case Qt::Key_Y: return SDLK_y;
        case Qt::Key_Z: return SDLK_z;

        // Numbers
        case Qt::Key_0: return SDLK_0;
        case Qt::Key_1: return SDLK_1;
        case Qt::Key_2: return SDLK_2;
        case Qt::Key_3: return SDLK_3;
        case Qt::Key_4: return SDLK_4;
        case Qt::Key_5: return SDLK_5;
        case Qt::Key_6: return SDLK_6;
        case Qt::Key_7: return SDLK_7;
        case Qt::Key_8: return SDLK_8;
        case Qt::Key_9: return SDLK_9;

        // Function keys
        case Qt::Key_F1: return SDLK_F1;
        case Qt::Key_F2: return SDLK_F2;
        case Qt::Key_F3: return SDLK_F3;
        case Qt::Key_F4: return SDLK_F4;
        case Qt::Key_F5: return SDLK_F5;
        case Qt::Key_F6: return SDLK_F6;
        case Qt::Key_F7: return SDLK_F7;
        case Qt::Key_F8: return SDLK_F8;
        case Qt::Key_F9: return SDLK_F9;
        case Qt::Key_F10: return SDLK_F10;
        case Qt::Key_F11: return SDLK_F11;
        case Qt::Key_F12: return SDLK_F12;

        // Special keys
        case Qt::Key_Space: return SDLK_SPACE;
        case Qt::Key_Return: return SDLK_RETURN;
        case Qt::Key_Enter: return SDLK_RETURN;
        case Qt::Key_Escape: return SDLK_ESCAPE;
        case Qt::Key_Backspace: return SDLK_BACKSPACE;
        case Qt::Key_Tab: return SDLK_TAB;
        case Qt::Key_Shift: return SDLK_LSHIFT;
        case Qt::Key_Control: return SDLK_LCTRL;
        case Qt::Key_Alt: return SDLK_LALT;
        case Qt::Key_Delete: return SDLK_DELETE;
        case Qt::Key_Insert: return SDLK_INSERT;
        case Qt::Key_Home: return SDLK_HOME;
        case Qt::Key_End: return SDLK_END;
        case Qt::Key_PageUp: return SDLK_PAGEUP;
        case Qt::Key_PageDown: return SDLK_PAGEDOWN;

        // Arrow keys
        case Qt::Key_Up: return SDLK_UP;
        case Qt::Key_Down: return SDLK_DOWN;
        case Qt::Key_Left: return SDLK_LEFT;
        case Qt::Key_Right: return SDLK_RIGHT;

        // Punctuation and symbols
        case Qt::Key_Minus: return SDLK_MINUS;
        case Qt::Key_Equal: return SDLK_EQUALS;
        case Qt::Key_BracketLeft: return SDLK_LEFTBRACKET;
        case Qt::Key_BracketRight: return SDLK_RIGHTBRACKET;
        case Qt::Key_Backslash: return SDLK_BACKSLASH;
        case Qt::Key_Semicolon: return SDLK_SEMICOLON;
        case Qt::Key_Apostrophe: return SDLK_QUOTE;
        case Qt::Key_Comma: return SDLK_COMMA;
        case Qt::Key_Period: return SDLK_PERIOD;
        case Qt::Key_Slash: return SDLK_SLASH;
        case Qt::Key_QuoteLeft: return SDLK_BACKQUOTE;

        default:
            return -1; // Unsupported key
    }
}

// ActionMappingPanel implementation
ActionMappingPanel::ActionMappingPanel(QWidget* parent)
    : QWidget(parent)
    , m_actionMap(nullptr) {
    setupUI();
}

void ActionMappingPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // File operations buttons at top
    m_fileButtonLayout = new QHBoxLayout();
    m_loadButton = new QPushButton("Load Action Map");
    m_saveButton = new QPushButton("Save Action Map");
    m_resetButton = new QPushButton("Reset to Default");
    
    m_fileButtonLayout->addWidget(m_loadButton);
    m_fileButtonLayout->addWidget(m_saveButton);
    m_fileButtonLayout->addWidget(m_resetButton);
    m_fileButtonLayout->addStretch();
    
    m_mainLayout->addLayout(m_fileButtonLayout);
    
    // Main splitter
    m_splitter = new QSplitter(Qt::Horizontal);
    m_mainLayout->addWidget(m_splitter);
    
    // Left side - Action list
    m_actionListWidget = new QWidget();
    m_actionListLayout = new QVBoxLayout(m_actionListWidget);
    
    QLabel* actionListLabel = new QLabel("Actions");
    actionListLabel->setStyleSheet("font-weight: bold;");
    m_actionListLayout->addWidget(actionListLabel);
    
    m_actionTree = new QTreeWidget();
    m_actionTree->setHeaderLabel("Action Name");
    m_actionTree->setRootIsDecorated(false);
    m_actionTree->setAlternatingRowColors(true);
    m_actionListLayout->addWidget(m_actionTree);
    
    m_actionButtonLayout = new QHBoxLayout();
    m_addActionButton = new QPushButton("Add Action");
    m_removeActionButton = new QPushButton("Remove Action");
    m_removeActionButton->setEnabled(false);
    
    m_actionButtonLayout->addWidget(m_addActionButton);
    m_actionButtonLayout->addWidget(m_removeActionButton);
    m_actionListLayout->addLayout(m_actionButtonLayout);
    
    m_splitter->addWidget(m_actionListWidget);
    
    // Right side - Action details
    m_actionDetailsWidget = new QWidget();
    m_actionDetailsLayout = new QVBoxLayout(m_actionDetailsWidget);
    
    // Action properties group
    m_actionPropertiesGroup = new QGroupBox("Action Properties");
    m_actionPropertiesLayout = new QVBoxLayout(m_actionPropertiesGroup);
    
    QLabel* nameLabel = new QLabel("Name:");
    m_actionNameEdit = new QLineEdit();
    m_actionNameEdit->setEnabled(false);
    
    QLabel* descLabel = new QLabel("Description:");
    m_actionDescriptionEdit = new QTextEdit();
    m_actionDescriptionEdit->setMaximumHeight(80);
    m_actionDescriptionEdit->setEnabled(false);
    
    m_actionPropertiesLayout->addWidget(nameLabel);
    m_actionPropertiesLayout->addWidget(m_actionNameEdit);
    m_actionPropertiesLayout->addWidget(descLabel);
    m_actionPropertiesLayout->addWidget(m_actionDescriptionEdit);
    
    m_actionDetailsLayout->addWidget(m_actionPropertiesGroup);
    
    // Bindings group
    m_bindingsGroup = new QGroupBox("Input Bindings");
    m_bindingsLayout = new QVBoxLayout(m_bindingsGroup);
    
    m_bindingTree = new QTreeWidget();
    QStringList bindingHeaders;
    bindingHeaders << "Device" << "Input" << "Type";
    m_bindingTree->setHeaderLabels(bindingHeaders);
    m_bindingTree->setRootIsDecorated(false);
    m_bindingTree->setAlternatingRowColors(true);
    m_bindingsLayout->addWidget(m_bindingTree);
    
    m_bindingButtonLayout = new QHBoxLayout();
    m_addBindingButton = new QPushButton("Add Binding");
    m_removeBindingButton = new QPushButton("Remove Binding");
    m_addBindingButton->setEnabled(false);
    m_removeBindingButton->setEnabled(false);
    
    m_bindingButtonLayout->addWidget(m_addBindingButton);
    m_bindingButtonLayout->addWidget(m_removeBindingButton);
    m_bindingsLayout->addLayout(m_bindingButtonLayout);
    
    m_actionDetailsLayout->addWidget(m_bindingsGroup);
    m_actionDetailsLayout->addStretch();
    
    m_splitter->addWidget(m_actionDetailsWidget);
    
    // Set splitter proportions
    m_splitter->setSizes({300, 500});
    
    // Connect signals
    connect(m_actionTree, &QTreeWidget::itemSelectionChanged, this, &ActionMappingPanel::onActionSelectionChanged);
    connect(m_addActionButton, &QPushButton::clicked, this, &ActionMappingPanel::onAddActionClicked);
    connect(m_removeActionButton, &QPushButton::clicked, this, &ActionMappingPanel::onRemoveActionClicked);
    connect(m_addBindingButton, &QPushButton::clicked, this, &ActionMappingPanel::onAddBindingClicked);
    connect(m_removeBindingButton, &QPushButton::clicked, this, &ActionMappingPanel::onRemoveBindingClicked);
    connect(m_loadButton, &QPushButton::clicked, this, &ActionMappingPanel::onLoadActionMapClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &ActionMappingPanel::onSaveActionMapClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &ActionMappingPanel::onResetToDefaultClicked);
    connect(m_actionNameEdit, &QLineEdit::textChanged, this, &ActionMappingPanel::onActionNameChanged);
    connect(m_actionDescriptionEdit, &QTextEdit::textChanged, this, &ActionMappingPanel::onActionDescriptionChanged);
    connect(m_bindingTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        m_removeBindingButton->setEnabled(m_bindingTree->currentItem() != nullptr);
    });
}

void ActionMappingPanel::SetActionMap(ActionMap* actionMap) {
    m_actionMap = actionMap;
    refreshActionList();
    clearActionDetails();
}

void ActionMappingPanel::onActionSelectionChanged() {
    QTreeWidgetItem* item = m_actionTree->currentItem();
    if (item) {
        m_currentActionName = item->text(0);
        m_removeActionButton->setEnabled(true);
        updateActionDetails();
    } else {
        m_currentActionName.clear();
        m_removeActionButton->setEnabled(false);
        clearActionDetails();
    }
}

void ActionMappingPanel::onAddActionClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Action", "Action name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty() && m_actionMap) {
        if (m_actionMap->GetAction(name.toStdString())) {
            QMessageBox::warning(this, "Error", "An action with this name already exists.");
            return;
        }
        
        QString description = QInputDialog::getText(this, "Add Action", "Action description (optional):", QLineEdit::Normal, "", &ok);
        if (!ok) description.clear();
        
        m_actionMap->AddAction(name.toStdString(), description.toStdString());
        refreshActionList();
        
        // Select the new action
        for (int i = 0; i < m_actionTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_actionTree->topLevelItem(i);
            if (item->text(0) == name) {
                m_actionTree->setCurrentItem(item);
                break;
            }
        }
        
        emit actionMapModified();
    }
}

void ActionMappingPanel::onRemoveActionClicked() {
    if (m_currentActionName.isEmpty() || !m_actionMap) return;

    int ret = QMessageBox::question(this, "Remove Action",
        QString("Are you sure you want to remove the action '%1'?").arg(m_currentActionName),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        m_actionMap->RemoveAction(m_currentActionName.toStdString());
        refreshActionList();
        clearActionDetails();
        emit actionMapModified();
    }
}

void ActionMappingPanel::onAddBindingClicked() {
    if (m_currentActionName.isEmpty() || !m_actionMap) return;

    InputCaptureDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted && dialog.HasCapturedBinding()) {
        ActionBinding binding = dialog.GetCapturedBinding();

        // Check for conflicts
        QString conflictingAction = checkBindingConflict(binding, m_currentActionName);
        if (!conflictingAction.isEmpty()) {
            if (!showBindingConflictWarning(binding, conflictingAction)) {
                return; // User cancelled
            }
        }

        m_actionMap->AddBinding(m_currentActionName.toStdString(), binding);
        refreshBindingList();
        emit actionMapModified();
    }
}

void ActionMappingPanel::onRemoveBindingClicked() {
    QTreeWidgetItem* item = m_bindingTree->currentItem();
    if (!item || m_currentActionName.isEmpty() || !m_actionMap) return;

    // Get binding info from the item
    QString deviceStr = item->text(0);
    QString inputStr = item->text(1);
    QString typeStr = item->text(2);

    // Find the corresponding binding
    Action* action = m_actionMap->GetAction(m_currentActionName.toStdString());
    if (!action) return;

    for (const auto& binding : action->bindings) {
        QString bindingDisplay = getBindingDisplayName(binding);
        if (bindingDisplay.contains(inputStr)) {
            m_actionMap->RemoveBinding(m_currentActionName.toStdString(), binding);
            refreshBindingList();
            emit actionMapModified();
            break;
        }
    }
}

void ActionMappingPanel::onLoadActionMapClicked() {
    QString filename = QFileDialog::getOpenFileName(this, "Load Action Map", "", "JSON Files (*.json)");
    if (!filename.isEmpty() && m_actionMap) {
        if (m_actionMap->LoadFromFile(filename.toStdString())) {
            refreshActionList();
            clearActionDetails();
            emit actionMapModified();
            QMessageBox::information(this, "Success", "Action map loaded successfully.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to load action map.");
        }
    }
}

void ActionMappingPanel::onSaveActionMapClicked() {
    if (!m_actionMap) return;

    QString filename = QFileDialog::getSaveFileName(this, "Save Action Map", "", "JSON Files (*.json)");
    if (!filename.isEmpty()) {
        if (m_actionMap->SaveToFile(filename.toStdString())) {
            QMessageBox::information(this, "Success", "Action map saved successfully.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to save action map.");
        }
    }
}

void ActionMappingPanel::onResetToDefaultClicked() {
    int ret = QMessageBox::question(this, "Reset to Default",
        "Are you sure you want to reset to the default action map? This will remove all custom actions and bindings.",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes && m_actionMap) {
        *m_actionMap = ActionMap::CreateDefault();
        refreshActionList();
        clearActionDetails();
        emit actionMapModified();
    }
}

void ActionMappingPanel::onActionNameChanged() {
    if (m_currentActionName.isEmpty() || !m_actionMap) return;

    QString newName = m_actionNameEdit->text();
    if (newName != m_currentActionName && !newName.isEmpty()) {
        // Check if new name already exists
        if (m_actionMap->GetAction(newName.toStdString())) {
            QMessageBox::warning(this, "Error", "An action with this name already exists.");
            m_actionNameEdit->setText(m_currentActionName); // Revert
            return;
        }

        // Rename action (remove old, add new with same bindings)
        Action* oldAction = m_actionMap->GetAction(m_currentActionName.toStdString());
        if (oldAction) {
            Action newAction(newName.toStdString(), oldAction->description);
            newAction.bindings = oldAction->bindings;

            m_actionMap->RemoveAction(m_currentActionName.toStdString());
            m_actionMap->AddAction(newAction.name, newAction.description);

            Action* addedAction = m_actionMap->GetAction(newAction.name);
            if (addedAction) {
                addedAction->bindings = newAction.bindings;
            }

            m_currentActionName = newName;
            refreshActionList();

            // Reselect the renamed action
            for (int i = 0; i < m_actionTree->topLevelItemCount(); ++i) {
                QTreeWidgetItem* item = m_actionTree->topLevelItem(i);
                if (item->text(0) == newName) {
                    m_actionTree->setCurrentItem(item);
                    break;
                }
            }

            emit actionMapModified();
        }
    }
}

void ActionMappingPanel::onActionDescriptionChanged() {
    if (m_currentActionName.isEmpty() || !m_actionMap) return;

    Action* action = m_actionMap->GetAction(m_currentActionName.toStdString());
    if (action) {
        action->description = m_actionDescriptionEdit->toPlainText().toStdString();
        emit actionMapModified();
    }
}

void ActionMappingPanel::refreshActionList() {
    m_actionTree->clear();

    if (!m_actionMap) return;

    for (const auto& [name, action] : m_actionMap->GetActions()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_actionTree);
        item->setText(0, QString::fromStdString(name));

        // Add tooltip with description
        if (!action.description.empty()) {
            item->setToolTip(0, QString::fromStdString(action.description));
        }
    }

    m_actionTree->sortItems(0, Qt::AscendingOrder);
}

void ActionMappingPanel::refreshBindingList() {
    m_bindingTree->clear();

    if (m_currentActionName.isEmpty() || !m_actionMap) return;

    const Action* action = m_actionMap->GetAction(m_currentActionName.toStdString());
    if (!action) return;

    for (const auto& binding : action->bindings) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_bindingTree);

        QString deviceName;
        QString inputName;

        switch (binding.device) {
            case InputDevice::Keyboard:
                deviceName = "Keyboard";
                inputName = QString::fromStdString(InputConstants::GetKeyName(binding.code));
                break;
            case InputDevice::Mouse:
                deviceName = "Mouse";
                inputName = QString::fromStdString(InputConstants::GetMouseButtonName(binding.code));
                break;
            case InputDevice::Gamepad:
                deviceName = "Gamepad";
                inputName = QString::fromStdString(InputConstants::GetGamepadButtonName(binding.code));
                break;
        }

        QString typeName;
        switch (binding.action_type) {
            case InputActionType::Pressed: typeName = "Pressed"; break;
            case InputActionType::Released: typeName = "Released"; break;
            case InputActionType::Held: typeName = "Held"; break;
        }

        item->setText(0, deviceName);
        item->setText(1, inputName);
        item->setText(2, typeName);
    }
}

void ActionMappingPanel::updateActionDetails() {
    if (m_currentActionName.isEmpty() || !m_actionMap) {
        clearActionDetails();
        return;
    }

    const Action* action = m_actionMap->GetAction(m_currentActionName.toStdString());
    if (!action) {
        clearActionDetails();
        return;
    }

    m_actionNameEdit->setEnabled(true);
    m_actionDescriptionEdit->setEnabled(true);
    m_addBindingButton->setEnabled(true);

    m_actionNameEdit->setText(QString::fromStdString(action->name));
    m_actionDescriptionEdit->setPlainText(QString::fromStdString(action->description));

    refreshBindingList();
}

void ActionMappingPanel::clearActionDetails() {
    m_actionNameEdit->clear();
    m_actionNameEdit->setEnabled(false);
    m_actionDescriptionEdit->clear();
    m_actionDescriptionEdit->setEnabled(false);
    m_addBindingButton->setEnabled(false);
    m_removeBindingButton->setEnabled(false);
    m_bindingTree->clear();
}

QString ActionMappingPanel::getBindingDisplayName(const ActionBinding& binding) const {
    QString deviceName;
    QString inputName;

    switch (binding.device) {
        case InputDevice::Keyboard:
            deviceName = "Keyboard";
            inputName = QString::fromStdString(InputConstants::GetKeyName(binding.code));
            break;
        case InputDevice::Mouse:
            deviceName = "Mouse";
            inputName = QString::fromStdString(InputConstants::GetMouseButtonName(binding.code));
            break;
        case InputDevice::Gamepad:
            deviceName = "Gamepad";
            inputName = QString::fromStdString(InputConstants::GetGamepadButtonName(binding.code));
            break;
    }

    return QString("%1: %2").arg(deviceName, inputName);
}

QString ActionMappingPanel::checkBindingConflict(const ActionBinding& binding, const QString& excludeAction) const {
    if (!m_actionMap) return "";

    for (const auto& [name, action] : m_actionMap->GetActions()) {
        QString actionName = QString::fromStdString(name);
        if (actionName == excludeAction) continue;

        if (action.HasBinding(binding)) {
            return actionName;
        }
    }

    return "";
}

bool ActionMappingPanel::showBindingConflictWarning(const ActionBinding& binding, const QString& conflictingAction) const {
    QString bindingName = getBindingDisplayName(binding);

    int ret = QMessageBox::question(const_cast<ActionMappingPanel*>(this), "Binding Conflict",
        QString("The binding '%1' is already used by action '%2'.\n\nDo you want to proceed anyway? This will create a conflict.")
        .arg(bindingName, conflictingAction),
        QMessageBox::Yes | QMessageBox::No);

    return ret == QMessageBox::Yes;
}

} // namespace Lupine
