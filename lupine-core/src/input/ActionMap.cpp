#include "lupine/input/ActionMap.h"
#include "lupine/input/InputConstants.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace Lupine {

ActionMap::ActionMap() {
}

ActionMap::~ActionMap() {
}

Action& ActionMap::AddAction(const std::string& name, const std::string& description) {
    m_actions[name] = Action(name, description);
    return m_actions[name];
}

void ActionMap::RemoveAction(const std::string& name) {
    m_actions.erase(name);
}

Action* ActionMap::GetAction(const std::string& name) {
    auto it = m_actions.find(name);
    return (it != m_actions.end()) ? &it->second : nullptr;
}

const Action* ActionMap::GetAction(const std::string& name) const {
    auto it = m_actions.find(name);
    return (it != m_actions.end()) ? &it->second : nullptr;
}

bool ActionMap::AddBinding(const std::string& action_name, const ActionBinding& binding) {
    auto* action = GetAction(action_name);
    if (action) {
        action->AddBinding(binding);
        return true;
    }
    return false;
}

bool ActionMap::RemoveBinding(const std::string& action_name, const ActionBinding& binding) {
    auto* action = GetAction(action_name);
    if (action) {
        action->RemoveBinding(binding);
        return true;
    }
    return false;
}

bool ActionMap::HasBinding(const std::string& action_name, const ActionBinding& binding) const {
    const auto* action = GetAction(action_name);
    return action ? action->HasBinding(binding) : false;
}

std::string ActionMap::FindActionForBinding(const ActionBinding& binding) const {
    for (const auto& [name, action] : m_actions) {
        if (action.HasBinding(binding)) {
            return name;
        }
    }
    return "";
}

void ActionMap::Clear() {
    m_actions.clear();
}

void ActionMap::ApplyToInputManager() {
    // Clear existing actions in InputManager
    InputManager::ClearAllBindings();

    // Register all actions with InputManager
    for (const auto& [name, action] : m_actions) {
        if (!action.bindings.empty()) {
            // Register the first binding as the primary action
            const auto& first_binding = action.bindings[0];
            InputManager::RegisterAction(name, first_binding.ToInputBinding(), nullptr);

            // Add additional bindings if any
            for (size_t i = 1; i < action.bindings.size(); ++i) {
                const auto& binding = action.bindings[i];
                InputManager::AddActionBinding(name, binding.ToInputBinding());
            }
        }
    }
}

bool ActionMap::LoadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open action map file: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json json;
        file >> json;
        return LoadFromJson(json);
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading action map from file: " << e.what() << std::endl;
        return false;
    }
}

bool ActionMap::SaveToFile(const std::string& filepath) const {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to create action map file: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json json = ToJson();
        file << json.dump(4); // Pretty print with 4 spaces
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving action map to file: " << e.what() << std::endl;
        return false;
    }
}

