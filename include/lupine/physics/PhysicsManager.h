#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

// Forward declarations for physics libraries
#include <box2d.h>
class btDiscreteDynamicsWorld;
class btRigidBody;
class btCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btCollisionDispatcher;
class btCollisionShape;
class btTriangleMesh;
class btConvexHullShape;
class btBvhTriangleMeshShape;
class btConvexTriangleMeshShape;
class btMotionState;

namespace Lupine {

class Node;
class Node2D;
class Node3D;
class CollisionMesh3D;

/**
 * @brief Physics body types
 */
enum class PhysicsBodyType {
    Static,     // Immovable objects (walls, floors)
    Kinematic,  // Movable but not affected by forces (moving platforms)
    Dynamic     // Fully simulated physics objects
};

/**
 * @brief Collision shape types
 */
enum class CollisionShapeType {
    Box,
    Circle,
    Capsule,
    Sphere,      // 3D sphere (alias for Circle in 3D)
    Cylinder,    // 3D cylinder
    Mesh,        // Custom mesh collision
    Heightfield
};

/**
 * @brief Physics material properties
 */
struct PhysicsMaterial {
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f; // Bounciness (0 = no bounce, 1 = perfect bounce)
    float linear_damping = 0.0f;
    float angular_damping = 0.0f;
    
    PhysicsMaterial() = default;
    PhysicsMaterial(float d, float f, float r) : density(d), friction(f), restitution(r) {}
};

/**
 * @brief Collision callback function type
 */
using CollisionCallback = std::function<void(Node* self, Node* other, const glm::vec3& contact_point, const glm::vec3& normal)>;

/**
 * @brief 2D Physics body wrapper
 */
class PhysicsBody2D {
public:
    PhysicsBody2D(b2BodyId bodyId, Node2D* owner);
    ~PhysicsBody2D();
    
    // Transform
    void SetPosition(const glm::vec2& position);
    glm::vec2 GetPosition() const;
    void SetRotation(float angle);
    float GetRotation() const;
    
    // Velocity
    void SetLinearVelocity(const glm::vec2& velocity);
    glm::vec2 GetLinearVelocity() const;
    void SetAngularVelocity(float velocity);
    float GetAngularVelocity() const;
    
    // Forces
    void ApplyForce(const glm::vec2& force, const glm::vec2& point = glm::vec2(0.0f));
    void ApplyImpulse(const glm::vec2& impulse, const glm::vec2& point = glm::vec2(0.0f));
    void ApplyTorque(float torque);
    
    // Properties
    void SetMass(float mass);
    float GetMass() const;
    void SetGravityScale(float scale);
    float GetGravityScale() const;
    
    // Collision callbacks
    void SetCollisionCallback(CollisionCallback callback);

    // Collision filtering
    void SetCollisionLayer(int layer);
    void SetCollisionMask(int mask);
    void SetIsSensor(bool is_sensor);

    // Debug and utility methods
    bool IsAwake() const;
    bool IsSleeping() const;
    glm::vec2 GetCenterOfMass() const;
    float GetInertia() const;

    Node2D* GetOwner() const { return m_owner; }
    b2BodyId GetBox2DBodyId() const { return m_bodyId; }
    b2ShapeId GetShapeId() const { return m_shapeId; }

private:
    b2BodyId m_bodyId;
    b2ShapeId m_shapeId;
    Node2D* m_owner;
    CollisionCallback m_collision_callback;

    friend class PhysicsManager;
};

/**
 * @brief 3D Physics body wrapper
 */
class PhysicsBody3D {
public:
    PhysicsBody3D(btRigidBody* body, Node3D* owner);
    ~PhysicsBody3D();
    
    // Transform
    void SetPosition(const glm::vec3& position);
    glm::vec3 GetPosition() const;
    void SetRotation(const glm::quat& rotation);
    glm::quat GetRotation() const;
    
