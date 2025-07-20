#include "lupine/scripting/LuaScriptComponent.h"
#include "lupine/core/Node.h"
#include "lupine/core/GlobalsManager.h"
#include "lupine/input/InputManager.h"
#include "lupine/localization/LocalizationAPI.h"
#include "lupine/scriptableobjects/ScriptableObjectBindings.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Lupine {

LuaScriptComponent::LuaScriptComponent()
    : Component("LuaScriptComponent")
    , m_script_path("")
    , m_script_source("")
    , m_lua_state(nullptr)
    , m_script_loaded(false)
    , m_script_error(false)
    , m_last_error("") {

    // Initialize Lua state
    m_lua_state = luaL_newstate();
    if (m_lua_state) {
        luaL_openlibs(m_lua_state);
        SetupLuaEnvironment();
    }

    // Initialize export variables
    InitializeExportVariables();
}

LuaScriptComponent::~LuaScriptComponent() {
    if (m_lua_state) {
        lua_close(m_lua_state);
        m_lua_state = nullptr;
    }
}

void LuaScriptComponent::SetScriptPath(const std::string& path) {
    m_script_path = path;
    SetExportVariable("script_path", path);
    m_script_loaded = false;
    LoadScript();
}

void LuaScriptComponent::SetScriptSource(const std::string& source) {
    m_script_source = source;
    SetExportVariable("script_source", source);
    m_script_loaded = false;
    LoadScript();
}

void LuaScriptComponent::OnAwake() {
    std::cout << "[LuaScript] OnAwake called for " << GetOwner()->GetName() << std::endl;
    LoadScript();
    CallLuaFunction("on_awake");
}

void LuaScriptComponent::OnReady() {
    std::cout << "[LuaScript] OnReady called for " << GetOwner()->GetName() << std::endl;
    CallLuaFunction("on_ready");
}

void LuaScriptComponent::OnUpdate(float delta_time) {
    CallLuaFunction("on_update", delta_time);
}

void LuaScriptComponent::OnPhysicsProcess(float delta_time) {
    CallLuaFunction("on_physics_process", delta_time);
}

void LuaScriptComponent::OnInput(const void* event) {
    // For now, just call the function without the event parameter
    CallLuaFunction("on_input");
}

void LuaScriptComponent::OnDestroy() {
    std::cout << "[LuaScript] OnDestroy called for " << GetOwner()->GetName() << std::endl;
    CallLuaFunction("on_destroy");
}

void LuaScriptComponent::InitializeExportVariables() {
    AddExportVariable("script_path", m_script_path, "Path to Lua script file", ExportVariableType::FilePath);
    AddExportVariable("script_source", m_script_source, "Lua script source code (leave empty to use file)", ExportVariableType::String);

    // Add component info variables that will be parsed from script
    AddExportVariable("script_name", std::string(""), "Script name (auto-detected)", ExportVariableType::String);
    AddExportVariable("script_category", std::string("Scripts"), "Script category (auto-detected)", ExportVariableType::String);
}

void LuaScriptComponent::LoadScript() {
    if (m_script_path.empty() && m_script_source.empty()) {
        return;
    }

    if (!m_lua_state) {
        HandleLuaError("Lua state not initialized");
        return;
    }

    std::string script_to_execute;

    // Load from file if path is specified
    if (!m_script_path.empty()) {
        std::ifstream file(m_script_path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            script_to_execute = buffer.str();
            file.close();
        } else {
            HandleLuaError("Failed to open script file: " + m_script_path);
            return;
        }
    } else {
        script_to_execute = m_script_source;
    }

    // Execute the script
    int result = luaL_dostring(m_lua_state, script_to_execute.c_str());
    if (result != LUA_OK) {
        std::string error = lua_tostring(m_lua_state, -1);
        lua_pop(m_lua_state, 1);
        HandleLuaError("Script execution error: " + error);
        return;
    }

    m_script_loaded = true;
    m_script_error = false;
    m_last_error.clear();

    // Parse export variables from the script
    ParseExportVariables();

    std::cout << "[LuaScript] Script loaded successfully for " << GetOwner()->GetName() << std::endl;
}

