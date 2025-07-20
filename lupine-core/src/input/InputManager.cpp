#include "lupine/input/InputManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Lupine {

// Static member definitions
bool InputManager::s_initialized = false;
std::unordered_set<SDL_Keycode> InputManager::s_pressed_keys;
std::unordered_set<SDL_Keycode> InputManager::s_just_pressed_keys;
std::unordered_set<SDL_Keycode> InputManager::s_just_released_keys;

std::unordered_set<int> InputManager::s_pressed_mouse_buttons;
std::unordered_set<int> InputManager::s_just_pressed_mouse_buttons;
std::unordered_set<int> InputManager::s_just_released_mouse_buttons;
glm::vec2 InputManager::s_mouse_position(0.0f);
glm::vec2 InputManager::s_mouse_delta(0.0f);
glm::vec2 InputManager::s_mouse_wheel_delta(0.0f);

InputManager::GamepadState InputManager::s_gamepads[4];

std::unordered_map<std::string, InputManager::InputAction> InputManager::s_actions;
std::unordered_map<std::string, InputManager::InputAxis> InputManager::s_axes;
std::unordered_map<std::string, InputManager::GamepadAxisBinding> InputManager::s_gamepad_axes;

float InputManager::s_mouse_sensitivity = 1.0f;
float InputManager::s_gamepad_deadzone = 0.15f;
bool InputManager::s_input_buffering_enabled = true;

bool InputManager::Initialize() {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing Input Manager..." << std::endl;

    // Initialize SDL game controller subsystem
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL game controller subsystem: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize gamepads
    for (int i = 0; i < 4; ++i) {
        if (SDL_IsGameController(i)) {
            s_gamepads[i].controller = SDL_GameControllerOpen(i);
            if (s_gamepads[i].controller) {
                s_gamepads[i].connected = true;
                std::cout << "Gamepad " << i << " connected: " << SDL_GameControllerName(s_gamepads[i].controller) << std::endl;
            }
        }
    }

    s_initialized = true;
    std::cout << "Input Manager initialized successfully!" << std::endl;
    return true;
}

void InputManager::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down Input Manager..." << std::endl;

    // Close all game controllers
    for (int i = 0; i < 4; ++i) {
        if (s_gamepads[i].controller) {
            SDL_GameControllerClose(s_gamepads[i].controller);
            s_gamepads[i].controller = nullptr;
            s_gamepads[i].connected = false;
        }
    }

    // Clear all input state
    s_pressed_keys.clear();
    s_just_pressed_keys.clear();
    s_just_released_keys.clear();
    s_pressed_mouse_buttons.clear();
    s_just_pressed_mouse_buttons.clear();
    s_just_released_mouse_buttons.clear();

    // Clear all bindings
    ClearAllBindings();

    s_initialized = false;
    std::cout << "Input Manager shutdown complete." << std::endl;
}

void InputManager::Update(float delta_time) {
    if (!s_initialized) {
        return;
    }

    (void)delta_time; // Unused for now, but may be needed for input buffering

    // Clear frame-specific input states
    ClearFrameInputs();

    // Update input states
    UpdateKeyboardState();
    UpdateMouseState();
    UpdateGamepadState();

    // Process actions and axes
    ProcessActions();
    ProcessAxes();
}