    // Velocity
    void SetLinearVelocity(const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity() const;
    void SetAngularVelocity(const glm::vec3& velocity);
    glm::vec3 GetAngularVelocity() const;
    
    // Forces
    void ApplyForce(const glm::vec3& force, const glm::vec3& point = glm::vec3(0.0f));
    void ApplyImpulse(const glm::vec3& impulse, const glm::vec3& point = glm::vec3(0.0f));
    void ApplyTorque(const glm::vec3& torque);
    
    // Properties
    void SetMass(float mass);
    float GetMass() const;
    void SetGravityScale(float scale);
    float GetGravityScale() const;
    
    // Collision callbacks
    void SetCollisionCallback(CollisionCallback callback);
    
    Node3D* GetOwner() const { return m_owner; }
    btRigidBody* GetBulletBody() const { return m_body; }

private:
    btRigidBody* m_body;
    Node3D* m_owner;
    CollisionCallback m_collision_callback;
    
    friend class PhysicsManager;
};

/**
 * @brief Comprehensive physics management system
 * 
 * Manages both 2D (Box2D) and 3D (Bullet3) physics simulation,
 * collision detection, and physics body management.
 */
class PhysicsManager {
public:
    /**
     * @brief Initialize the physics manager
     * @return True if successful
     */
    static bool Initialize();
    
    /**
     * @brief Shutdown the physics manager
     */
    static void Shutdown();
    
    /**
     * @brief Update physics simulation
     * @param delta_time Time step for physics simulation
     */
    static void Update(float delta_time);
    
    // === 2D Physics ===
    
    /**
     * @brief Set 2D gravity
     * @param gravity Gravity vector
     */
    static void SetGravity2D(const glm::vec2& gravity);
    
    /**
     * @brief Get 2D gravity
     * @return Current 2D gravity vector
     */
    static glm::vec2 GetGravity2D();
    
    /**
     * @brief Create a 2D physics body
     * @param node Owner node
     * @param body_type Type of physics body
     * @param shape_type Collision shape type
     * @param size Shape size (width/height for box, radius for circle)
     * @param material Physics material properties
     * @return Pointer to created physics body
     */
    static PhysicsBody2D* CreatePhysicsBody2D(Node2D* node, PhysicsBodyType body_type,
                                             CollisionShapeType shape_type, const glm::vec2& size,
                                             const PhysicsMaterial& material = PhysicsMaterial(),
                                             const glm::vec2& offset = glm::vec2(0.0f, 0.0f));
    
    /**
     * @brief Remove a 2D physics body
     * @param body Physics body to remove
     */
    static void RemovePhysicsBody2D(PhysicsBody2D* body);

    /**
     * @brief Perform a 2D raycast
     * @param start Start point of the ray
     * @param end End point of the ray
     * @param collision_mask Collision mask for filtering
     * @return True if hit something, false otherwise
     */
    static bool Raycast2D(const glm::vec2& start, const glm::vec2& end, int collision_mask = 0xFFFF);

    /**
     * @brief Query for overlapping bodies in a 2D area
     * @param center Center of the query area
     * @param size Size of the query area
     * @param collision_mask Collision mask for filtering
     * @return Vector of overlapping physics bodies
     */
    static std::vector<PhysicsBody2D*> OverlapArea2D(const glm::vec2& center, const glm::vec2& size, int collision_mask = 0xFFFF);

    /**
     * @brief Perform a shape cast to detect collisions along a movement path
     * @param body Physics body to cast
     * @param start_pos Starting position
     * @param end_pos Ending position
     * @param hit_body Output: first body hit (if any)
     * @param hit_point Output: contact point
     * @param hit_normal Output: contact normal
     * @param hit_fraction Output: fraction along the cast where hit occurred (0.0 to 1.0)
     * @return True if collision detected
     */
    static bool ShapeCast2D(PhysicsBody2D* body, const glm::vec2& start_pos, const glm::vec2& end_pos,
                           PhysicsBody2D*& hit_body, glm::vec2& hit_point, glm::vec2& hit_normal, float& hit_fraction);

    /**
     * @brief Test if a body would overlap with others at a specific position
     * @param body Physics body to test
     * @param position Position to test at
     * @param overlapping_bodies Output: vector of overlapping bodies
     * @return True if any overlaps detected
     */
    static bool TestBodyOverlap2D(PhysicsBody2D* body, const glm::vec2& position, std::vector<PhysicsBody2D*>& overlapping_bodies);

