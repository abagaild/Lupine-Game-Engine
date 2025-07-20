#include "lupine/scriptableobjects/ScriptableObjectManager.h"
#include <algorithm>
#include <iostream>

namespace Lupine {

ScriptableObjectManager& ScriptableObjectManager::Instance() {
    static ScriptableObjectManager instance;
    return instance;
}

void ScriptableObjectManager::Initialize() {
    // Initialize any required resources
}

void ScriptableObjectManager::Shutdown() {
    Clear();
}

std::shared_ptr<ScriptableObjectTemplate> ScriptableObjectManager::CreateTemplate(const std::string& name) {
    // Check if name already exists
    if (TemplateNameExists(name)) {
        return nullptr;
    }
    
    auto template_obj = std::make_shared<ScriptableObjectTemplate>(name);
    m_templates[template_obj->GetUUID()] = template_obj;
    m_template_name_to_uuid[name] = template_obj->GetUUID();
    
    return template_obj;
}

void ScriptableObjectManager::RemoveTemplate(const UUID& template_uuid) {
    auto it = m_templates.find(template_uuid);
    if (it != m_templates.end()) {
        // Remove from name lookup
        const std::string& name = it->second->GetName();
        m_template_name_to_uuid.erase(name);
        
        // Remove all instances for this template
        RemoveInstancesForTemplate(template_uuid);
        
        // Remove template
        m_templates.erase(it);
    }
}

std::shared_ptr<ScriptableObjectTemplate> ScriptableObjectManager::GetTemplate(const UUID& uuid) const {
    auto it = m_templates.find(uuid);
    return (it != m_templates.end()) ? it->second : nullptr;
}

std::shared_ptr<ScriptableObjectTemplate> ScriptableObjectManager::GetTemplate(const std::string& name) const {
    auto it = m_template_name_to_uuid.find(name);
    if (it != m_template_name_to_uuid.end()) {
        return GetTemplate(it->second);
    }
    return nullptr;
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectManager::CreateInstance(const UUID& template_uuid, const std::string& name) {
    auto template_obj = GetTemplate(template_uuid);
    if (!template_obj) {
        return nullptr;
    }
    
    // Check if instance name already exists for this template
    if (InstanceNameExists(template_obj->GetName(), name)) {
        return nullptr;
    }
    
    auto instance = std::make_shared<ScriptableObjectInstance>(template_obj, name);
    m_instances[instance->GetUUID()] = instance;
    
    // Update name lookup
    const std::string& template_name = template_obj->GetName();
    m_instance_name_to_uuid[template_name][name] = instance->GetUUID();
    
    return instance;
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectManager::CreateInstance(const std::string& template_name, const std::string& instance_name) {
    auto template_obj = GetTemplate(template_name);
    if (!template_obj) {
        return nullptr;
    }
    
    return CreateInstance(template_obj->GetUUID(), instance_name);
}

void ScriptableObjectManager::RemoveInstance(const UUID& instance_uuid) {
    auto it = m_instances.find(instance_uuid);
    if (it != m_instances.end()) {
        auto instance = it->second;
        auto template_obj = instance->GetTemplate();
        
        if (template_obj) {
            // Remove from name lookup
            const std::string& template_name = template_obj->GetName();
            const std::string& instance_name = instance->GetName();
            
            auto template_it = m_instance_name_to_uuid.find(template_name);
            if (template_it != m_instance_name_to_uuid.end()) {
                template_it->second.erase(instance_name);
                if (template_it->second.empty()) {
                    m_instance_name_to_uuid.erase(template_it);
                }
            }
        }
        
        // Remove instance
        m_instances.erase(it);
    }
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectManager::GetInstance(const UUID& uuid) const {
    auto it = m_instances.find(uuid);
    return (it != m_instances.end()) ? it->second : nullptr;
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectManager::GetInstance(const std::string& template_name, const std::string& instance_name) const {
    auto template_it = m_instance_name_to_uuid.find(template_name);
    if (template_it != m_instance_name_to_uuid.end()) {
        auto instance_it = template_it->second.find(instance_name);
        if (instance_it != template_it->second.end()) {
            return GetInstance(instance_it->second);
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<ScriptableObjectInstance>> ScriptableObjectManager::GetInstancesForTemplate(const UUID& template_uuid) const {
    std::vector<std::shared_ptr<ScriptableObjectInstance>> instances;
    
    for (const auto& pair : m_instances) {
        auto instance = pair.second;
        if (instance->GetTemplate() && instance->GetTemplate()->GetUUID() == template_uuid) {
            instances.push_back(instance);
        }
    }
    
    return instances;
}

std::vector<std::shared_ptr<ScriptableObjectInstance>> ScriptableObjectManager::GetInstancesForTemplate(const std::string& template_name) const {
    auto template_obj = GetTemplate(template_name);
    if (template_obj) {
        return GetInstancesForTemplate(template_obj->GetUUID());
    }
    return {};
}

bool ScriptableObjectManager::TemplateNameExists(const std::string& name) const {
    return m_template_name_to_uuid.find(name) != m_template_name_to_uuid.end();
}

bool ScriptableObjectManager::InstanceNameExists(const std::string& template_name, const std::string& instance_name) const {
    auto template_it = m_instance_name_to_uuid.find(template_name);
    if (template_it != m_instance_name_to_uuid.end()) {
        return template_it->second.find(instance_name) != template_it->second.end();
    }
    return false;
}

bool ScriptableObjectManager::SaveToFile(const std::string& file_path) const {
    try {
        JsonNode json = ToJson();
        return JsonUtils::SaveToFile(json, file_path, true);
    } catch (const std::exception& e) {
        std::cerr << "Error saving scriptable objects: " << e.what() << std::endl;
        return false;
    }
}

bool ScriptableObjectManager::LoadFromFile(const std::string& file_path) {
    try {
        JsonNode json = JsonUtils::LoadFromFile(file_path);
        FromJson(json);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading scriptable objects: " << e.what() << std::endl;
        return false;
    }
}

JsonNode ScriptableObjectManager::ToJson() const {
    JsonNode json;
    
    // Serialize templates
    std::vector<JsonNode> templates_json;
    for (const auto& pair : m_templates) {
        templates_json.push_back(pair.second->ToJson());
    }
    json["templates"] = templates_json;
    
    // Serialize instances
    std::vector<JsonNode> instances_json;
    for (const auto& pair : m_instances) {
        instances_json.push_back(pair.second->ToJson());
    }
    json["instances"] = instances_json;
    
    return json;
}

void ScriptableObjectManager::FromJson(const JsonNode& json) {
    Clear();
    
    // Load templates first
    if (json.HasKey("templates") && json["templates"].IsArray()) {
        const auto& templates_array = json["templates"].AsArray();
        for (const auto& template_json : templates_array) {
            auto template_obj = std::make_shared<ScriptableObjectTemplate>();
            template_obj->FromJson(template_json);
            
            m_templates[template_obj->GetUUID()] = template_obj;
            m_template_name_to_uuid[template_obj->GetName()] = template_obj->GetUUID();
        }
    }
    
    // Load instances
    if (json.HasKey("instances") && json["instances"].IsArray()) {
        const auto& instances_array = json["instances"].AsArray();
        for (const auto& instance_json : instances_array) {
            auto instance = std::make_shared<ScriptableObjectInstance>();
            instance->FromJson(instance_json, m_templates);
            
            if (instance->GetTemplate()) {
                m_instances[instance->GetUUID()] = instance;
                
                const std::string& template_name = instance->GetTemplate()->GetName();
                const std::string& instance_name = instance->GetName();
                m_instance_name_to_uuid[template_name][instance_name] = instance->GetUUID();
            }
        }
    }
}

void ScriptableObjectManager::Clear() {
    m_templates.clear();
    m_instances.clear();
    m_template_name_to_uuid.clear();
    m_instance_name_to_uuid.clear();
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectManager::GetScriptableObject(const std::string& template_name, const std::string& instance_name) const {
    return GetInstance(template_name, instance_name);
}

void ScriptableObjectManager::RemoveInstancesForTemplate(const UUID& template_uuid) {
    auto instances_to_remove = GetInstancesForTemplate(template_uuid);
    for (auto instance : instances_to_remove) {
        RemoveInstance(instance->GetUUID());
    }
}

} // namespace Lupine
