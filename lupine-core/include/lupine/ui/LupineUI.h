#pragma once

#include <memory>
#include <string>
#include <functional>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>

namespace LupineUI {

// Forward declarations
class Widget;
class Window;
class Panel;
class DockablePanel;
class MenuBar;
class Toolbar;
class Label;
class TextInput;
class Button;

/**
 * @brief UI Core system for managing the UI framework
 */
class UICore {
public:
    /**
     * @brief Initialize the UI system
     * @return True if successful
     */
    static bool Initialize();
    
    /**
     * @brief Shutdown the UI system
     */
    static void Shutdown();
    
    /**
     * @brief Update the UI system
     * @param deltaTime Time since last frame
     */
    static void Update(float deltaTime);
    
    /**
     * @brief Render the UI
     */
    static void Render();
    
    /**
     * @brief Handle SDL events
     * @param event SDL event
     * @return True if event was handled
     */
    static bool HandleEvent(const SDL_Event& event);
    
    /**
     * @brief Set the main window
     * @param window Main window instance
     */
    static void SetMainWindow(std::shared_ptr<Window> window);
    
    /**
     * @brief Get the main window
     * @return Main window instance
     */
    static std::shared_ptr<Window> GetMainWindow();
};

/**
 * @brief UI Theme structure
 */
struct Theme {
    glm::vec4 background_color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 text_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 accent_color = glm::vec4(0.5f, 0.3f, 0.8f, 1.0f);
    glm::vec4 button_color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    glm::vec4 button_hover_color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
    glm::vec4 button_pressed_color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
};

/**
 * @brief Initialize the UI system
 * @return True if successful
 */
bool Initialize();

/**
 * @brief Shutdown the UI system
 */
void Shutdown();

/**
 * @brief Set the current theme
 * @param theme Theme to set
 */
void SetTheme(const Theme& theme);

/**
 * @brief Create a dark purple theme
 * @return Dark purple theme
 */
Theme CreateDarkPurpleTheme();

/**
 * @brief Window class for creating UI windows
 */
class Window {
public:
    enum class Flags {
        None = 0,
        Resizable = 1,
        Fullscreen = 2,
        Borderless = 4
    };
    
    Window(const std::string& title, int width, int height, Flags flags = Flags::None);
    ~Window();
    
    bool Initialize();
    void Show();
    void Hide();
    void MakeCurrent();
    void SwapBuffers();
    
    void SetRootWidget(std::shared_ptr<Widget> widget);
    std::shared_ptr<Widget> GetRootWidget() const;
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
private:
    std::string m_title;
    int m_width, m_height;
    Flags m_flags;
    SDL_Window* m_sdl_window = nullptr;
    SDL_GLContext m_gl_context = nullptr;
    std::shared_ptr<Widget> m_root_widget;
};

} // namespace LupineUI