    /**
     * @brief Test AABB overlap as a fallback collision detection method
     * @param body Physics body to test
     * @param position Position to test at
     * @param overlapping_bodies Output: vector of overlapping bodies
     * @return True if any overlaps detected
     */
    static bool TestSimpleAABBOverlap(PhysicsBody2D* body, const glm::vec2& position, std::vector<PhysicsBody2D*>& overlapping_bodies);

    /**
     * @brief Perform swept AABB collision detection
     * @param movingAABB AABB of the moving object
     * @param staticAABB AABB of the static object
     * @param velocity Movement vector
     * @return Time of collision (0-1) or -1 if no collision
     */
    static float SweptAABB(const b2AABB& movingAABB, const b2AABB& staticAABB, const glm::vec2& velocity);

    /**
     * @brief Update physics body transform from node (for kinematic bodies)
     * @param body Physics body to update
     * @param position New position
     * @param rotation New rotation
     */
    static void UpdatePhysicsBodyTransform(PhysicsBody2D* body, const glm::vec2& position, float rotation);

    /**
     * @brief Render debug visualization for 2D physics
     * @param camera_matrix Camera view-projection matrix for rendering
     */
    static void RenderDebug2D(const glm::mat4& camera_matrix);

    /**
     * @brief Get all 2D physics bodies for debugging
     * @return Vector of all physics bodies
     */
    static const std::vector<std::unique_ptr<PhysicsBody2D>>& GetAllBodies2D() { return s_bodies_2d; }
    
    // === 3D Physics ===
    
    /**
     * @brief Set 3D gravity
     * @param gravity Gravity vector
     */
    static void SetGravity3D(const glm::vec3& gravity);
    
    /**
     * @brief Get 3D gravity
     * @return Current 3D gravity vector
     */
    static glm::vec3 GetGravity3D();
    
    /**
     * @brief Create a 3D physics body
     * @param node Owner node
     * @param body_type Type of physics body
     * @param shape_type Collision shape type
     * @param size Shape size (width/height/depth for box, radius for sphere)
     * @param material Physics material properties
     * @return Pointer to created physics body
     */
    static PhysicsBody3D* CreatePhysicsBody3D(Node3D* node, PhysicsBodyType body_type,
                                             CollisionShapeType shape_type, const glm::vec3& size,
                                             const PhysicsMaterial& material = PhysicsMaterial());

    /**
     * @brief Create a 3D physics body with mesh collision
     * @param node Owner node
     * @param body_type Type of physics body
     * @param collision_mesh CollisionMesh3D component for mesh data
     * @param material Physics material properties
     * @return Pointer to created physics body
     */
    static PhysicsBody3D* CreatePhysicsBody3D(Node3D* node, PhysicsBodyType body_type,
                                             CollisionMesh3D* collision_mesh,
                                             const PhysicsMaterial& material = PhysicsMaterial());
    
    /**
     * @brief Remove a 3D physics body
     * @param body Physics body to remove
     */
    static void RemovePhysicsBody3D(PhysicsBody3D* body);
    
    // === Raycasting ===
    
    /**
     * @brief Perform 2D raycast
     * @param start Ray start position
     * @param end Ray end position
     * @param hit_node Output: hit node (if any)
     * @param hit_point Output: hit point
     * @param hit_normal Output: hit normal
     * @return True if ray hit something
     */
    static bool Raycast2D(const glm::vec2& start, const glm::vec2& end, 
                         Node2D*& hit_node, glm::vec2& hit_point, glm::vec2& hit_normal);
    
    /**
     * @brief Perform 3D raycast
     * @param start Ray start position
     * @param end Ray end position
     * @param hit_node Output: hit node (if any)
     * @param hit_point Output: hit point
     * @param hit_normal Output: hit normal
     * @return True if ray hit something
     */
    static bool Raycast3D(const glm::vec3& start, const glm::vec3& end,
                         Node3D*& hit_node, glm::vec3& hit_point, glm::vec3& hit_normal);

