#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>

namespace Lupine {

/**
 * @brief Input constants for easy access to key codes, mouse buttons, and gamepad buttons
 * 
 * This class provides a comprehensive set of input constants that can be used
 * in C++, Python, and Lua scripts for consistent input handling.
 */
class InputConstants {
public:
    // === Keyboard Keys ===
    
    // Letters
    static constexpr int KEY_A = SDLK_a;
    static constexpr int KEY_B = SDLK_b;
    static constexpr int KEY_C = SDLK_c;
    static constexpr int KEY_D = SDLK_d;
    static constexpr int KEY_E = SDLK_e;
    static constexpr int KEY_F = SDLK_f;
    static constexpr int KEY_G = SDLK_g;
    static constexpr int KEY_H = SDLK_h;
    static constexpr int KEY_I = SDLK_i;
    static constexpr int KEY_J = SDLK_j;
    static constexpr int KEY_K = SDLK_k;
    static constexpr int KEY_L = SDLK_l;
    static constexpr int KEY_M = SDLK_m;
    static constexpr int KEY_N = SDLK_n;
    static constexpr int KEY_O = SDLK_o;
    static constexpr int KEY_P = SDLK_p;
    static constexpr int KEY_Q = SDLK_q;
    static constexpr int KEY_R = SDLK_r;
    static constexpr int KEY_S = SDLK_s;
    static constexpr int KEY_T = SDLK_t;
    static constexpr int KEY_U = SDLK_u;
    static constexpr int KEY_V = SDLK_v;
    static constexpr int KEY_W = SDLK_w;
    static constexpr int KEY_X = SDLK_x;
    static constexpr int KEY_Y = SDLK_y;
    static constexpr int KEY_Z = SDLK_z;
    
    // Numbers
    static constexpr int KEY_0 = SDLK_0;
    static constexpr int KEY_1 = SDLK_1;
    static constexpr int KEY_2 = SDLK_2;
    static constexpr int KEY_3 = SDLK_3;
    static constexpr int KEY_4 = SDLK_4;
    static constexpr int KEY_5 = SDLK_5;
    static constexpr int KEY_6 = SDLK_6;
    static constexpr int KEY_7 = SDLK_7;
    static constexpr int KEY_8 = SDLK_8;
    static constexpr int KEY_9 = SDLK_9;
    
    // Function Keys
    static constexpr int KEY_F1 = SDLK_F1;
    static constexpr int KEY_F2 = SDLK_F2;
    static constexpr int KEY_F3 = SDLK_F3;
    static constexpr int KEY_F4 = SDLK_F4;
    static constexpr int KEY_F5 = SDLK_F5;
    static constexpr int KEY_F6 = SDLK_F6;
    static constexpr int KEY_F7 = SDLK_F7;
    static constexpr int KEY_F8 = SDLK_F8;
    static constexpr int KEY_F9 = SDLK_F9;
    static constexpr int KEY_F10 = SDLK_F10;
    static constexpr int KEY_F11 = SDLK_F11;
    static constexpr int KEY_F12 = SDLK_F12;
    
    // Special Keys
    static constexpr int KEY_SPACE = SDLK_SPACE;
    static constexpr int KEY_ENTER = SDLK_RETURN;
    static constexpr int KEY_ESCAPE = SDLK_ESCAPE;
    static constexpr int KEY_BACKSPACE = SDLK_BACKSPACE;
    static constexpr int KEY_TAB = SDLK_TAB;
    static constexpr int KEY_SHIFT_LEFT = SDLK_LSHIFT;
    static constexpr int KEY_SHIFT_RIGHT = SDLK_RSHIFT;
    static constexpr int KEY_CTRL_LEFT = SDLK_LCTRL;
    static constexpr int KEY_CTRL_RIGHT = SDLK_RCTRL;
    static constexpr int KEY_ALT_LEFT = SDLK_LALT;
    static constexpr int KEY_ALT_RIGHT = SDLK_RALT;
    static constexpr int KEY_DELETE = SDLK_DELETE;
    static constexpr int KEY_INSERT = SDLK_INSERT;
    static constexpr int KEY_HOME = SDLK_HOME;
    static constexpr int KEY_END = SDLK_END;
    static constexpr int KEY_PAGE_UP = SDLK_PAGEUP;
    static constexpr int KEY_PAGE_DOWN = SDLK_PAGEDOWN;
    
    // Arrow Keys
    static constexpr int KEY_UP = SDLK_UP;
    static constexpr int KEY_DOWN = SDLK_DOWN;
    static constexpr int KEY_LEFT = SDLK_LEFT;
    static constexpr int KEY_RIGHT = SDLK_RIGHT;
    
