#pragma once

#include "lupine/core/Component.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace Lupine {

// Forward declarations
class VScriptGraph;

/**
 * @brief Visual script component for attaching visual script graphs to nodes
 *
 * VisualScriptComponent allows attaching visual script graphs (.vscript files) to nodes
 * with export variables and lifecycle methods. Visual scripts are converted to Python
 * code at runtime and executed through the Python interpreter.
 */
class VisualScriptComponent : public Component {
public:
    /**
     * @brief Constructor
     */
    VisualScriptComponent();

    /**
     * @brief Virtual destructor
     */
    virtual ~VisualScriptComponent() = default;

    /**
     * @brief Get visual script file path
     * @return Path to .vscript file
     */
    const std::string& GetScriptPath() const { return m_script_path; }

    /**
     * @brief Set visual script file path
     * @param path Path to .vscript file
     */
    void SetScriptPath(const std::string& path);

    /**
     * @brief Get generated Python script path
     * @return Path to generated .py file
     */
    const std::string& GetGeneratedScriptPath() const { return m_generated_script_path; }

    /**
     * @brief Get visual script graph
     * @return Pointer to the loaded graph or nullptr if not loaded
     */
    VScriptGraph* GetGraph() const { return m_graph.get(); }

    /**
     * @brief Check if script is loaded successfully
     * @return True if script is loaded and ready
     */
    bool IsScriptLoaded() const { return m_script_loaded; }

    /**
     * @brief Check if there was an error loading the script
     * @return True if there was an error
     */
    bool HasScriptError() const { return m_script_error; }

    /**
     * @brief Get last error message
     * @return Error message string
     */
    const std::string& GetLastError() const { return m_last_error; }

    /**
     * @brief Reload the visual script from file
     */
    void ReloadScript();

    /**
     * @brief Generate Python code from the visual script
     * @return Generated Python code as string
     */
    std::string GeneratePythonCode() const;

    /**
     * @brief Export the visual script to a Python file
     * @param output_path Path where to save the generated Python file
     * @return True if export was successful
     */
    bool ExportToPython(const std::string& output_path) const;

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "VisualScriptComponent"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Scripting"; }

    // Component lifecycle methods
    virtual void OnAwake() override;
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnPhysicsProcess(float delta_time) override;
    virtual void OnInput(const void* event) override;
    virtual void OnDestroy() override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;

private:
    /**
     * @brief Load visual script from file
     */
    void LoadScript();

    /**
     * @brief Handle script loading errors
     * @param error_message Error message to log and store
     */
    void HandleScriptError(const std::string& error_message);

    /**
     * @brief Generate Python code and save to temporary file
     */
    void GenerateAndSavePythonScript();

    /**
     * @brief Execute Python function if it exists
     * @param function_name Name of the Python function to call
     */
    void CallPythonFunction(const std::string& function_name);

    // Script data
    std::string m_script_path;                    ///< Path to .vscript file
    std::string m_generated_script_path;          ///< Path to generated .py file
    std::unique_ptr<VScriptGraph> m_graph;        ///< Loaded visual script graph
    
    // Script state
    bool m_script_loaded;                         ///< True if script is loaded successfully
    bool m_script_error;                          ///< True if there was an error loading
    std::string m_last_error;                     ///< Last error message
    
    // Python execution state
    bool m_python_initialized;                    ///< True if Python environment is set up
    void* m_python_globals;                       ///< Python globals dictionary (py::dict*)
};

} // namespace Lupine
