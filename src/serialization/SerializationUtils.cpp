#include "lupine/serialization/SerializationUtils.h"
#include "lupine/core/UUID.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cctype>

namespace Lupine {

std::string SerializationUtils::SerializeExportValue(const ExportValue& value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<float>(value)) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << std::get<float>(value);
        return oss.str();
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + EscapeJsonString(std::get<std::string>(value)) + "\"";
    } else if (std::holds_alternative<glm::vec2>(value)) {
        return SerializeVec2(std::get<glm::vec2>(value));
    } else if (std::holds_alternative<glm::vec3>(value)) {
        return SerializeVec3(std::get<glm::vec3>(value));
    } else if (std::holds_alternative<glm::vec4>(value)) {
        return SerializeVec4(std::get<glm::vec4>(value));
    } else if (std::holds_alternative<UUID>(value)) {
        return "\"" + std::get<UUID>(value).ToString() + "\"";
    }
    return "\"\"";
}

ExportValue SerializationUtils::ParseExportValue(const std::string& valueStr, ExportVariableType type) {
    std::string trimmed = Trim(valueStr);
    
    switch (type) {
        case ExportVariableType::Bool:
            return trimmed == "true" || trimmed == "True" || trimmed == "TRUE";
        case ExportVariableType::Int:
            try {
                return std::stoi(trimmed);
            } catch (...) {
                return 0;
            }
        case ExportVariableType::Float:
            try {
                return std::stof(trimmed);
            } catch (...) {
                return 0.0f;
            }
        case ExportVariableType::String:
        case ExportVariableType::FilePath:
            return UnescapeJsonString(trimmed);
        case ExportVariableType::Vec2:
            return ParseVec2(trimmed);
        case ExportVariableType::Vec3:
            return ParseVec3(trimmed);
        case ExportVariableType::Color:
        case ExportVariableType::Vec4:
            return ParseVec4(trimmed);
        case ExportVariableType::NodeReference:
            return UUID::FromString(UnescapeJsonString(trimmed));
        case ExportVariableType::Enum:
            // Enum values are stored as integers
            try {
                return std::stoi(trimmed);
            } catch (...) {
                return 0;
            }
        default:
            return std::string("");
    }
}

std::string SerializationUtils::ExportVariableTypeToString(ExportVariableType type) {
    switch (type) {
        case ExportVariableType::Bool: return "bool";
        case ExportVariableType::Int: return "int";
        case ExportVariableType::Float: return "float";
        case ExportVariableType::String: return "string";
        case ExportVariableType::Vec2: return "vec2";
        case ExportVariableType::Vec3: return "vec3";
        case ExportVariableType::Vec4: return "vec4";
        case ExportVariableType::FilePath: return "filepath";
        case ExportVariableType::NodeReference: return "noderef";
        case ExportVariableType::Color: return "color";
        case ExportVariableType::Enum: return "enum";
        default: return "string";
    }
}

ExportVariableType SerializationUtils::StringToExportVariableType(const std::string& typeStr) {
    if (typeStr == "bool") return ExportVariableType::Bool;
    if (typeStr == "int") return ExportVariableType::Int;
    if (typeStr == "float") return ExportVariableType::Float;
    if (typeStr == "string") return ExportVariableType::String;
    if (typeStr == "vec2") return ExportVariableType::Vec2;
    if (typeStr == "vec3") return ExportVariableType::Vec3;
    if (typeStr == "vec4") return ExportVariableType::Vec4;
    if (typeStr == "filepath") return ExportVariableType::FilePath;
    if (typeStr == "noderef") return ExportVariableType::NodeReference;
    if (typeStr == "color") return ExportVariableType::Color;
    if (typeStr == "enum") return ExportVariableType::Enum;
    return ExportVariableType::String;
}

std::string SerializationUtils::EscapeJsonString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string SerializationUtils::UnescapeJsonString(const std::string& str) {
    std::string result = str;

    // Remove surrounding quotes if present
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.size() - 2);
    }

    result = ReplaceAll(result, "\\\"", "\"");
    result = ReplaceAll(result, "\\n", "\n");
    result = ReplaceAll(result, "\\r", "\r");
    result = ReplaceAll(result, "\\t", "\t");
    result = ReplaceAll(result, "\\b", "\b");
    result = ReplaceAll(result, "\\f", "\f");
    result = ReplaceAll(result, "\\\\", "\\");
    return result;
}

// Keep YAML functions for backward compatibility during transition
std::string SerializationUtils::EscapeYamlString(const std::string& str) {
    return EscapeJsonString(str);
}

std::string SerializationUtils::UnescapeYamlString(const std::string& str) {
    return UnescapeJsonString(str);
}

