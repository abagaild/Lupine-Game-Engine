#include "lupine/scripting/VisualScriptComponent.h"
#include "lupine/core/Node.h"
#include "lupine/visualscripting/VScriptGraph.h"
#include "lupine/visualscripting/CodeGenerator.h"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace py = pybind11;

namespace Lupine {

VisualScriptComponent::VisualScriptComponent()
    : Component("VisualScriptComponent")
    , m_script_loaded(false)
    , m_script_error(false)
    , m_python_initialized(false)
    , m_python_globals(nullptr)
{
    InitializeExportVariables();
}

void VisualScriptComponent::InitializeExportVariables() {
    AddExportVariable("script_path", std::string(""), "Path to the visual script (.vscript) file");
}

void VisualScriptComponent::SetScriptPath(const std::string& path) {
    m_script_path = path;
    SetExportVariable("script_path", path);
    m_script_loaded = false;
    LoadScript();
}

void VisualScriptComponent::OnAwake() {
    std::cout << "[VisualScript] OnAwake called for " << GetOwner()->GetName() << std::endl;
    LoadScript();
    CallPythonFunction("on_awake");
}

void VisualScriptComponent::OnReady() {
    std::cout << "[VisualScript] OnReady called for " << GetOwner()->GetName() << std::endl;
    CallPythonFunction("on_ready");
}

void VisualScriptComponent::OnUpdate(float delta_time) {
    CallPythonFunction("on_update");
}

void VisualScriptComponent::OnPhysicsProcess(float delta_time) {
    CallPythonFunction("on_physics_process");
}

void VisualScriptComponent::OnInput(const void* event) {
    CallPythonFunction("on_input");
}

void VisualScriptComponent::OnDestroy() {
    std::cout << "[VisualScript] OnDestroy called for " << GetOwner()->GetName() << std::endl;
    CallPythonFunction("on_destroy");
}

void VisualScriptComponent::LoadScript() {
    if (m_script_path.empty()) {
        return;
    }

    try {
        // Load the visual script graph from file
        m_graph = std::make_unique<VScriptGraph>();
        if (!m_graph->LoadFromFile(m_script_path)) {
            HandleScriptError("Failed to load visual script file: " + m_script_path);
            return;
        }

        // Generate Python code from the graph
        GenerateAndSavePythonScript();

        m_script_loaded = true;
        m_script_error = false;
        m_last_error.clear();

        std::cout << "[VisualScript] Script loaded successfully for " << GetOwner()->GetName() << std::endl;

    } catch (const std::exception& e) {
        HandleScriptError("Exception during visual script loading: " + std::string(e.what()));
    }
}

void VisualScriptComponent::GenerateAndSavePythonScript() {
    if (!m_graph) {
        return;
    }

    // Generate Python code from the visual script graph
    CodeGenerator generator;
    std::string python_code = generator.GenerateCode(*m_graph);

    // Create a temporary Python file
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string filename = "vscript_" + GetOwner()->GetUUID().ToString() + "_" + GetUUID().ToString() + ".py";
    m_generated_script_path = (temp_dir / filename).string();

    // Write the generated code to file
    std::ofstream file(m_generated_script_path);
    if (file.is_open()) {
        file << python_code;
        file.close();
        std::cout << "[VisualScript] Generated Python code saved to: " << m_generated_script_path << std::endl;
    } else {
        HandleScriptError("Failed to write generated Python code to: " + m_generated_script_path);
    }
}

std::string VisualScriptComponent::GeneratePythonCode() const {
    if (!m_graph) {
        return "";
    }

    CodeGenerator generator;
    return generator.GenerateCode(*m_graph);
}

bool VisualScriptComponent::ExportToPython(const std::string& output_path) const {
    if (!m_graph) {
        return false;
    }

    try {
        CodeGenerator generator;
        std::string python_code = generator.GenerateCode(*m_graph);

        std::ofstream file(output_path);
        if (file.is_open()) {
            file << python_code;
            file.close();
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[VisualScript] Error exporting to Python: " << e.what() << std::endl;
    }

    return false;
}

void VisualScriptComponent::ReloadScript() {
    m_script_loaded = false;
    LoadScript();
}

void VisualScriptComponent::HandleScriptError(const std::string& error_message) {
    m_script_error = true;
    m_script_loaded = false;
    m_last_error = error_message;
    std::cerr << "[VisualScript] Error: " << error_message << std::endl;
}

void VisualScriptComponent::CallPythonFunction(const std::string& function_name) {
    // TODO: Implement Python function calling
    // This will be similar to PythonScriptComponent but using the generated script
    // For now, this is a placeholder
    (void)function_name; // Suppress unused parameter warning
}

} // namespace Lupine