    /**
     * @brief Perform a 3D convex sweep test for collision detection
     * @param body Physics body to cast
     * @param start_pos Starting position
     * @param end_pos Ending position
     * @param hit_body Output: first body hit (if any)
     * @param hit_point Output: contact point
     * @param hit_normal Output: contact normal
     * @param hit_fraction Output: fraction along the cast where hit occurred (0.0 to 1.0)
     * @return True if collision detected
     */
    static bool ConvexSweepTest3D(PhysicsBody3D* body, const glm::vec3& start_pos, const glm::vec3& end_pos,
                                 PhysicsBody3D*& hit_body, glm::vec3& hit_point, glm::vec3& hit_normal, float& hit_fraction);

    /**
     * @brief Test for overlapping bodies at a specific position
     * @param body Physics body to test
     * @param position Position to test at
     * @param overlapping_bodies Output: vector of overlapping bodies
     * @return True if any overlaps detected
     */
    static bool TestBodyOverlap3D(PhysicsBody3D* body, const glm::vec3& position, std::vector<PhysicsBody3D*>& overlapping_bodies);

    // === World Access ===

    /**
     * @brief Get the 3D physics world for advanced collision queries
     * @return Pointer to the Bullet3D dynamics world
     */
    static btDiscreteDynamicsWorld* GetWorld3D() { return s_world_3d.get(); }

    /**
     * @brief Get all 3D physics bodies for collision detection
     * @return Reference to the vector of all 3D physics bodies
     */
    static const std::vector<std::unique_ptr<PhysicsBody3D>>& GetAllBodies3D() { return s_bodies_3d; }

    // === Settings ===
    
    /**
     * @brief Set physics time step
     * @param time_step Fixed time step for physics simulation
     */
    static void SetTimeStep(float time_step);
    
    /**
     * @brief Get physics time step
     * @return Current physics time step
     */
    static float GetTimeStep();
    
    /**
     * @brief Enable/disable physics debug rendering
     * @param enabled True to enable debug rendering
     */
    static void SetDebugRenderingEnabled(bool enabled);
    
    /**
     * @brief Check if physics debug rendering is enabled
     * @return True if debug rendering is enabled
     */
    static bool IsDebugRenderingEnabled();

private:
    static bool s_initialized;
    static float s_time_step;
    static float s_accumulator;
    static bool s_debug_rendering_enabled;
    
    // 2D Physics (Box2D)
    static b2WorldId s_world_2d;
    static std::vector<std::unique_ptr<PhysicsBody2D>> s_bodies_2d;
    
    // 3D Physics (Bullet3)
    static std::unique_ptr<btDiscreteDynamicsWorld> s_world_3d;
    static std::unique_ptr<btCollisionConfiguration> s_collision_config_3d;
    static std::unique_ptr<btBroadphaseInterface> s_broadphase_3d;
    static std::unique_ptr<btConstraintSolver> s_solver_3d;
    static std::unique_ptr<btCollisionDispatcher> s_dispatcher_3d;
    static std::vector<std::unique_ptr<PhysicsBody3D>> s_bodies_3d;
    
    // Helper methods
    static void UpdatePhysics2D(float delta_time);
    static void UpdatePhysics3D(float delta_time);
    static void ProcessCollisionEvents3D();
    static void SyncNodeTransforms();

    // 3D Mesh collision utilities
    static std::unique_ptr<btCollisionShape> CreateMeshCollisionShape(CollisionMesh3D* collision_mesh);
    static std::unique_ptr<btCollisionShape> CreatePrimitiveMeshShape(CollisionMesh3D* collision_mesh);
    static std::unique_ptr<btCollisionShape> CreateExternalMeshShape(const std::string& mesh_path,
                                                                    int mesh_type);
    static std::unique_ptr<btConvexHullShape> CreateConvexHullFromMesh(const std::vector<glm::vec3>& vertices);
    static std::unique_ptr<btBvhTriangleMeshShape> CreateTriangleMeshFromMesh(const std::vector<glm::vec3>& vertices,
                                                                             const std::vector<unsigned int>& indices);

    // Mesh collision shape storage (to keep shapes alive)
    static std::vector<std::unique_ptr<btCollisionShape>> s_mesh_shapes;
    static std::vector<std::unique_ptr<btTriangleMesh>> s_triangle_meshes;
    static std::vector<std::unique_ptr<btMotionState>> s_motion_states;
};

} // namespace Lupine
