#pragma once

#include "VScriptNode.h"
#include <memory>

namespace Lupine {

/**
 * @brief Factory for creating visual script nodes
 */
class VScriptNodeFactory {
public:
    /**
     * @brief Create a node by type name
     * @param type Node type name
     * @param id Unique node ID
     * @return Created node or nullptr if type not found
     */
    static std::unique_ptr<VScriptNode> CreateNode(const std::string& type, const std::string& id);

    /**
     * @brief Get all available node types
     * @return Vector of node type names
     */
    static std::vector<std::string> GetAvailableNodeTypes();

    /**
     * @brief Get node types by category
     * @param category Node category
     * @return Vector of node type names in the category
     */
    static std::vector<std::string> GetNodeTypesByCategory(VScriptNodeCategory category);
};

// Event Nodes
class OnReadyNode : public VScriptNode {
public:
    OnReadyNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnUpdateNode : public VScriptNode {
public:
    OnUpdateNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnInputNode : public VScriptNode {
public:
    OnInputNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnClickNode : public VScriptNode {
public:
    OnClickNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnMouseDownNode : public VScriptNode {
public:
    OnMouseDownNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnMouseUpNode : public VScriptNode {
public:
    OnMouseUpNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnKeyPressNode : public VScriptNode {
public:
    OnKeyPressNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OnKeyReleaseNode : public VScriptNode {
public:
    OnKeyReleaseNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Flow Control Nodes
class IfNode : public VScriptNode {
public:
    IfNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ForLoopNode : public VScriptNode {
public:
    ForLoopNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class WhileLoopNode : public VScriptNode {
public:
    WhileLoopNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Variable Nodes
class GetVariableNode : public VScriptNode {
public:
    GetVariableNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class SetVariableNode : public VScriptNode {
public:
    SetVariableNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class LiteralStringNode : public VScriptNode {
public:
    LiteralStringNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class LiteralNumberNode : public VScriptNode {
public:
    LiteralNumberNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class LiteralBoolNode : public VScriptNode {
public:
    LiteralBoolNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Math Nodes
class AddNode : public VScriptNode {
public:
    AddNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class SubtractNode : public VScriptNode {
public:
    SubtractNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class MultiplyNode : public VScriptNode {
public:
    MultiplyNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class DivideNode : public VScriptNode {
public:
    DivideNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Logic Nodes
class AndNode : public VScriptNode {
public:
    AndNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class OrNode : public VScriptNode {
public:
    OrNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class NotNode : public VScriptNode {
public:
    NotNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class CompareNode : public VScriptNode {
public:
    CompareNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Function Nodes
class PrintNode : public VScriptNode {
public:
    PrintNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class FunctionCallNode : public VScriptNode {
public:
    FunctionCallNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// System Nodes
class SystemTimeNode : public VScriptNode {
public:
    SystemTimeNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ElapsedTimeNode : public VScriptNode {
public:
    ElapsedTimeNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class DeltaTimeNode : public VScriptNode {
public:
    DeltaTimeNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class RandomNode : public VScriptNode {
public:
    RandomNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class DelayNode : public VScriptNode {
public:
    DelayNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Input Nodes
class GetInputNode : public VScriptNode {
public:
    GetInputNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class IsKeyPressedNode : public VScriptNode {
public:
    IsKeyPressedNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class MousePositionNode : public VScriptNode {
public:
    MousePositionNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Custom Nodes
class CustomSnippetNode : public VScriptNode {
public:
    CustomSnippetNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Advanced Flow Control Nodes
class SequenceNode : public VScriptNode {
public:
    SequenceNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class BranchNode : public VScriptNode {
public:
    BranchNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class SwitchNode : public VScriptNode {
public:
    SwitchNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ForEachLoopNode : public VScriptNode {
public:
    ForEachLoopNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class GateNode : public VScriptNode {
public:
    GateNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class DoOnceNode : public VScriptNode {
public:
    DoOnceNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class DoNNode : public VScriptNode {
public:
    DoNNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class FlipFlopNode : public VScriptNode {
public:
    FlipFlopNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Utility Nodes
class MakeArrayNode : public VScriptNode {
public:
    MakeArrayNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class BreakArrayNode : public VScriptNode {
public:
    BreakArrayNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ArrayLengthNode : public VScriptNode {
public:
    ArrayLengthNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ArrayGetNode : public VScriptNode {
public:
    ArrayGetNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ArraySetNode : public VScriptNode {
public:
    ArraySetNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class ArrayAddNode : public VScriptNode {
public:
    ArrayAddNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class SelectNode : public VScriptNode {
public:
    SelectNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class CastNode : public VScriptNode {
public:
    CastNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

class IsValidNode : public VScriptNode {
public:
    IsValidNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

// Comment and Organization Nodes
class CommentNode : public VScriptNode {
public:
    CommentNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;

    // Comment-specific methods
    void SetCommentText(const std::string& text) { SetProperty("comment_text", text); }
    std::string GetCommentText() const { return GetProperty("comment_text", "Comment"); }
    void SetCommentColor(const std::string& color) { SetProperty("comment_color", color); }
    std::string GetCommentColor() const { return GetProperty("comment_color", "#FFFF88"); }
    void SetCommentSize(float width, float height);
    std::pair<float, float> GetCommentSize() const;

    // Override to indicate this is a comment node
    bool IsCommentNode() const override { return true; }

protected:
    void InitializePins() override;
};

class RerouteNode : public VScriptNode {
public:
    RerouteNode(const std::string& id);
    std::vector<std::string> GenerateCode(int indent_level = 0) const override;
protected:
    void InitializePins() override;
};

} // namespace Lupine