std::string SerializationUtils::Indent(int level) {
    return std::string(level * 2, ' ');
}

std::string SerializationUtils::SerializeVec2(const glm::vec2& vec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[" << vec.x << ", " << vec.y << "]";
    return oss.str();
}

std::string SerializationUtils::SerializeVec3(const glm::vec3& vec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]";
    return oss.str();
}

std::string SerializationUtils::SerializeVec4(const glm::vec4& vec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]";
    return oss.str();
}

glm::vec2 SerializationUtils::ParseVec2(const std::string& str) {
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());
    
    auto parts = Split(cleaned, ',');
    glm::vec2 result(0.0f);
    
    if (parts.size() >= 1) {
        try { result.x = std::stof(Trim(parts[0])); } catch (...) {}
    }
    if (parts.size() >= 2) {
        try { result.y = std::stof(Trim(parts[1])); } catch (...) {}
    }
    
    return result;
}

glm::vec3 SerializationUtils::ParseVec3(const std::string& str) {
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());
    
    auto parts = Split(cleaned, ',');
    glm::vec3 result(0.0f);
    
    if (parts.size() >= 1) {
        try { result.x = std::stof(Trim(parts[0])); } catch (...) {}
    }
    if (parts.size() >= 2) {
        try { result.y = std::stof(Trim(parts[1])); } catch (...) {}
    }
    if (parts.size() >= 3) {
        try { result.z = std::stof(Trim(parts[2])); } catch (...) {}
    }
    
    return result;
}

glm::vec4 SerializationUtils::ParseVec4(const std::string& str) {
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());
    
    auto parts = Split(cleaned, ',');
    glm::vec4 result(0.0f);
    
    if (parts.size() >= 1) {
        try { result.x = std::stof(Trim(parts[0])); } catch (...) {}
    }
    if (parts.size() >= 2) {
        try { result.y = std::stof(Trim(parts[1])); } catch (...) {}
    }
    if (parts.size() >= 3) {
        try { result.z = std::stof(Trim(parts[2])); } catch (...) {}
    }
    if (parts.size() >= 4) {
        try { result.w = std::stof(Trim(parts[3])); } catch (...) {}
    }
    
    return result;
}

std::string SerializationUtils::SerializeQuat(const glm::quat& quat) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << "]";
    return oss.str();
}

glm::quat SerializationUtils::DeserializeQuat(const std::string& str) {
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());

    auto parts = Split(cleaned, ',');
    glm::quat result(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion

    if (parts.size() >= 1) {
        try { result.w = std::stof(Trim(parts[0])); } catch (...) {}
    }
    if (parts.size() >= 2) {
        try { result.x = std::stof(Trim(parts[1])); } catch (...) {}
    }
    if (parts.size() >= 3) {
        try { result.y = std::stof(Trim(parts[2])); } catch (...) {}
    }
    if (parts.size() >= 4) {
        try { result.z = std::stof(Trim(parts[3])); } catch (...) {}
    }

    return result;
}

bool SerializationUtils::ValidateYamlStructure(const std::string& yaml) {
    // Basic validation - check for balanced indentation
    auto lines = Split(yaml, '\n');
    int lastIndent = -1;
    
    for (const auto& line : lines) {
        if (Trim(line).empty() || Trim(line)[0] == '#') continue;
        
        int indent = GetIndentationLevel(line);
        if (lastIndent >= 0 && indent > lastIndent + 2) {
            return false; // Invalid indentation jump
        }
        lastIndent = indent;
    }
    
    return true;
}

std::string SerializationUtils::ExtractYamlValue(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos && colonPos + 1 < line.length()) {
        return Trim(line.substr(colonPos + 1));
    }
    return "";
}

int SerializationUtils::GetIndentationLevel(const std::string& line) {
    int count = 0;
    for (char c : line) {
        if (c == ' ') count++;
        else if (c == '\t') count += 4; // Treat tab as 4 spaces
        else break;
    }
    return count;
}

bool SerializationUtils::IsYamlKey(const std::string& line) {
    std::string trimmed = Trim(line);
    return !trimmed.empty() && trimmed.find(':') != std::string::npos;
}

bool SerializationUtils::IsYamlListItem(const std::string& line) {
    std::string trimmed = Trim(line);
    return !trimmed.empty() && trimmed[0] == '-' && (trimmed.length() == 1 || trimmed[1] == ' ');
}

std::string SerializationUtils::ExtractYamlKey(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        return Trim(line.substr(0, colonPos));
    }
    return Trim(line);
}

std::vector<std::string> SerializationUtils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

std::string SerializationUtils::Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool SerializationUtils::StartsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool SerializationUtils::EndsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

