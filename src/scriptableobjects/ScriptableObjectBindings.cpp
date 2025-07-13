#include "lupine/scriptableobjects/ScriptableObjectBindings.h"
#include "lupine/scriptableobjects/ScriptableObjectManager.h"
#include "lupine/scriptableobjects/ScriptableObjectInstance.h"
#include "lupine/scriptableobjects/ScriptableObjectTemplate.h"
#include <pybind11/stl.h>
#include <iostream>
#include <variant>
#include <glm/glm.hpp>

namespace py = pybind11;

namespace Lupine {

// Python Bindings Implementation

void ScriptableObjectPythonBindings::Initialize(py::dict& globals) {
    // Create SO accessor
    auto so_accessor = CreateSOAccessor();
    globals["SO"] = so_accessor;
}

py::object ScriptableObjectPythonBindings::CreateSOAccessor() {
    py::module_ types = py::module_::import("types");
    py::object SimpleNamespace = types.attr("SimpleNamespace");
    py::object so_obj = SimpleNamespace();
    
    auto& manager = ScriptableObjectManager::Instance();
    const auto& templates = manager.GetTemplates();
    
    for (const auto& pair : templates) {
        auto template_obj = pair.second;
        const std::string& template_name = template_obj->GetName();
        
        // Create template accessor
        py::object template_accessor = SimpleNamespace();
        
        // Get instances for this template
        auto instances = manager.GetInstancesForTemplate(template_obj->GetUUID());
        for (auto instance : instances) {
            const std::string& instance_name = instance->GetName();
            
            // Create instance wrapper
            auto wrapper = std::make_shared<InstanceWrapper>(instance);
            py::object instance_obj = py::cast(wrapper);
            
            // Add to template accessor
            template_accessor.attr(instance_name.c_str()) = instance_obj;
        }
        
        // Add template accessor to SO
        so_obj.attr(template_name.c_str()) = template_accessor;
    }
    
    return so_obj;
}

// SOAccessor implementation
py::object ScriptableObjectPythonBindings::SOAccessor::GetTemplate(const std::string& template_name) {
    py::module_ types = py::module_::import("types");
    py::object SimpleNamespace = types.attr("SimpleNamespace");
    py::object template_accessor = SimpleNamespace();
    
    auto& manager = ScriptableObjectManager::Instance();
    auto instances = manager.GetInstancesForTemplate(template_name);
    
    for (auto instance : instances) {
        const std::string& instance_name = instance->GetName();
        auto wrapper = std::make_shared<InstanceWrapper>(instance);
        py::object instance_obj = py::cast(wrapper);
        template_accessor.attr(instance_name.c_str()) = instance_obj;
    }
    
    return template_accessor;
}

// TemplateAccessor implementation
ScriptableObjectPythonBindings::TemplateAccessor::TemplateAccessor(const std::string& template_name)
    : m_template_name(template_name) {
}

py::object ScriptableObjectPythonBindings::TemplateAccessor::GetInstance(const std::string& instance_name) {
    auto& manager = ScriptableObjectManager::Instance();
    auto instance = manager.GetInstance(m_template_name, instance_name);
    
    if (instance) {
        auto wrapper = std::make_shared<InstanceWrapper>(instance);
        return py::cast(wrapper);
    }
    
    return py::none();
}

// InstanceWrapper implementation
ScriptableObjectPythonBindings::InstanceWrapper::InstanceWrapper(std::shared_ptr<ScriptableObjectInstance> instance)
    : m_instance(instance) {
}

py::object ScriptableObjectPythonBindings::InstanceWrapper::GetField(const std::string& field_name) {
    if (!m_instance || !m_instance->HasField(field_name)) {
        return py::none();
    }
    
    auto template_obj = m_instance->GetTemplate();
    if (!template_obj) return py::none();
    
    const auto* field = template_obj->GetField(field_name);
    if (!field) return py::none();
    
    auto value = m_instance->GetFieldValue(field_name);
    
    switch (field->type) {
        case ScriptableObjectFieldType::String:
        case ScriptableObjectFieldType::FilePath:
        case ScriptableObjectFieldType::NodeReference:
            return py::cast(std::get<std::string>(value));
        case ScriptableObjectFieldType::Integer:
            return py::cast(std::get<int>(value));
        case ScriptableObjectFieldType::Float:
            return py::cast(std::get<float>(value));
        case ScriptableObjectFieldType::Boolean:
            return py::cast(std::get<bool>(value));
        case ScriptableObjectFieldType::Vector2: {
            auto vec = std::get<glm::vec2>(value);
            return py::make_tuple(vec.x, vec.y);
        }
        case ScriptableObjectFieldType::Vector3: {
            auto vec = std::get<glm::vec3>(value);
            return py::make_tuple(vec.x, vec.y, vec.z);
        }
        case ScriptableObjectFieldType::Vector4:
        case ScriptableObjectFieldType::Color: {
            auto vec = std::get<glm::vec4>(value);
            return py::make_tuple(vec.x, vec.y, vec.z, vec.w);
        }
    }
    
    return py::none();
}

void ScriptableObjectPythonBindings::InstanceWrapper::SetField(const std::string& field_name, const py::object& value) {
    if (!m_instance || !m_instance->HasField(field_name)) {
        return;
    }
    
    auto template_obj = m_instance->GetTemplate();
    if (!template_obj) return;
    
    const auto* field = template_obj->GetField(field_name);
    if (!field) return;
    
    try {
        switch (field->type) {
            case ScriptableObjectFieldType::String:
            case ScriptableObjectFieldType::FilePath:
            case ScriptableObjectFieldType::NodeReference:
                m_instance->SetFieldValue(field_name, value.cast<std::string>());
                break;
            case ScriptableObjectFieldType::Integer:
                m_instance->SetFieldValue(field_name, value.cast<int>());
                break;
            case ScriptableObjectFieldType::Float:
                m_instance->SetFieldValue(field_name, value.cast<float>());
                break;
            case ScriptableObjectFieldType::Boolean:
                m_instance->SetFieldValue(field_name, value.cast<bool>());
                break;
            case ScriptableObjectFieldType::Vector2: {
                auto tuple = value.cast<py::tuple>();
                if (tuple.size() >= 2) {
                    glm::vec2 vec(tuple[0].cast<float>(), tuple[1].cast<float>());
                    m_instance->SetFieldValue(field_name, vec);
                }
                break;
            }
            case ScriptableObjectFieldType::Vector3: {
                auto tuple = value.cast<py::tuple>();
                if (tuple.size() >= 3) {
                    glm::vec3 vec(tuple[0].cast<float>(), tuple[1].cast<float>(), tuple[2].cast<float>());
                    m_instance->SetFieldValue(field_name, vec);
                }
                break;
            }
            case ScriptableObjectFieldType::Vector4:
            case ScriptableObjectFieldType::Color: {
                auto tuple = value.cast<py::tuple>();
                if (tuple.size() >= 4) {
                    glm::vec4 vec(tuple[0].cast<float>(), tuple[1].cast<float>(), 
                                 tuple[2].cast<float>(), tuple[3].cast<float>());
                    m_instance->SetFieldValue(field_name, vec);
                }
                break;
            }
        }
    } catch (const py::cast_error& e) {
        std::cerr << "ScriptableObject SetField cast error: " << e.what() << std::endl;
    }
}

std::string ScriptableObjectPythonBindings::InstanceWrapper::GetFieldAsString(const std::string& field_name) {
    return m_instance ? m_instance->GetFieldValueAsString(field_name) : "";
}

int ScriptableObjectPythonBindings::InstanceWrapper::GetFieldAsInt(const std::string& field_name) {
    return m_instance ? m_instance->GetFieldValueAsInt(field_name) : 0;
}

float ScriptableObjectPythonBindings::InstanceWrapper::GetFieldAsFloat(const std::string& field_name) {
    return m_instance ? m_instance->GetFieldValueAsFloat(field_name) : 0.0f;
}

bool ScriptableObjectPythonBindings::InstanceWrapper::GetFieldAsBool(const std::string& field_name) {
    return m_instance ? m_instance->GetFieldValueAsBool(field_name) : false;
}

void ScriptableObjectPythonBindings::InstanceWrapper::SetFieldAsString(const std::string& field_name, const std::string& value) {
    if (m_instance) {
        m_instance->SetFieldValueFromString(field_name, value);
    }
}

void ScriptableObjectPythonBindings::InstanceWrapper::SetFieldAsInt(const std::string& field_name, int value) {
    if (m_instance) {
        m_instance->SetFieldValueAsInt(field_name, value);
    }
}

void ScriptableObjectPythonBindings::InstanceWrapper::SetFieldAsFloat(const std::string& field_name, float value) {
    if (m_instance) {
        m_instance->SetFieldValueAsFloat(field_name, value);
    }
}

void ScriptableObjectPythonBindings::InstanceWrapper::SetFieldAsBool(const std::string& field_name, bool value) {
    if (m_instance) {
        m_instance->SetFieldValueAsBool(field_name, value);
    }
}

// Lua Bindings Implementation

void ScriptableObjectLuaBindings::Initialize(lua_State* L) {
    // Create SO table
    lua_newtable(L);

    // Set metatable for SO to handle dynamic access
    lua_newtable(L);
    lua_pushcfunction(L, [](lua_State* L) -> int {
        // __index metamethod for SO.TemplateName access
        const char* template_name = luaL_checkstring(L, 2);

        auto& manager = ScriptableObjectManager::Instance();
        auto template_obj = manager.GetTemplate(template_name);

        if (!template_obj) {
            lua_pushnil(L);
            return 1;
        }

        // Create template table
        lua_newtable(L);

        // Set metatable for template to handle instance access
        lua_newtable(L);
        lua_pushstring(L, template_name);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            // __index metamethod for SO.TemplateName.InstanceName access
            const char* template_name = lua_tostring(L, lua_upvalueindex(1));
            const char* instance_name = luaL_checkstring(L, 2);

            auto& manager = ScriptableObjectManager::Instance();
            auto instance = manager.GetInstance(template_name, instance_name);

            if (instance) {
                PushInstanceToStack(L, instance);
            } else {
                lua_pushnil(L);
            }
            return 1;
        }, 1);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);