void LuaScriptComponent::ParseExportVariables() {
    if (!m_lua_state || !m_script_loaded) {
        return;
    }

    // Parse script metadata first
    ParseScriptMetadata();

    // Look for export_vars table in the script
    lua_getglobal(m_lua_state, "export_vars");
    if (lua_istable(m_lua_state, -1)) {
        lua_pushnil(m_lua_state);  // First key
        while (lua_next(m_lua_state, -2) != 0) {
            // Key is at index -2, value is at index -1
            if (lua_isstring(m_lua_state, -2)) {
                std::string key = lua_tostring(m_lua_state, -2);

                // Convert Lua values to ExportValue
                if (lua_isboolean(m_lua_state, -1)) {
                    bool value = lua_toboolean(m_lua_state, -1);
                    AddExportVariable(key, value, "Exported from Lua script");
                } else if (lua_isinteger(m_lua_state, -1)) {
                    int value = (int)lua_tointeger(m_lua_state, -1);
                    AddExportVariable(key, value, "Exported from Lua script");
                } else if (lua_isnumber(m_lua_state, -1)) {
                    float value = (float)lua_tonumber(m_lua_state, -1);
                    AddExportVariable(key, value, "Exported from Lua script");
                } else if (lua_isstring(m_lua_state, -1)) {
                    std::string value = lua_tostring(m_lua_state, -1);
                    AddExportVariable(key, value, "Exported from Lua script");
                }

                std::cout << "[LuaScript] Exported variable: " << key << std::endl;
            }
            lua_pop(m_lua_state, 1);  // Remove value, keep key for next iteration
        }
    }
    lua_pop(m_lua_state, 1);  // Remove export_vars table from stack
}

void LuaScriptComponent::ParseScriptMetadata() {
    if (!m_lua_state || !m_script_loaded) {
        return;
    }

    // Look for script_name global variable
    lua_getglobal(m_lua_state, "script_name");
    if (lua_isstring(m_lua_state, -1)) {
        std::string scriptName = lua_tostring(m_lua_state, -1);
        SetExportVariable("script_name", scriptName);
    }
    lua_pop(m_lua_state, 1);

    // Look for script_category global variable
    lua_getglobal(m_lua_state, "script_category");
    if (lua_isstring(m_lua_state, -1)) {
        std::string scriptCategory = lua_tostring(m_lua_state, -1);
        SetExportVariable("script_category", scriptCategory);
    }
    lua_pop(m_lua_state, 1);
}

template<typename... Args>
void LuaScriptComponent::CallLuaFunction(const std::string& function_name, Args&&... args) {
    if (!m_lua_state || !m_script_loaded || m_script_error) {
        return;
    }

    lua_getglobal(m_lua_state, function_name.c_str());
    if (lua_isfunction(m_lua_state, -1)) {
        // Push arguments to stack
        int arg_count = 0;
        if constexpr (sizeof...(args) > 0) {
            (PushLuaValue(std::forward<Args>(args)), ...);
            arg_count = sizeof...(args);
        }

        // Call the function
        int result = lua_pcall(m_lua_state, arg_count, 0, 0);
        if (result != LUA_OK) {
            std::string error = lua_tostring(m_lua_state, -1);
            lua_pop(m_lua_state, 1);
            HandleLuaError("Error calling " + function_name + ": " + error);
        }
    } else {
        lua_pop(m_lua_state, 1);  // Remove non-function from stack
    }
}

void LuaScriptComponent::HandleLuaError(const std::string& error) {
    m_script_error = true;
    m_last_error = error;
    std::cerr << "[LuaScript] Error: " << error << std::endl;
}