void InputManager::ProcessEvent(const SDL_Event& event) {
    if (!s_initialized) {
        return;
    }

    switch (event.type) {
        case SDL_KEYDOWN:
            if (!event.key.repeat) { // Ignore key repeat events
                s_just_pressed_keys.insert(event.key.keysym.sym);
                s_pressed_keys.insert(event.key.keysym.sym);
            }
            break;

        case SDL_KEYUP:
            s_just_released_keys.insert(event.key.keysym.sym);
            s_pressed_keys.erase(event.key.keysym.sym);
            break;

        case SDL_MOUSEBUTTONDOWN:
            s_just_pressed_mouse_buttons.insert(event.button.button);
            s_pressed_mouse_buttons.insert(event.button.button);
            break;

        case SDL_MOUSEBUTTONUP:
            s_just_released_mouse_buttons.insert(event.button.button);
            s_pressed_mouse_buttons.erase(event.button.button);
            break;

        case SDL_MOUSEMOTION:
            s_mouse_position = glm::vec2(event.motion.x, event.motion.y);
            s_mouse_delta = glm::vec2(event.motion.xrel, event.motion.yrel) * s_mouse_sensitivity;
            break;

        case SDL_MOUSEWHEEL:
            s_mouse_wheel_delta = glm::vec2(event.wheel.x, event.wheel.y);
            break;

        case SDL_CONTROLLERDEVICEADDED:
            {
                int device_index = event.cdevice.which;
                if (device_index >= 0 && device_index < 4) {
                    s_gamepads[device_index].controller = SDL_GameControllerOpen(device_index);
                    if (s_gamepads[device_index].controller) {
                        s_gamepads[device_index].connected = true;
                        std::cout << "Gamepad " << device_index << " connected: "
                                  << SDL_GameControllerName(s_gamepads[device_index].controller) << std::endl;
                    }
                }
            }
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            {
                SDL_JoystickID instance_id = event.cdevice.which;
                for (int i = 0; i < 4; ++i) {
                    if (s_gamepads[i].controller &&
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_gamepads[i].controller)) == instance_id) {
                        SDL_GameControllerClose(s_gamepads[i].controller);
                        s_gamepads[i].controller = nullptr;
                        s_gamepads[i].connected = false;
                        s_gamepads[i].pressed_buttons.clear();
                        s_gamepads[i].axis_values.clear();
                        std::cout << "Gamepad " << i << " disconnected" << std::endl;
                        break;
                    }
                }
            }
            break;

        case SDL_CONTROLLERBUTTONDOWN:
            {
                SDL_JoystickID instance_id = event.cbutton.which;
                for (int i = 0; i < 4; ++i) {
                    if (s_gamepads[i].controller &&
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_gamepads[i].controller)) == instance_id) {
                        s_gamepads[i].just_pressed_buttons.insert(event.cbutton.button);
                        s_gamepads[i].pressed_buttons.insert(event.cbutton.button);
                        break;
                    }
                }
            }
            break;

        case SDL_CONTROLLERBUTTONUP:
            {
                SDL_JoystickID instance_id = event.cbutton.which;
                for (int i = 0; i < 4; ++i) {
                    if (s_gamepads[i].controller &&
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_gamepads[i].controller)) == instance_id) {
                        s_gamepads[i].just_released_buttons.insert(event.cbutton.button);
                        s_gamepads[i].pressed_buttons.erase(event.cbutton.button);
                        break;
                    }
                }
            }
            break;

        case SDL_CONTROLLERAXISMOTION:
            {
                SDL_JoystickID instance_id = event.caxis.which;
                for (int i = 0; i < 4; ++i) {
                    if (s_gamepads[i].controller &&
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_gamepads[i].controller)) == instance_id) {
                        float value = event.caxis.value / 32767.0f; // Normalize to -1.0 to 1.0
                        value = ApplyDeadzone(value, s_gamepad_deadzone);
                        s_gamepads[i].axis_values[event.caxis.axis] = value;
                        break;
                    }
                }
            }
            break;
    }
}

// === Keyboard Input ===

bool InputManager::IsKeyPressed(SDL_Keycode key) {
    return s_pressed_keys.find(key) != s_pressed_keys.end();
}

bool InputManager::IsKeyJustPressed(SDL_Keycode key) {
    return s_just_pressed_keys.find(key) != s_just_pressed_keys.end();
}

bool InputManager::IsKeyJustReleased(SDL_Keycode key) {
    return s_just_released_keys.find(key) != s_just_released_keys.end();
}

// === Mouse Input ===

