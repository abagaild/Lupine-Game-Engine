#include "lupine/visualscripting/VScriptNodeTypes.h"
#include <unordered_map>
#include <sstream>

namespace Lupine {

// Factory Implementation
std::unique_ptr<VScriptNode> VScriptNodeFactory::CreateNode(const std::string& type, const std::string& id) {
    // Event nodes
    if (type == "OnReady") return std::make_unique<OnReadyNode>(id);
    if (type == "OnUpdate") return std::make_unique<OnUpdateNode>(id);
    if (type == "OnInput") return std::make_unique<OnInputNode>(id);
    if (type == "OnClick") return std::make_unique<OnClickNode>(id);
    if (type == "OnMouseDown") return std::make_unique<OnMouseDownNode>(id);
    if (type == "OnMouseUp") return std::make_unique<OnMouseUpNode>(id);
    if (type == "OnKeyPress") return std::make_unique<OnKeyPressNode>(id);
    if (type == "OnKeyRelease") return std::make_unique<OnKeyReleaseNode>(id);

    // Flow control nodes
    if (type == "If") return std::make_unique<IfNode>(id);
    if (type == "ForLoop") return std::make_unique<ForLoopNode>(id);
    if (type == "WhileLoop") return std::make_unique<WhileLoopNode>(id);

    // Variable nodes
    if (type == "GetVariable") return std::make_unique<GetVariableNode>(id);
    if (type == "SetVariable") return std::make_unique<SetVariableNode>(id);
    if (type == "LiteralString") return std::make_unique<LiteralStringNode>(id);
    if (type == "LiteralNumber") return std::make_unique<LiteralNumberNode>(id);
    if (type == "LiteralBool") return std::make_unique<LiteralBoolNode>(id);

    // Math nodes
    if (type == "Add") return std::make_unique<AddNode>(id);
    if (type == "Subtract") return std::make_unique<SubtractNode>(id);
    if (type == "Multiply") return std::make_unique<MultiplyNode>(id);
    if (type == "Divide") return std::make_unique<DivideNode>(id);

    // Logic nodes
    if (type == "And") return std::make_unique<AndNode>(id);
    if (type == "Or") return std::make_unique<OrNode>(id);
    if (type == "Not") return std::make_unique<NotNode>(id);
    if (type == "Compare") return std::make_unique<CompareNode>(id);

    // Function nodes
    if (type == "Print") return std::make_unique<PrintNode>(id);
    if (type == "FunctionCall") return std::make_unique<FunctionCallNode>(id);

    // System nodes
    if (type == "SystemTime") return std::make_unique<SystemTimeNode>(id);
    if (type == "ElapsedTime") return std::make_unique<ElapsedTimeNode>(id);
    if (type == "DeltaTime") return std::make_unique<DeltaTimeNode>(id);
    if (type == "Random") return std::make_unique<RandomNode>(id);
    if (type == "Delay") return std::make_unique<DelayNode>(id);

    // Input nodes
    if (type == "GetInput") return std::make_unique<GetInputNode>(id);
    if (type == "IsKeyPressed") return std::make_unique<IsKeyPressedNode>(id);
    if (type == "MousePosition") return std::make_unique<MousePositionNode>(id);

    // Advanced flow control nodes
    if (type == "Sequence") return std::make_unique<SequenceNode>(id);
    if (type == "Branch") return std::make_unique<BranchNode>(id);
    if (type == "Switch") return std::make_unique<SwitchNode>(id);
    if (type == "ForEachLoop") return std::make_unique<ForEachLoopNode>(id);
    if (type == "Gate") return std::make_unique<GateNode>(id);
    if (type == "DoOnce") return std::make_unique<DoOnceNode>(id);
    if (type == "DoN") return std::make_unique<DoNNode>(id);
    if (type == "FlipFlop") return std::make_unique<FlipFlopNode>(id);

    // Array nodes
    if (type == "MakeArray") return std::make_unique<MakeArrayNode>(id);
    if (type == "BreakArray") return std::make_unique<BreakArrayNode>(id);
    if (type == "ArrayLength") return std::make_unique<ArrayLengthNode>(id);
    if (type == "ArrayGet") return std::make_unique<ArrayGetNode>(id);
    if (type == "ArraySet") return std::make_unique<ArraySetNode>(id);
    if (type == "ArrayAdd") return std::make_unique<ArrayAddNode>(id);

    // Utility nodes
    if (type == "Select") return std::make_unique<SelectNode>(id);
    if (type == "Cast") return std::make_unique<CastNode>(id);
    if (type == "IsValid") return std::make_unique<IsValidNode>(id);

    // Organization nodes
    if (type == "Comment") return std::make_unique<CommentNode>(id);
    if (type == "Reroute") return std::make_unique<RerouteNode>(id);

    // Custom nodes
    if (type == "CustomSnippet") return std::make_unique<CustomSnippetNode>(id);

    return nullptr;
}

std::vector<std::string> VScriptNodeFactory::GetAvailableNodeTypes() {
    return {
        // Event nodes
        "OnReady", "OnUpdate", "OnInput", "OnClick", "OnMouseDown", "OnMouseUp", "OnKeyPress", "OnKeyRelease",
        // Flow control
        "If", "ForLoop", "WhileLoop", "Sequence", "Branch", "Switch", "ForEachLoop", "Gate", "DoOnce", "DoN", "FlipFlop",
        // Variables
        "GetVariable", "SetVariable", "LiteralString", "LiteralNumber", "LiteralBool",
        // Math
        "Add", "Subtract", "Multiply", "Divide",
        // Logic
        "And", "Or", "Not", "Compare",
        // Functions
        "Print", "FunctionCall",
        // System
        "SystemTime", "ElapsedTime", "DeltaTime", "Random", "Delay",
        // Input
        "GetInput", "IsKeyPressed", "MousePosition",
        // Arrays
        "MakeArray", "BreakArray", "ArrayLength", "ArrayGet", "ArraySet", "ArrayAdd",
        // Utilities
        "Select", "Cast", "IsValid",
        // Organization
        "Comment", "Reroute",
        // Custom
        "CustomSnippet"
    };
}

std::vector<std::string> VScriptNodeFactory::GetNodeTypesByCategory(VScriptNodeCategory category) {
    switch (category) {
        case VScriptNodeCategory::Event:
            return {"OnReady", "OnUpdate", "OnInput", "OnClick", "OnMouseDown", "OnMouseUp", "OnKeyPress", "OnKeyRelease"};
        case VScriptNodeCategory::FlowControl:
            return {"If", "ForLoop", "WhileLoop", "Sequence", "Branch", "Switch", "ForEachLoop", "Gate", "DoOnce", "DoN", "FlipFlop", "Delay"};
        case VScriptNodeCategory::Variable:
            return {"GetVariable", "SetVariable", "LiteralString", "LiteralNumber", "LiteralBool", "MakeArray", "BreakArray", "ArrayLength", "ArrayGet", "ArraySet", "ArrayAdd", "SystemTime", "ElapsedTime", "DeltaTime"};
        case VScriptNodeCategory::Math:
            return {"Add", "Subtract", "Multiply", "Divide", "Random"};
        case VScriptNodeCategory::Logic:
            return {"And", "Or", "Not", "Compare", "Select", "Cast", "IsValid"};
        case VScriptNodeCategory::Function:
            return {"Print", "FunctionCall", "GetInput", "IsKeyPressed", "MousePosition"};
        case VScriptNodeCategory::Custom:
            return {"CustomSnippet", "Comment", "Reroute"};
        default:
            return {};
    }
}

// Event Nodes Implementation
OnReadyNode::OnReadyNode(const std::string& id) 
    : VScriptNode(id, "OnReady", VScriptNodeCategory::Event) {
    SetDisplayName("On Ready");
    SetDescription("Called when the node is ready");
}

void OnReadyNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> OnReadyNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnUpdateNode::OnUpdateNode(const std::string& id) 
    : VScriptNode(id, "OnUpdate", VScriptNodeCategory::Event) {
    SetDisplayName("On Update");
    SetDescription("Called every frame");
}

void OnUpdateNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("delta_time", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> OnUpdateNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnInputNode::OnInputNode(const std::string& id) 
    : VScriptNode(id, "OnInput", VScriptNodeCategory::Event) {
    SetDisplayName("On Input");
    SetDescription("Called when input is received");
}

void OnInputNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("event", VScriptDataType::Object, VScriptPinDirection::Output));
}

