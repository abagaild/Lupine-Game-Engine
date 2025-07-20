#include "lupine/core/GlobalsManager.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/scripting/PythonScriptComponent.h"
#include "lupine/scripting/LuaScriptComponent.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Lupine {

// Static member definitions
std::unique_ptr<GlobalsManager> GlobalsManager::s_instance = nullptr;
bool GlobalsManager::s_initialized = false;

GlobalsManager& GlobalsManager::Instance() {
    if (!s_instance) {
        s_instance = std::unique_ptr<GlobalsManager>(new GlobalsManager());
    }
    return *s_instance;
}

bool GlobalsManager::Initialize() {
    if (s_initialized) {
        return true;
    }
    
    std::cout << "Initializing GlobalsManager..." << std::endl;
    s_initialized = true;
    return true;
}

void GlobalsManager::Shutdown() {
    if (s_instance) {
        s_instance->CleanupAutoloads();
        s_instance->Clear();
        s_instance.reset();
    }
    s_initialized = false;
}

// === Autoload Scripts ===

bool GlobalsManager::RegisterAutoload(const AutoloadScript& autoload) {
    if (autoload.name.empty()) {
        std::cerr << "Cannot register autoload with empty name" << std::endl;
        return false;
    }
    
    if (autoload.script_path.empty()) {
        std::cerr << "Cannot register autoload '" << autoload.name << "' with empty script path" << std::endl;
        return false;
    }
    
    if (autoload.script_type != "python" && autoload.script_type != "lua") {
        std::cerr << "Invalid script type '" << autoload.script_type << "' for autoload '" << autoload.name << "'" << std::endl;
        return false;
    }
    
    // Check if name already exists
    if (m_autoloads.find(autoload.name) != m_autoloads.end()) {
        std::cerr << "Autoload with name '" << autoload.name << "' already exists" << std::endl;
        return false;
    }
    
    m_autoloads[autoload.name] = autoload;
    std::cout << "Registered autoload: " << autoload.name << " (" << autoload.script_type << ")" << std::endl;
    return true;
}

bool GlobalsManager::UnregisterAutoload(const std::string& name) {
    auto it = m_autoloads.find(name);
    if (it == m_autoloads.end()) {
        return false;
    }
    
    // Cleanup runtime data if exists
    if (it->second.instance_node) {
        // Note: In a real implementation, we'd need to properly remove the node from the scene
        it->second.instance_node = nullptr;
        it->second.script_component = nullptr;
    }
    
    m_autoloads.erase(it);
    std::cout << "Unregistered autoload: " << name << std::endl;
    return true;
}

AutoloadScript* GlobalsManager::GetAutoload(const std::string& name) {
    auto it = m_autoloads.find(name);
    return (it != m_autoloads.end()) ? &it->second : nullptr;
}

void GlobalsManager::InitializeAutoloads(Scene* scene) {
    if (!scene) {
        std::cerr << "Cannot initialize autoloads with null scene" << std::endl;
        return;
    }
    
    std::cout << "Initializing " << m_autoloads.size() << " autoload scripts..." << std::endl;
    
    for (auto& [name, autoload] : m_autoloads) {
        if (!autoload.enabled) {
            continue;
        }
        
        // Create a node for this autoload
        auto node = std::make_unique<Node>(autoload.name);
        Node* node_ptr = node.get();
        
        // Add the appropriate script component
        std::unique_ptr<Component> script_component;
        
        if (autoload.script_type == "python") {
            auto python_script = std::make_unique<PythonScriptComponent>();
            python_script->SetScriptPath(autoload.script_path);
            script_component = std::move(python_script);
        } else if (autoload.script_type == "lua") {
            auto lua_script = std::make_unique<LuaScriptComponent>();
            lua_script->SetScriptPath(autoload.script_path);
            script_component = std::move(lua_script);
        }
        
        if (script_component) {
            Component* script_ptr = script_component.get();
            node_ptr->AddComponent(std::move(script_component));
            
            // Store runtime references
            autoload.instance_node = node_ptr;
            autoload.script_component = script_ptr;
            
            // Add node to scene root
            if (scene->GetRootNode()) {
                scene->GetRootNode()->AddChild(std::move(node));
            }
            
            std::cout << "Initialized autoload: " << name << std::endl;
        } else {
            std::cerr << "Failed to create script component for autoload: " << name << std::endl;
        }
    }
}

