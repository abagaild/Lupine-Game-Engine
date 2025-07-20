#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

namespace Lupine {

// Simple JSON value types
using JsonValue = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::vector<class JsonNode>,
    std::map<std::string, class JsonNode>
>;

class JsonNode {
public:
    JsonValue value;

    JsonNode() : value(nullptr) {}
    JsonNode(std::nullptr_t) : value(nullptr) {}
    JsonNode(bool v) : value(v) {}
    JsonNode(int v) : value(static_cast<int64_t>(v)) {}
    JsonNode(int64_t v) : value(v) {}
    JsonNode(double v) : value(v) {}
    JsonNode(const std::string& v) : value(v) {}
    JsonNode(const char* v) : value(std::string(v)) {}
    JsonNode(const std::vector<JsonNode>& v) : value(v) {}
    JsonNode(const std::map<std::string, JsonNode>& v) : value(v) {}

    // Type checking
    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool IsBool() const { return std::holds_alternative<bool>(value); }
    bool IsInt() const { return std::holds_alternative<int64_t>(value); }
    bool IsDouble() const { return std::holds_alternative<double>(value); }
    bool IsString() const { return std::holds_alternative<std::string>(value); }
    bool IsArray() const { return std::holds_alternative<std::vector<JsonNode>>(value); }
    bool IsObject() const { return std::holds_alternative<std::map<std::string, JsonNode>>(value); }

    // Value getters
    bool AsBool() const { return std::get<bool>(value); }
    int64_t AsInt() const { return std::get<int64_t>(value); }
    double AsDouble() const { return std::get<double>(value); }
    const std::string& AsString() const { return std::get<std::string>(value); }
    const std::vector<JsonNode>& AsArray() const { return std::get<std::vector<JsonNode>>(value); }
    const std::map<std::string, JsonNode>& AsObject() const { return std::get<std::map<std::string, JsonNode>>(value); }

    // Mutable getters
    std::vector<JsonNode>& AsArray() { return std::get<std::vector<JsonNode>>(value); }
    std::map<std::string, JsonNode>& AsObject() { return std::get<std::map<std::string, JsonNode>>(value); }

    // Object access
    JsonNode& operator[](const std::string& key) {
        if (!IsObject()) {
            value = std::map<std::string, JsonNode>();
        }
        return AsObject()[key];
    }

    const JsonNode& operator[](const std::string& key) const {
        static JsonNode null_node;
        if (!IsObject()) return null_node;
        auto& obj = AsObject();
        auto it = obj.find(key);
        return (it != obj.end()) ? it->second : null_node;
    }

    // Array access
    JsonNode& operator[](size_t index) {
        if (!IsArray()) {
            value = std::vector<JsonNode>();
        }
        auto& arr = AsArray();
        if (index >= arr.size()) {
            arr.resize(index + 1);
        }
        return arr[index];
    }

    const JsonNode& operator[](size_t index) const {
        static JsonNode null_node;
        if (!IsArray()) return null_node;
        auto& arr = AsArray();
        return (index < arr.size()) ? arr[index] : null_node;
    }

    // Convenience methods
    bool HasKey(const std::string& key) const {
        if (!IsObject()) return false;
        return AsObject().find(key) != AsObject().end();
    }

    size_t Size() const {
        if (IsArray()) return AsArray().size();
        if (IsObject()) return AsObject().size();
        return 0;
    }

    void Push(const JsonNode& node) {
        if (!IsArray()) {
            value = std::vector<JsonNode>();
        }
        AsArray().push_back(node);
    }
};

class JsonUtils {
public:
    // Parse JSON from string
    static JsonNode Parse(const std::string& json);
    
    // Generate JSON string
    static std::string Stringify(const JsonNode& node, bool pretty = false, int indent = 0);
    
    // Load JSON from file
    static JsonNode LoadFromFile(const std::string& filepath);
    
    // Save JSON to file
    static bool SaveToFile(const JsonNode& node, const std::string& filepath, bool pretty = true);

private:
    // Internal parsing helpers
    static JsonNode ParseValue(const std::string& json, size_t& pos);
    static JsonNode ParseObject(const std::string& json, size_t& pos);
    static JsonNode ParseArray(const std::string& json, size_t& pos);
    static std::string ParseString(const std::string& json, size_t& pos);
    static double ParseNumber(const std::string& json, size_t& pos);
    static void SkipWhitespace(const std::string& json, size_t& pos);
    static std::string EscapeString(const std::string& str);
    static std::string UnescapeString(const std::string& str);
};

} // namespace Lupine