    // Numpad
    static constexpr int KEY_NUMPAD_0 = SDLK_KP_0;
    static constexpr int KEY_NUMPAD_1 = SDLK_KP_1;
    static constexpr int KEY_NUMPAD_2 = SDLK_KP_2;
    static constexpr int KEY_NUMPAD_3 = SDLK_KP_3;
    static constexpr int KEY_NUMPAD_4 = SDLK_KP_4;
    static constexpr int KEY_NUMPAD_5 = SDLK_KP_5;
    static constexpr int KEY_NUMPAD_6 = SDLK_KP_6;
    static constexpr int KEY_NUMPAD_7 = SDLK_KP_7;
    static constexpr int KEY_NUMPAD_8 = SDLK_KP_8;
    static constexpr int KEY_NUMPAD_9 = SDLK_KP_9;
    static constexpr int KEY_NUMPAD_PLUS = SDLK_KP_PLUS;
    static constexpr int KEY_NUMPAD_MINUS = SDLK_KP_MINUS;
    static constexpr int KEY_NUMPAD_MULTIPLY = SDLK_KP_MULTIPLY;
    static constexpr int KEY_NUMPAD_DIVIDE = SDLK_KP_DIVIDE;
    static constexpr int KEY_NUMPAD_ENTER = SDLK_KP_ENTER;
    static constexpr int KEY_NUMPAD_PERIOD = SDLK_KP_PERIOD;
    
    // === Mouse Buttons ===
    static constexpr int MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT;
    static constexpr int MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE;
    static constexpr int MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT;
    static constexpr int MOUSE_BUTTON_X1 = SDL_BUTTON_X1;
    static constexpr int MOUSE_BUTTON_X2 = SDL_BUTTON_X2;
    
    // === Gamepad Buttons ===
    static constexpr int GAMEPAD_BUTTON_A = SDL_CONTROLLER_BUTTON_A;
    static constexpr int GAMEPAD_BUTTON_B = SDL_CONTROLLER_BUTTON_B;
    static constexpr int GAMEPAD_BUTTON_X = SDL_CONTROLLER_BUTTON_X;
    static constexpr int GAMEPAD_BUTTON_Y = SDL_CONTROLLER_BUTTON_Y;
    static constexpr int GAMEPAD_BUTTON_BACK = SDL_CONTROLLER_BUTTON_BACK;
    static constexpr int GAMEPAD_BUTTON_GUIDE = SDL_CONTROLLER_BUTTON_GUIDE;
    static constexpr int GAMEPAD_BUTTON_START = SDL_CONTROLLER_BUTTON_START;
    static constexpr int GAMEPAD_BUTTON_LEFT_STICK = SDL_CONTROLLER_BUTTON_LEFTSTICK;
    static constexpr int GAMEPAD_BUTTON_RIGHT_STICK = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    static constexpr int GAMEPAD_BUTTON_LEFT_SHOULDER = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    static constexpr int GAMEPAD_BUTTON_RIGHT_SHOULDER = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    static constexpr int GAMEPAD_BUTTON_DPAD_UP = SDL_CONTROLLER_BUTTON_DPAD_UP;
    static constexpr int GAMEPAD_BUTTON_DPAD_DOWN = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    static constexpr int GAMEPAD_BUTTON_DPAD_LEFT = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    static constexpr int GAMEPAD_BUTTON_DPAD_RIGHT = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    
    // === Utility Methods ===
    
    /**
     * @brief Get human-readable name for a key code
     * @param keycode SDL key code
     * @return Human-readable key name
     */
    static std::string GetKeyName(int keycode);
    
    /**
     * @brief Get human-readable name for a mouse button
     * @param button Mouse button code
     * @return Human-readable button name
     */
    static std::string GetMouseButtonName(int button);
    
    /**
     * @brief Get human-readable name for a gamepad button
     * @param button Gamepad button code
     * @return Human-readable button name
     */
    static std::string GetGamepadButtonName(int button);
    
    /**
     * @brief Get key code from name
     * @param name Key name (case-insensitive)
     * @return Key code or -1 if not found
     */
    static int GetKeyFromName(const std::string& name);
    
    /**
     * @brief Get mouse button code from name
     * @param name Button name (case-insensitive)
     * @return Button code or -1 if not found
     */
    static int GetMouseButtonFromName(const std::string& name);
    
    /**
     * @brief Get gamepad button code from name
     * @param name Button name (case-insensitive)
     * @return Button code or -1 if not found
     */
    static int GetGamepadButtonFromName(const std::string& name);

private:
    static std::unordered_map<int, std::string> s_key_names;
    static std::unordered_map<int, std::string> s_mouse_button_names;
    static std::unordered_map<int, std::string> s_gamepad_button_names;
    static std::unordered_map<std::string, int> s_name_to_key;
    static std::unordered_map<std::string, int> s_name_to_mouse_button;
    static std::unordered_map<std::string, int> s_name_to_gamepad_button;
    
    static void InitializeMaps();
    static bool s_maps_initialized;
};

} // namespace Lupine