void GlobalsManager::CleanupAutoloads() {
    for (auto& [name, autoload] : m_autoloads) {
        autoload.instance_node = nullptr;
        autoload.script_component = nullptr;
    }
    std::cout << "Cleaned up autoload scripts" << std::endl;
}

Component* GlobalsManager::GetAutoloadComponent(const std::string& name) {
    auto it = m_autoloads.find(name);
    return (it != m_autoloads.end()) ? it->second.script_component : nullptr;
}

// === Global Variables ===

bool GlobalsManager::RegisterGlobalVariable(const GlobalVariable& variable) {
    if (variable.name.empty()) {
        std::cerr << "Cannot register global variable with empty name" << std::endl;
        return false;
    }
    
    if (variable.type.empty()) {
        std::cerr << "Cannot register global variable '" << variable.name << "' with empty type" << std::endl;
        return false;
    }
    
    // Validate type matches value
    if (!ValidateVariableType(variable.type, variable.value)) {
        std::cerr << "Type mismatch for global variable '" << variable.name << "'" << std::endl;
        return false;
    }
    
    m_global_variables[variable.name] = variable;
    std::cout << "Registered global variable: " << variable.name << " (" << variable.type << ")" << std::endl;
    return true;
}

bool GlobalsManager::UnregisterGlobalVariable(const std::string& name) {
    auto it = m_global_variables.find(name);
    if (it == m_global_variables.end()) {
        return false;
    }
    
    m_global_variables.erase(it);
    std::cout << "Unregistered global variable: " << name << std::endl;
    return true;
}

bool GlobalsManager::SetGlobalVariable(const std::string& name, const GlobalVariableValue& value) {
    auto it = m_global_variables.find(name);
    if (it == m_global_variables.end()) {
        std::cerr << "Global variable '" << name << "' not found" << std::endl;
        return false;
    }
    
    // Validate type
    if (!ValidateVariableType(it->second.type, value)) {
        std::cerr << "Type mismatch when setting global variable '" << name << "'" << std::endl;
        return false;
    }
    
    it->second.value = value;
    return true;
}

const GlobalVariableValue* GlobalsManager::GetGlobalVariable(const std::string& name) const {
    auto it = m_global_variables.find(name);
    return (it != m_global_variables.end()) ? &it->second.value : nullptr;
}

const GlobalVariable* GlobalsManager::GetGlobalVariableDefinition(const std::string& name) const {
    auto it = m_global_variables.find(name);
    return (it != m_global_variables.end()) ? &it->second : nullptr;
}

bool GlobalsManager::ResetGlobalVariable(const std::string& name) {
    auto it = m_global_variables.find(name);
    if (it == m_global_variables.end()) {
        return false;
    }
    
    it->second.value = it->second.default_value;
    return true;
}

void GlobalsManager::ResetAllGlobalVariables() {
    for (auto& [name, variable] : m_global_variables) {
        variable.value = variable.default_value;
    }
    std::cout << "Reset all global variables to default values" << std::endl;
}

// === Helper Methods ===

std::string GlobalsManager::GetVariableTypeString(const GlobalVariableValue& value) const {
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<int>(value)) return "int";
    if (std::holds_alternative<float>(value)) return "float";
    if (std::holds_alternative<std::string>(value)) return "string";
    if (std::holds_alternative<glm::vec2>(value)) return "vec2";
    if (std::holds_alternative<glm::vec3>(value)) return "vec3";
    if (std::holds_alternative<glm::vec4>(value)) return "vec4";
    return "unknown";
}

bool GlobalsManager::ValidateVariableType(const std::string& type, const GlobalVariableValue& value) const {
    std::string actual_type = GetVariableTypeString(value);
    return type == actual_type;
}

