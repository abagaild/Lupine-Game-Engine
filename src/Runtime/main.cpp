#include "lupine/Engine.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#ifdef LUPINE_HAS_SDL3
#include <SDL3/SDL.h>
#endif

class RuntimeApplication {
public:
    RuntimeApplication() : m_running(false), m_window(nullptr) {}
    
    ~RuntimeApplication() {
        Shutdown();
    }
    
    bool Initialize() {
        std::cout << "Initializing Lupine Runtime..." << std::endl;
        
        // Initialize the engine
        if (!m_engine.Initialize()) {
            std::cerr << "Failed to initialize engine" << std::endl;
            return false;
        }
        
#ifdef LUPINE_HAS_SDL3
        // Initialize SDL3
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to initialize SDL3: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create window
        m_window = SDL_CreateWindow(
            "Lupine Runtime - Hello World",
            800, 600,
            SDL_WINDOW_RESIZABLE
        );
        
        if (!m_window) {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }
        
        std::cout << "SDL3 window created successfully" << std::endl;
#else
        std::cout << "SDL3 not available, running in console mode" << std::endl;
#endif
        
        m_engine.Log("Runtime application initialized successfully");
        return true;
    }
    
    void Run() {
        m_running = true;
        m_engine.Log("Starting runtime main loop");
        
#ifdef LUPINE_HAS_SDL3
        if (m_window) {
            RunWithWindow();
        } else {
            RunConsoleMode();
        }
#else
        RunConsoleMode();
#endif
    }
    
    void Shutdown() {
        if (m_running) {
            m_running = false;
            m_engine.Log("Shutting down runtime application");
        }
        
#ifdef LUPINE_HAS_SDL3
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        SDL_Quit();
#endif
        
        m_engine.Shutdown();
    }
    
private:
    void RunWithWindow() {
#ifdef LUPINE_HAS_SDL3
        SDL_Event event;
        auto startTime = std::chrono::steady_clock::now();
        
        while (m_running) {
            // Handle events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    m_running = false;
                }
                else if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        m_running = false;
                    }
                }
            }
            
            // Simple frame limiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            
            // Check if we should exit after some time (for CI testing)
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
            if (elapsed.count() > 30) { // Exit after 30 seconds for CI
                m_engine.Log("Runtime test completed, exiting");
                m_running = false;
            }
        }
#endif
    }
    
    void RunConsoleMode() {
        m_engine.Log("Running in console mode");
        std::cout << "Lupine Runtime - Console Mode" << std::endl;
        std::cout << "Engine: " << Lupine::Engine::GetName() << " v" << Lupine::Engine::GetVersion() << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        
        // For CI testing, don't wait for input
        auto startTime = std::chrono::steady_clock::now();
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
            if (elapsed.count() > 5) { // Exit after 5 seconds for CI
                m_engine.Log("Console test completed, exiting");
                m_running = false;
            }
        }
    }
    
private:
    Lupine::Engine m_engine;
    bool m_running;
    
#ifdef LUPINE_HAS_SDL3
    SDL_Window* m_window;
#else
    void* m_window; // Placeholder
#endif
};

int main(int argc, char* argv[]) {
    std::cout << "=== Lupine Game Engine Runtime ===" << std::endl;
    std::cout << "Version: " << Lupine::Engine::GetVersion() << std::endl;
    std::cout << "Arguments: " << argc << std::endl;
    
    for (int i = 0; i < argc; ++i) {
        std::cout << "  [" << i << "]: " << argv[i] << std::endl;
    }
    
    RuntimeApplication app;
    
    if (!app.Initialize()) {
        std::cerr << "Failed to initialize runtime application" << std::endl;
        return 1;
    }
    
    app.Run();
    
    std::cout << "Runtime application finished successfully" << std::endl;
    return 0;
}
