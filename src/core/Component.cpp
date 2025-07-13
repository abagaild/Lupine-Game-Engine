#include "lupine/core/Component.h"
#include "lupine/core/Node.h"
#include <algorithm>
#include <iostream>

namespace Lupine {

Component::Component(const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_owner(nullptr)
    , m_active(true) {
    InitializeExportVariables();
}

void Component::AddExportVariable(const std::string& name, const ExportValue& value, const std::string& description, ExportVariableType type) {
    // If type is String (default), try to auto-detect the actual type
    if (type == ExportVariableType::String) {
        type = GetExportVariableType(value);
    }
    m_export_variables[name] = ExportVariable(name, value, description, type);
}

void Component::AddEnumExportVariable(const std::string& name, int value, const std::string& description, const std::vector<std::string>& options) {
    m_export_variables[name] = ExportVariable(name, value, description, ExportVariableType::Enum, options);
}

bool Component::SetExportVariable(const std::string& name, const ExportValue& value) {
    auto it = m_export_variables.find(name);
    if (it != m_export_variables.end()) {
        it->second.value = value;
        return true;
    }
    return false;
}

const ExportValue* Component::GetExportVariable(const std::string& name) const {
    auto it = m_export_variables.find(name);
    if (it != m_export_variables.end()) {
        return &it->second.value;
    }
    return nullptr;
}

bool Component::ResetExportVariable(const std::string& name) {
    auto it = m_export_variables.find(name);
    if (it != m_export_variables.end()) {
        it->second.value = it->second.defaultValue;
        return true;
    }
    return false;
}

// ComponentRegistry implementation
ComponentRegistry& ComponentRegistry::Instance() {
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::RegisterComponent(const std::string& name, const ComponentInfo& info) {
    m_components[name] = info;
}

std::unique_ptr<Component> ComponentRegistry::CreateComponent(const std::string& name) {
    auto it = m_components.find(name);
    if (it != m_components.end()) {
        return it->second.factory();
    }
    return nullptr;
}

std::vector<std::string> ComponentRegistry::GetComponentNames() const {
    // Cache component names for better performance
    static std::vector<std::string> cachedNames;
    static size_t lastSize = 0;

    // Only rebuild cache if registry has changed
    if (m_components.size() != lastSize) {
        cachedNames.clear();
        cachedNames.reserve(m_components.size());
        for (const auto& pair : m_components) {
            cachedNames.push_back(pair.first);
        }
        lastSize = m_components.size();
    }

    return cachedNames;
}

const ComponentInfo* ComponentRegistry::GetComponentInfo(const std::string& name) const {
    auto it = m_components.find(name);
    if (it != m_components.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> ComponentRegistry::GetComponentsByCategory(const std::string& category) const {
    std::vector<std::string> names;
    for (const auto& pair : m_components) {
        if (pair.second.category == category) {
            names.push_back(pair.first);
        }
    }
    return names;
}

std::vector<std::string> ComponentRegistry::GetCategories() const {
    std::vector<std::string> categories;
    for (const auto& pair : m_components) {
        if (std::find(categories.begin(), categories.end(), pair.second.category) == categories.end()) {
            categories.push_back(pair.second.category);
        }
    }
    return categories;
}

} // namespace Lupine
