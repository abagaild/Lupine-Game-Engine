#pragma once

#include "VScriptNode.h"
#include <string>
#include <vector>
#include <unordered_set>

namespace Lupine {

// Forward declarations
class VScriptGraph;

/**
 * @brief Generates Python code from visual script graphs
 * 
 * Traverses the visual script graph and converts it to executable Python code
 * with proper indentation, variable handling, and execution flow.
 */
class CodeGenerator {
public:
    /**
     * @brief Constructor
     */
    CodeGenerator();

    /**
     * @brief Generate Python code from a visual script graph
     * @param graph The visual script graph to convert
     * @return Generated Python code as a string
     */
    std::string GenerateCode(const VScriptGraph& graph) const;

    /**
     * @brief Set whether to include debug comments in generated code
     * @param include_debug True to include debug comments
     */
    void SetIncludeDebugComments(bool include_debug) { m_include_debug_comments = include_debug; }

    /**
     * @brief Set indentation string (default is 4 spaces)
     * @param indent Indentation string to use
     */
    void SetIndentation(const std::string& indent) { m_indentation = indent; }

private:
    /**
     * @brief Generate code for a specific execution path starting from a node
     * @param graph The graph containing the nodes
     * @param start_node The node to start code generation from
     * @param visited Set of already visited nodes to prevent infinite loops
     * @param indent_level Current indentation level
     * @return Vector of generated code lines
     */
    std::vector<std::string> GenerateExecutionPath(const VScriptGraph& graph, 
                                                   VScriptNode* start_node,
                                                   std::unordered_set<std::string>& visited,
                                                   int indent_level = 0) const;

    /**
     * @brief Generate the header section of the Python script
     * @return Vector of header lines
     */
    std::vector<std::string> GenerateHeader() const;

    /**
     * @brief Generate import statements
     * @return Vector of import lines
     */
    std::vector<std::string> GenerateImports() const;

    /**
     * @brief Generate class definition and lifecycle methods
     * @param graph The visual script graph
     * @return Vector of class definition lines
     */
    std::vector<std::string> GenerateClassDefinition(const VScriptGraph& graph) const;

    /**
     * @brief Generate variable declarations from the graph
     * @param graph The visual script graph
     * @return Vector of variable declaration lines
     */
    std::vector<std::string> GenerateVariableDeclarations(const VScriptGraph& graph) const;

    /**
     * @brief Get the next node in execution flow from a given node
     * @param graph The graph containing the nodes
     * @param current_node Current node
     * @param output_pin_name Name of the output execution pin to follow
     * @return Next node in execution flow or nullptr if none
     */
    VScriptNode* GetNextExecutionNode(const VScriptGraph& graph, 
                                     VScriptNode* current_node,
                                     const std::string& output_pin_name = "exec_out") const;

    /**
     * @brief Get input value for a data pin (either from connection or default value)
     * @param graph The graph containing the connections
     * @param node The node containing the pin
     * @param pin_name Name of the input pin
     * @return String representation of the input value
     */
    std::string GetInputValue(const VScriptGraph& graph, 
                             VScriptNode* node, 
                             const std::string& pin_name) const;

    /**
     * @brief Apply indentation to a line of code
     * @param line The code line
     * @param indent_level Indentation level
     * @return Indented line
     */
    std::string ApplyIndentation(const std::string& line, int indent_level) const;

    /**
     * @brief Convert data type to Python type hint
     * @param data_type Visual script data type
     * @return Python type hint string
     */
    std::string DataTypeToPythonType(VScriptDataType data_type) const;

    /**
     * @brief Generate a variable name for a node's output pin
     * @param node The node
     * @param pin_name The output pin name
     * @return Generated variable name
     */
    std::string GenerateVariableName(VScriptNode* node, const std::string& pin_name) const;

    /**
     * @brief Format a default value based on its data type
     * @param value The default value string
     * @param data_type The data type
     * @return Properly formatted value for Python
     */
    std::string FormatDefaultValue(const std::string& value, VScriptDataType data_type) const;

    /**
     * @brief Generate input variable assignments for a node
     * @param graph The visual script graph
     * @param node The node to generate assignments for
     * @param indent_level Current indentation level
     * @return Vector of assignment lines
     */
    std::vector<std::string> GenerateInputAssignments(const VScriptGraph& graph,
                                                      VScriptNode* node,
                                                      int indent_level) const;

    /**
     * @brief Generate output variable assignments for a node
     * @param graph The visual script graph
     * @param node The node to generate assignments for
     * @param indent_level Current indentation level
     * @return Vector of assignment lines
     */
    std::vector<std::string> GenerateOutputAssignments(const VScriptGraph& graph,
                                                       VScriptNode* node,
                                                       int indent_level) const;

    /**
     * @brief Generate enhanced node code with connection context
     * @param graph The visual script graph
     * @param node The node to generate code for
     * @param indent_level Current indentation level
     * @return Vector of code lines
     */
    std::vector<std::string> GenerateNodeCode(const VScriptGraph& graph,
                                              VScriptNode* node,
                                              int indent_level) const;

    /**
     * @brief Replace placeholders in generated code with actual connected values
     * @param graph The visual script graph
     * @param node The node that generated the code
     * @param line The code line to process (modified in place)
     */
    void ReplaceCodePlaceholders(const VScriptGraph& graph,
                                VScriptNode* node,
                                std::string& line) const;

    bool m_include_debug_comments;     ///< Whether to include debug comments
    std::string m_indentation;         ///< Indentation string (default: 4 spaces)
};

} // namespace Lupine
