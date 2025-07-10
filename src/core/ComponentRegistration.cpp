#include "lupine/core/Component.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Label.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/SkinnedMesh.h"
#include "lupine/components/Camera2D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/components/Button.h"
#include "lupine/components/Panel.h"
#include "lupine/components/TextureRectangle.h"
#include "lupine/components/ColorRectangle.h"
#include "lupine/components/NinePatchPanel.h"
#include "lupine/components/ProgressBar.h"
#include "lupine/components/Sprite3D.h"
#include "lupine/components/Skybox3D.h"
#include "lupine/components/OmniLight.h"
#include "lupine/components/DirectionalLight.h"
#include "lupine/components/SpotLight.h"
#include "lupine/components/AudioSource.h"
#include "lupine/components/Tilemap2D.h"
#include "lupine/components/Tilemap25D.h"
#include "lupine/components/Tilemap3D.h"
#include "lupine/components/RigidBody2D.h"
#include "lupine/components/RigidBody3D.h"
#include "lupine/components/PlayerController2D.h"
#include "lupine/components/PlayerController3D.h"
#include "lupine/components/PlatformerController2D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/components/CollisionPolygon2D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/Area2D.h"
#include "lupine/components/Area3D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/TerrainRenderer.h"
#include "lupine/components/TerrainCollider.h"
#include "lupine/components/TerrainLoader.h"
#include "lupine/components/Animator.h"
#include "lupine/components/StateAnimator.h"
#include "lupine/scripting/VisualScriptComponent.h"
#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/components/AnimatedSprite3D.h"
#include "lupine/components/AnimationPlayer.h"
#include "lupine/scripting/LuaScriptComponent.h"
#include "lupine/scripting/PythonScriptComponent.h"

namespace Lupine {

void InitializeComponentRegistry() {
    auto& registry = ComponentRegistry::Instance();
    
    // Manually register all components to ensure they are available
    registry.RegisterComponent("Sprite2D", ComponentInfo(
        "Sprite2D", "2D", "2D sprite component for displaying textures",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Sprite2D>();
        }
    ));
    
