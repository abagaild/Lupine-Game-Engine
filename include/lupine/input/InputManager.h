#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <vector>

namespace Lupine {

/**
 * @brief Input action types
 */
enum class InputActionType {
    Pressed,    // Action triggered when key/button is first pressed
    Released,   // Action triggered when key/button is released
    Held        // Action triggered while key/button is held down
};

/**
 * @brief Input device types
 */
enum class InputDevice {
    Keyboard,
    Mouse,
    Gamepad
};

/**
 * @brief Mouse button enumeration
 */
enum class MouseButton {
    Left = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right = SDL_BUTTON_RIGHT,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2
};

/**
 * @brief Gamepad button enumeration
 */
enum class GamepadButton {
    A = SDL_CONTROLLER_BUTTON_A,
    B = SDL_CONTROLLER_BUTTON_B,
    X = SDL_CONTROLLER_BUTTON_X,
    Y = SDL_CONTROLLER_BUTTON_Y,
    Back = SDL_CONTROLLER_BUTTON_BACK,
    Guide = SDL_CONTROLLER_BUTTON_GUIDE,
    Start = SDL_CONTROLLER_BUTTON_START,
    LeftStick = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    RightStick = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    LeftShoulder = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    RightShoulder = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    DPadUp = SDL_CONTROLLER_BUTTON_DPAD_UP,
    DPadDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    DPadLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    DPadRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

/**
 * @brief Gamepad axis enumeration
 */
enum class GamepadAxis {
    LeftX = SDL_CONTROLLER_AXIS_LEFTX,
    LeftY = SDL_CONTROLLER_AXIS_LEFTY,
    RightX = SDL_CONTROLLER_AXIS_RIGHTX,
    RightY = SDL_CONTROLLER_AXIS_RIGHTY,
    TriggerLeft = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    TriggerRight = SDL_CONTROLLER_AXIS_TRIGGERRIGHT
};

/**
 * @brief Input binding structure
 */
struct InputBinding {
    InputDevice device;
    int code; // SDL key code, mouse button, or gamepad button
    InputActionType action_type;

    InputBinding(InputDevice dev, int c, InputActionType type)
        : device(dev), code(c), action_type(type) {}
};

/**
 * @brief Input action callback function type
 */
using InputActionCallback = std::function<void()>;

/**
 * @brief Input axis callback function type (for analog inputs)
 */
using InputAxisCallback = std::function<void(float value)>;

/**
 * @brief Comprehensive input management system
 *
 * Handles keyboard, mouse, and gamepad input with action mapping,
 * input buffering, and customizable key bindings.
 */
class InputManager {
public:
    /**
     * @brief Initialize the input manager
     * @return True if successful
     */
    static bool Initialize();

    /**
     * @brief Shutdown the input manager
     */
    static void Shutdown();

    /**
     * @brief Update input state (call once per frame)
     * @param delta_time Time since last frame
     */
    static void Update(float delta_time);

    /**
     * @brief Process SDL event
     * @param event SDL event to process
     */
    static void ProcessEvent(const SDL_Event& event);

    // === Keyboard Input ===

    /**
     * @brief Check if a key is currently pressed
     * @param key SDL key code
     * @return True if key is pressed
     */
    static bool IsKeyPressed(SDL_Keycode key);

    /**
     * @brief Check if a key was just pressed this frame
     * @param key SDL key code
     * @return True if key was just pressed
     */
    static bool IsKeyJustPressed(SDL_Keycode key);

    /**
     * @brief Check if a key was just released this frame
     * @param key SDL key code
     * @return True if key was just released
     */
    static bool IsKeyJustReleased(SDL_Keycode key);

    // === Mouse Input ===

    /**
     * @brief Check if a mouse button is currently pressed
     * @param button Mouse button
     * @return True if button is pressed
     */
    static bool IsMouseButtonPressed(MouseButton button);

    /**
     * @brief Check if a mouse button was just pressed this frame
     * @param button Mouse button
     * @return True if button was just pressed
     */
    static bool IsMouseButtonJustPressed(MouseButton button);

    /**
     * @brief Check if a mouse button was just released this frame
     * @param button Mouse button
     * @return True if button was just released
     */
    static bool IsMouseButtonJustReleased(MouseButton button);

    /**
     * @brief Get current mouse position
     * @return Mouse position in screen coordinates
     */
    static glm::vec2 GetMousePosition();

    /**
     * @brief Get mouse position delta since last frame
     * @return Mouse movement delta
     */
    static glm::vec2 GetMouseDelta();

    /**
     * @brief Get mouse wheel delta
     * @return Mouse wheel movement (positive = up, negative = down)
     */
    static glm::vec2 GetMouseWheelDelta();