bool InputManager::IsMouseButtonPressed(MouseButton button) {
    return s_pressed_mouse_buttons.find(static_cast<int>(button)) != s_pressed_mouse_buttons.end();
}

bool InputManager::IsMouseButtonJustPressed(MouseButton button) {
    return s_just_pressed_mouse_buttons.find(static_cast<int>(button)) != s_just_pressed_mouse_buttons.end();
}

bool InputManager::IsMouseButtonJustReleased(MouseButton button) {
    return s_just_released_mouse_buttons.find(static_cast<int>(button)) != s_just_released_mouse_buttons.end();
}

glm::vec2 InputManager::GetMousePosition() {
    return s_mouse_position;
}

glm::vec2 InputManager::GetMouseDelta() {
    return s_mouse_delta;
}

glm::vec2 InputManager::GetMouseWheelDelta() {
    return s_mouse_wheel_delta;
}

void InputManager::SetMouseCursorVisible(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void InputManager::SetMouseRelativeMode(bool relative) {
    SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE);
}

// === Gamepad Input ===

bool InputManager::IsGamepadConnected(int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4) {
        return false;
    }
    return s_gamepads[gamepad_id].connected;
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button, int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4 || !s_gamepads[gamepad_id].connected) {
        return false;
    }
    return s_gamepads[gamepad_id].pressed_buttons.find(static_cast<int>(button)) !=
           s_gamepads[gamepad_id].pressed_buttons.end();
}

bool InputManager::IsGamepadButtonJustPressed(GamepadButton button, int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4 || !s_gamepads[gamepad_id].connected) {
        return false;
    }
    return s_gamepads[gamepad_id].just_pressed_buttons.find(static_cast<int>(button)) !=
           s_gamepads[gamepad_id].just_pressed_buttons.end();
}

bool InputManager::IsGamepadButtonJustReleased(GamepadButton button, int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4 || !s_gamepads[gamepad_id].connected) {
        return false;
    }
    return s_gamepads[gamepad_id].just_released_buttons.find(static_cast<int>(button)) !=
           s_gamepads[gamepad_id].just_released_buttons.end();
}

float InputManager::GetGamepadAxis(GamepadAxis axis, int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4 || !s_gamepads[gamepad_id].connected) {
        return 0.0f;
    }

    auto it = s_gamepads[gamepad_id].axis_values.find(static_cast<int>(axis));
    if (it != s_gamepads[gamepad_id].axis_values.end()) {
        return it->second;
    }
    return 0.0f;
}

void InputManager::SetGamepadVibration(float low_frequency, float high_frequency, int duration_ms, int gamepad_id) {
    if (gamepad_id < 0 || gamepad_id >= 4 || !s_gamepads[gamepad_id].connected) {
        return;
    }

    // Clamp values to valid range
    low_frequency = std::max(0.0f, std::min(1.0f, low_frequency));
    high_frequency = std::max(0.0f, std::min(1.0f, high_frequency));

    // Convert to SDL values (0-65535)
    Uint16 low_freq_value = static_cast<Uint16>(low_frequency * 65535);
    Uint16 high_freq_value = static_cast<Uint16>(high_frequency * 65535);

    SDL_GameControllerRumble(s_gamepads[gamepad_id].controller, low_freq_value, high_freq_value, duration_ms);
}

// === Action Mapping ===

void InputManager::RegisterAction(const std::string& action_name, const InputBinding& binding, InputActionCallback callback) {
    s_actions[action_name] = InputAction(binding, callback);
}

void InputManager::AddActionBinding(const std::string& action_name, const InputBinding& binding) {
    auto it = s_actions.find(action_name);
    if (it != s_actions.end()) {
        it->second.AddBinding(binding);
    }
}

void InputManager::RegisterAxis(const std::string& axis_name, const InputBinding& positive_binding,
                               const InputBinding& negative_binding, InputAxisCallback callback) {
    s_axes[axis_name] = InputAxis(positive_binding, negative_binding, callback);
}