GlobalVariableValue GlobalsManager::ParseVariableValue(const std::string& type, const std::string& value_str) const {
    try {
        if (type == "bool") {
            return value_str == "true" || value_str == "1";
        } else if (type == "int") {
            return std::stoi(value_str);
        } else if (type == "float") {
            return std::stof(value_str);
        } else if (type == "string") {
            return value_str;
        } else if (type == "vec2") {
            std::istringstream iss(value_str);
            float x, y;
            if (iss >> x >> y) {
                return glm::vec2(x, y);
            }
        } else if (type == "vec3") {
            std::istringstream iss(value_str);
            float x, y, z;
            if (iss >> x >> y >> z) {
                return glm::vec3(x, y, z);
            }
        } else if (type == "vec4") {
            std::istringstream iss(value_str);
            float x, y, z, w;
            if (iss >> x >> y >> z >> w) {
                return glm::vec4(x, y, z, w);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing variable value: " << e.what() << std::endl;
    }
    
    // Return default value for type
    if (type == "bool") return false;
    if (type == "int") return 0;
    if (type == "float") return 0.0f;
    if (type == "string") return std::string("");
    if (type == "vec2") return glm::vec2(0.0f);
    if (type == "vec3") return glm::vec3(0.0f);
    if (type == "vec4") return glm::vec4(0.0f);
    
    return std::string("");
}

// === Serialization ===

nlohmann::json GlobalsManager::SerializeToJson() const {
    nlohmann::json json;

    // Serialize autoloads
    nlohmann::json autoloads_json = nlohmann::json::array();
    for (const auto& [name, autoload] : m_autoloads) {
        nlohmann::json autoload_json;
        autoload_json["name"] = autoload.name;
        autoload_json["script_path"] = autoload.script_path;
        autoload_json["script_type"] = autoload.script_type;
        autoload_json["enabled"] = autoload.enabled;
        autoload_json["description"] = autoload.description;
        autoloads_json.push_back(autoload_json);
    }
    json["autoloads"] = autoloads_json;

    // Serialize global variables
    nlohmann::json variables_json = nlohmann::json::array();
    for (const auto& [name, variable] : m_global_variables) {
        nlohmann::json var_json;
        var_json["name"] = variable.name;
        var_json["type"] = variable.type;
        var_json["description"] = variable.description;
        var_json["is_exported"] = variable.is_exported;

        // Serialize value based on type
        if (variable.type == "bool") {
            var_json["value"] = std::get<bool>(variable.value);
            var_json["default_value"] = std::get<bool>(variable.default_value);
        } else if (variable.type == "int") {
            var_json["value"] = std::get<int>(variable.value);
            var_json["default_value"] = std::get<int>(variable.default_value);
        } else if (variable.type == "float") {
            var_json["value"] = std::get<float>(variable.value);
            var_json["default_value"] = std::get<float>(variable.default_value);
        } else if (variable.type == "string") {
            var_json["value"] = std::get<std::string>(variable.value);
            var_json["default_value"] = std::get<std::string>(variable.default_value);
        } else if (variable.type == "vec2") {
            auto vec = std::get<glm::vec2>(variable.value);
            var_json["value"] = {vec.x, vec.y};
            auto default_vec = std::get<glm::vec2>(variable.default_value);
            var_json["default_value"] = {default_vec.x, default_vec.y};
        } else if (variable.type == "vec3") {
            auto vec = std::get<glm::vec3>(variable.value);
            var_json["value"] = {vec.x, vec.y, vec.z};
            auto default_vec = std::get<glm::vec3>(variable.default_value);
            var_json["default_value"] = {default_vec.x, default_vec.y, default_vec.z};
        } else if (variable.type == "vec4") {
            auto vec = std::get<glm::vec4>(variable.value);
            var_json["value"] = {vec.x, vec.y, vec.z, vec.w};
            auto default_vec = std::get<glm::vec4>(variable.default_value);
            var_json["default_value"] = {default_vec.x, default_vec.y, default_vec.z, default_vec.w};
        }

        variables_json.push_back(var_json);
    }
    json["global_variables"] = variables_json;

    return json;
}

bool GlobalsManager::DeserializeFromJson(const nlohmann::json& json) {
    try {
        Clear();

        // Deserialize autoloads
        if (json.contains("autoloads") && json["autoloads"].is_array()) {
            for (const auto& autoload_json : json["autoloads"]) {
                AutoloadScript autoload;
                autoload.name = autoload_json.value("name", "");
                autoload.script_path = autoload_json.value("script_path", "");
                autoload.script_type = autoload_json.value("script_type", "");
                autoload.enabled = autoload_json.value("enabled", true);
                autoload.description = autoload_json.value("description", "");

                if (!autoload.name.empty() && !autoload.script_path.empty()) {
                    RegisterAutoload(autoload);
                }
            }
        }

        // Deserialize global variables
        if (json.contains("global_variables") && json["global_variables"].is_array()) {
            for (const auto& var_json : json["global_variables"]) {
                GlobalVariable variable;
                variable.name = var_json.value("name", "");
                variable.type = var_json.value("type", "");
                variable.description = var_json.value("description", "");
                variable.is_exported = var_json.value("is_exported", true);

                // Deserialize value based on type
                if (variable.type == "bool") {
                    variable.value = var_json.value("value", false);
                    variable.default_value = var_json.value("default_value", false);
                } else if (variable.type == "int") {
                    variable.value = var_json.value("value", 0);
                    variable.default_value = var_json.value("default_value", 0);
                } else if (variable.type == "float") {
                    variable.value = var_json.value("value", 0.0f);
                    variable.default_value = var_json.value("default_value", 0.0f);
                } else if (variable.type == "string") {
                    variable.value = var_json.value("value", std::string(""));
                    variable.default_value = var_json.value("default_value", std::string(""));
                } else if (variable.type == "vec2") {
                    auto value_array = var_json.value("value", std::vector<float>{0.0f, 0.0f});
                    if (value_array.size() >= 2) {
                        variable.value = glm::vec2(value_array[0], value_array[1]);
                    } else {
                        variable.value = glm::vec2(0.0f);
                    }
                    auto default_array = var_json.value("default_value", std::vector<float>{0.0f, 0.0f});
                    if (default_array.size() >= 2) {
                        variable.default_value = glm::vec2(default_array[0], default_array[1]);
                    } else {
                        variable.default_value = glm::vec2(0.0f);
                    }
                } else if (variable.type == "vec3") {
                    auto value_array = var_json.value("value", std::vector<float>{0.0f, 0.0f, 0.0f});
                    if (value_array.size() >= 3) {
                        variable.value = glm::vec3(value_array[0], value_array[1], value_array[2]);
                    } else {
                        variable.value = glm::vec3(0.0f);
                    }
                    auto default_array = var_json.value("default_value", std::vector<float>{0.0f, 0.0f, 0.0f});
                    if (default_array.size() >= 3) {
                        variable.default_value = glm::vec3(default_array[0], default_array[1], default_array[2]);
                    } else {
                        variable.default_value = glm::vec3(0.0f);
                    }
                } else if (variable.type == "vec4") {
                    auto value_array = var_json.value("value", std::vector<float>{0.0f, 0.0f, 0.0f, 0.0f});
                    if (value_array.size() >= 4) {
                        variable.value = glm::vec4(value_array[0], value_array[1], value_array[2], value_array[3]);
                    } else {
                        variable.value = glm::vec4(0.0f);
                    }
                    auto default_array = var_json.value("default_value", std::vector<float>{0.0f, 0.0f, 0.0f, 0.0f});
                    if (default_array.size() >= 4) {
                        variable.default_value = glm::vec4(default_array[0], default_array[1], default_array[2], default_array[3]);
                    } else {
                        variable.default_value = glm::vec4(0.0f);
                    }
                }

                if (!variable.name.empty() && !variable.type.empty()) {
                    RegisterGlobalVariable(variable);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing globals: " << e.what() << std::endl;
        return false;
    }
}

void GlobalsManager::Clear() {
    CleanupAutoloads();
    m_autoloads.clear();
    m_global_variables.clear();
    std::cout << "Cleared all globals data" << std::endl;
}

} // namespace Lupine
