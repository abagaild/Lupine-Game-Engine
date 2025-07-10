#include "lupine/scripting/PythonScriptComponent.h"
#include "lupine/core/Node.h"
#include "lupine/core/GlobalsManager.h"
#include "lupine/input/InputManager.h"
#include "lupine/localization/LocalizationAPI.h"
#include "lupine/scriptableobjects/ScriptableObjectBindings.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace py = pybind11;

namespace Lupine {

bool PythonScriptComponent::s_python_initialized = false;

PythonScriptComponent::PythonScriptComponent()
    : Component("PythonScriptComponent")
    , m_script_path("")
    , m_script_source("")
    , m_script_loaded(false)
    , m_script_error(false)
    , m_last_error("") {

    InitializePython();

    // Initialize export variables
    InitializeExportVariables();
}

void PythonScriptComponent::SetScriptPath(const std::string& path) {
    m_script_path = path;
    SetExportVariable("script_path", path);
    m_script_loaded = false;
    LoadScript();
}

void PythonScriptComponent::SetScriptSource(const std::string& source) {
    m_script_source = source;
    SetExportVariable("script_source", source);
    m_script_loaded = false;
    LoadScript();
}

void PythonScriptComponent::OnAwake() {
    std::cout << "[PythonScript] OnAwake called for " << GetOwner()->GetName() << std::endl;
    LoadScript();
    CallPythonFunction("on_awake");
}

void PythonScriptComponent::OnReady() {
    std::cout << "[PythonScript] OnReady called for " << GetOwner()->GetName() << std::endl;
    CallPythonFunction("on_ready");
}

void PythonScriptComponent::OnUpdate(float delta_time) {
    CallPythonFunction("on_update", delta_time);
}

void PythonScriptComponent::OnPhysicsProcess(float delta_time) {
    CallPythonFunction("on_physics_process", delta_time);
}

void PythonScriptComponent::OnInput(const void* event) {
    // For now, just call the function without the event parameter
    CallPythonFunction("on_input");
}

void PythonScriptComponent::OnDestroy() {
    std::cout << "[PythonScript] OnDestroy called for " << GetOwner()->GetName() << std::endl;
    CallPythonFunction("on_destroy");
}

void PythonScriptComponent::InitializeExportVariables() {
    AddExportVariable("script_path", m_script_path, "Path to Python script file", ExportVariableType::FilePath);
    AddExportVariable("script_source", m_script_source, "Python script source code (leave empty to use file)", ExportVariableType::String);

    // Add component info variables that will be parsed from script
    AddExportVariable("script_name", std::string(""), "Script name (auto-detected)", ExportVariableType::String);
    AddExportVariable("script_category", std::string("Scripts"), "Script category (auto-detected)", ExportVariableType::String);
}

void PythonScriptComponent::LoadScript() {
    if (m_script_path.empty() && m_script_source.empty()) {
        return;
    }

    if (!IsPythonInitialized()) {
        HandlePythonError("Python interpreter not initialized");
        return;
    }

    try {
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
                HandlePythonError("Failed to open script file: " + m_script_path);
                return;
            }
        } else {
            script_to_execute = m_script_source;
        }

        // Create a new globals dictionary for this script
        m_script_globals = py::dict();
        SetupPythonEnvironment();

        // Execute the script
        py::exec(script_to_execute, m_script_globals);

        m_script_loaded = true;
        m_script_error = false;
        m_last_error.clear();

        // Parse export variables from the script
        ParseExportVariables();

        std::cout << "[PythonScript] Script loaded successfully for " << GetOwner()->GetName() << std::endl;

    } catch (const py::error_already_set& e) {
        HandlePythonError("Python execution error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        HandlePythonError("Exception during script loading: " + std::string(e.what()));
    }
}