    /**
     * @brief Set mouse cursor visibility
     * @param visible True to show cursor, false to hide
     */
    static void SetMouseCursorVisible(bool visible);

    /**
     * @brief Set mouse relative mode (for FPS-style mouse look)
     * @param relative True to enable relative mode
     */
    static void SetMouseRelativeMode(bool relative);

    // === Gamepad Input ===

    /**
     * @brief Check if a gamepad is connected
     * @param gamepad_id Gamepad ID (0-3)
     * @return True if gamepad is connected
     */
    static bool IsGamepadConnected(int gamepad_id = 0);

    /**
     * @brief Check if a gamepad button is currently pressed
     * @param button Gamepad button
     * @param gamepad_id Gamepad ID (0-3)
     * @return True if button is pressed
     */
    static bool IsGamepadButtonPressed(GamepadButton button, int gamepad_id = 0);

    /**
     * @brief Check if a gamepad button was just pressed this frame
     * @param button Gamepad button
     * @param gamepad_id Gamepad ID (0-3)
     * @return True if button was just pressed
     */
    static bool IsGamepadButtonJustPressed(GamepadButton button, int gamepad_id = 0);

    /**
     * @brief Check if a gamepad button was just released this frame
     * @param button Gamepad button
     * @param gamepad_id Gamepad ID (0-3)
     * @return True if button was just released
     */
    static bool IsGamepadButtonJustReleased(GamepadButton button, int gamepad_id = 0);

    /**
     * @brief Get gamepad axis value
     * @param axis Gamepad axis
     * @param gamepad_id Gamepad ID (0-3)
     * @return Axis value (-1.0 to 1.0)
     */
    static float GetGamepadAxis(GamepadAxis axis, int gamepad_id = 0);

    /**
     * @brief Set gamepad vibration
     * @param low_frequency Low frequency motor strength (0.0 to 1.0)
     * @param high_frequency High frequency motor strength (0.0 to 1.0)
     * @param duration_ms Duration in milliseconds
     * @param gamepad_id Gamepad ID (0-3)
     */
    static void SetGamepadVibration(float low_frequency, float high_frequency, int duration_ms, int gamepad_id = 0);

    // === Action Mapping ===

    /**
     * @brief Register an input action
     * @param action_name Name of the action
     * @param binding Input binding for this action
     * @param callback Function to call when action is triggered
     */
    static void RegisterAction(const std::string& action_name, const InputBinding& binding, InputActionCallback callback);

    /**
     * @brief Add an additional binding to an existing action
     * @param action_name Name of the existing action
     * @param binding Additional input binding for this action
     */
    static void AddActionBinding(const std::string& action_name, const InputBinding& binding);

    /**
     * @brief Register an input axis (for analog inputs)
     * @param axis_name Name of the axis
     * @param positive_binding Binding for positive direction
     * @param negative_binding Binding for negative direction (optional)
     * @param callback Function to call with axis value
     */
    static void RegisterAxis(const std::string& axis_name, const InputBinding& positive_binding,
                           const InputBinding& negative_binding, InputAxisCallback callback);

    /**
     * @brief Register an input axis for gamepad analog stick/trigger
     * @param axis_name Name of the axis
     * @param gamepad_axis Gamepad axis
     * @param callback Function to call with axis value
     * @param gamepad_id Gamepad ID (0-3)
     */
    static void RegisterGamepadAxis(const std::string& axis_name, GamepadAxis gamepad_axis,
                                  InputAxisCallback callback, int gamepad_id = 0);

    /**
     * @brief Unregister an action
     * @param action_name Name of the action to remove
     */
    static void UnregisterAction(const std::string& action_name);

    /**
     * @brief Unregister an axis
     * @param axis_name Name of the axis to remove
     */
    static void UnregisterAxis(const std::string& axis_name);

    /**
     * @brief Clear all registered actions and axes
     */
    static void ClearAllBindings();

    /**
     * @brief Check if an action is currently active
     * @param action_name Name of the action
     * @return True if action is active
     */
    static bool IsActionActive(const std::string& action_name);

    /**
     * @brief Check if an action is currently pressed (held down)
     * @param action_name Name of the action
     * @return True if action is pressed
     */
    static bool IsActionPressed(const std::string& action_name);

    /**
     * @brief Check if an action was just pressed this frame
     * @param action_name Name of the action
     * @return True if action was just pressed
     */
    static bool IsActionJustPressed(const std::string& action_name);

    /**
     * @brief Check if an action was just released this frame
     * @param action_name Name of the action
     * @return True if action was just released
     */
    static bool IsActionJustReleased(const std::string& action_name);