    registry.RegisterComponent("Label", ComponentInfo(
        "Label", "UI", "Text label component for displaying text",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Label>();
        }
    ));
    
    registry.RegisterComponent("PrimitiveMesh", ComponentInfo(
        "PrimitiveMesh", "3D", "3D primitive mesh component for rendering basic shapes",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<PrimitiveMesh>();
        }
    ));

    registry.RegisterComponent("StaticMesh", ComponentInfo(
        "StaticMesh", "3D", "3D static mesh component for rendering complex models with multiple meshes",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<StaticMesh>();
        }
    ));

    registry.RegisterComponent("SkinnedMesh", ComponentInfo(
        "SkinnedMesh", "3D", "Animated 3D mesh component with skeletal animation support",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<SkinnedMesh>();
        }
    ));

    registry.RegisterComponent("Camera2D", ComponentInfo(
        "Camera2D", "Camera", "2D camera component for orthographic rendering",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Camera2D>();
        }
    ));

    registry.RegisterComponent("Camera3D", ComponentInfo(
        "Camera3D", "Camera", "3D camera component for perspective rendering",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Camera3D>();
        }
    ));

    registry.RegisterComponent("Button", ComponentInfo(
        "Button", "UI", "Interactive button component with text and click handling",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Button>();
        }
    ));

    registry.RegisterComponent("Panel", ComponentInfo(
        "Panel", "UI", "Basic rectangular background panel component",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Panel>();
        }
    ));

    registry.RegisterComponent("TextureRectangle", ComponentInfo(
        "TextureRectangle", "UI", "Texture display component with various stretch modes",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<TextureRectangle>();
        }
    ));

    registry.RegisterComponent("ColorRectangle", ComponentInfo(
        "ColorRectangle", "UI", "Solid color rectangle component",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<ColorRectangle>();
        }
    ));

    registry.RegisterComponent("NinePatchPanel", ComponentInfo(
        "NinePatchPanel", "UI", "Nine-patch scalable panel component",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<NinePatchPanel>();
        }
    ));

    registry.RegisterComponent("ProgressBar", ComponentInfo(
        "ProgressBar", "UI", "Progress bar component for displaying progress",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<ProgressBar>();
        }
    ));

    registry.RegisterComponent("Sprite3D", ComponentInfo(
        "Sprite3D", "3D", "3D sprite component for rendering 2D textures in 3D space",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Sprite3D>();
        }
    ));

    registry.RegisterComponent("Skybox3D", ComponentInfo(
        "Skybox3D", "3D", "3D skybox component for background rendering with solid color, panoramic textures, and procedural sky",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Skybox3D>();
        }
    ));

    registry.RegisterComponent("OmniLight", ComponentInfo(
        "OmniLight", "Light", "Omnidirectional point light with attenuation and shadows",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<OmniLight>();
        }
    ));

    registry.RegisterComponent("DirectionalLight", ComponentInfo(
        "DirectionalLight", "Light", "Directional light for uniform lighting like sunlight",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<DirectionalLight>();
        }
    ));

    registry.RegisterComponent("SpotLight", ComponentInfo(
        "SpotLight", "Light", "Spot light with cone-shaped illumination and attenuation",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<SpotLight>();
        }
    ));

    registry.RegisterComponent("LuaScriptComponent", ComponentInfo(
        "LuaScriptComponent", "Scripting", "Lua script component for custom behavior",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<LuaScriptComponent>();
        }
    ));

    registry.RegisterComponent("PythonScriptComponent", ComponentInfo(
        "PythonScriptComponent", "Scripting", "Python script component for custom behavior",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<PythonScriptComponent>();
        }
    ));

    registry.RegisterComponent("VisualScriptComponent", ComponentInfo(
        "VisualScriptComponent", "Scripting", "Visual script component for node-based scripting",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<VisualScriptComponent>();
        }
    ));

    registry.RegisterComponent("AudioSource", ComponentInfo(
        "AudioSource", "Audio", "Audio source component for playing sound effects and music",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<AudioSource>();
        }
    ));

    // Tilemap Components
    registry.RegisterComponent("Tilemap2D", ComponentInfo(
        "Tilemap2D", "2D", "2D tilemap component for sprite-based tile rendering",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Tilemap2D>();
        }
    ));

    registry.RegisterComponent("Tilemap25D", ComponentInfo(
        "Tilemap25D", "2D", "2.5D tilemap component for sprite3D-based tile rendering",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Tilemap25D>();
        }
    ));

    registry.RegisterComponent("Tilemap3D", ComponentInfo(
        "Tilemap3D", "3D", "3D tilemap component for mesh-based tile rendering with frustum culling",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Tilemap3D>();
        }
    ));

    registry.RegisterComponent("Camera2D", ComponentInfo(
        "Camera2D", "Camera", "2D camera component for controlling 2D view and projection",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Camera2D>();
        }
    ));

    registry.RegisterComponent("Camera3D", ComponentInfo(
        "Camera3D", "Camera", "3D camera component for controlling 3D view and projection",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Camera3D>();
        }
    ));

    registry.RegisterComponent("RigidBody2D", ComponentInfo(
        "RigidBody2D", "Physics", "2D physics body component for Box2D physics simulation",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<RigidBody2D>();
        }
    ));

    registry.RegisterComponent("RigidBody3D", ComponentInfo(
        "RigidBody3D", "Physics", "3D physics body component for Bullet physics simulation",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<RigidBody3D>();
        }
    ));

    registry.RegisterComponent("PlayerController2D", ComponentInfo(
        "PlayerController2D", "Player", "Basic 2D player controller with action-based movement",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<PlayerController2D>();
        }
    ));

    registry.RegisterComponent("PlayerController3D", ComponentInfo(
        "PlayerController3D", "Player", "Basic 3D player controller with WASD movement, jump, and gravity",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<PlayerController3D>();
        }
    ));

    registry.RegisterComponent("PlatformerController2D", ComponentInfo(
        "PlatformerController2D", "Player", "2D platformer controller with gravity, jumping, and left/right movement",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<PlatformerController2D>();
        }
    ));

    // Physics Components
    registry.RegisterComponent("CollisionShape2D", ComponentInfo(
        "CollisionShape2D", "Physics", "2D collision shape for physics bodies",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<CollisionShape2D>();
        }
    ));

    registry.RegisterComponent("CollisionPolygon2D", ComponentInfo(
        "CollisionPolygon2D", "Physics", "2D collision polygon for custom shapes",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<CollisionPolygon2D>();
        }
    ));

    registry.RegisterComponent("CollisionMesh3D", ComponentInfo(
        "CollisionMesh3D", "Physics", "3D collision mesh for complex shapes",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<CollisionMesh3D>();
        }
    ));

    registry.RegisterComponent("Area2D", ComponentInfo(
        "Area2D", "Physics", "2D area for trigger detection",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Area2D>();
        }
    ));

    registry.RegisterComponent("KinematicBody2D", ComponentInfo(
        "KinematicBody2D", "Physics", "2D kinematic body for programmatic movement",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<KinematicBody2D>();
        }
    ));

    registry.RegisterComponent("Area3D", ComponentInfo(
        "Area3D", "Physics", "3D area for trigger detection",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Area3D>();
        }
    ));

    registry.RegisterComponent("KinematicBody3D", ComponentInfo(
        "KinematicBody3D", "Physics", "3D kinematic body for programmatic movement",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<KinematicBody3D>();
        }
    ));

    // Animation Components
    registry.RegisterComponent("Animator", ComponentInfo(
        "Animator", "Animation", "Tween animation component for property-based animations",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<Animator>();
        }
    ));

    registry.RegisterComponent("StateAnimator", ComponentInfo(
        "StateAnimator", "Animation", "Unity-style state machine animator for complex animation control",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<StateAnimator>();
        }
    ));

    registry.RegisterComponent("AnimatedSprite2D", ComponentInfo(
        "AnimatedSprite2D", "2D", "Animated 2D sprite component for frame-based animations",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<AnimatedSprite2D>();
        }
    ));

    registry.RegisterComponent("AnimatedSprite3D", ComponentInfo(
        "AnimatedSprite3D", "3D", "Animated 3D sprite component for frame-based animations in 3D space",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<AnimatedSprite3D>();
        }
    ));

    registry.RegisterComponent("AnimationPlayer", ComponentInfo(
        "AnimationPlayer", "Animation", "Skeletal animation player for controlling SkinnedMesh animations",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<AnimationPlayer>();
        }
    ));

    // Terrain Components
    registry.RegisterComponent("TerrainRenderer", ComponentInfo(
        "TerrainRenderer", "Terrain", "Terrain rendering component with height painting, texture layers, and asset scattering",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<TerrainRenderer>();
        }
    ));

    registry.RegisterComponent("TerrainCollider", ComponentInfo(
        "TerrainCollider", "Terrain", "Terrain collision component for physics interaction with terrain surfaces",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<TerrainCollider>();
        }
    ));

    registry.RegisterComponent("TerrainLoader", ComponentInfo(
        "TerrainLoader", "Terrain", "Terrain file I/O component for loading and saving terrain data",
        []() -> std::unique_ptr<Component> {
            return std::make_unique<TerrainLoader>();
        }
    ));
}

} // namespace Lupine