void PythonScriptComponent::ParseExportVariables() {
    if (!m_script_loaded) {
        return;
    }

    try {
        // Parse script metadata first
        ParseScriptMetadata();

        // Look for export_vars dictionary in the script
        if (m_script_globals.contains("export_vars")) {
            py::dict export_vars = m_script_globals["export_vars"];

            for (auto item : export_vars) {
                std::string key = py::str(item.first);
                py::object value = py::reinterpret_borrow<py::object>(item.second);

                // Convert Python values to ExportValue
                if (py::isinstance<py::bool_>(value)) {
                    AddExportVariable(key, value.cast<bool>(), "Exported from Python script");
                } else if (py::isinstance<py::int_>(value)) {
                    AddExportVariable(key, value.cast<int>(), "Exported from Python script");
                } else if (py::isinstance<py::float_>(value)) {
                    AddExportVariable(key, value.cast<float>(), "Exported from Python script");
                } else if (py::isinstance<py::str>(value)) {
                    AddExportVariable(key, value.cast<std::string>(), "Exported from Python script");
                }

                std::cout << "[PythonScript] Exported variable: " << key << std::endl;
            }
        }
    } catch (const py::error_already_set& e) {
        HandlePythonError("Error parsing export variables: " + std::string(e.what()));
    } catch (const std::exception& e) {
        HandlePythonError("Error parsing export variables: " + std::string(e.what()));
    }
}

void PythonScriptComponent::ParseScriptMetadata() {
    if (!m_script_loaded) {
        return;
    }

    try {
        // Look for script_name global variable
        if (m_script_globals.contains("script_name")) {
            std::string scriptName = m_script_globals["script_name"].cast<std::string>();
            SetExportVariable("script_name", scriptName);
        }

        // Look for script_category global variable
        if (m_script_globals.contains("script_category")) {
            std::string scriptCategory = m_script_globals["script_category"].cast<std::string>();
            SetExportVariable("script_category", scriptCategory);
        }
    } catch (const py::error_already_set& e) {
        HandlePythonError("Error parsing script metadata: " + std::string(e.what()));
    } catch (const std::exception& e) {
        HandlePythonError("Error parsing script metadata: " + std::string(e.what()));
    }
}

template<typename... Args>
void PythonScriptComponent::CallPythonFunction(const std::string& function_name, Args&&... args) {
    if (!m_script_loaded || m_script_error) {
        return;
    }

    try {
        if (m_script_globals.contains(function_name.c_str())) {
            py::object func = m_script_globals[function_name.c_str()];
            if (py::isinstance<py::function>(func)) {
                func(std::forward<Args>(args)...);
            }
        }
    } catch (const py::error_already_set& e) {
        HandlePythonError("Error calling " + function_name + ": " + std::string(e.what()));
    } catch (const std::exception& e) {
        HandlePythonError("Exception calling " + function_name + ": " + std::string(e.what()));
    }
}

void PythonScriptComponent::HandlePythonError(const std::string& error) {
    m_script_error = true;
    m_last_error = error;
    std::cerr << "[PythonScript] Error: " << error << std::endl;
}

