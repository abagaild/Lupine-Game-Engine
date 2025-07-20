#pragma once

#include "lupine/core/Component.h"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Lupine {

/**
 * @brief Utility class for serialization operations
 */
class SerializationUtils {
public:
    /**
     * @brief Serialize an export value to string
     */
    static std::string SerializeExportValue(const ExportValue& value);
    
    /**
     * @brief Parse an export value from string
     */
    static ExportValue ParseExportValue(const std::string& valueStr, ExportVariableType type);
    
    /**
     * @brief Convert export variable type to string
     */
    static std::string ExportVariableTypeToString(ExportVariableType type);
    
    /**
     * @brief Convert string to export variable type
     */
    static ExportVariableType StringToExportVariableType(const std::string& typeStr);
    
    /**
     * @brief Escape string for JSON
     */
    static std::string EscapeJsonString(const std::string& str);

    /**
     * @brief Unescape JSON string
     */
    static std::string UnescapeJsonString(const std::string& str);

    /**
     * @brief Escape string for YAML (deprecated, use EscapeJsonString)
     */
    static std::string EscapeYamlString(const std::string& str);

    /**
     * @brief Unescape YAML string (deprecated, use UnescapeJsonString)
     */
    static std::string UnescapeYamlString(const std::string& str);
    
    /**
     * @brief Generate indentation string
     */
    static std::string Indent(int level);
    
    /**
     * @brief Serialize glm::vec2 to string
     */
    static std::string SerializeVec2(const glm::vec2& vec);
    
    /**
     * @brief Serialize glm::vec3 to string
     */
    static std::string SerializeVec3(const glm::vec3& vec);
    
    /**
     * @brief Serialize glm::vec4 to string
     */
    static std::string SerializeVec4(const glm::vec4& vec);
    
    /**
     * @brief Parse glm::vec2 from string
     */
    static glm::vec2 ParseVec2(const std::string& str);
    
    /**
     * @brief Parse glm::vec3 from string
     */
    static glm::vec3 ParseVec3(const std::string& str);
    
    /**
     * @brief Parse glm::vec4 from string
     */
    static glm::vec4 ParseVec4(const std::string& str);

    /**
     * @brief Serialize glm::quat to string
     */
    static std::string SerializeQuat(const glm::quat& quat);

    /**
     * @brief Parse glm::quat from string
     */
    static glm::quat DeserializeQuat(const std::string& str);

    /**
     * @brief Alias for ParseVec2 for consistency
     */
    static glm::vec2 DeserializeVec2(const std::string& str) { return ParseVec2(str); }

    /**
     * @brief Alias for ParseVec3 for consistency
     */
    static glm::vec3 DeserializeVec3(const std::string& str) { return ParseVec3(str); }

    /**
     * @brief Alias for ParseVec4 for consistency
     */
    static glm::vec4 DeserializeVec4(const std::string& str) { return ParseVec4(str); }
    
    /**
     * @brief Validate YAML structure
     */
    static bool ValidateYamlStructure(const std::string& yaml);
    
    /**
     * @brief Extract value from YAML line
     */
    static std::string ExtractYamlValue(const std::string& line);
    
    /**
     * @brief Get indentation level of YAML line
     */
    static int GetIndentationLevel(const std::string& line);
    
    /**
     * @brief Check if line is a YAML key
     */
    static bool IsYamlKey(const std::string& line);

    /**
     * @brief Check if line is a YAML list item (starts with -)
     */
    static bool IsYamlListItem(const std::string& line);
    
    /**
     * @brief Extract key from YAML line
     */
    static std::string ExtractYamlKey(const std::string& line);
    
    /**
     * @brief Split string by delimiter
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    
    /**
     * @brief Trim whitespace from string
     */
    static std::string Trim(const std::string& str);
    
    /**
     * @brief Check if string starts with prefix
     */
    static bool StartsWith(const std::string& str, const std::string& prefix);
    
    /**
     * @brief Check if string ends with suffix
     */
    static bool EndsWith(const std::string& str, const std::string& suffix);
    
    /**
     * @brief Replace all occurrences of substring
     */
    static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);
};

/**
 * @brief Simple YAML parser for basic serialization needs
 */
class SimpleYamlParser {
public:
    struct YamlNode {
        std::string key;
        std::string value;
        int indentLevel;
        std::vector<YamlNode> children;
        
        YamlNode() : indentLevel(0) {}
        YamlNode(const std::string& k, const std::string& v, int indent) 
            : key(k), value(v), indentLevel(indent) {}
    };
    
    /**
     * @brief Parse YAML string into node tree
     */
    static YamlNode Parse(const std::string& yaml);
    
    /**
     * @brief Find child node by key
     */
    static const YamlNode* FindChild(const YamlNode& parent, const std::string& key);
    
    /**
     * @brief Get all children with specific key
     */
    static std::vector<const YamlNode*> FindChildren(const YamlNode& parent, const std::string& key);
    
    /**
     * @brief Convert node tree back to YAML string
     */
    static std::string ToString(const YamlNode& root);

private:
    static void ParseLines(const std::vector<std::string>& lines, YamlNode& root);
    static void ToStringRecursive(const YamlNode& node, std::stringstream& ss, int baseIndent = 0);
};

} // namespace Lupine