void LuaScriptComponent::SetupLuaEnvironment() {
    if (!m_lua_state) {
        return;
    }

    // Register custom print function
    lua_register(m_lua_state, "print", [](lua_State* L) -> int {
        const char* message = luaL_checkstring(L, 1);
        std::cout << "[Lua] " << message << std::endl;
        return 0;
    });

    // Add node reference if we have an owner
    if (GetOwner()) {
        lua_pushstring(m_lua_state, GetOwner()->GetName().c_str());
        lua_setglobal(m_lua_state, "node_name");

        lua_register(m_lua_state, "get_node_name", [](lua_State* L) -> int {
            // This is a simplified version - in a real implementation,
            // we'd need to store a reference to the component
            lua_pushstring(L, "LuaScriptNode");
            return 1;
        });
    }

    // Add GlobalsManager access functions
    auto& globals_manager = GlobalsManager::Instance();

    // Register autoload access function
    lua_register(m_lua_state, "get_autoload", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        auto& gm = GlobalsManager::Instance();
        auto component = gm.GetAutoloadComponent(name);
        if (component) {
            lua_pushstring(L, component->GetOwner()->GetName().c_str());
        } else {
            lua_pushnil(L);
        }
        return 1;
    });

    // Register global variable getter functions
    lua_register(m_lua_state, "get_global_bool", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        bool default_val = lua_gettop(L) > 1 ? lua_toboolean(L, 2) : false;
        auto& gm = GlobalsManager::Instance();
        bool value = gm.GetGlobalVariableValue<bool>(name, default_val);
        lua_pushboolean(L, value);
        return 1;
    });

    lua_register(m_lua_state, "get_global_int", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        int default_val = lua_gettop(L) > 1 ? luaL_checkinteger(L, 2) : 0;
        auto& gm = GlobalsManager::Instance();
        int value = gm.GetGlobalVariableValue<int>(name, default_val);
        lua_pushinteger(L, value);
        return 1;
    });

    lua_register(m_lua_state, "get_global_float", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        float default_val = lua_gettop(L) > 1 ? luaL_checknumber(L, 2) : 0.0f;
        auto& gm = GlobalsManager::Instance();
        float value = gm.GetGlobalVariableValue<float>(name, default_val);
        lua_pushnumber(L, value);
        return 1;
    });

    lua_register(m_lua_state, "get_global_string", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        const char* default_val = lua_gettop(L) > 1 ? luaL_checkstring(L, 2) : "";
        auto& gm = GlobalsManager::Instance();
        std::string value = gm.GetGlobalVariableValue<std::string>(name, std::string(default_val));
        lua_pushstring(L, value.c_str());
        return 1;
    });

    // Register global variable setter functions
    lua_register(m_lua_state, "set_global_bool", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        bool value = lua_toboolean(L, 2);
        auto& gm = GlobalsManager::Instance();
        bool success = gm.SetGlobalVariable(name, value);
        lua_pushboolean(L, success);
        return 1;
    });

    lua_register(m_lua_state, "set_global_int", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        int value = luaL_checkinteger(L, 2);
        auto& gm = GlobalsManager::Instance();
        bool success = gm.SetGlobalVariable(name, value);
        lua_pushboolean(L, success);
        return 1;
    });

    lua_register(m_lua_state, "set_global_float", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        float value = luaL_checknumber(L, 2);
        auto& gm = GlobalsManager::Instance();
        bool success = gm.SetGlobalVariable(name, value);
        lua_pushboolean(L, success);
        return 1;
    });

    lua_register(m_lua_state, "set_global_string", [](lua_State* L) -> int {
        const char* name = luaL_checkstring(L, 1);
        const char* value = luaL_checkstring(L, 2);
        auto& gm = GlobalsManager::Instance();
        bool success = gm.SetGlobalVariable(name, std::string(value));
        lua_pushboolean(L, success);
        return 1;
    });

    // Register input action functions
    lua_register(m_lua_state, "is_action_pressed", [](lua_State* L) -> int {
        const char* action_name = luaL_checkstring(L, 1);
        bool result = InputManager::IsActionPressed(action_name);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "is_action_just_pressed", [](lua_State* L) -> int {
        const char* action_name = luaL_checkstring(L, 1);
        bool result = InputManager::IsActionJustPressed(action_name);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "is_action_just_released", [](lua_State* L) -> int {
        const char* action_name = luaL_checkstring(L, 1);
        bool result = InputManager::IsActionJustReleased(action_name);
        lua_pushboolean(L, result);
        return 1;
    });

    // Register direct key/mouse input functions
    lua_register(m_lua_state, "is_key_pressed", [](lua_State* L) -> int {
        int keycode = luaL_checkinteger(L, 1);
        bool result = InputManager::IsKeyPressed(keycode);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "is_key_just_pressed", [](lua_State* L) -> int {
        int keycode = luaL_checkinteger(L, 1);
        bool result = InputManager::IsKeyJustPressed(keycode);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "is_mouse_button_pressed", [](lua_State* L) -> int {
        int button = luaL_checkinteger(L, 1);
        bool result = InputManager::IsMouseButtonPressed(static_cast<MouseButton>(button));
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "is_mouse_button_just_pressed", [](lua_State* L) -> int {
        int button = luaL_checkinteger(L, 1);
        bool result = InputManager::IsMouseButtonJustPressed(static_cast<MouseButton>(button));
        lua_pushboolean(L, result);
        return 1;
    });

    // Register localization functions
    lua_register(m_lua_state, "get_localized_string", [](lua_State* L) -> int {
        const char* key = luaL_checkstring(L, 1);
        std::string result;
        if (lua_gettop(L) > 1) {
            const char* fallback = luaL_checkstring(L, 2);
            result = LocalizationAPI::GetString(key, fallback);
        } else {
            result = LocalizationAPI::GetString(key);
        }
        lua_pushstring(L, result.c_str());
        return 1;
    });

    lua_register(m_lua_state, "set_locale", [](lua_State* L) -> int {
        const char* locale = luaL_checkstring(L, 1);
        bool result = LocalizationAPI::SetLocale(locale);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "get_current_locale", [](lua_State* L) -> int {
        std::string result = LocalizationAPI::GetCurrentLocale();
        lua_pushstring(L, result.c_str());
        return 1;
    });

    lua_register(m_lua_state, "get_supported_locales", [](lua_State* L) -> int {
        auto locales = LocalizationAPI::GetSupportedLocales();
        lua_newtable(L);
        for (size_t i = 0; i < locales.size(); ++i) {
            lua_pushstring(L, locales[i].c_str());
            lua_rawseti(L, -2, i + 1);
        }
        return 1;
    });

    lua_register(m_lua_state, "is_locale_supported", [](lua_State* L) -> int {
        const char* locale = luaL_checkstring(L, 1);
        bool result = LocalizationAPI::IsLocaleSupported(locale);
        lua_pushboolean(L, result);
        return 1;
    });

    lua_register(m_lua_state, "has_localization_key", [](lua_State* L) -> int {
        const char* key = luaL_checkstring(L, 1);
        bool result = LocalizationAPI::HasKey(key);
        lua_pushboolean(L, result);
        return 1;
    });

    // Add scriptable objects bindings
    ScriptableObjectLuaBindings::Initialize(m_lua_state);
}

template<typename T>
void LuaScriptComponent::PushLuaValue(T&& value) {
    if constexpr (std::is_same_v<std::decay_t<T>, float>) {
        lua_pushnumber(m_lua_state, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
        lua_pushnumber(m_lua_state, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, int>) {
        lua_pushinteger(m_lua_state, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        lua_pushboolean(m_lua_state, value);
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        lua_pushstring(m_lua_state, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char*>) {
        lua_pushstring(m_lua_state, value);
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(LuaScriptComponent, "Scripting", "Lua script component for custom behavior")