bool ActionMap::LoadFromJson(const nlohmann::json& json) {
    try {
        Clear();
        
        if (!json.contains("actions") || !json["actions"].is_array()) {
            std::cerr << "Invalid action map JSON: missing 'actions' array" << std::endl;
            return false;
        }
        
        for (const auto& actionJson : json["actions"]) {
            if (!actionJson.contains("name") || !actionJson["name"].is_string()) {
                continue;
            }
            
            std::string name = actionJson["name"];
            std::string description = actionJson.value("description", "");
            
            Action& action = AddAction(name, description);
            
            if (actionJson.contains("bindings") && actionJson["bindings"].is_array()) {
                for (const auto& bindingJson : actionJson["bindings"]) {
                    if (!bindingJson.contains("device") || !bindingJson.contains("code") || 
                        !bindingJson.contains("action_type")) {
                        continue;
                    }
                    
                    InputDevice device = StringToDevice(bindingJson["device"]);
                    int code = bindingJson["code"];
                    InputActionType actionType = StringToActionType(bindingJson["action_type"]);
                    
                    action.AddBinding(ActionBinding(device, code, actionType));
                }
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing action map JSON: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::json ActionMap::ToJson() const {
    nlohmann::json json;
    json["version"] = "1.0";
    json["actions"] = nlohmann::json::array();
    
    for (const auto& [name, action] : m_actions) {
        nlohmann::json actionJson;
        actionJson["name"] = action.name;
        actionJson["description"] = action.description;
        actionJson["bindings"] = nlohmann::json::array();
        
        for (const auto& binding : action.bindings) {
            nlohmann::json bindingJson;
            bindingJson["device"] = DeviceToString(binding.device);
            bindingJson["code"] = binding.code;
            bindingJson["action_type"] = ActionTypeToString(binding.action_type);
            actionJson["bindings"].push_back(bindingJson);
        }
        
        json["actions"].push_back(actionJson);
    }
    
    return json;
}

ActionMap ActionMap::CreateDefault() {
    ActionMap actionMap;
    
    // Movement actions
    auto& moveUp = actionMap.AddAction("move_up", "Move character up");
    moveUp.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_W, InputActionType::Held));
    moveUp.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_UP, InputActionType::Held));
    
    auto& moveDown = actionMap.AddAction("move_down", "Move character down");
    moveDown.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_S, InputActionType::Held));
    moveDown.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_DOWN, InputActionType::Held));
    
    auto& moveLeft = actionMap.AddAction("move_left", "Move character left");
    moveLeft.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_A, InputActionType::Held));
    moveLeft.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_LEFT, InputActionType::Held));
    
    auto& moveRight = actionMap.AddAction("move_right", "Move character right");
    moveRight.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_D, InputActionType::Held));
    moveRight.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_RIGHT, InputActionType::Held));

    // 3D Movement actions (forward/backward instead of up/down)
    auto& moveForward = actionMap.AddAction("move_forward", "Move character forward");
    moveForward.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_W, InputActionType::Held));
    moveForward.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_UP, InputActionType::Held));

    auto& moveBackward = actionMap.AddAction("move_backward", "Move character backward");
    moveBackward.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_S, InputActionType::Held));
    moveBackward.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_DOWN, InputActionType::Held));

    // Common game actions
    auto& jump = actionMap.AddAction("jump", "Jump or confirm action");
    jump.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_SPACE, InputActionType::Pressed));
    jump.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_ENTER, InputActionType::Pressed));
    
    auto& interact = actionMap.AddAction("interact", "Interact with objects");
    interact.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_E, InputActionType::Pressed));
    
    auto& cancel = actionMap.AddAction("cancel", "Cancel or go back");
    cancel.AddBinding(ActionBinding(InputDevice::Keyboard, InputConstants::KEY_ESCAPE, InputActionType::Pressed));
    
    // Mouse actions
    auto& primaryAction = actionMap.AddAction("primary_action", "Primary action (left click)");
    primaryAction.AddBinding(ActionBinding(InputDevice::Mouse, InputConstants::MOUSE_BUTTON_LEFT, InputActionType::Pressed));
    
    auto& secondaryAction = actionMap.AddAction("secondary_action", "Secondary action (right click)");
    secondaryAction.AddBinding(ActionBinding(InputDevice::Mouse, InputConstants::MOUSE_BUTTON_RIGHT, InputActionType::Pressed));
    
    return actionMap;
}

std::string ActionMap::DeviceToString(InputDevice device) {
    switch (device) {
        case InputDevice::Keyboard: return "keyboard";
        case InputDevice::Mouse: return "mouse";
        case InputDevice::Gamepad: return "gamepad";
        default: return "unknown";
    }
}

InputDevice ActionMap::StringToDevice(const std::string& str) {
    if (str == "keyboard") return InputDevice::Keyboard;
    if (str == "mouse") return InputDevice::Mouse;
    if (str == "gamepad") return InputDevice::Gamepad;
    return InputDevice::Keyboard; // Default
}

std::string ActionMap::ActionTypeToString(InputActionType type) {
    switch (type) {
        case InputActionType::Pressed: return "pressed";
        case InputActionType::Released: return "released";
        case InputActionType::Held: return "held";
        default: return "pressed";
    }
}

InputActionType ActionMap::StringToActionType(const std::string& str) {
    if (str == "pressed") return InputActionType::Pressed;
    if (str == "released") return InputActionType::Released;
    if (str == "held") return InputActionType::Held;
    return InputActionType::Pressed; // Default
}

} // namespace Lupine
