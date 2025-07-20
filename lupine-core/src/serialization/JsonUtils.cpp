#include "lupine/serialization/JsonUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

namespace Lupine {

JsonNode JsonUtils::Parse(const std::string& json) {
    size_t pos = 0;
    SkipWhitespace(json, pos);
    return ParseValue(json, pos);
}

JsonNode JsonUtils::ParseValue(const std::string& json, size_t& pos) {
    SkipWhitespace(json, pos);
    
    if (pos >= json.length()) {
        return JsonNode();
    }
    
    char c = json[pos];
    
    if (c == '{') {
        return ParseObject(json, pos);
    } else if (c == '[') {
        return ParseArray(json, pos);
    } else if (c == '"') {
        return JsonNode(ParseString(json, pos));
    } else if (c == 't' && json.substr(pos, 4) == "true") {
        pos += 4;
        return JsonNode(true);
    } else if (c == 'f' && json.substr(pos, 5) == "false") {
        pos += 5;
        return JsonNode(false);
    } else if (c == 'n' && json.substr(pos, 4) == "null") {
        pos += 4;
        return JsonNode();
    } else if (c == '-' || std::isdigit(c)) {
        return JsonNode(ParseNumber(json, pos));
    }
    
    throw std::runtime_error("Invalid JSON at position " + std::to_string(pos));
}

JsonNode JsonUtils::ParseObject(const std::string& json, size_t& pos) {
    JsonNode obj;
    obj.value = std::map<std::string, JsonNode>();

    pos++; // Skip '{'
    SkipWhitespace(json, pos);

    if (pos < json.length() && json[pos] == '}') {
        pos++; // Skip '}'
        return obj;
    }
    
    while (pos < json.length()) {
        SkipWhitespace(json, pos);
        
        // Parse key
        if (json[pos] != '"') {
            throw std::runtime_error("Expected string key at position " + std::to_string(pos));
        }
        std::string key = ParseString(json, pos);
        
        SkipWhitespace(json, pos);
        
        // Expect ':'
        if (pos >= json.length() || json[pos] != ':') {
            throw std::runtime_error("Expected ':' at position " + std::to_string(pos));
        }
        pos++; // Skip ':'
        
        // Parse value
        JsonNode value = ParseValue(json, pos);
        obj.AsObject()[key] = value;
        
        SkipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        
        if (json[pos] == '}') {
            pos++; // Skip '}'
            break;
        } else if (json[pos] == ',') {
            pos++; // Skip ','
        } else {
            throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
        }
    }
    
    return obj;
}

JsonNode JsonUtils::ParseArray(const std::string& json, size_t& pos) {
    JsonNode arr;
    arr.value = std::vector<JsonNode>();

    pos++; // Skip '['
    SkipWhitespace(json, pos);

    if (pos < json.length() && json[pos] == ']') {
        pos++; // Skip ']'
        return arr;
    }
    
    while (pos < json.length()) {
        JsonNode value = ParseValue(json, pos);
        arr.Push(value);
        
        SkipWhitespace(json, pos);
        
        if (pos >= json.length()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        
        if (json[pos] == ']') {
            pos++; // Skip ']'
            break;
        } else if (json[pos] == ',') {
            pos++; // Skip ','
            SkipWhitespace(json, pos);
        } else {
            throw std::runtime_error("Expected ',' or ']' at position " + std::to_string(pos));
        }
    }
    
    return arr;
}

std::string JsonUtils::ParseString(const std::string& json, size_t& pos) {
    if (pos >= json.length() || json[pos] != '"') {
        throw std::runtime_error("Expected '\"' at position " + std::to_string(pos));
    }
    
    pos++; // Skip opening '"'
    std::string result;
    
    while (pos < json.length() && json[pos] != '"') {
        if (json[pos] == '\\') {
            pos++; // Skip '\'
            if (pos >= json.length()) {
                throw std::runtime_error("Unexpected end of string");
            }
            
            char escaped = json[pos];
            switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default:
                    result += escaped;
                    break;
            }
        } else {
            result += json[pos];
        }
        pos++;
    }
    
    if (pos >= json.length()) {
        throw std::runtime_error("Unterminated string");
    }
    
    pos++; // Skip closing '"'
    return result;
}

double JsonUtils::ParseNumber(const std::string& json, size_t& pos) {
    size_t start = pos;
    
    if (json[pos] == '-') {
        pos++;
    }
    
    if (pos >= json.length() || !std::isdigit(json[pos])) {
        throw std::runtime_error("Invalid number at position " + std::to_string(start));
    }
    
    while (pos < json.length() && std::isdigit(json[pos])) {
        pos++;
    }
    
    if (pos < json.length() && json[pos] == '.') {
        pos++;
        while (pos < json.length() && std::isdigit(json[pos])) {
            pos++;
        }
    }
    
    if (pos < json.length() && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        if (pos < json.length() && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        while (pos < json.length() && std::isdigit(json[pos])) {
            pos++;
        }
    }
    
    std::string numberStr = json.substr(start, pos - start);
    return std::stod(numberStr);
}

void JsonUtils::SkipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.length() && std::isspace(json[pos])) {
        pos++;
    }
}

std::string JsonUtils::Stringify(const JsonNode& node, bool pretty, int indent) {
    std::string indentStr = pretty ? std::string(indent * 2, ' ') : "";
    std::string newline = pretty ? "\n" : "";
    
    if (node.IsNull()) {
        return "null";
    } else if (node.IsBool()) {
        return node.AsBool() ? "true" : "false";
    } else if (node.IsInt()) {
        return std::to_string(node.AsInt());
    } else if (node.IsDouble()) {
        return std::to_string(node.AsDouble());
    } else if (node.IsString()) {
        return "\"" + EscapeString(node.AsString()) + "\"";
    } else if (node.IsArray()) {
        std::string result = "[";
        const auto& arr = node.AsArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) result += ",";
            if (pretty) result += newline + std::string((indent + 1) * 2, ' ');
            result += Stringify(arr[i], pretty, indent + 1);
        }
        if (pretty && !arr.empty()) result += newline + indentStr;
        result += "]";
        return result;
    } else if (node.IsObject()) {
        std::string result = "{";
        const auto& obj = node.AsObject();
        bool first = true;
        for (const auto& pair : obj) {
            if (!first) result += ",";
            if (pretty) result += newline + std::string((indent + 1) * 2, ' ');
            result += "\"" + EscapeString(pair.first) + "\":" + (pretty ? " " : "");
            result += Stringify(pair.second, pretty, indent + 1);
            first = false;
        }
        if (pretty && !obj.empty()) result += newline + indentStr;
        result += "}";
        return result;
    }
    
    return "null";
}

std::string JsonUtils::EscapeString(const std::string& str) {
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

JsonNode JsonUtils::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return Parse(content);
}

bool JsonUtils::SaveToFile(const JsonNode& node, const std::string& filepath, bool pretty) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << Stringify(node, pretty);
    file.close();
    return true;
}

} // namespace Lupine
