#pragma once

/**
 * @file Lupine.h
 * @brief Main header file for the Lupine Game Engine
 * 
 * This header includes all the core components of the Lupine engine.
 * Include this file to get access to the full engine API.
 */

// Core systems
#include "lupine/core/UUID.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Project.h"
#include "lupine/core/Engine.h"

// Node types
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"

// Components
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Label.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/Camera2D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/components/PlayerController2D.h"
#include "lupine/components/PlayerController3D.h"
#include "lupine/components/PlatformerController2D.h"
#include "lupine/components/PlayerController25D.h"

// Script Components
#include "lupine/scripting/LuaScriptComponent.h"
#include "lupine/scripting/PythonScriptComponent.h"

// Input System
#include "lupine/input/InputManager.h"

// Audio System
#include "lupine/audio/AudioManager.h"
#include "lupine/components/AudioSource.h"

// Physics System
#include "lupine/physics/PhysicsManager.h"
#include "lupine/components/RigidBody2D.h"
#include "lupine/components/RigidBody3D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/components/CollisionPolygon2D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/Area2D.h"
#include "lupine/components/Area3D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/KinematicBody3D.h"

// Terrain Components
#include "lupine/components/TerrainRenderer.h"
#include "lupine/components/TerrainCollider.h"
#include "lupine/components/TerrainLoader.h"

// Serialization
#include "lupine/serialization/SceneSerializer.h"
#include "lupine/serialization/ProjectSerializer.h"
#include "lupine/serialization/SerializationUtils.h"

/**
 * @namespace Lupine
 * @brief Main namespace for the Lupine Game Engine
 * 
 * All engine classes and functions are contained within this namespace.
 */
namespace Lupine {

/**
 * @brief Engine version information
 */
struct Version {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    
    /**
     * @brief Get version string
     * @return Version string in format "MAJOR.MINOR.PATCH"
     */
    static std::string GetVersionString() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
};

// Global utility functions for runtime scene management
/**
 * @brief Switch to a different scene (global function for script access)
 * @param scene_path Path to the scene file or bundle path
 * @return True if scene was switched successfully
 */
inline bool SwitchScene(const std::string& scene_path) {
    Engine* engine = Engine::GetInstance();
    return engine ? engine->SwitchScene(scene_path) : false;
}

} // namespace Lupine