std::string SerializationUtils::ReplaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

// SimpleYamlParser implementation
SimpleYamlParser::YamlNode SimpleYamlParser::Parse(const std::string& yaml) {
    YamlNode root;
    auto lines = SerializationUtils::Split(yaml, '\n');
    ParseLines(lines, root);
    return root;
}

const SimpleYamlParser::YamlNode* SimpleYamlParser::FindChild(const YamlNode& parent, const std::string& key) {
    for (const auto& child : parent.children) {
        if (child.key == key) {
            return &child;
        }
    }
    return nullptr;
}

std::vector<const SimpleYamlParser::YamlNode*> SimpleYamlParser::FindChildren(const YamlNode& parent, const std::string& key) {
    std::vector<const YamlNode*> result;
    for (const auto& child : parent.children) {
        if (child.key == key) {
            result.push_back(&child);
        }
    }
    return result;
}

std::string SimpleYamlParser::ToString(const YamlNode& root) {
    std::stringstream ss;
    ToStringRecursive(root, ss);
    return ss.str();
}

void SimpleYamlParser::ParseLines(const std::vector<std::string>& lines, YamlNode& root) {
    std::vector<YamlNode*> nodeStack;
    nodeStack.push_back(&root);

    for (size_t lineNum = 0; lineNum < lines.size(); ++lineNum) {
        const auto& line = lines[lineNum];
        std::string trimmed = SerializationUtils::Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        int indent = SerializationUtils::GetIndentationLevel(line);

        std::cout << "YAML Parser: Line " << lineNum << " (indent " << indent << "): '" << line << "'" << std::endl;

        // Adjust stack based on indentation
        while (nodeStack.size() > 1 && nodeStack.back()->indentLevel >= indent) {
            std::cout << "YAML Parser: Popping stack (current level: " << nodeStack.back()->indentLevel << ", new indent: " << indent << ")" << std::endl;
            nodeStack.pop_back();
        }

        if (SerializationUtils::IsYamlListItem(line)) {
            // Handle YAML list item (starts with -)
            std::string content = SerializationUtils::Trim(line.substr(line.find('-') + 1));
            std::cout << "YAML Parser: Found list item, content: '" << content << "'" << std::endl;

            // List items are children of the current node on the stack

            if (content.empty()) {
                // Empty list item, create a container node
                YamlNode newNode("", "", indent);
                nodeStack.back()->children.push_back(newNode);
                nodeStack.push_back(&nodeStack.back()->children.back());
                std::cout << "YAML Parser: Created empty list item container under " << nodeStack[nodeStack.size()-2]->key << std::endl;
            } else if (content.find(':') != std::string::npos) {
                // List item with immediate key-value pair (e.g., "- type: Sprite2D")
                std::string key = SerializationUtils::ExtractYamlKey(content);
                std::string value = SerializationUtils::ExtractYamlValue(content);

                // Create a container node for the list item
                YamlNode listItemNode("", "", indent);
                nodeStack.back()->children.push_back(listItemNode);

                // Add the key-value pair as a child of the list item
                YamlNode keyValueNode(key, value, indent + 1);
                nodeStack.back()->children.back().children.push_back(keyValueNode);
                nodeStack.push_back(&nodeStack.back()->children.back());
                std::cout << "YAML Parser: Created list item with key-value: " << key << " = " << value << " under " << nodeStack[nodeStack.size()-2]->key << std::endl;
            } else {
                // List item with scalar value
                YamlNode newNode("", content, indent);
                nodeStack.back()->children.push_back(newNode);
                std::cout << "YAML Parser: Created list item with scalar: " << content << " under " << nodeStack.back()->key << std::endl;
            }
        } else if (SerializationUtils::IsYamlKey(line)) {
            std::string key = SerializationUtils::ExtractYamlKey(line);
            std::string value = SerializationUtils::ExtractYamlValue(line);

            YamlNode newNode(key, value, indent);
            nodeStack.back()->children.push_back(newNode);
            nodeStack.push_back(&nodeStack.back()->children.back());
            std::cout << "YAML Parser: Created key-value node: " << key << " = " << value << std::endl;
        }
    }
}

void SimpleYamlParser::ToStringRecursive(const YamlNode& node, std::stringstream& ss, int baseIndent) {
    if (!node.key.empty()) {
        ss << SerializationUtils::Indent(baseIndent) << node.key;
        if (!node.value.empty()) {
            ss << ": " << node.value;
        } else {
            ss << ":";
        }
        ss << "\n";
    }

    for (const auto& child : node.children) {
        ToStringRecursive(child, ss, baseIndent + (node.key.empty() ? 0 : 1));
    }
}

} // namespace Lupine
