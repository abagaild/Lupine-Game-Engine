#include "lupine/input/InputConstants.h"
#include <algorithm>
#include <cctype>

namespace Lupine {

// Static member definitions
std::unordered_map<int, std::string> InputConstants::s_key_names;
std::unordered_map<int, std::string> InputConstants::s_mouse_button_names;
std::unordered_map<int, std::string> InputConstants::s_gamepad_button_names;
std::unordered_map<std::string, int> InputConstants::s_name_to_key;
std::unordered_map<std::string, int> InputConstants::s_name_to_mouse_button;
std::unordered_map<std::string, int> InputConstants::s_name_to_gamepad_button;
bool InputConstants::s_maps_initialized = false;

void InputConstants::InitializeMaps() {
    if (s_maps_initialized) return;
    
    // Initialize key names
    s_key_names[KEY_A] = "A";
    s_key_names[KEY_B] = "B";
    s_key_names[KEY_C] = "C";
    s_key_names[KEY_D] = "D";
    s_key_names[KEY_E] = "E";
    s_key_names[KEY_F] = "F";
    s_key_names[KEY_G] = "G";
    s_key_names[KEY_H] = "H";
    s_key_names[KEY_I] = "I";
    s_key_names[KEY_J] = "J";
    s_key_names[KEY_K] = "K";
    s_key_names[KEY_L] = "L";
    s_key_names[KEY_M] = "M";
    s_key_names[KEY_N] = "N";
    s_key_names[KEY_O] = "O";
    s_key_names[KEY_P] = "P";
    s_key_names[KEY_Q] = "Q";
    s_key_names[KEY_R] = "R";
    s_key_names[KEY_S] = "S";
    s_key_names[KEY_T] = "T";
    s_key_names[KEY_U] = "U";
    s_key_names[KEY_V] = "V";
    s_key_names[KEY_W] = "W";
    s_key_names[KEY_X] = "X";
    s_key_names[KEY_Y] = "Y";
    s_key_names[KEY_Z] = "Z";
    
    s_key_names[KEY_0] = "0";
    s_key_names[KEY_1] = "1";
    s_key_names[KEY_2] = "2";
    s_key_names[KEY_3] = "3";
    s_key_names[KEY_4] = "4";
    s_key_names[KEY_5] = "5";
    s_key_names[KEY_6] = "6";
    s_key_names[KEY_7] = "7";
    s_key_names[KEY_8] = "8";
    s_key_names[KEY_9] = "9";
    
    s_key_names[KEY_F1] = "F1";
    s_key_names[KEY_F2] = "F2";
    s_key_names[KEY_F3] = "F3";
    s_key_names[KEY_F4] = "F4";
    s_key_names[KEY_F5] = "F5";
    s_key_names[KEY_F6] = "F6";
    s_key_names[KEY_F7] = "F7";
    s_key_names[KEY_F8] = "F8";
    s_key_names[KEY_F9] = "F9";
    s_key_names[KEY_F10] = "F10";
    s_key_names[KEY_F11] = "F11";
    s_key_names[KEY_F12] = "F12";
    
    s_key_names[KEY_SPACE] = "Space";
    s_key_names[KEY_ENTER] = "Enter";
    s_key_names[KEY_ESCAPE] = "Escape";
    s_key_names[KEY_BACKSPACE] = "Backspace";
    s_key_names[KEY_TAB] = "Tab";
    s_key_names[KEY_SHIFT_LEFT] = "Left Shift";
    s_key_names[KEY_SHIFT_RIGHT] = "Right Shift";
    s_key_names[KEY_CTRL_LEFT] = "Left Ctrl";
    s_key_names[KEY_CTRL_RIGHT] = "Right Ctrl";
    s_key_names[KEY_ALT_LEFT] = "Left Alt";
    s_key_names[KEY_ALT_RIGHT] = "Right Alt";
    s_key_names[KEY_DELETE] = "Delete";
    s_key_names[KEY_INSERT] = "Insert";
    s_key_names[KEY_HOME] = "Home";
    s_key_names[KEY_END] = "End";
    s_key_names[KEY_PAGE_UP] = "Page Up";
    s_key_names[KEY_PAGE_DOWN] = "Page Down";
    
    s_key_names[KEY_UP] = "Up";
    s_key_names[KEY_DOWN] = "Down";
    s_key_names[KEY_LEFT] = "Left";
    s_key_names[KEY_RIGHT] = "Right";
    
    s_key_names[KEY_NUMPAD_0] = "Numpad 0";
    s_key_names[KEY_NUMPAD_1] = "Numpad 1";
    s_key_names[KEY_NUMPAD_2] = "Numpad 2";
    s_key_names[KEY_NUMPAD_3] = "Numpad 3";
    s_key_names[KEY_NUMPAD_4] = "Numpad 4";
    s_key_names[KEY_NUMPAD_5] = "Numpad 5";
    s_key_names[KEY_NUMPAD_6] = "Numpad 6";
    s_key_names[KEY_NUMPAD_7] = "Numpad 7";
    s_key_names[KEY_NUMPAD_8] = "Numpad 8";
    s_key_names[KEY_NUMPAD_9] = "Numpad 9";
    s_key_names[KEY_NUMPAD_PLUS] = "Numpad +";
    s_key_names[KEY_NUMPAD_MINUS] = "Numpad -";
    s_key_names[KEY_NUMPAD_MULTIPLY] = "Numpad *";
    s_key_names[KEY_NUMPAD_DIVIDE] = "Numpad /";
    s_key_names[KEY_NUMPAD_ENTER] = "Numpad Enter";
    s_key_names[KEY_NUMPAD_PERIOD] = "Numpad .";
    
    // Initialize mouse button names
    s_mouse_button_names[MOUSE_BUTTON_LEFT] = "Left Mouse";
    s_mouse_button_names[MOUSE_BUTTON_MIDDLE] = "Middle Mouse";
    s_mouse_button_names[MOUSE_BUTTON_RIGHT] = "Right Mouse";
    s_mouse_button_names[MOUSE_BUTTON_X1] = "Mouse X1";
    s_mouse_button_names[MOUSE_BUTTON_X2] = "Mouse X2";
    
    // Initialize gamepad button names
    s_gamepad_button_names[GAMEPAD_BUTTON_A] = "Gamepad A";
    s_gamepad_button_names[GAMEPAD_BUTTON_B] = "Gamepad B";
    s_gamepad_button_names[GAMEPAD_BUTTON_X] = "Gamepad X";
    s_gamepad_button_names[GAMEPAD_BUTTON_Y] = "Gamepad Y";
    s_gamepad_button_names[GAMEPAD_BUTTON_BACK] = "Gamepad Back";
    s_gamepad_button_names[GAMEPAD_BUTTON_GUIDE] = "Gamepad Guide";
    s_gamepad_button_names[GAMEPAD_BUTTON_START] = "Gamepad Start";
    s_gamepad_button_names[GAMEPAD_BUTTON_LEFT_STICK] = "Gamepad Left Stick";
    s_gamepad_button_names[GAMEPAD_BUTTON_RIGHT_STICK] = "Gamepad Right Stick";
    s_gamepad_button_names[GAMEPAD_BUTTON_LEFT_SHOULDER] = "Gamepad Left Shoulder";
    s_gamepad_button_names[GAMEPAD_BUTTON_RIGHT_SHOULDER] = "Gamepad Right Shoulder";
    s_gamepad_button_names[GAMEPAD_BUTTON_DPAD_UP] = "Gamepad D-Pad Up";
    s_gamepad_button_names[GAMEPAD_BUTTON_DPAD_DOWN] = "Gamepad D-Pad Down";
    s_gamepad_button_names[GAMEPAD_BUTTON_DPAD_LEFT] = "Gamepad D-Pad Left";
    s_gamepad_button_names[GAMEPAD_BUTTON_DPAD_RIGHT] = "Gamepad D-Pad Right";
    
    // Create reverse mappings (name to code)
    for (const auto& pair : s_key_names) {
        std::string lower_name = pair.second;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        s_name_to_key[lower_name] = pair.first;
    }

    for (const auto& pair : s_mouse_button_names) {
        std::string lower_name = pair.second;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        s_name_to_mouse_button[lower_name] = pair.first;
    }

    for (const auto& pair : s_gamepad_button_names) {
        std::string lower_name = pair.second;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        s_name_to_gamepad_button[lower_name] = pair.first;
    }
    
    s_maps_initialized = true;
}

std::string InputConstants::GetKeyName(int keycode) {
    InitializeMaps();
    auto it = s_key_names.find(keycode);
    if (it != s_key_names.end()) {
        return it->second;
    }
    return "Unknown Key";
}

std::string InputConstants::GetMouseButtonName(int button) {
    InitializeMaps();
    auto it = s_mouse_button_names.find(button);
    if (it != s_mouse_button_names.end()) {
        return it->second;
    }
    return "Unknown Mouse Button";
}

std::string InputConstants::GetGamepadButtonName(int button) {
    InitializeMaps();
    auto it = s_gamepad_button_names.find(button);
    if (it != s_gamepad_button_names.end()) {
        return it->second;
    }
    return "Unknown Gamepad Button";
}

int InputConstants::GetKeyFromName(const std::string& name) {
    InitializeMaps();
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = s_name_to_key.find(lower_name);
    if (it != s_name_to_key.end()) {
        return it->second;
    }
    return -1;
}

int InputConstants::GetMouseButtonFromName(const std::string& name) {
    InitializeMaps();
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = s_name_to_mouse_button.find(lower_name);
    if (it != s_name_to_mouse_button.end()) {
        return it->second;
    }
    return -1;
}

int InputConstants::GetGamepadButtonFromName(const std::string& name) {
    InitializeMaps();
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = s_name_to_gamepad_button.find(lower_name);
    if (it != s_name_to_gamepad_button.end()) {
        return it->second;
    }
    return -1;
}

} // namespace Lupine
