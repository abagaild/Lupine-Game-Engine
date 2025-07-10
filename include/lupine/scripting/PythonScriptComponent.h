#pragma once

#include "lupine/core/Component.h"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <string>
#include <memory>

namespace Lupine {

/**
 * @brief Python script component for attaching Python scripts to nodes
 *
 * PythonScriptComponent allows attaching Python scripts to nodes with export variables
 * and lifecycle methods. Scripts can define export variables that are parsed
 * and exposed to the editor, and implement lifecycle methods that are called
 * by the engine.
 */
class PythonScriptComponent : public Component {
public:
    /**
     * @brief Constructor
     */
    PythonScriptComponent();

    /**
     * @brief Virtual destructor
     */
    virtual ~PythonScriptComponent() = default;

    /**
     * @brief Get script file path
     * @return Path to Python script file
     */
    const std::string& GetScriptPath() const { return m_script_path; }

    /**
     * @brief Set script file path
     * @param path Path to Python script file
     */
    void SetScriptPath(const std::string& path);

    /**
     * @brief Get script source code
     * @return Python script source code
     */
    const std::string& GetScriptSource() const { return m_script_source; }

    /**
     * @brief Set script source code
     * @param source Python script source code
     */
    void SetScriptSource(const std::string& source);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "PythonScriptComponent"; }

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
    pybind11::dict m_script_globals;
    bool m_script_loaded;
    bool m_script_error;
    std::string m_last_error;

    /**
     * @brief Load and execute the Python script
     */
    void LoadScript();

    /**
     * @brief Parse export variables from the script
     */
    void ParseExportVariables();

    /**
     * @brief Call a Python function if it exists
     * @param function_name Name of the function to call
     * @param args Arguments to pass to the function
     */
    template<typename... Args>
    void CallPythonFunction(const std::string& function_name, Args&&... args);

    /**
     * @brief Handle Python errors
     * @param error Error message
     */
    void HandlePythonError(const std::string& error);

    /**
     * @brief Setup Python environment with engine bindings
     */
    void SetupPythonEnvironment();

    /**
     * @brief Parse script metadata (name, category)
     */
    void ParseScriptMetadata();

    /**
     * @brief Initialize Python interpreter (static)
     */
    static void InitializePython();

    /**
     * @brief Check if Python is initialized
     */
    static bool IsPythonInitialized();

private:
    static bool s_python_initialized;
};

} // namespace Lupine