std::vector<std::string> OnInputNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnClickNode::OnClickNode(const std::string& id)
    : VScriptNode(id, "OnClick", VScriptNodeCategory::Event) {
    SetDisplayName("On Click");
    SetDescription("Called when mouse is clicked");
}

void OnClickNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("mouse_pos", VScriptDataType::Vector2, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("button", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> OnClickNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnMouseDownNode::OnMouseDownNode(const std::string& id)
    : VScriptNode(id, "OnMouseDown", VScriptNodeCategory::Event) {
    SetDisplayName("On Mouse Down");
    SetDescription("Called when mouse button is pressed");
}

void OnMouseDownNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("mouse_pos", VScriptDataType::Vector2, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("button", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> OnMouseDownNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnMouseUpNode::OnMouseUpNode(const std::string& id)
    : VScriptNode(id, "OnMouseUp", VScriptNodeCategory::Event) {
    SetDisplayName("On Mouse Up");
    SetDescription("Called when mouse button is released");
}

void OnMouseUpNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("mouse_pos", VScriptDataType::Vector2, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("button", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> OnMouseUpNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnKeyPressNode::OnKeyPressNode(const std::string& id)
    : VScriptNode(id, "OnKeyPress", VScriptNodeCategory::Event) {
    SetDisplayName("On Key Press");
    SetDescription("Called when a key is pressed");
}

void OnKeyPressNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("key_code", VScriptDataType::Integer, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("key_name", VScriptDataType::String, VScriptPinDirection::Output));
}

std::vector<std::string> OnKeyPressNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

OnKeyReleaseNode::OnKeyReleaseNode(const std::string& id)
    : VScriptNode(id, "OnKeyRelease", VScriptNodeCategory::Event) {
    SetDisplayName("On Key Release");
    SetDescription("Called when a key is released");
}

void OnKeyReleaseNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("key_code", VScriptDataType::Integer, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("key_name", VScriptDataType::String, VScriptPinDirection::Output));
}

std::vector<std::string> OnKeyReleaseNode::GenerateCode(int indent_level) const {
    return {}; // Entry point, no code generation needed
}

// Flow Control Nodes Implementation
IfNode::IfNode(const std::string& id) 
    : VScriptNode(id, "If", VScriptNodeCategory::FlowControl) {
    SetDisplayName("If");
    SetDescription("Conditional execution");
}

void IfNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("condition", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("true", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("false", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> IfNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    std::string condition = GetProperty("condition", "True");
    lines.push_back(indent + "if " + condition + ":");
    
    return lines;
}

ForLoopNode::ForLoopNode(const std::string& id) 
    : VScriptNode(id, "ForLoop", VScriptNodeCategory::FlowControl) {
    SetDisplayName("For Loop");
    SetDescription("Loop with counter");
}

void ForLoopNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("start", VScriptDataType::Integer, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("end", VScriptDataType::Integer, VScriptPinDirection::Input, "10"));
    AddPin(std::make_unique<VScriptPin>("step", VScriptDataType::Integer, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("loop_body", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("index", VScriptDataType::Integer, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("completed", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> ForLoopNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    std::string start = GetProperty("start", "0");
    std::string end = GetProperty("end", "10");
    std::string step = GetProperty("step", "1");
    
    lines.push_back(indent + "for i in range(" + start + ", " + end + ", " + step + "):");
    
    return lines;
}

WhileLoopNode::WhileLoopNode(const std::string& id) 
    : VScriptNode(id, "WhileLoop", VScriptNodeCategory::FlowControl) {
    SetDisplayName("While Loop");
    SetDescription("Loop while condition is true");
}

void WhileLoopNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("condition", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("loop_body", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("completed", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> WhileLoopNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    std::string condition = GetProperty("condition", "True");
    lines.push_back(indent + "while " + condition + ":");
    
    return lines;
}

// Variable Nodes Implementation
GetVariableNode::GetVariableNode(const std::string& id) 
    : VScriptNode(id, "GetVariable", VScriptNodeCategory::Variable) {
    SetDisplayName("Get Variable");
    SetDescription("Get the value of a variable");
    SetProperty("variable_name", "my_var");
    SetProperty("variable_type", "Any");
}

void GetVariableNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Any, VScriptPinDirection::Output));
}

std::vector<std::string> GetVariableNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    std::string var_name = GetProperty("variable_name", "my_var");
    lines.push_back(indent + "# Get variable: " + var_name);
    lines.push_back(indent + "value = self." + var_name);
    
    return lines;
}

SetVariableNode::SetVariableNode(const std::string& id) 
    : VScriptNode(id, "SetVariable", VScriptNodeCategory::Variable) {
    SetDisplayName("Set Variable");
    SetDescription("Set the value of a variable");
    SetProperty("variable_name", "my_var");
    SetProperty("variable_type", "Any");
}

void SetVariableNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> SetVariableNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    
    std::string var_name = GetProperty("variable_name", "my_var");
    lines.push_back(indent + "# Set variable: " + var_name);
    lines.push_back(indent + "self." + var_name + " = value");
    
    return lines;
}

LiteralStringNode::LiteralStringNode(const std::string& id)
    : VScriptNode(id, "LiteralString", VScriptNodeCategory::Variable) {
    SetDisplayName("String");
    SetDescription("String literal value");
    SetProperty("value", "Hello World");
}

void LiteralStringNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::String, VScriptPinDirection::Output));
}

std::vector<std::string> LiteralStringNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string value = GetProperty("value", "Hello World");
    lines.push_back("\"" + value + "\"");
    return lines;
}

LiteralNumberNode::LiteralNumberNode(const std::string& id)
    : VScriptNode(id, "LiteralNumber", VScriptNodeCategory::Variable) {
    SetDisplayName("Number");
    SetDescription("Numeric literal value");
    SetProperty("value", "0");
    SetProperty("type", "float");
}

void LiteralNumberNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> LiteralNumberNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string value = GetProperty("value", "0");
    lines.push_back(value);
    return lines;
}

LiteralBoolNode::LiteralBoolNode(const std::string& id)
    : VScriptNode(id, "LiteralBool", VScriptNodeCategory::Variable) {
    SetDisplayName("Boolean");
    SetDescription("Boolean literal value");
    SetProperty("value", "True");
}

void LiteralBoolNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> LiteralBoolNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string value = GetProperty("value", "True");
    lines.push_back(value);
    return lines;
}

// Math Nodes Implementation
AddNode::AddNode(const std::string& id)
    : VScriptNode(id, "Add", VScriptNodeCategory::Math) {
    SetDisplayName("Add");
    SetDescription("Add two numbers");
}

void AddNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Float, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Float, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> AddNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a + b");
    return lines;
}