        return 1;
    });
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    // Set SO as global
    lua_setglobal(L, "SO");

    // Register instance methods
    luaL_newmetatable(L, "ScriptableObjectInstance");

    lua_pushcfunction(L, GetField);
    lua_setfield(L, -2, "GetField");

    lua_pushcfunction(L, SetField);
    lua_setfield(L, -2, "SetField");

    lua_pushcfunction(L, GetFieldAsString);
    lua_setfield(L, -2, "GetFieldAsString");

    lua_pushcfunction(L, GetFieldAsNumber);
    lua_setfield(L, -2, "GetFieldAsNumber");

    lua_pushcfunction(L, GetFieldAsBool);
    lua_setfield(L, -2, "GetFieldAsBool");

    lua_pushcfunction(L, SetFieldAsString);
    lua_setfield(L, -2, "SetFieldAsString");

    lua_pushcfunction(L, SetFieldAsNumber);
    lua_setfield(L, -2, "SetFieldAsNumber");

    lua_pushcfunction(L, SetFieldAsBool);
    lua_setfield(L, -2, "SetFieldAsBool");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);
}

int ScriptableObjectLuaBindings::GetField(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        lua_pushnil(L);
        return 1;
    }

    const char* field_name = luaL_checkstring(L, 2);
    if (!instance->HasField(field_name)) {
        lua_pushnil(L);
        return 1;
    }

    auto template_obj = instance->GetTemplate();
    if (!template_obj) {
        lua_pushnil(L);
        return 1;
    }

    const auto* field = template_obj->GetField(field_name);
    if (!field) {
        lua_pushnil(L);
        return 1;
    }

    auto value = instance->GetFieldValue(field_name);

    switch (field->type) {
        case ScriptableObjectFieldType::String:
        case ScriptableObjectFieldType::FilePath:
        case ScriptableObjectFieldType::NodeReference:
            lua_pushstring(L, std::get<std::string>(value).c_str());
            break;
        case ScriptableObjectFieldType::Integer:
            lua_pushinteger(L, static_cast<lua_Integer>(std::get<int>(value)));
            break;
        case ScriptableObjectFieldType::Float:
            lua_pushnumber(L, std::get<float>(value));
            break;
        case ScriptableObjectFieldType::Boolean:
            lua_pushboolean(L, std::get<bool>(value));
            break;
        case ScriptableObjectFieldType::Vector2: {
            auto vec = std::get<glm::vec2>(value);
            lua_newtable(L);
            lua_pushnumber(L, vec.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, vec.y);
            lua_rawseti(L, -2, 2);
            break;
        }
        case ScriptableObjectFieldType::Vector3: {
            auto vec = std::get<glm::vec3>(value);
            lua_newtable(L);
            lua_pushnumber(L, vec.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, vec.y);
            lua_rawseti(L, -2, 2);
            lua_pushnumber(L, vec.z);
            lua_rawseti(L, -2, 3);
            break;
        }
        case ScriptableObjectFieldType::Vector4:
        case ScriptableObjectFieldType::Color: {
            auto vec = std::get<glm::vec4>(value);
            lua_newtable(L);
            lua_pushnumber(L, vec.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, vec.y);
            lua_rawseti(L, -2, 2);
            lua_pushnumber(L, vec.z);
            lua_rawseti(L, -2, 3);
            lua_pushnumber(L, vec.w);
            lua_rawseti(L, -2, 4);
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

int ScriptableObjectLuaBindings::SetField(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        return 0;
    }

    const char* field_name = luaL_checkstring(L, 2);
    if (!instance->HasField(field_name)) {
        return 0;
    }

    auto template_obj = instance->GetTemplate();
    if (!template_obj) {
        return 0;
    }

    const auto* field = template_obj->GetField(field_name);
    if (!field) {
        return 0;
    }

    switch (field->type) {
        case ScriptableObjectFieldType::String:
        case ScriptableObjectFieldType::FilePath:
        case ScriptableObjectFieldType::NodeReference:
            if (lua_isstring(L, 3)) {
                instance->SetFieldValue(field_name, std::string(lua_tostring(L, 3)));
            }
            break;
        case ScriptableObjectFieldType::Integer:
            if (lua_isinteger(L, 3)) {
                instance->SetFieldValue(field_name, static_cast<int>(lua_tointeger(L, 3)));
            }
            break;
        case ScriptableObjectFieldType::Float:
            if (lua_isnumber(L, 3)) {
                instance->SetFieldValue(field_name, static_cast<float>(lua_tonumber(L, 3)));
            }
            break;
        case ScriptableObjectFieldType::Boolean:
            if (lua_isboolean(L, 3)) {
                instance->SetFieldValue(field_name, lua_toboolean(L, 3) != 0);
            }
            break;
        case ScriptableObjectFieldType::Vector2:
            if (lua_istable(L, 3)) {
                lua_rawgeti(L, 3, 1);
                lua_rawgeti(L, 3, 2);
                if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    glm::vec2 vec(lua_tonumber(L, -2), lua_tonumber(L, -1));
                    instance->SetFieldValue(field_name, vec);
                }
                lua_pop(L, 2);
            }
            break;
        case ScriptableObjectFieldType::Vector3:
            if (lua_istable(L, 3)) {
                lua_rawgeti(L, 3, 1);
                lua_rawgeti(L, 3, 2);
                lua_rawgeti(L, 3, 3);
                if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    glm::vec3 vec(lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
                    instance->SetFieldValue(field_name, vec);
                }
                lua_pop(L, 3);
            }
            break;
        case ScriptableObjectFieldType::Vector4:
        case ScriptableObjectFieldType::Color:
            if (lua_istable(L, 3)) {
                lua_rawgeti(L, 3, 1);
                lua_rawgeti(L, 3, 2);
                lua_rawgeti(L, 3, 3);
                lua_rawgeti(L, 3, 4);
                if (lua_isnumber(L, -4) && lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    glm::vec4 vec(lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
                    instance->SetFieldValue(field_name, vec);
                }
                lua_pop(L, 4);
            }
            break;
    }

    return 0;
}

int ScriptableObjectLuaBindings::GetFieldAsString(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        lua_pushstring(L, "");
        return 1;
    }

    const char* field_name = luaL_checkstring(L, 2);
    std::string value = instance->GetFieldValueAsString(field_name);
    lua_pushstring(L, value.c_str());
    return 1;
}

