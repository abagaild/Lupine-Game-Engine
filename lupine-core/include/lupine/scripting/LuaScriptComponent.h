#pragma once

#include "lupine/core/Component.h"
#include <lua.hpp>
#include <string>
#include <memory>

namespace Lupine {

/**
 * @brief Lua script component for attaching Lua scripts to nodes
 *
 * LuaScriptComponent allows attaching Lua scripts to nodes with export variables
 * and lifecycle methods. Scripts can define export variables that are parsed
 * and exposed to the editor, and implement lifecycle methods that are called
 * by the engine.
 */
class LuaScriptComponent : public Component {
public:
    /**
     * @brief Constructor
     */
    LuaScriptComponent();

    /**
     * @brief Virtual destructor
     */
    virtual ~LuaScriptComponent();

    /**
     * @brief Get script file path
     * @return Path to Lua script file
     */
    const std::string& GetScriptPath() const { return m_script_path; }

    /**
     * @brief Set script file path
     * @param path Path to Lua script file
     */
    void SetScriptPath(const std::string& path);

    /**
     * @brief Get script source code
     * @return Lua script source code
     */
    const std::string& GetScriptSource() const { return m_script_source; }

    /**
     * @brief Set script source code
     * @param source Lua script source code
     */
    void SetScriptSource(const std::string& source);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "LuaScriptComponent"; }

    /**
     * @brief Get component category
     * @return Category string
     */
    std::string GetCategory() const override { return "Scripting"; }

    // Lifecycle methods
    void OnAwake() override;
    void OnReady() override;
    void OnUpdate(float delta_time) override;
    void OnPhysicsProcess(float delta_time) override;
    void OnInput(const void* event) override;
    void OnDestroy() override;

protected:
    void InitializeExportVariables() override;

private:
    std::string m_script_path;
    std::string m_script_source;
    lua_State* m_lua_state;
    bool m_script_loaded;
    bool m_script_error;
    std::string m_last_error;

    /**
     * @brief Load and execute the Lua script
     */
    void LoadScript();

    /**
     * @brief Parse export variables from the script
     */
    void ParseExportVariables();

    /**
     * @brief Call a Lua function if it exists
     * @param function_name Name of the function to call
     * @param args Arguments to pass to the function
     */
    template<typename... Args>
    void CallLuaFunction(const std::string& function_name, Args&&... args);

    /**
     * @brief Handle Lua errors
     * @param error Error message
     */
    void HandleLuaError(const std::string& error);

    /**
     * @brief Setup Lua environment with engine bindings
     */
    void SetupLuaEnvironment();

    /**
     * @brief Parse script metadata (name, category)
     */
    void ParseScriptMetadata();

    /**
     * @brief Push a value to the Lua stack
     */
    template<typename T>
    void PushLuaValue(T&& value);
};

} // namespace Lupine