SubtractNode::SubtractNode(const std::string& id)
    : VScriptNode(id, "Subtract", VScriptNodeCategory::Math) {
    SetDisplayName("Subtract");
    SetDescription("Subtract two numbers");
}

void SubtractNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Float, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Float, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> SubtractNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a - b");
    return lines;
}

MultiplyNode::MultiplyNode(const std::string& id)
    : VScriptNode(id, "Multiply", VScriptNodeCategory::Math) {
    SetDisplayName("Multiply");
    SetDescription("Multiply two numbers");
}

void MultiplyNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Float, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Float, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> MultiplyNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a * b");
    return lines;
}

DivideNode::DivideNode(const std::string& id)
    : VScriptNode(id, "Divide", VScriptNodeCategory::Math) {
    SetDisplayName("Divide");
    SetDescription("Divide two numbers");
}

void DivideNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Float, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Float, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> DivideNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a / b if b != 0 else 0");
    return lines;
}

// Logic Nodes Implementation
AndNode::AndNode(const std::string& id)
    : VScriptNode(id, "And", VScriptNodeCategory::Logic) {
    SetDisplayName("And");
    SetDescription("Logical AND operation");
}

void AndNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> AndNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a and b");
    return lines;
}

OrNode::OrNode(const std::string& id)
    : VScriptNode(id, "Or", VScriptNodeCategory::Logic) {
    SetDisplayName("Or");
    SetDescription("Logical OR operation");
}

void OrNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Boolean, VScriptPinDirection::Input, "False"));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Boolean, VScriptPinDirection::Input, "False"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> OrNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = a or b");
    return lines;
}

NotNode::NotNode(const std::string& id)
    : VScriptNode(id, "Not", VScriptNodeCategory::Logic) {
    SetDisplayName("Not");
    SetDescription("Logical NOT operation");
}

void NotNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("input", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> NotNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "result = not input");
    return lines;
}

CompareNode::CompareNode(const std::string& id)
    : VScriptNode(id, "Compare", VScriptNodeCategory::Logic) {
    SetDisplayName("Compare");
    SetDescription("Compare two values");
    SetProperty("operator", "==");
}

void CompareNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> CompareNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string op = GetProperty("operator", "==");
    lines.push_back(indent + "result = a " + op + " b");
    return lines;
}

// Function Nodes Implementation
PrintNode::PrintNode(const std::string& id)
    : VScriptNode(id, "Print", VScriptNodeCategory::Function) {
    SetDisplayName("Print");
    SetDescription("Print a value to console");
}

void PrintNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Any, VScriptPinDirection::Input, "Hello World"));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> PrintNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "print(value)");
    return lines;
}

FunctionCallNode::FunctionCallNode(const std::string& id)
    : VScriptNode(id, "FunctionCall", VScriptNodeCategory::Function) {
    SetDisplayName("Function Call");
    SetDescription("Call a function");
    SetProperty("function_name", "my_function");
}

void FunctionCallNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Any, VScriptPinDirection::Output));
}

std::vector<std::string> FunctionCallNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string func_name = GetProperty("function_name", "my_function");
    lines.push_back(indent + "result = " + func_name + "()");
    return lines;
}

// Custom Nodes Implementation
CustomSnippetNode::CustomSnippetNode(const std::string& id)
    : VScriptNode(id, "CustomSnippet", VScriptNodeCategory::Custom) {
    SetDisplayName("Custom Snippet");
    SetDescription("Custom Python code snippet");
    SetProperty("code", "# Custom Python code here");
}

void CustomSnippetNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> CustomSnippetNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string code = GetProperty("code", "# Custom Python code here");

    // Split code by newlines and indent each line
    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(indent + line);
    }

    return lines;
}

// System Nodes Implementation
SystemTimeNode::SystemTimeNode(const std::string& id)
    : VScriptNode(id, "SystemTime", VScriptNodeCategory::Variable) {
    SetDisplayName("System Time");
    SetDescription("Get current system time");
}

void SystemTimeNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("timestamp", VScriptDataType::Float, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("formatted", VScriptDataType::String, VScriptPinDirection::Output));
}

std::vector<std::string> SystemTimeNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "import time");
    lines.push_back(indent + "timestamp = time.time()");
    lines.push_back(indent + "formatted = time.strftime('%Y-%m-%d %H:%M:%S')");
    return lines;
}

ElapsedTimeNode::ElapsedTimeNode(const std::string& id)
    : VScriptNode(id, "ElapsedTime", VScriptNodeCategory::Variable) {
    SetDisplayName("Elapsed Time");
    SetDescription("Get elapsed time since start");
}

void ElapsedTimeNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("seconds", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> ElapsedTimeNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Get elapsed time from engine");
    lines.push_back(indent + "seconds = self.get_elapsed_time()");
    return lines;
}

DeltaTimeNode::DeltaTimeNode(const std::string& id)
    : VScriptNode(id, "DeltaTime", VScriptNodeCategory::Variable) {
    SetDisplayName("Delta Time");
    SetDescription("Get frame delta time");
}

void DeltaTimeNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("delta", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> DeltaTimeNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Get delta time from engine");
    lines.push_back(indent + "delta = self.get_delta_time()");
    return lines;
}

RandomNode::RandomNode(const std::string& id)
    : VScriptNode(id, "Random", VScriptNodeCategory::Math) {
    SetDisplayName("Random");
    SetDescription("Generate random number");
    SetProperty("min_value", "0.0");
    SetProperty("max_value", "1.0");
}

void RandomNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("min", VScriptDataType::Float, VScriptPinDirection::Input, "0.0"));
    AddPin(std::make_unique<VScriptPin>("max", VScriptDataType::Float, VScriptPinDirection::Input, "1.0"));
    AddPin(std::make_unique<VScriptPin>("value", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> RandomNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "import random");
    lines.push_back(indent + "value = random.uniform(min, max)");
    return lines;
}

DelayNode::DelayNode(const std::string& id)
    : VScriptNode(id, "Delay", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Delay");
    SetDescription("Delay execution for specified time");
    SetProperty("duration", "1.0");
}

void DelayNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("duration", VScriptDataType::Float, VScriptPinDirection::Input, "1.0"));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> DelayNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "import time");
    lines.push_back(indent + "time.sleep(duration)");
    return lines;
}

// Input Nodes Implementation
GetInputNode::GetInputNode(const std::string& id)
    : VScriptNode(id, "GetInput", VScriptNodeCategory::Function) {
    SetDisplayName("Get Input");
    SetDescription("Get input action state");
    SetProperty("action_name", "move_left");
}

void GetInputNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("action", VScriptDataType::String, VScriptPinDirection::Input, "move_left"));
    AddPin(std::make_unique<VScriptPin>("pressed", VScriptDataType::Boolean, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("strength", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> GetInputNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Get input action state");
    lines.push_back(indent + "pressed = self.is_action_pressed(action)");
    lines.push_back(indent + "strength = self.get_action_strength(action)");
    return lines;
}

IsKeyPressedNode::IsKeyPressedNode(const std::string& id)
    : VScriptNode(id, "IsKeyPressed", VScriptNodeCategory::Function) {
    SetDisplayName("Is Key Pressed");
    SetDescription("Check if a key is currently pressed");
    SetProperty("key_code", "32");  // Space key
}

void IsKeyPressedNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("key_code", VScriptDataType::Integer, VScriptPinDirection::Input, "32"));
    AddPin(std::make_unique<VScriptPin>("pressed", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> IsKeyPressedNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Check if key is pressed");
    lines.push_back(indent + "pressed = self.is_key_pressed(key_code)");
    return lines;
}

MousePositionNode::MousePositionNode(const std::string& id)
    : VScriptNode(id, "MousePosition", VScriptNodeCategory::Function) {
    SetDisplayName("Mouse Position");
    SetDescription("Get current mouse position");
}

void MousePositionNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("position", VScriptDataType::Vector2, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("x", VScriptDataType::Float, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("y", VScriptDataType::Float, VScriptPinDirection::Output));
}

std::vector<std::string> MousePositionNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Get mouse position");
    lines.push_back(indent + "position = self.get_mouse_position()");
    lines.push_back(indent + "x = position[0]");
    lines.push_back(indent + "y = position[1]");
    return lines;
}

// Advanced Flow Control Nodes Implementation
SequenceNode::SequenceNode(const std::string& id)
    : VScriptNode(id, "Sequence", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Sequence");
    SetDescription("Execute multiple outputs in sequence");
}

void SequenceNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("then_0", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("then_1", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("then_2", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> SequenceNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Sequence execution");
    return lines;
}

BranchNode::BranchNode(const std::string& id)
    : VScriptNode(id, "Branch", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Branch");
    SetDescription("Branch execution based on boolean condition");
}

void BranchNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("condition", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("true", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("false", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> BranchNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string condition = GetProperty("condition", "True");
    lines.push_back(indent + "if " + condition + ":");
    lines.push_back(indent + "    # True branch");
    lines.push_back(indent + "else:");
    lines.push_back(indent + "    # False branch");
    return lines;
}

SwitchNode::SwitchNode(const std::string& id)
    : VScriptNode(id, "Switch", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Switch");
    SetDescription("Switch execution based on integer value");
}

void SwitchNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("selection", VScriptDataType::Integer, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("case_0", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("case_1", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("case_2", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("default", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> SwitchNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string selection = GetProperty("selection", "0");
    lines.push_back(indent + "if " + selection + " == 0:");
    lines.push_back(indent + "    # Case 0");
    lines.push_back(indent + "elif " + selection + " == 1:");
    lines.push_back(indent + "    # Case 1");
    lines.push_back(indent + "elif " + selection + " == 2:");
    lines.push_back(indent + "    # Case 2");
    lines.push_back(indent + "else:");
    lines.push_back(indent + "    # Default case");
    return lines;
}

ForEachLoopNode::ForEachLoopNode(const std::string& id)
    : VScriptNode(id, "ForEachLoop", VScriptNodeCategory::FlowControl) {
    SetDisplayName("For Each Loop");
    SetDescription("Iterate over each element in an array");
}

void ForEachLoopNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("loop_body", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("element", VScriptDataType::Any, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("index", VScriptDataType::Integer, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("completed", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> ForEachLoopNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "for index, element in enumerate(array):");
    lines.push_back(indent + "    # Loop body");
    return lines;
}

GateNode::GateNode(const std::string& id)
    : VScriptNode(id, "Gate", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Gate");
    SetDescription("Control execution flow with open/close gate");
}

void GateNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("enter", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("open", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("close", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("toggle", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("start_closed", VScriptDataType::Boolean, VScriptPinDirection::Input, "False"));
    AddPin(std::make_unique<VScriptPin>("exit", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> GateNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Gate logic");
    lines.push_back(indent + "if gate_is_open:");
    lines.push_back(indent + "    # Execute when gate is open");
    return lines;
}

DoOnceNode::DoOnceNode(const std::string& id)
    : VScriptNode(id, "DoOnce", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Do Once");
    SetDescription("Execute only once, then block further execution");
}

void DoOnceNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("reset", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> DoOnceNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "if not self.has_executed:");
    lines.push_back(indent + "    self.has_executed = True");
    lines.push_back(indent + "    # Execute once");
    return lines;
}

DoNNode::DoNNode(const std::string& id)
    : VScriptNode(id, "DoN", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Do N");
    SetDescription("Execute N times, then block further execution");
}

void DoNNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("reset", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("n", VScriptDataType::Integer, VScriptPinDirection::Input, "1"));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("counter", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> DoNNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string n = GetProperty("n", "1");
    lines.push_back(indent + "if self.execution_count < " + n + ":");
    lines.push_back(indent + "    self.execution_count += 1");
    lines.push_back(indent + "    # Execute");
    return lines;
}

FlipFlopNode::FlipFlopNode(const std::string& id)
    : VScriptNode(id, "FlipFlop", VScriptNodeCategory::FlowControl) {
    SetDisplayName("Flip Flop");
    SetDescription("Alternate between two outputs on each execution");
}

void FlipFlopNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("a", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("b", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("is_a", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> FlipFlopNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "self.flip_state = not self.flip_state");
    lines.push_back(indent + "if self.flip_state:");
    lines.push_back(indent + "    # Execute A");
    lines.push_back(indent + "else:");
    lines.push_back(indent + "    # Execute B");
    return lines;
}

// Array Nodes Implementation
MakeArrayNode::MakeArrayNode(const std::string& id)
    : VScriptNode(id, "MakeArray", VScriptNodeCategory::Variable) {
    SetDisplayName("Make Array");
    SetDescription("Create an array from individual elements");
}

void MakeArrayNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("element_0", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("element_1", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("element_2", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Output, true));
}

std::vector<std::string> MakeArrayNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "array = [element_0, element_1, element_2]");
    return lines;
}

BreakArrayNode::BreakArrayNode(const std::string& id)
    : VScriptNode(id, "BreakArray", VScriptNodeCategory::Variable) {
    SetDisplayName("Break Array");
    SetDescription("Break an array into individual elements");
}

void BreakArrayNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("element_0", VScriptDataType::Any, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("element_1", VScriptDataType::Any, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("element_2", VScriptDataType::Any, VScriptPinDirection::Output));
}

std::vector<std::string> BreakArrayNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "element_0 = array[0] if len(array) > 0 else None");
    lines.push_back(indent + "element_1 = array[1] if len(array) > 1 else None");
    lines.push_back(indent + "element_2 = array[2] if len(array) > 2 else None");
    return lines;
}

ArrayLengthNode::ArrayLengthNode(const std::string& id)
    : VScriptNode(id, "ArrayLength", VScriptNodeCategory::Variable) {
    SetDisplayName("Array Length");
    SetDescription("Get the length of an array");
}

void ArrayLengthNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("length", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> ArrayLengthNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "length = len(array)");
    return lines;
}

ArrayGetNode::ArrayGetNode(const std::string& id)
    : VScriptNode(id, "ArrayGet", VScriptNodeCategory::Variable) {
    SetDisplayName("Array Get");
    SetDescription("Get an element from an array by index");
}

void ArrayGetNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("index", VScriptDataType::Integer, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("element", VScriptDataType::Any, VScriptPinDirection::Output));
}

std::vector<std::string> ArrayGetNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string index = GetProperty("index", "0");
    lines.push_back(indent + "element = array[" + index + "] if " + index + " < len(array) else None");
    return lines;
}

ArraySetNode::ArraySetNode(const std::string& id)
    : VScriptNode(id, "ArraySet", VScriptNodeCategory::Variable) {
    SetDisplayName("Array Set");
    SetDescription("Set an element in an array by index");
}

void ArraySetNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("index", VScriptDataType::Integer, VScriptPinDirection::Input, "0"));
    AddPin(std::make_unique<VScriptPin>("element", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
}

std::vector<std::string> ArraySetNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string index = GetProperty("index", "0");
    lines.push_back(indent + "if " + index + " < len(array):");
    lines.push_back(indent + "    array[" + index + "] = element");
    return lines;
}

ArrayAddNode::ArrayAddNode(const std::string& id)
    : VScriptNode(id, "ArrayAdd", VScriptNodeCategory::Variable) {
    SetDisplayName("Array Add");
    SetDescription("Add an element to the end of an array");
}

void ArrayAddNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("array", VScriptDataType::Any, VScriptPinDirection::Input, true));
    AddPin(std::make_unique<VScriptPin>("element", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("exec_out", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("index", VScriptDataType::Integer, VScriptPinDirection::Output));
}

std::vector<std::string> ArrayAddNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "array.append(element)");
    lines.push_back(indent + "index = len(array) - 1");
    return lines;
}

// Utility Nodes Implementation
SelectNode::SelectNode(const std::string& id)
    : VScriptNode(id, "Select", VScriptNodeCategory::Logic) {
    SetDisplayName("Select");
    SetDescription("Select between two values based on a boolean condition");
}

void SelectNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("condition", VScriptDataType::Boolean, VScriptPinDirection::Input, "True"));
    AddPin(std::make_unique<VScriptPin>("true_value", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("false_value", VScriptDataType::Any, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("result", VScriptDataType::Any, VScriptPinDirection::Output));
}

std::vector<std::string> SelectNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string condition = GetProperty("condition", "True");
    lines.push_back(indent + "result = true_value if " + condition + " else false_value");
    return lines;
}

CastNode::CastNode(const std::string& id)
    : VScriptNode(id, "Cast", VScriptNodeCategory::Logic) {
    SetDisplayName("Cast");
    SetDescription("Cast an object to a specific type");
}

void CastNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("exec_in", VScriptDataType::Execution, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("object", VScriptDataType::Object, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("success", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("failed", VScriptDataType::Execution, VScriptPinDirection::Output));
    AddPin(std::make_unique<VScriptPin>("cast_result", VScriptDataType::Object, VScriptPinDirection::Output));
}

std::vector<std::string> CastNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string target_type = GetProperty("target_type", "object");
    lines.push_back(indent + "try:");
    lines.push_back(indent + "    cast_result = " + target_type + "(object)");
    lines.push_back(indent + "    # Success path");
    lines.push_back(indent + "except:");
    lines.push_back(indent + "    cast_result = None");
    lines.push_back(indent + "    # Failed path");
    return lines;
}

IsValidNode::IsValidNode(const std::string& id)
    : VScriptNode(id, "IsValid", VScriptNodeCategory::Logic) {
    SetDisplayName("Is Valid");
    SetDescription("Check if an object reference is valid (not null)");
}

void IsValidNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("object", VScriptDataType::Object, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("is_valid", VScriptDataType::Boolean, VScriptPinDirection::Output));
}

std::vector<std::string> IsValidNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "is_valid = object is not None");
    return lines;
}

// Comment and Organization Nodes Implementation
CommentNode::CommentNode(const std::string& id)
    : VScriptNode(id, "Comment", VScriptNodeCategory::Custom) {
    SetDisplayName("Comment");
    SetDescription("Add comments and notes to your graph");
    SetProperty("comment_text", "Comment");
    SetProperty("comment_color", "#FFFF88");
    SetProperty("width", "200");
    SetProperty("height", "100");
}

void CommentNode::InitializePins() {
    // Comment nodes don't have pins
}

std::vector<std::string> CommentNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    std::string comment_text = GetProperty("comment_text", "Comment");
    lines.push_back(indent + "# " + comment_text);
    return lines;
}

void CommentNode::SetCommentSize(float width, float height) {
    SetProperty("width", std::to_string(width));
    SetProperty("height", std::to_string(height));
}

std::pair<float, float> CommentNode::GetCommentSize() const {
    float width = std::stof(GetProperty("width", "200"));
    float height = std::stof(GetProperty("height", "100"));
    return {width, height};
}

RerouteNode::RerouteNode(const std::string& id)
    : VScriptNode(id, "Reroute", VScriptNodeCategory::Custom) {
    SetDisplayName("Reroute");
    SetDescription("Reroute connections for cleaner graph layout");
}

void RerouteNode::InitializePins() {
    AddPin(std::make_unique<VScriptPin>("input", VScriptDataType::Wildcard, VScriptPinDirection::Input));
    AddPin(std::make_unique<VScriptPin>("output", VScriptDataType::Wildcard, VScriptPinDirection::Output));
}

std::vector<std::string> RerouteNode::GenerateCode(int indent_level) const {
    std::vector<std::string> lines;
    std::string indent(indent_level * 4, ' ');
    lines.push_back(indent + "# Reroute (pass-through)");
    lines.push_back(indent + "output = input");
    return lines;
}

} // namespace Lupine
