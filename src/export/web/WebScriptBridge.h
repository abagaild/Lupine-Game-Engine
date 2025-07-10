#pragma once

#include <lua.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <string>
#include <vector>
#include <memory>

namespace Lupine {

/**
 * @brief Bridge for script systems in web environment
 * 
 * This class provides integration between Lua/Python scripts and the web runtime,
 * handling web-specific functionality and JavaScript interop.
 */
class WebScriptBridge {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the WebScriptBridge instance
     */
    static WebScriptBridge& Instance();
    
    /**
     * @brief Constructor
     */
    WebScriptBridge();
    
    /**
     * @brief Destructor
     */
    ~WebScriptBridge();
    
    /**
     * @brief Execute a Lua script in web environment
     * @param script Lua script code to execute
     * @return True if execution succeeded
     */
    bool ExecuteLuaScript(const std::string& script);
    
    /**
     * @brief Execute a Python script in web environment
     * @param script Python script code to execute
     * @return True if execution succeeded
     */
    bool ExecutePythonScript(const std::string& script);
    
    /**
     * @brief Call a Lua function with string arguments
     * @param function_name Name of the function to call
     * @param args Vector of string arguments
     */
    void CallLuaFunction(const std::string& function_name, const std::vector<std::string>& args = {});
    
    /**
     * @brief Check if Lua is available in web environment
     * @return True if Lua is initialized and available
     */
    bool IsLuaAvailable() const;
    
    /**
     * @brief Check if Python is available in web environment
     * @return True if Python is initialized and available
     */
    bool IsPythonAvailable() const;
    
    /**
     * @brief Shutdown script systems
     */
    void ShutdownScriptSystems();

private:
    lua_State* m_lua_state = nullptr;
    bool m_lua_initialized = false;
    bool m_python_initialized = false;
    
    /**
     * @brief Initialize Lua for web environment
     */
    void InitializeLuaForWeb();
    
    /**
     * @brief Initialize Python for web environment
     */
    void InitializePythonForWeb();
    
    /**
     * @brief Register web-specific Lua functions
     */
    void RegisterLuaWebFunctions();
    
    /**
     * @brief Register web-specific Python functions
     */
    void RegisterPythonWebFunctions();
    
    // Prevent copying
    WebScriptBridge(const WebScriptBridge&) = delete;
    WebScriptBridge& operator=(const WebScriptBridge&) = delete;
};

} // namespace Lupine
