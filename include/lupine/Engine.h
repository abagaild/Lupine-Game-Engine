#pragma once

#include <string>
#include <memory>

namespace Lupine {
    
    /**
     * @brief Main engine class that provides core functionality
     */
    class Engine {
    public:
        Engine();
        ~Engine();
        
        /**
         * @brief Initialize the engine
         * @return true if initialization was successful
         */
        bool Initialize();
        
        /**
         * @brief Shutdown the engine and cleanup resources
         */
        void Shutdown();
        
        /**
         * @brief Get the engine version
         * @return Version string
         */
        static std::string GetVersion();
        
        /**
         * @brief Get the engine name
         * @return Engine name
         */
        static std::string GetName();
        
        /**
         * @brief Check if the engine is initialized
         * @return true if initialized
         */
        bool IsInitialized() const;
        
        /**
         * @brief Log a message to the engine's logging system
         * @param message The message to log
         */
        void Log(const std::string& message);
        
    private:
        bool m_initialized;
        
        // Private implementation to hide dependencies
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
} // namespace Lupine
