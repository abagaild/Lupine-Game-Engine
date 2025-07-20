#pragma once

#include <pybind11/pybind11.h>
#include <lua.hpp>
#include <string>
#include <memory>

namespace Lupine {

// Forward declarations
class ScriptableObjectInstance;

/**
 * @brief Python bindings for scriptable objects
 */
class ScriptableObjectPythonBindings {
public:
    /**
     * @brief Initialize Python bindings for scriptable objects
     */
    static void Initialize(pybind11::dict& globals);
    
    /**
     * @brief Create SO accessor object for Python
     */
    static pybind11::object CreateSOAccessor();

private:
    /**
     * @brief SO accessor class for Python
     */
    class SOAccessor {
    public:
        /**
         * @brief Get template accessor
         */
        pybind11::object GetTemplate(const std::string& template_name);
    };
    
    /**
     * @brief Template accessor class for Python
     */
    class TemplateAccessor {
    public:
        TemplateAccessor(const std::string& template_name);
        
        /**
         * @brief Get instance
         */
        pybind11::object GetInstance(const std::string& instance_name);
        
    private:
        std::string m_template_name;
    };
    
    /**
     * @brief Instance wrapper class for Python
     */
    class InstanceWrapper {
    public:
        InstanceWrapper(std::shared_ptr<ScriptableObjectInstance> instance);
        
        /**
         * @brief Get field value
         */
        pybind11::object GetField(const std::string& field_name);
        
        /**
         * @brief Set field value
         */
        void SetField(const std::string& field_name, const pybind11::object& value);
        
        /**
         * @brief Get field as string
         */
        std::string GetFieldAsString(const std::string& field_name);
        
        /**
         * @brief Get field as integer
         */
        int GetFieldAsInt(const std::string& field_name);
        
        /**
         * @brief Get field as float
         */
        float GetFieldAsFloat(const std::string& field_name);
        
        /**
         * @brief Get field as boolean
         */
        bool GetFieldAsBool(const std::string& field_name);
        
        /**
         * @brief Set field as string
         */
        void SetFieldAsString(const std::string& field_name, const std::string& value);
        
        /**
         * @brief Set field as integer
         */
        void SetFieldAsInt(const std::string& field_name, int value);
        
        /**
         * @brief Set field as float
         */
        void SetFieldAsFloat(const std::string& field_name, float value);
        
        /**
         * @brief Set field as boolean
         */
        void SetFieldAsBool(const std::string& field_name, bool value);
        
    private:
        std::shared_ptr<ScriptableObjectInstance> m_instance;
    };
};

/**
 * @brief Lua bindings for scriptable objects
 */
class ScriptableObjectLuaBindings {
public:
    /**
     * @brief Initialize Lua bindings for scriptable objects
     */
    static void Initialize(lua_State* L);

private:
    /**
     * @brief Lua function to get scriptable object
     * Usage: SO.TemplateName.InstanceName
     */
    static int GetScriptableObject(lua_State* L);
    
    /**
     * @brief Lua function to get field value
     */
    static int GetField(lua_State* L);
    
    /**
     * @brief Lua function to set field value
     */
    static int SetField(lua_State* L);
    
    /**
     * @brief Lua function to get field as string
     */
    static int GetFieldAsString(lua_State* L);
    
    /**
     * @brief Lua function to get field as number
     */
    static int GetFieldAsNumber(lua_State* L);
    
    /**
     * @brief Lua function to get field as boolean
     */
    static int GetFieldAsBool(lua_State* L);
    
    /**
     * @brief Lua function to set field as string
     */
    static int SetFieldAsString(lua_State* L);
    
    /**
     * @brief Lua function to set field as number
     */
    static int SetFieldAsNumber(lua_State* L);
    
    /**
     * @brief Lua function to set field as boolean
     */
    static int SetFieldAsBool(lua_State* L);
    
    /**
     * @brief Helper to get instance from Lua stack
     */
    static std::shared_ptr<ScriptableObjectInstance> GetInstanceFromStack(lua_State* L, int index);
    
    /**
     * @brief Helper to push instance to Lua stack
     */
    static void PushInstanceToStack(lua_State* L, std::shared_ptr<ScriptableObjectInstance> instance);
};

} // namespace Lupine
