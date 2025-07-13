#include "lupine/Engine.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Lupine {
    
    // Private implementation class
    class Engine::Impl {
    public:
        Impl() = default;
        ~Impl() = default;
        
        void LogMessage(const std::string& message) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            std::ostringstream oss;
            oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << message;
            std::cout << oss.str() << std::endl;
        }
    };
    
    Engine::Engine() : m_initialized(false), m_impl(std::make_unique<Impl>()) {
        Log("Engine constructor called");
    }
    
    Engine::~Engine() {
        if (m_initialized) {
            Shutdown();
        }
        Log("Engine destructor called");
    }
    
    bool Engine::Initialize() {
        if (m_initialized) {
            Log("Engine already initialized");
            return true;
        }
        
        Log("Initializing Lupine Game Engine...");
        
        // Basic initialization
        Log("Engine core systems initialized");
        
        m_initialized = true;
        Log("Engine initialization complete");
        return true;
    }
    
    void Engine::Shutdown() {
        if (!m_initialized) {
            Log("Engine not initialized, nothing to shutdown");
            return;
        }
        
        Log("Shutting down Lupine Game Engine...");
        
        // Cleanup resources
        Log("Engine resources cleaned up");
        
        m_initialized = false;
        Log("Engine shutdown complete");
    }
    
    std::string Engine::GetVersion() {
        return "1.0.0";
    }
    
    std::string Engine::GetName() {
        return "Lupine Game Engine";
    }
    
    bool Engine::IsInitialized() const {
        return m_initialized;
    }
    
    void Engine::Log(const std::string& message) {
        if (m_impl) {
            m_impl->LogMessage(message);
        }
    }
    
} // namespace Lupine