void PythonScriptComponent::SetupPythonEnvironment() {
    if (!IsPythonInitialized()) {
        return;
    }

    try {
        // Add built-in functions
        m_script_globals["print"] = py::cpp_function([](const std::string& message) {
            std::cout << "[Python] " << message << std::endl;
        });

        // Add node reference if we have an owner
        if (GetOwner()) {
            m_script_globals["node_name"] = GetOwner()->GetName();
            m_script_globals["get_node_name"] = py::cpp_function([this]() -> std::string {
                return GetOwner() ? GetOwner()->GetName() : "Unknown";
            });
        }

        // Add common Python modules
        py::exec("import sys", m_script_globals);
        py::exec("import math", m_script_globals);

        // Add GlobalsManager access
        auto& globals_manager = GlobalsManager::Instance();

        // Add function to get autoload components
        m_script_globals["get_autoload"] = py::cpp_function([&globals_manager](const std::string& name) -> py::object {
            auto component = globals_manager.GetAutoloadComponent(name);
            if (component) {
                // Return a simple wrapper - in a full implementation, we'd expose the component properly
                return py::cast(component->GetOwner()->GetName());
            }
            return py::none();
        });

        // Add input action functions
        m_script_globals["is_action_pressed"] = py::cpp_function([](const std::string& action_name) -> bool {
            return InputManager::IsActionPressed(action_name);
        });

        m_script_globals["is_action_just_pressed"] = py::cpp_function([](const std::string& action_name) -> bool {
            return InputManager::IsActionJustPressed(action_name);
        });

        m_script_globals["is_action_just_released"] = py::cpp_function([](const std::string& action_name) -> bool {
            return InputManager::IsActionJustReleased(action_name);
        });

        // Add direct key/mouse input functions for convenience
        m_script_globals["is_key_pressed"] = py::cpp_function([](int keycode) -> bool {
            return InputManager::IsKeyPressed(keycode);
        });

        m_script_globals["is_key_just_pressed"] = py::cpp_function([](int keycode) -> bool {
            return InputManager::IsKeyJustPressed(keycode);
        });

        m_script_globals["is_mouse_button_pressed"] = py::cpp_function([](int button) -> bool {
            return InputManager::IsMouseButtonPressed(static_cast<MouseButton>(button));
        });

        m_script_globals["is_mouse_button_just_pressed"] = py::cpp_function([](int button) -> bool {
            return InputManager::IsMouseButtonJustPressed(static_cast<MouseButton>(button));
        });

        // Add global variable access functions
        m_script_globals["get_global_bool"] = py::cpp_function([&globals_manager](const std::string& name, bool default_val = false) -> bool {
            return globals_manager.GetGlobalVariableValue<bool>(name, default_val);
        });

        m_script_globals["get_global_int"] = py::cpp_function([&globals_manager](const std::string& name, int default_val = 0) -> int {
            return globals_manager.GetGlobalVariableValue<int>(name, default_val);
        });

        m_script_globals["get_global_float"] = py::cpp_function([&globals_manager](const std::string& name, float default_val = 0.0f) -> float {
            return globals_manager.GetGlobalVariableValue<float>(name, default_val);
        });

        m_script_globals["get_global_string"] = py::cpp_function([&globals_manager](const std::string& name, const std::string& default_val = "") -> std::string {
            return globals_manager.GetGlobalVariableValue<std::string>(name, default_val);
        });

        // Add global variable setter functions
        m_script_globals["set_global_bool"] = py::cpp_function([&globals_manager](const std::string& name, bool value) -> bool {
            return globals_manager.SetGlobalVariable(name, value);
        });

        m_script_globals["set_global_int"] = py::cpp_function([&globals_manager](const std::string& name, int value) -> bool {
            return globals_manager.SetGlobalVariable(name, value);
        });

        m_script_globals["set_global_float"] = py::cpp_function([&globals_manager](const std::string& name, float value) -> bool {
            return globals_manager.SetGlobalVariable(name, value);
        });

        m_script_globals["set_global_string"] = py::cpp_function([&globals_manager](const std::string& name, const std::string& value) -> bool {
            return globals_manager.SetGlobalVariable(name, value);
        });

        // Add localization functions
        m_script_globals["get_localized_string"] = py::cpp_function([](const std::string& key, const std::string& fallback = "") -> std::string {
            if (fallback.empty()) {
                return LocalizationAPI::GetString(key);
            } else {
                return LocalizationAPI::GetString(key, fallback);
            }
        });

        m_script_globals["set_locale"] = py::cpp_function([](const std::string& locale) -> bool {
            return LocalizationAPI::SetLocale(locale);
        });

        m_script_globals["get_current_locale"] = py::cpp_function([]() -> std::string {
            return LocalizationAPI::GetCurrentLocale();
        });

        m_script_globals["get_supported_locales"] = py::cpp_function([]() -> std::vector<std::string> {
            return LocalizationAPI::GetSupportedLocales();
        });

        m_script_globals["is_locale_supported"] = py::cpp_function([](const std::string& locale) -> bool {
            return LocalizationAPI::IsLocaleSupported(locale);
        });

        m_script_globals["has_localization_key"] = py::cpp_function([](const std::string& key) -> bool {
            return LocalizationAPI::HasKey(key);
        });

        // Add scriptable objects bindings
        ScriptableObjectPythonBindings::Initialize(m_script_globals);

    } catch (const py::error_already_set& e) {
        HandlePythonError("Error setting up Python environment: " + std::string(e.what()));
    }
}

void PythonScriptComponent::InitializePython() {
    if (s_python_initialized) {
        return;
    }

    try {
        py::initialize_interpreter();
        s_python_initialized = true;
        std::cout << "[PythonScript] Python interpreter initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[PythonScript] Failed to initialize Python: " << e.what() << std::endl;
    }
}

bool PythonScriptComponent::IsPythonInitialized() {
    return s_python_initialized && Py_IsInitialized();
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(PythonScriptComponent, "Scripting", "Python script component for custom behavior")