    /**
     * @brief Get axis value by name
     * @param axis_name Name of the axis
     * @return Axis value (-1.0 to 1.0)
     */
    static float GetAxisValue(const std::string& axis_name);

    // === Input Settings ===

    /**
     * @brief Set mouse sensitivity
     * @param sensitivity Mouse sensitivity multiplier
     */
    static void SetMouseSensitivity(float sensitivity);

    /**
     * @brief Get mouse sensitivity
     * @return Current mouse sensitivity
     */
    static float GetMouseSensitivity();

    /**
     * @brief Set gamepad deadzone
     * @param deadzone Deadzone value (0.0 to 1.0)
     */
    static void SetGamepadDeadzone(float deadzone);

    /**
     * @brief Get gamepad deadzone
     * @return Current gamepad deadzone
     */
    static float GetGamepadDeadzone();

    /**
     * @brief Enable/disable input buffering
     * @param enabled True to enable input buffering
     */
    static void SetInputBuffering(bool enabled);

    /**
     * @brief Check if input buffering is enabled
     * @return True if input buffering is enabled
     */
    static bool IsInputBufferingEnabled();

private:
    struct GamepadState {
        SDL_GameController* controller;
        std::unordered_set<int> pressed_buttons;
        std::unordered_set<int> just_pressed_buttons;
        std::unordered_set<int> just_released_buttons;
        std::unordered_map<int, float> axis_values;
        bool connected;

        GamepadState() : controller(nullptr), connected(false) {}
    };

    struct InputAction {
        std::vector<InputBinding> bindings;
        InputActionCallback callback;
        bool is_active;
        bool was_active;
        bool just_triggered;
        bool just_released;

        InputAction() : is_active(false), was_active(false), just_triggered(false), just_released(false) {}
        InputAction(const InputBinding& b, InputActionCallback c)
            : callback(c), is_active(false), was_active(false),
              just_triggered(false), just_released(false) {
            bindings.push_back(b);
        }

        void AddBinding(const InputBinding& binding) {
            bindings.push_back(binding);
        }
    };

    struct InputAxis {
        InputBinding positive_binding;
        InputBinding negative_binding;
        InputAxisCallback callback;
        float current_value;
        bool has_negative_binding;

        InputAxis() : positive_binding(InputDevice::Keyboard, 0, InputActionType::Held),
                     negative_binding(InputDevice::Keyboard, 0, InputActionType::Held),
                     current_value(0.0f), has_negative_binding(false) {}

        InputAxis(const InputBinding& pos, const InputBinding& neg, InputAxisCallback c)
            : positive_binding(pos), negative_binding(neg), callback(c),
              current_value(0.0f), has_negative_binding(true) {}

        InputAxis(const InputBinding& pos, InputAxisCallback c)
            : positive_binding(pos), negative_binding(InputDevice::Keyboard, 0, InputActionType::Held),
              callback(c), current_value(0.0f), has_negative_binding(false) {}
    };

    struct GamepadAxisBinding {
        GamepadAxis axis;
        InputAxisCallback callback;
        int gamepad_id;
        float current_value;

        GamepadAxisBinding() : axis(GamepadAxis::LeftX), gamepad_id(0), current_value(0.0f) {}
        GamepadAxisBinding(GamepadAxis a, InputAxisCallback c, int id)
            : axis(a), callback(c), gamepad_id(id), current_value(0.0f) {}
    };

    static bool s_initialized;
    static std::unordered_set<SDL_Keycode> s_pressed_keys;
    static std::unordered_set<SDL_Keycode> s_just_pressed_keys;
    static std::unordered_set<SDL_Keycode> s_just_released_keys;

    static std::unordered_set<int> s_pressed_mouse_buttons;
    static std::unordered_set<int> s_just_pressed_mouse_buttons;
    static std::unordered_set<int> s_just_released_mouse_buttons;
    static glm::vec2 s_mouse_position;
    static glm::vec2 s_mouse_delta;
    static glm::vec2 s_mouse_wheel_delta;

    static GamepadState s_gamepads[4]; // Support up to 4 gamepads

    static std::unordered_map<std::string, InputAction> s_actions;
    static std::unordered_map<std::string, InputAxis> s_axes;
    static std::unordered_map<std::string, GamepadAxisBinding> s_gamepad_axes;

    static float s_mouse_sensitivity;
    static float s_gamepad_deadzone;
    static bool s_input_buffering_enabled;

    // Helper methods
    static void UpdateKeyboardState();
    static void UpdateMouseState();
    static void UpdateGamepadState();
    static void ProcessActions();
    static void ProcessAxes();
    static void ClearFrameInputs();
    static bool IsBindingActive(const InputBinding& binding);
    static float ApplyDeadzone(float value, float deadzone);
};

} // namespace Lupine
