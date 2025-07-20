#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "lupine/core/UUID.h"
#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include "lupine/scriptableobjects/ScriptableObjectInstance.h"
#include "lupine/serialization/JsonUtils.h"

namespace Lupine {

/**
 * @brief Manager for scriptable object templates and instances
 * 
 * Provides centralized management of all scriptable objects in the project.
 * Handles serialization, script binding, and access patterns.
 */
class ScriptableObjectManager {
public:
    /**
     * @brief Get singleton instance
     */
    static ScriptableObjectManager& Instance();
    
    /**
     * @brief Initialize the manager
     */
    void Initialize();
    
    /**
     * @brief Shutdown the manager
     */
    void Shutdown();
    
    /**
     * @brief Create a new template
     */
    std::shared_ptr<ScriptableObjectTemplate> CreateTemplate(const std::string& name);
    
    /**
     * @brief Remove a template (and all its instances)
     */
    void RemoveTemplate(const UUID& template_uuid);
    
    /**
     * @brief Get template by UUID
     */
    std::shared_ptr<ScriptableObjectTemplate> GetTemplate(const UUID& uuid) const;
    
    /**
     * @brief Get template by name
     */
    std::shared_ptr<ScriptableObjectTemplate> GetTemplate(const std::string& name) const;
    
    /**
     * @brief Get all templates
     */
    const std::unordered_map<UUID, std::shared_ptr<ScriptableObjectTemplate>>& GetTemplates() const { return m_templates; }
    
    /**
     * @brief Create a new instance from template
     */
    std::shared_ptr<ScriptableObjectInstance> CreateInstance(const UUID& template_uuid, const std::string& name);
    
    /**
     * @brief Create a new instance from template by name
     */
    std::shared_ptr<ScriptableObjectInstance> CreateInstance(const std::string& template_name, const std::string& instance_name);
    
    /**
     * @brief Remove an instance
     */
    void RemoveInstance(const UUID& instance_uuid);
    
    /**
     * @brief Get instance by UUID
     */
    std::shared_ptr<ScriptableObjectInstance> GetInstance(const UUID& uuid) const;
    
    /**
     * @brief Get instance by template and instance name
     */
    std::shared_ptr<ScriptableObjectInstance> GetInstance(const std::string& template_name, const std::string& instance_name) const;
    
    /**
     * @brief Get all instances
     */
    const std::unordered_map<UUID, std::shared_ptr<ScriptableObjectInstance>>& GetInstances() const { return m_instances; }
    
    /**
     * @brief Get instances for a specific template
     */
    std::vector<std::shared_ptr<ScriptableObjectInstance>> GetInstancesForTemplate(const UUID& template_uuid) const;
    
    /**
     * @brief Get instances for a specific template by name
     */
    std::vector<std::shared_ptr<ScriptableObjectInstance>> GetInstancesForTemplate(const std::string& template_name) const;
    
    /**
     * @brief Check if template name exists
     */
    bool TemplateNameExists(const std::string& name) const;
    
    /**
     * @brief Check if instance name exists for template
     */
    bool InstanceNameExists(const std::string& template_name, const std::string& instance_name) const;
    
    /**
     * @brief Save all scriptable objects to file
     */
    bool SaveToFile(const std::string& file_path) const;
    
    /**
     * @brief Load all scriptable objects from file
     */
    bool LoadFromFile(const std::string& file_path);
    
    /**
     * @brief Serialize to JSON
     */
    JsonNode ToJson() const;
    
    /**
     * @brief Deserialize from JSON
     */
    void FromJson(const JsonNode& json);
    
    /**
     * @brief Clear all templates and instances
     */
    void Clear();
    
    /**
     * @brief Get scriptable object for script access (SO.TemplateName.InstanceName)
     * Used by Python/Lua bindings
     */
    std::shared_ptr<ScriptableObjectInstance> GetScriptableObject(const std::string& template_name, const std::string& instance_name) const;

private:
    /**
     * @brief Private constructor for singleton
     */
    ScriptableObjectManager() = default;
    
    /**
     * @brief Remove instances for a template
     */
    void RemoveInstancesForTemplate(const UUID& template_uuid);

private:
    std::unordered_map<UUID, std::shared_ptr<ScriptableObjectTemplate>> m_templates;
    std::unordered_map<UUID, std::shared_ptr<ScriptableObjectInstance>> m_instances;
    
    // Name-based lookup caches for faster access
    std::unordered_map<std::string, UUID> m_template_name_to_uuid;
    std::unordered_map<std::string, std::unordered_map<std::string, UUID>> m_instance_name_to_uuid; // template_name -> instance_name -> uuid
};

} // namespace Lupine