void InputManager::RegisterGamepadAxis(const std::string& axis_name, GamepadAxis gamepad_axis,
                                     InputAxisCallback callback, int gamepad_id) {
    s_gamepad_axes[axis_name] = GamepadAxisBinding(gamepad_axis, callback, gamepad_id);
}

void InputManager::UnregisterAction(const std::string& action_name) {
    s_actions.erase(action_name);
}

void InputManager::UnregisterAxis(const std::string& axis_name) {
    s_axes.erase(axis_name);
    s_gamepad_axes.erase(axis_name);
}

void InputManager::ClearAllBindings() {
    s_actions.clear();
    s_axes.clear();
    s_gamepad_axes.clear();
}

bool InputManager::IsActionActive(const std::string& action_name) {
    auto it = s_actions.find(action_name);
    if (it != s_actions.end()) {
        return it->second.is_active;
    }
    return false;
}

bool InputManager::IsActionPressed(const std::string& action_name) {
    auto it = s_actions.find(action_name);
    if (it != s_actions.end()) {
        const InputAction& action = it->second;
        // Check if any binding is active
        for (const auto& binding : action.bindings) {
            if (binding.action_type == InputActionType::Held) {
                if (action.is_active) {
                    return true;
                }
            }
            else if (binding.action_type == InputActionType::Pressed) {
                if (IsBindingActive(binding)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool InputManager::IsActionJustPressed(const std::string& action_name) {
    auto it = s_actions.find(action_name);
    if (it != s_actions.end()) {
        const InputAction& action = it->second;
        return action.is_active && action.just_triggered;
    }
    return false;
}

bool InputManager::IsActionJustReleased(const std::string& action_name) {
    auto it = s_actions.find(action_name);
    if (it != s_actions.end()) {
        const InputAction& action = it->second;
        return !action.is_active && action.just_released;
    }
    return false;
}

float InputManager::GetAxisValue(const std::string& axis_name) {
    auto it = s_axes.find(axis_name);
    if (it != s_axes.end()) {
        return it->second.current_value;
    }

    auto gamepad_it = s_gamepad_axes.find(axis_name);
    if (gamepad_it != s_gamepad_axes.end()) {
        return gamepad_it->second.current_value;
    }

    return 0.0f;
}

// === Input Settings ===

void InputManager::SetMouseSensitivity(float sensitivity) {
    s_mouse_sensitivity = std::max(0.1f, sensitivity);
}

float InputManager::GetMouseSensitivity() {
    return s_mouse_sensitivity;
}

void InputManager::SetGamepadDeadzone(float deadzone) {
    s_gamepad_deadzone = std::max(0.0f, std::min(1.0f, deadzone));
}

float InputManager::GetGamepadDeadzone() {
    return s_gamepad_deadzone;
}

void InputManager::SetInputBuffering(bool enabled) {
    s_input_buffering_enabled = enabled;
}

bool InputManager::IsInputBufferingEnabled() {
    return s_input_buffering_enabled;
}

// === Helper Methods ===

void InputManager::UpdateKeyboardState() {
    // Keyboard state is updated in ProcessEvent
}

void InputManager::UpdateMouseState() {
    // Mouse state is updated in ProcessEvent
}

void InputManager::UpdateGamepadState() {
    // Gamepad state is updated in ProcessEvent
}

void InputManager::ProcessActions() {
    for (auto& [action_name, action] : s_actions) {
        bool was_active = action.is_active;
        bool is_active = false;

        // Check if any binding is active
        for (const auto& binding : action.bindings) {
            if (IsBindingActive(binding)) {
                is_active = true;
                break;
            }
        }

        // Update action state
        action.was_active = was_active;
        action.is_active = is_active;
        action.just_triggered = false;
        action.just_released = false;

        // Check if we should trigger the action based on its type
        // Use the first binding's action type as the reference
        bool should_trigger = false;
        if (!action.bindings.empty()) {
            switch (action.bindings[0].action_type) {
                case InputActionType::Pressed:
                    should_trigger = is_active && !was_active;
                    action.just_triggered = should_trigger;
                    break;
                case InputActionType::Released:
                    should_trigger = !is_active && was_active;
                    action.just_triggered = should_trigger;
                    action.just_released = should_trigger;
                    break;
                case InputActionType::Held:
                    should_trigger = is_active;
                    action.just_triggered = is_active && !was_active;
                    action.just_released = !is_active && was_active;
                    break;
            }
        }

        if (should_trigger && action.callback) {
            action.callback();
        }
    }
}

void InputManager::ProcessAxes() {
    // Process regular axes (keyboard/mouse button based)
    for (auto& [axis_name, axis] : s_axes) {
        float value = 0.0f;

        if (IsBindingActive(axis.positive_binding)) {
            value += 1.0f;
        }

        if (axis.has_negative_binding && IsBindingActive(axis.negative_binding)) {
            value -= 1.0f;
        }

        axis.current_value = value;

        if (axis.callback) {
            axis.callback(value);
        }
    }

    // Process gamepad axes
    for (auto& [axis_name, gamepad_axis] : s_gamepad_axes) {
        float value = GetGamepadAxis(gamepad_axis.axis, gamepad_axis.gamepad_id);
        gamepad_axis.current_value = value;

        if (gamepad_axis.callback) {
            gamepad_axis.callback(value);
        }
    }
}

void InputManager::ClearFrameInputs() {
    s_just_pressed_keys.clear();
    s_just_released_keys.clear();
    s_just_pressed_mouse_buttons.clear();
    s_just_released_mouse_buttons.clear();
    s_mouse_wheel_delta = glm::vec2(0.0f);

    // Clear gamepad frame inputs
    for (int i = 0; i < 4; ++i) {
        s_gamepads[i].just_pressed_buttons.clear();
        s_gamepads[i].just_released_buttons.clear();
    }
}

bool InputManager::IsBindingActive(const InputBinding& binding) {
    switch (binding.device) {
        case InputDevice::Keyboard:
            switch (binding.action_type) {
                case InputActionType::Pressed:
                    return IsKeyJustPressed(static_cast<SDL_Keycode>(binding.code));
                case InputActionType::Released:
                    return IsKeyJustReleased(static_cast<SDL_Keycode>(binding.code));
                case InputActionType::Held:
                    return IsKeyPressed(static_cast<SDL_Keycode>(binding.code));
            }
            break;

        case InputDevice::Mouse:
            switch (binding.action_type) {
                case InputActionType::Pressed:
                    return IsMouseButtonJustPressed(static_cast<MouseButton>(binding.code));
                case InputActionType::Released:
                    return IsMouseButtonJustReleased(static_cast<MouseButton>(binding.code));
                case InputActionType::Held:
                    return IsMouseButtonPressed(static_cast<MouseButton>(binding.code));
            }
            break;

        case InputDevice::Gamepad:
            // For gamepad, we assume gamepad 0 for simplicity in bindings
            switch (binding.action_type) {
                case InputActionType::Pressed:
                    return IsGamepadButtonJustPressed(static_cast<GamepadButton>(binding.code), 0);
                case InputActionType::Released:
                    return IsGamepadButtonJustReleased(static_cast<GamepadButton>(binding.code), 0);
                case InputActionType::Held:
                    return IsGamepadButtonPressed(static_cast<GamepadButton>(binding.code), 0);
            }
            break;
    }

    return false;
}

float InputManager::ApplyDeadzone(float value, float deadzone) {
    if (std::abs(value) < deadzone) {
        return 0.0f;
    }

    // Scale the value to maintain smooth transition
    float sign = (value > 0.0f) ? 1.0f : -1.0f;
    float abs_value = std::abs(value);
    float scaled_value = (abs_value - deadzone) / (1.0f - deadzone);

    return sign * std::max(0.0f, std::min(1.0f, scaled_value));
}

} // namespace Lupine