int ScriptableObjectLuaBindings::GetFieldAsNumber(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    const char* field_name = luaL_checkstring(L, 2);
    float value = instance->GetFieldValueAsFloat(field_name);
    lua_pushnumber(L, value);
    return 1;
}

int ScriptableObjectLuaBindings::GetFieldAsBool(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        lua_pushboolean(L, 0);
        return 1;
    }

    const char* field_name = luaL_checkstring(L, 2);
    bool value = instance->GetFieldValueAsBool(field_name);
    lua_pushboolean(L, value);
    return 1;
}

int ScriptableObjectLuaBindings::SetFieldAsString(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        return 0;
    }

    const char* field_name = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    instance->SetFieldValueFromString(field_name, value);
    return 0;
}

int ScriptableObjectLuaBindings::SetFieldAsNumber(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        return 0;
    }

    const char* field_name = luaL_checkstring(L, 2);
    float value = static_cast<float>(luaL_checknumber(L, 3));
    instance->SetFieldValueAsFloat(field_name, value);
    return 0;
}

int ScriptableObjectLuaBindings::SetFieldAsBool(lua_State* L) {
    auto instance = GetInstanceFromStack(L, 1);
    if (!instance) {
        return 0;
    }

    const char* field_name = luaL_checkstring(L, 2);
    bool value = lua_toboolean(L, 3) != 0;
    instance->SetFieldValueAsBool(field_name, value);
    return 0;
}

std::shared_ptr<ScriptableObjectInstance> ScriptableObjectLuaBindings::GetInstanceFromStack(lua_State* L, int index) {
    if (!lua_isuserdata(L, index)) {
        return nullptr;
    }

    auto* instance_ptr = static_cast<std::shared_ptr<ScriptableObjectInstance>*>(
        luaL_checkudata(L, index, "ScriptableObjectInstance"));

    return instance_ptr ? *instance_ptr : nullptr;
}

void ScriptableObjectLuaBindings::PushInstanceToStack(lua_State* L, std::shared_ptr<ScriptableObjectInstance> instance) {
    auto* instance_ptr = static_cast<std::shared_ptr<ScriptableObjectInstance>*>(
        lua_newuserdata(L, sizeof(std::shared_ptr<ScriptableObjectInstance>)));

    new(instance_ptr) std::shared_ptr<ScriptableObjectInstance>(instance);

    luaL_getmetatable(L, "ScriptableObjectInstance");
    lua_setmetatable(L, -2);
}

} // namespace Lupine
