#define SDL_MAIN_HANDLED
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "lupine/Lupine.h"
#include "lupine/scripting/LuaScriptComponent.h"
#include "lupine/scripting/PythonScriptComponent.h"

void PrintUsage() {
    std::cout << "Lupine Game Engine CLI v" << Lupine::Version::GetVersionString() << std::endl;
    std::cout << "Usage: lupine-cli <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  create-project <name> [directory]  Create a new project" << std::endl;
    std::cout << "  create-scene <name>                Create a new scene" << std::endl;
    std::cout << "  add-node <type> <name> [parent]    Add a node to current scene" << std::endl;
    std::cout << "  remove-node <name>                 Remove a node from current scene" << std::endl;
    std::cout << "  add-component <type> <node>        Add a component to a node" << std::endl;
    std::cout << "  remove-component <uuid> <node>     Remove a component from a node" << std::endl;
    std::cout << "  list-nodes                         List all nodes in current scene" << std::endl;
    std::cout << "  list-components <node>             List all components on a node" << std::endl;
    std::cout << "  set-property <node> <component> <property> <value>  Set component property" << std::endl;
    std::cout << "  get-property <node> <component> <property>          Get component property" << std::endl;
    std::cout << "  save-scene [filename]              Save current scene" << std::endl;
    std::cout << "  load-scene <filename>              Load a scene" << std::endl;
    std::cout << "  run [scene]                        Run scene in runtime" << std::endl;
    std::cout << "  help                               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Node types: Node, Node2D, Node3D, Control" << std::endl;
    std::cout << "Component types: Sprite2D, Label, PrimitiveMesh, LuaScriptComponent, PythonScriptComponent" << std::endl;
}

bool CreateProject(const std::string& name, const std::string& directory) {
    std::cout << "Creating project '" << name << "' in directory '" << directory << "'..." << std::endl;
    
    if (Lupine::Project::CreateProject(directory, name)) {
        std::cout << "Project created successfully!" << std::endl;
        std::cout << "Project file: " << directory << "/" << name << ".lupine" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to create project!" << std::endl;
        return false;
    }
}

bool CreateScene(const std::string& name) {
    std::cout << "Creating scene '" << name << "'..." << std::endl;

    // Create a new scene
    auto scene = std::make_unique<Lupine::Scene>(name);

    // Create a root node
    auto root = scene->CreateRootNode<Lupine::Node>("Root");

    // Add some example nodes
    auto node2d = std::make_unique<Lupine::Node2D>("Player");
    auto sprite = std::make_unique<Lupine::Sprite2D>();
    sprite->SetTexturePath("assets/textures/player.png");
    sprite->SetSize(glm::vec2(64.0f, 64.0f));
    node2d->AddComponent(std::move(sprite));
    root->AddChild(std::move(node2d));

    // Add a node with Lua script component
    auto lua_node = std::make_unique<Lupine::Node>("LuaScriptNode");
    auto lua_script = std::make_unique<Lupine::LuaScriptComponent>();
    lua_script->SetScriptPath("examples/scripts/example_lua_script.lua");
    lua_node->AddComponent(std::move(lua_script));
    root->AddChild(std::move(lua_node));

    // Add a node with Python script component
    auto python_node = std::make_unique<Lupine::Node>("PythonScriptNode");
    auto python_script = std::make_unique<Lupine::PythonScriptComponent>();
    python_script->SetScriptPath("examples/scripts/example_python_script.py");
    python_node->AddComponent(std::move(python_script));
    root->AddChild(std::move(python_node));

    // Save the scene
    std::string filename = name + ".scene";
    if (scene->SaveToFile(filename)) {
        std::cout << "Scene created successfully: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to save scene!" << std::endl;
        return false;
    }
}

void ShowHelp() {
    PrintUsage();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "help" || command == "--help" || command == "-h") {
        ShowHelp();
        return 0;
    }
    
    if (command == "create-project") {
        if (argc < 3) {
            std::cerr << "Error: Project name required" << std::endl;
            std::cerr << "Usage: lupine-cli create-project <name> [directory]" << std::endl;
            return 1;
        }
        
        std::string name = argv[2];
        std::string directory = (argc >= 4) ? argv[3] : ".";
        
        if (!CreateProject(name, directory)) {
            return 1;
        }
    }
    else if (command == "create-scene") {
        if (argc < 3) {
            std::cerr << "Error: Scene name required" << std::endl;
            std::cerr << "Usage: lupine-cli create-scene <name>" << std::endl;
            return 1;
        }
        
        std::string name = argv[2];
        
        if (!CreateScene(name)) {
            return 1;
        }
    }
    else if (command == "add-node") {
        if (argc < 4) {
            std::cerr << "Error: Node type and name required" << std::endl;
            std::cerr << "Usage: lupine-cli add-node <type> <name> [parent]" << std::endl;
            return 1;
        }
        
        std::string type = argv[2];
        std::string name = argv[3];
        std::string parent = (argc >= 5) ? argv[4] : "";
        
        std::cout << "Adding " << type << " node '" << name << "'";
        if (!parent.empty()) {
            std::cout << " to parent '" << parent << "'";
        }
        std::cout << "..." << std::endl;
        
        // TODO: Implement node creation and scene management
        std::cout << "Node creation not yet implemented in CLI" << std::endl;
    }
    else if (command == "run") {
        std::string scene_name = (argc >= 3) ? argv[2] : "";
        
        std::cout << "Running runtime";
        if (!scene_name.empty()) {
            std::cout << " with scene '" << scene_name << "'";
        }
        std::cout << "..." << std::endl;
        
        // TODO: Launch runtime executable
        std::cout << "Runtime execution not yet implemented" << std::endl;
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << "Use 'lupine-cli help' for usage information" << std::endl;
        return 1;
    }
    
    return 0;
}
