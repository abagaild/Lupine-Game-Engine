#include "lupine/physics/PhysicsManager.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/resources/MeshLoader.h"

// Box2D v3.0 includes
#include <box2d.h>

// Bullet3 includes
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <bullet/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <bullet/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h>
#include <bullet/BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <bullet/BulletCollision/CollisionShapes/btCylinderShape.h>

#include <iostream>
#include <algorithm>

namespace Lupine {

// Static member definitions
bool PhysicsManager::s_initialized = false;
float PhysicsManager::s_time_step = 1.0f / 60.0f;
float PhysicsManager::s_accumulator = 0.0f;
bool PhysicsManager::s_debug_rendering_enabled = false;

b2WorldId PhysicsManager::s_world_2d = b2_nullWorldId;
std::vector<std::unique_ptr<PhysicsBody2D>> PhysicsManager::s_bodies_2d;

std::unique_ptr<btDiscreteDynamicsWorld> PhysicsManager::s_world_3d;
std::unique_ptr<btCollisionConfiguration> PhysicsManager::s_collision_config_3d;
std::unique_ptr<btBroadphaseInterface> PhysicsManager::s_broadphase_3d;
std::unique_ptr<btConstraintSolver> PhysicsManager::s_solver_3d;
std::unique_ptr<btCollisionDispatcher> PhysicsManager::s_dispatcher_3d;
std::vector<std::unique_ptr<PhysicsBody3D>> PhysicsManager::s_bodies_3d;
std::vector<std::unique_ptr<btCollisionShape>> PhysicsManager::s_mesh_shapes;
std::vector<std::unique_ptr<btTriangleMesh>> PhysicsManager::s_triangle_meshes;
std::vector<std::unique_ptr<btMotionState>> PhysicsManager::s_motion_states;

// Contact event handling for Box2D v3.x will be done in the Update loop

bool PhysicsManager::Initialize() {
    if (s_initialized) {
        return true;
    }
    
    std::cout << "Initializing Physics Manager..." << std::endl;
    
    // Initialize 2D Physics (Box2D v3.x)
    try {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, -9.81f};
        s_world_2d = b2CreateWorld(&worldDef);

        if (!b2World_IsValid(s_world_2d)) {
            std::cerr << "Failed to create Box2D world!" << std::endl;
            return false;
        }

        std::cout << "Box2D v3.x initialized successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Box2D: " << e.what() << std::endl;
        return false;
    }
    
    // Initialize 3D Physics (Bullet3)
    try {
        s_collision_config_3d = std::make_unique<btDefaultCollisionConfiguration>();
        s_dispatcher_3d = std::make_unique<btCollisionDispatcher>(s_collision_config_3d.get());
        s_broadphase_3d = std::make_unique<btDbvtBroadphase>();
        s_solver_3d = std::make_unique<btSequentialImpulseConstraintSolver>();
        
        s_world_3d = std::make_unique<btDiscreteDynamicsWorld>(
            s_dispatcher_3d.get(), s_broadphase_3d.get(), s_solver_3d.get(), s_collision_config_3d.get());
        
        s_world_3d->setGravity(btVector3(0.0f, -9.81f, 0.0f));
        
        std::cout << "Bullet3 initialized successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Bullet3: " << e.what() << std::endl;
        return false;
    }
    
    s_initialized = true;
    std::cout << "Physics Manager initialized successfully!" << std::endl;
    return true;
}

void PhysicsManager::Shutdown() {
    if (!s_initialized) {
        return;
    }
    
    std::cout << "Shutting down Physics Manager..." << std::endl;
    
    // Clean up 2D physics bodies
    s_bodies_2d.clear();
    
    // Clean up 3D physics bodies
    for (auto& body : s_bodies_3d) {
        if (body->GetBulletBody()) {
            s_world_3d->removeRigidBody(body->GetBulletBody());
        }
    }
    s_bodies_3d.clear();

    // Clean up 3D physics resources
    s_mesh_shapes.clear();
    s_triangle_meshes.clear();
    s_motion_states.clear();

    // Shutdown 3D physics
    s_world_3d.reset();
    s_solver_3d.reset();
    s_broadphase_3d.reset();
    s_dispatcher_3d.reset();
    s_collision_config_3d.reset();
    
    // Shutdown 2D physics
    if (b2World_IsValid(s_world_2d)) {
        b2DestroyWorld(s_world_2d);
        s_world_2d = b2_nullWorldId;
    }
    
    s_initialized = false;
    std::cout << "Physics Manager shutdown complete." << std::endl;
}

void PhysicsManager::Update(float delta_time) {
    if (!s_initialized) {
        return;
    }
    
    // Use fixed timestep for stable physics simulation
    s_accumulator += delta_time;
    
    while (s_accumulator >= s_time_step) {
        UpdatePhysics2D(s_time_step);
        UpdatePhysics3D(s_time_step);
        s_accumulator -= s_time_step;
    }
    
    // Sync node transforms with physics bodies
    SyncNodeTransforms();
}

void PhysicsManager::UpdatePhysics2D(float delta_time) {
    if (b2World_IsValid(s_world_2d)) {
        // Box2D v3.x uses sub-steps instead of velocity/position iterations
        int subStepCount = 4;

        b2World_Step(s_world_2d, delta_time, subStepCount);

        // Handle contact events
        b2ContactEvents contactEvents = b2World_GetContactEvents(s_world_2d);

        // Process begin contact events
        for (int i = 0; i < contactEvents.beginCount; ++i) {
            const b2ContactBeginTouchEvent& event = contactEvents.beginEvents[i];

            // Get user data from shapes
            void* userDataA = b2Shape_GetUserData(event.shapeIdA);
            void* userDataB = b2Shape_GetUserData(event.shapeIdB);

            PhysicsBody2D* physicsBodyA = static_cast<PhysicsBody2D*>(userDataA);
            PhysicsBody2D* physicsBodyB = static_cast<PhysicsBody2D*>(userDataB);

            std::cout << "Collision detected! Bodies: " << physicsBodyA << ", " << physicsBodyB << std::endl;

            if (physicsBodyA && physicsBodyB) {
                std::cout << "Both bodies valid - processing collision" << std::endl;
                // Use the manifold from the event
                const b2Manifold& manifold = event.manifold;

                if (manifold.pointCount > 0) {
                    glm::vec2 contact_point(manifold.points[0].point.x, manifold.points[0].point.y);
                    glm::vec2 normal(manifold.normal.x, manifold.normal.y);
                    glm::vec3 contact_point_3d(contact_point.x, contact_point.y, 0.0f);
                    glm::vec3 normal_3d(normal.x, normal.y, 0.0f);

                    // Call collision callbacks
                    if (physicsBodyA->m_collision_callback) {
                        physicsBodyA->m_collision_callback(physicsBodyA->GetOwner(), physicsBodyB->GetOwner(),
                                                         contact_point_3d, normal_3d);
                    }

                    if (physicsBodyB->m_collision_callback) {
                        physicsBodyB->m_collision_callback(physicsBodyB->GetOwner(), physicsBodyA->GetOwner(),
                                                         contact_point_3d, -normal_3d);
                    }
                }
            }
        }

        // Process sensor events for Area2D components
        b2SensorEvents sensorEvents = b2World_GetSensorEvents(s_world_2d);

        // Process sensor begin events (area enter)
        for (int i = 0; i < sensorEvents.beginCount; ++i) {
            const b2SensorBeginTouchEvent& event = sensorEvents.beginEvents[i];

            void* sensorUserData = b2Shape_GetUserData(event.sensorShapeId);
            void* visitorUserData = b2Shape_GetUserData(event.visitorShapeId);

            PhysicsBody2D* sensorBody = static_cast<PhysicsBody2D*>(sensorUserData);
            PhysicsBody2D* visitorBody = static_cast<PhysicsBody2D*>(visitorUserData);

            if (sensorBody && visitorBody && sensorBody->m_collision_callback) {
                glm::vec3 zero_point(0.0f);
                glm::vec3 zero_normal(0.0f);
                sensorBody->m_collision_callback(sensorBody->GetOwner(), visitorBody->GetOwner(),
                                               zero_point, zero_normal);
            }
        }

        // Process sensor end events (area exit)
        for (int i = 0; i < sensorEvents.endCount; ++i) {
            const b2SensorEndTouchEvent& event = sensorEvents.endEvents[i];

            void* sensorUserData = b2Shape_GetUserData(event.sensorShapeId);
            void* visitorUserData = b2Shape_GetUserData(event.visitorShapeId);

            PhysicsBody2D* sensorBody = static_cast<PhysicsBody2D*>(sensorUserData);
            PhysicsBody2D* visitorBody = static_cast<PhysicsBody2D*>(visitorUserData);

            if (sensorBody && visitorBody && sensorBody->m_collision_callback) {
                // Use negative normal to indicate exit event
                glm::vec3 zero_point(0.0f);
                glm::vec3 exit_normal(-1.0f, 0.0f, 0.0f); // Special indicator for exit
                sensorBody->m_collision_callback(sensorBody->GetOwner(), visitorBody->GetOwner(),
                                               zero_point, exit_normal);
            }
        }
    }
}

void PhysicsManager::UpdatePhysics3D(float delta_time) {
    if (s_world_3d) {
        try {
            s_world_3d->stepSimulation(delta_time, 10);

            // Process collision events
            ProcessCollisionEvents3D();
        } catch (const std::exception& e) {
            std::cerr << "Physics3D simulation error: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown Physics3D simulation error" << std::endl;
        }
    }
}

void PhysicsManager::SyncNodeTransforms() {
    // Sync 2D physics bodies with their nodes
    for (auto& body : s_bodies_2d) {
        if (!body || !body->GetOwner()) {
            continue;
        }

        Node2D* node = body->GetOwner();
        b2BodyId bodyId = body->GetBox2DBodyId();

        if (b2Body_IsValid(bodyId)) {
            // Only sync dynamic and kinematic bodies to nodes
            // Static bodies should not move, so we don't sync them
            b2BodyType bodyType = b2Body_GetType(bodyId);

            if (bodyType == b2_dynamicBody || bodyType == b2_kinematicBody) {
                b2Vec2 position = b2Body_GetPosition(bodyId);
                b2Rot rotation = b2Body_GetRotation(bodyId);
                float angle = b2Rot_GetAngle(rotation);

                // Update node transform to match physics body
                node->SetPosition(glm::vec2(position.x, position.y));
                node->SetRotation(angle);
            }
        }
    }
    
    // Sync 3D physics bodies with their nodes
    static int sync_debug_counter = 0;
    sync_debug_counter++;

    for (auto& body : s_bodies_3d) {
        if (!body || !body->GetOwner() || !body->GetBulletBody()) {
            continue;
        }

        try {
            Node3D* node = body->GetOwner();
            btRigidBody* btbody = body->GetBulletBody();

            // Additional safety checks
            if (!btbody->getMotionState()) {
                std::cerr << "Warning: 3D physics body has no motion state" << std::endl;
                continue;
            }

            // Only sync dynamic bodies (static and kinematic bodies don't move via physics)
            if (btbody->getInvMass() == 0.0f && btbody->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT) {
                continue; // Skip static bodies
            }

            btTransform transform;
            btbody->getMotionState()->getWorldTransform(transform);

            btVector3 position = transform.getOrigin();
            btQuaternion rotation = transform.getRotation();

            // Debug output every 60 frames for dynamic bodies
            if (sync_debug_counter % 60 == 0 && btbody->getInvMass() > 0.0f) {
                std::cout << "SyncTransforms: " << node->GetName()
                          << " physics pos: (" << position.x() << ", " << position.y() << ", " << position.z() << ")" << std::endl;
            }

            // Validate the transform values before applying
            if (std::isfinite(position.x()) && std::isfinite(position.y()) && std::isfinite(position.z()) &&
                std::isfinite(rotation.x()) && std::isfinite(rotation.y()) && std::isfinite(rotation.z()) && std::isfinite(rotation.w())) {
                node->SetPosition(glm::vec3(position.x(), position.y(), position.z()));
                node->SetRotation(glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z()));
            } else {
                std::cerr << "Warning: Invalid transform values from 3D physics body" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error syncing 3D physics body transform: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error syncing 3D physics body transform" << std::endl;
        }
    }
}

void PhysicsManager::ProcessCollisionEvents3D() {
    if (!s_world_3d) {
        return;
    }

    // Get the collision dispatcher
    btDispatcher* dispatcher = s_world_3d->getDispatcher();
    if (!dispatcher) {
        return;
    }

    // Iterate through all contact manifolds
    int numManifolds = dispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
        if (!contactManifold) {
            continue;
        }

        // Get the two collision objects
        const btCollisionObject* objA = contactManifold->getBody0();
        const btCollisionObject* objB = contactManifold->getBody1();

        if (!objA || !objB) {
            continue;
        }

        // Get user data (PhysicsBody3D pointers)
        PhysicsBody3D* physicsBodyA = static_cast<PhysicsBody3D*>(objA->getUserPointer());
        PhysicsBody3D* physicsBodyB = static_cast<PhysicsBody3D*>(objB->getUserPointer());

        if (!physicsBodyA || !physicsBodyB) {
            continue;
        }

        // Check if there are contact points
        int numContacts = contactManifold->getNumContacts();
        if (numContacts > 0) {
            // Optional debug output (commented out for production)
            // std::cout << "3D Collision detected! Bodies: " << physicsBodyA->GetOwner()->GetName()
            //           << " <-> " << physicsBodyB->GetOwner()->GetName()
            //           << " (contacts: " << numContacts << ")" << std::endl;

            // Get the first contact point for callback
            btManifoldPoint& pt = contactManifold->getContactPoint(0);

            // Convert contact point and normal to glm vectors
            btVector3 contactPoint = pt.getPositionWorldOnA();
            btVector3 normal = pt.m_normalWorldOnB;

            glm::vec3 contact_point_3d(contactPoint.x(), contactPoint.y(), contactPoint.z());
            glm::vec3 normal_3d(normal.x(), normal.y(), normal.z());

            // Call collision callbacks for both bodies
            if (physicsBodyA->m_collision_callback) {
                physicsBodyA->m_collision_callback(physicsBodyA->GetOwner(), physicsBodyB->GetOwner(),
                                                 contact_point_3d, normal_3d);
            }

            if (physicsBodyB->m_collision_callback) {
                physicsBodyB->m_collision_callback(physicsBodyB->GetOwner(), physicsBodyA->GetOwner(),
                                                 contact_point_3d, -normal_3d);
            }
        }
    }
}

// === 2D Physics ===

void PhysicsManager::SetGravity2D(const glm::vec2& gravity) {
    if (b2World_IsValid(s_world_2d)) {
        b2World_SetGravity(s_world_2d, {gravity.x, gravity.y});
    }
}

glm::vec2 PhysicsManager::GetGravity2D() {
    if (b2World_IsValid(s_world_2d)) {
        b2Vec2 gravity = b2World_GetGravity(s_world_2d);
        return glm::vec2(gravity.x, gravity.y);
    }
    return glm::vec2(0.0f, -9.81f);
}

// === 3D Physics ===

void PhysicsManager::SetGravity3D(const glm::vec3& gravity) {
    if (s_world_3d) {
        s_world_3d->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }
}

glm::vec3 PhysicsManager::GetGravity3D() {
    if (s_world_3d) {
        btVector3 gravity = s_world_3d->getGravity();
        return glm::vec3(gravity.x(), gravity.y(), gravity.z());
    }
    return glm::vec3(0.0f, -9.81f, 0.0f);
}

// === Settings ===

void PhysicsManager::SetTimeStep(float time_step) {
    s_time_step = std::max(0.001f, time_step); // Minimum 1ms timestep
}

float PhysicsManager::GetTimeStep() {
    return s_time_step;
}

void PhysicsManager::SetDebugRenderingEnabled(bool enabled) {
    s_debug_rendering_enabled = enabled;
}

bool PhysicsManager::IsDebugRenderingEnabled() {
    return s_debug_rendering_enabled;
}

PhysicsBody2D* PhysicsManager::CreatePhysicsBody2D(Node2D* node, PhysicsBodyType body_type,
                                                   CollisionShapeType shape_type, const glm::vec2& size,
                                                   const PhysicsMaterial& material, const glm::vec2& offset) {
    if (!s_initialized || !b2World_IsValid(s_world_2d) || !node) {
        return nullptr;
    }

    // Create Box2D body definition
    b2BodyDef bodyDef = b2DefaultBodyDef();

    switch (body_type) {
        case PhysicsBodyType::Static:
            bodyDef.type = b2_staticBody;
            break;
        case PhysicsBodyType::Kinematic:
            bodyDef.type = b2_kinematicBody;
            break;
        case PhysicsBodyType::Dynamic:
            bodyDef.type = b2_dynamicBody;
            break;
    }

    glm::vec2 position = node->GetPosition();
    bodyDef.position = {position.x, position.y};
    bodyDef.rotation = b2MakeRot(node->GetRotation());
    bodyDef.linearDamping = material.linear_damping;
    bodyDef.angularDamping = material.angular_damping;

    // Create the body
    b2BodyId bodyId = b2CreateBody(s_world_2d, &bodyDef);

    // Create shape definition
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = material.density;
    shapeDef.material.friction = material.friction;
    shapeDef.material.restitution = material.restitution;
    shapeDef.enableContactEvents = true; // Enable contact events for collision callbacks

    // Create collision shape and attach to body
    b2ShapeId shapeId;

    switch (shape_type) {
        case CollisionShapeType::Box: {
            b2Polygon box = b2MakeOffsetBox(size.x * 0.5f, size.y * 0.5f, {offset.x, offset.y}, b2MakeRot(0.0f));
            shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
            break;
        }
        case CollisionShapeType::Circle: {
            b2Circle circle = {{offset.x, offset.y}, size.x}; // Use x component as radius
            shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
            break;
        }
        case CollisionShapeType::Capsule: {
            // Create capsule using height (y) and radius (x)
            float radius = size.x;
            float height = size.y;
            b2Capsule capsule = {{offset.x, offset.y - (height - radius) * 0.5f}, {offset.x, offset.y + (height - radius) * 0.5f}, radius};
            shapeId = b2CreateCapsuleShape(bodyId, &shapeDef, &capsule);
            break;
        }
        default:
            std::cerr << "Unsupported 2D collision shape type!" << std::endl;
            b2DestroyBody(bodyId);
            return nullptr;
    }

    // Create physics body wrapper
    auto physics_body = std::make_unique<PhysicsBody2D>(bodyId, node);
    PhysicsBody2D* result = physics_body.get();

    // Store shape ID for later access
    result->m_shapeId = shapeId;

    // Set user data for collision callbacks
    b2Shape_SetUserData(shapeId, result);

    std::cout << "Created PhysicsBody2D for node: " << node->GetName()
              << " (body=" << result << ", bodyId valid=" << b2Body_IsValid(bodyId)
              << ", shapeId valid=" << b2Shape_IsValid(shapeId) << ")" << std::endl;

    s_bodies_2d.push_back(std::move(physics_body));
    return result;
}

// PhysicsBody2D Implementation
PhysicsBody2D::PhysicsBody2D(b2BodyId bodyId, Node2D* owner)
    : m_bodyId(bodyId), m_owner(owner) {
}

PhysicsBody2D::~PhysicsBody2D() {
    if (b2Body_IsValid(m_bodyId)) {
        b2DestroyBody(m_bodyId);
    }
}

void PhysicsBody2D::SetPosition(const glm::vec2& position) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 pos = {position.x, position.y};
        b2Rot rot = b2Body_GetRotation(m_bodyId);
        b2Body_SetTransform(m_bodyId, pos, rot);
    }
}

glm::vec2 PhysicsBody2D::GetPosition() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 pos = b2Body_GetPosition(m_bodyId);
        return glm::vec2(pos.x, pos.y);
    }
    return glm::vec2(0.0f);
}

void PhysicsBody2D::SetRotation(float angle) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 pos = b2Body_GetPosition(m_bodyId);
        b2Rot rot = b2MakeRot(angle);
        b2Body_SetTransform(m_bodyId, pos, rot);
    }
}

float PhysicsBody2D::GetRotation() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2Rot rot = b2Body_GetRotation(m_bodyId);
        return b2Rot_GetAngle(rot);
    }
    return 0.0f;
}

void PhysicsBody2D::SetLinearVelocity(const glm::vec2& velocity) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 vel = {velocity.x, velocity.y};
        b2Body_SetLinearVelocity(m_bodyId, vel);
    }
}

glm::vec2 PhysicsBody2D::GetLinearVelocity() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 vel = b2Body_GetLinearVelocity(m_bodyId);
        return glm::vec2(vel.x, vel.y);
    }
    return glm::vec2(0.0f);
}

void PhysicsBody2D::SetAngularVelocity(float velocity) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Body_SetAngularVelocity(m_bodyId, velocity);
    }
}

float PhysicsBody2D::GetAngularVelocity() const {
    if (b2Body_IsValid(m_bodyId)) {
        return b2Body_GetAngularVelocity(m_bodyId);
    }
    return 0.0f;
}

void PhysicsBody2D::ApplyForce(const glm::vec2& force, const glm::vec2& point) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 f = {force.x, force.y};
        b2Vec2 p = {point.x, point.y};
        b2Body_ApplyForce(m_bodyId, f, p, true);
    }
}

void PhysicsBody2D::ApplyImpulse(const glm::vec2& impulse, const glm::vec2& point) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 imp = {impulse.x, impulse.y};
        b2Vec2 p = {point.x, point.y};
        b2Body_ApplyLinearImpulse(m_bodyId, imp, p, true);
    }
}

void PhysicsBody2D::ApplyTorque(float torque) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Body_ApplyTorque(m_bodyId, torque, true);
    }
}

void PhysicsBody2D::SetMass(float mass) {
    if (b2Body_IsValid(m_bodyId)) {
        b2MassData massData = b2Body_GetMassData(m_bodyId);
        massData.mass = mass;
        b2Body_SetMassData(m_bodyId, massData);
    }
}

float PhysicsBody2D::GetMass() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2MassData massData = b2Body_GetMassData(m_bodyId);
        return massData.mass;
    }
    return 0.0f;
}

void PhysicsBody2D::SetGravityScale(float scale) {
    if (b2Body_IsValid(m_bodyId)) {
        b2Body_SetGravityScale(m_bodyId, scale);
    }
}

float PhysicsBody2D::GetGravityScale() const {
    if (b2Body_IsValid(m_bodyId)) {
        return b2Body_GetGravityScale(m_bodyId);
    }
    return 1.0f;
}

void PhysicsBody2D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;
}

void PhysicsBody2D::SetCollisionLayer(int layer) {
    if (b2Shape_IsValid(m_shapeId)) {
        b2Filter filter = b2Shape_GetFilter(m_shapeId);
        filter.categoryBits = (1u << static_cast<unsigned int>(layer));
        b2Shape_SetFilter(m_shapeId, filter);
        std::cout << "PhysicsBody2D: Set collision layer " << layer
                  << " (categoryBits=" << filter.categoryBits << ")" << std::endl;
    }
}

void PhysicsBody2D::SetCollisionMask(int mask) {
    if (b2Shape_IsValid(m_shapeId)) {
        b2Filter filter = b2Shape_GetFilter(m_shapeId);
        filter.maskBits = mask;
        b2Shape_SetFilter(m_shapeId, filter);
        std::cout << "PhysicsBody2D: Set collision mask " << mask
                  << " (maskBits=" << filter.maskBits << ")" << std::endl;
    }
}

void PhysicsBody2D::SetIsSensor(bool is_sensor) {
    if (b2Shape_IsValid(m_shapeId)) {
        b2Shape_EnableSensorEvents(m_shapeId, is_sensor);
    }
}

bool PhysicsBody2D::IsAwake() const {
    if (b2Body_IsValid(m_bodyId)) {
        return b2Body_IsAwake(m_bodyId);
    }
    return false;
}

bool PhysicsBody2D::IsSleeping() const {
    return !IsAwake();
}

glm::vec2 PhysicsBody2D::GetCenterOfMass() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2Vec2 center = b2Body_GetLocalCenterOfMass(m_bodyId);
        return glm::vec2(center.x, center.y);
    }
    return glm::vec2(0.0f);
}

float PhysicsBody2D::GetInertia() const {
    if (b2Body_IsValid(m_bodyId)) {
        b2MassData massData = b2Body_GetMassData(m_bodyId);
        return massData.rotationalInertia;
    }
    return 0.0f;
}

void PhysicsManager::RemovePhysicsBody2D(PhysicsBody2D* body) {
    if (!body || !b2World_IsValid(s_world_2d)) {
        return;
    }

    // Remove from world
    b2BodyId bodyId = body->GetBox2DBodyId();
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }

    // Remove from our list
    auto it = std::find_if(s_bodies_2d.begin(), s_bodies_2d.end(),
        [body](const std::unique_ptr<PhysicsBody2D>& ptr) {
            return ptr.get() == body;
        });

    if (it != s_bodies_2d.end()) {
        s_bodies_2d.erase(it);
    }
}

bool PhysicsManager::Raycast2D(const glm::vec2& start, const glm::vec2& end, int collision_mask) {
    if (!s_initialized || !b2World_IsValid(s_world_2d)) {
        return false;
    }

    (void)collision_mask; // TODO: Implement collision mask filtering

    b2Vec2 point1 = {start.x, start.y};
    b2Vec2 point2 = {end.x, end.y};

    // Create a raycast input
    b2RayCastInput input;
    input.origin = point1;
    input.translation = {point2.x - point1.x, point2.y - point1.y};
    input.maxFraction = 1.0f;

    // For now, just check if we hit anything by iterating through bodies
    // A proper implementation would use Box2D's world query functions
    for (const auto& body : s_bodies_2d) {
        if (body && b2Body_IsValid(body->GetBox2DBodyId())) {
            // Simple AABB check for demonstration
            glm::vec2 body_pos = body->GetPosition();
            // TODO: Implement proper ray-shape intersection
            (void)body_pos; // Suppress unused variable warning
        }
    }

    return false; // Simplified implementation
}

std::vector<PhysicsBody2D*> PhysicsManager::OverlapArea2D(const glm::vec2& center, const glm::vec2& size, int collision_mask) {
    std::vector<PhysicsBody2D*> overlapping_bodies;

    if (!s_initialized || !b2World_IsValid(s_world_2d)) {
        return overlapping_bodies;
    }

    // Create a temporary AABB for the query
    b2AABB query_aabb;
    query_aabb.lowerBound = {center.x - size.x * 0.5f, center.y - size.y * 0.5f};
    query_aabb.upperBound = {center.x + size.x * 0.5f, center.y + size.y * 0.5f};

    // Use Box2D's proper AABB query
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = collision_mask;

    // Query callback to collect overlapping bodies
    auto queryCallback = [](b2ShapeId shapeId, void* context) -> bool {
        auto* bodies = static_cast<std::vector<PhysicsBody2D*>*>(context);

        // Get user data from shape
        void* userData = b2Shape_GetUserData(shapeId);
        if (userData) {
            PhysicsBody2D* physicsBody = static_cast<PhysicsBody2D*>(userData);
            bodies->push_back(physicsBody);
        }

        return true; // Continue query
    };

    b2World_OverlapAABB(s_world_2d, query_aabb, filter, queryCallback, &overlapping_bodies);

    return overlapping_bodies;
}

bool PhysicsManager::ShapeCast2D(PhysicsBody2D* body, const glm::vec2& start_pos, const glm::vec2& end_pos,
                                PhysicsBody2D*& hit_body, glm::vec2& hit_point, glm::vec2& hit_normal, float& hit_fraction) {
    if (!s_initialized || !b2World_IsValid(s_world_2d) || !body) {
        return false;
    }

    hit_body = nullptr;
    hit_fraction = 1.0f;

    b2BodyId bodyId = body->GetBox2DBodyId();
    if (!b2Body_IsValid(bodyId)) {
        return false;
    }

    b2ShapeId shapeId = body->GetShapeId();
    if (!b2Shape_IsValid(shapeId)) {
        return false;
    }

    // Get body's AABB at start position
    b2ShapeType shapeType = b2Shape_GetType(shapeId);
    b2AABB bodyAABB;

    if (shapeType == b2_circleShape) {
        b2Circle circle = b2Shape_GetCircle(shapeId);
        float radius = circle.radius;
        float centerX = start_pos.x + circle.center.x;
        float centerY = start_pos.y + circle.center.y;
        bodyAABB.lowerBound = {centerX - radius, centerY - radius};
        bodyAABB.upperBound = {centerX + radius, centerY + radius};
    } else if (shapeType == b2_polygonShape) {
        b2Polygon polygon = b2Shape_GetPolygon(shapeId);
        float minX = start_pos.x + polygon.vertices[0].x;
        float maxX = minX;
        float minY = start_pos.y + polygon.vertices[0].y;
        float maxY = minY;

        for (int i = 1; i < polygon.count; ++i) {
            float x = start_pos.x + polygon.vertices[i].x;
            float y = start_pos.y + polygon.vertices[i].y;
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }

        bodyAABB.lowerBound = {minX, minY};
        bodyAABB.upperBound = {maxX, maxY};
    } else {
        bodyAABB.lowerBound = {start_pos.x - 0.5f, start_pos.y - 0.5f};
        bodyAABB.upperBound = {start_pos.x + 0.5f, start_pos.y + 0.5f};
    }

    glm::vec2 movement = end_pos - start_pos;
    float closest_time = 1.0f;
    PhysicsBody2D* closest_body = nullptr;
    glm::vec2 closest_normal(0.0f, 1.0f);

    // Check collision against all other bodies using swept AABB
    for (const auto& other_body : s_bodies_2d) {
        if (!other_body || other_body.get() == body) {
            continue;
        }

        b2BodyId otherBodyId = other_body->GetBox2DBodyId();
        if (!b2Body_IsValid(otherBodyId)) {
            continue;
        }

        b2ShapeId otherShapeId = other_body->GetShapeId();
        if (!b2Shape_IsValid(otherShapeId)) {
            continue;
        }

        // Get other body's AABB
        glm::vec2 other_pos = other_body->GetPosition();
        b2ShapeType otherShapeType = b2Shape_GetType(otherShapeId);
        b2AABB otherAABB;

        if (otherShapeType == b2_circleShape) {
            b2Circle otherCircle = b2Shape_GetCircle(otherShapeId);
            float radius = otherCircle.radius;
            float centerX = other_pos.x + otherCircle.center.x;
            float centerY = other_pos.y + otherCircle.center.y;
            otherAABB.lowerBound = {centerX - radius, centerY - radius};
            otherAABB.upperBound = {centerX + radius, centerY + radius};
        } else if (otherShapeType == b2_polygonShape) {
            b2Polygon otherPolygon = b2Shape_GetPolygon(otherShapeId);
            float minX = other_pos.x + otherPolygon.vertices[0].x;
            float maxX = minX;
            float minY = other_pos.y + otherPolygon.vertices[0].y;
            float maxY = minY;

            for (int i = 1; i < otherPolygon.count; ++i) {
                float x = other_pos.x + otherPolygon.vertices[i].x;
                float y = other_pos.y + otherPolygon.vertices[i].y;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }

            otherAABB.lowerBound = {minX, minY};
            otherAABB.upperBound = {maxX, maxY};
        } else {
            otherAABB.lowerBound = {other_pos.x - 0.5f, other_pos.y - 0.5f};
            otherAABB.upperBound = {other_pos.x + 0.5f, other_pos.y + 0.5f};
        }

        // Perform swept AABB collision test
        float collision_time = SweptAABB(bodyAABB, otherAABB, movement);

        if (collision_time >= 0.0f && collision_time < closest_time) {
            closest_time = collision_time;
            closest_body = other_body.get();

            // Calculate collision normal based on which side was hit
            glm::vec2 collision_pos = start_pos + movement * collision_time;
            glm::vec2 body_center = glm::vec2((bodyAABB.lowerBound.x + bodyAABB.upperBound.x) * 0.5f,
                                            (bodyAABB.lowerBound.y + bodyAABB.upperBound.y) * 0.5f) + movement * collision_time;
            glm::vec2 other_center = glm::vec2((otherAABB.lowerBound.x + otherAABB.upperBound.x) * 0.5f,
                                             (otherAABB.lowerBound.y + otherAABB.upperBound.y) * 0.5f);

            glm::vec2 to_other = other_center - body_center;
            if (glm::length(to_other) > 0.001f) {
                closest_normal = -glm::normalize(to_other);
            } else {
                closest_normal = glm::vec2(0.0f, 1.0f);
            }
        }
    }

    if (closest_body) {
        hit_body = closest_body;
        hit_point = start_pos + movement * closest_time;
        hit_normal = closest_normal;
        hit_fraction = closest_time;
        return true;
    }

    return false;
}

float PhysicsManager::SweptAABB(const b2AABB& movingAABB, const b2AABB& staticAABB, const glm::vec2& velocity) {
    // If there's no movement, check for immediate overlap
    if (glm::length(velocity) < 0.001f) {
        bool overlap = (movingAABB.lowerBound.x <= staticAABB.upperBound.x) &&
                      (movingAABB.upperBound.x >= staticAABB.lowerBound.x) &&
                      (movingAABB.lowerBound.y <= staticAABB.upperBound.y) &&
                      (movingAABB.upperBound.y >= staticAABB.lowerBound.y);
        return overlap ? 0.0f : -1.0f;
    }

    // Calculate the time of collision for each axis
    float xInvEntry, yInvEntry;
    float xInvExit, yInvExit;

    // Find the distance between the objects on the near and far sides for both x and y
    if (velocity.x > 0.0f) {
        xInvEntry = staticAABB.lowerBound.x - movingAABB.upperBound.x;
        xInvExit = staticAABB.upperBound.x - movingAABB.lowerBound.x;
    } else {
        xInvEntry = staticAABB.upperBound.x - movingAABB.lowerBound.x;
        xInvExit = staticAABB.lowerBound.x - movingAABB.upperBound.x;
    }

    if (velocity.y > 0.0f) {
        yInvEntry = staticAABB.lowerBound.y - movingAABB.upperBound.y;
        yInvExit = staticAABB.upperBound.y - movingAABB.lowerBound.y;
    } else {
        yInvEntry = staticAABB.upperBound.y - movingAABB.lowerBound.y;
        yInvExit = staticAABB.lowerBound.y - movingAABB.upperBound.y;
    }

    // Find time of collision and time of leaving for each axis
    float xEntry, yEntry;
    float xExit, yExit;

    if (velocity.x == 0.0f) {
        xEntry = -std::numeric_limits<float>::infinity();
        xExit = std::numeric_limits<float>::infinity();
    } else {
        xEntry = xInvEntry / velocity.x;
        xExit = xInvExit / velocity.x;
    }

    if (velocity.y == 0.0f) {
        yEntry = -std::numeric_limits<float>::infinity();
        yExit = std::numeric_limits<float>::infinity();
    } else {
        yEntry = yInvEntry / velocity.y;
        yExit = yInvExit / velocity.y;
    }

    // Find the earliest/latest times of collision
    float entryTime = std::max(xEntry, yEntry);
    float exitTime = std::min(xExit, yExit);

    // If there was no collision
    if (entryTime > exitTime || (xEntry < 0.0f && yEntry < 0.0f) || entryTime > 1.0f) {
        return -1.0f;
    }

    // Return the time of collision (clamped to [0, 1])
    return std::max(0.0f, entryTime);
}

bool PhysicsManager::TestBodyOverlap2D(PhysicsBody2D* body, const glm::vec2& position, std::vector<PhysicsBody2D*>& overlapping_bodies) {
    overlapping_bodies.clear();

    if (!s_initialized || !b2World_IsValid(s_world_2d) || !body) {
        return false;
    }

    b2BodyId bodyId = body->GetBox2DBodyId();
    if (!b2Body_IsValid(bodyId)) {
        return false;
    }

    b2ShapeId shapeId = body->GetShapeId();
    if (!b2Shape_IsValid(shapeId)) {
        return false;
    }

    // Create shape proxy for overlap testing
    b2ShapeType shapeType = b2Shape_GetType(shapeId);
    b2ShapeProxy proxy;

    if (shapeType == b2_circleShape) {
        b2Circle circle = b2Shape_GetCircle(shapeId);
        b2Vec2 point = {position.x + circle.center.x, position.y + circle.center.y};
        proxy = b2MakeProxy(&point, 1, circle.radius);
    } else if (shapeType == b2_polygonShape) {
        b2Polygon polygon = b2Shape_GetPolygon(shapeId);
        b2Vec2 points[B2_MAX_POLYGON_VERTICES];
        for (int i = 0; i < polygon.count; ++i) {
            points[i] = {polygon.vertices[i].x + position.x, polygon.vertices[i].y + position.y};
        }
        proxy = b2MakeProxy(points, polygon.count, 0.0f);
    } else {
        // Default to a small box
        b2Vec2 points[4] = {
            {position.x - 0.5f, position.y - 0.5f},
            {position.x + 0.5f, position.y - 0.5f},
            {position.x + 0.5f, position.y + 0.5f},
            {position.x - 0.5f, position.y + 0.5f}
        };
        proxy = b2MakeProxy(points, 4, 0.0f);
    }

    // Set up query filter
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = 0xFFFF; // TODO: Use body's collision mask

    // Overlap callback
    auto overlapCallback = [](b2ShapeId hitShapeId, void* context) -> bool {
        struct OverlapContext {
            std::vector<PhysicsBody2D*>* bodies;
            PhysicsBody2D* self;
        };

        OverlapContext* ctx = static_cast<OverlapContext*>(context);

        void* userData = b2Shape_GetUserData(hitShapeId);
        if (userData) {
            PhysicsBody2D* hitBody = static_cast<PhysicsBody2D*>(userData);
            // Don't include ourselves in the results
            if (hitBody != ctx->self) {
                ctx->bodies->push_back(hitBody);
            }
        }

        return true; // Continue query
    };

    struct {
        std::vector<PhysicsBody2D*>* bodies;
        PhysicsBody2D* self;
    } context = {&overlapping_bodies, body};

    b2World_OverlapShape(s_world_2d, &proxy, filter, overlapCallback, &context);

    return !overlapping_bodies.empty();
}

bool PhysicsManager::TestSimpleAABBOverlap(PhysicsBody2D* body, const glm::vec2& position, std::vector<PhysicsBody2D*>& overlapping_bodies) {
    overlapping_bodies.clear();

    if (!s_initialized || !b2World_IsValid(s_world_2d) || !body) {
        return false;
    }

    b2BodyId bodyId = body->GetBox2DBodyId();
    if (!b2Body_IsValid(bodyId)) {
        return false;
    }

    b2ShapeId shapeId = body->GetShapeId();
    if (!b2Shape_IsValid(shapeId)) {
        return false;
    }

    // Get shape type and calculate AABB with proper bounds
    b2ShapeType shapeType = b2Shape_GetType(shapeId);
    b2AABB bodyAABB;

    if (shapeType == b2_circleShape) {
        b2Circle circle = b2Shape_GetCircle(shapeId);
        float radius = circle.radius;
        // Account for circle center offset
        float centerX = position.x + circle.center.x;
        float centerY = position.y + circle.center.y;
        bodyAABB.lowerBound = {centerX - radius, centerY - radius};
        bodyAABB.upperBound = {centerX + radius, centerY + radius};
    } else if (shapeType == b2_polygonShape) {
        b2Polygon polygon = b2Shape_GetPolygon(shapeId);
        // Calculate AABB from polygon vertices with proper transformation
        float minX = position.x + polygon.vertices[0].x;
        float maxX = minX;
        float minY = position.y + polygon.vertices[0].y;
        float maxY = minY;

        for (int i = 1; i < polygon.count; ++i) {
            float x = position.x + polygon.vertices[i].x;
            float y = position.y + polygon.vertices[i].y;
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }

        bodyAABB.lowerBound = {minX, minY};
        bodyAABB.upperBound = {maxX, maxY};
    } else {
        // Default to a 1x1 box centered at position
        bodyAABB.lowerBound = {position.x - 0.5f, position.y - 0.5f};
        bodyAABB.upperBound = {position.x + 0.5f, position.y + 0.5f};
    }

    // Check against all other bodies using simple AABB overlap
    for (const auto& other_body : s_bodies_2d) {
        if (!other_body || other_body.get() == body) {
            continue;
        }

        b2BodyId otherBodyId = other_body->GetBox2DBodyId();
        if (!b2Body_IsValid(otherBodyId)) {
            continue;
        }

        b2ShapeId otherShapeId = other_body->GetShapeId();
        if (!b2Shape_IsValid(otherShapeId)) {
            continue;
        }

        // Get other body's AABB
        glm::vec2 other_pos = other_body->GetPosition();
        b2ShapeType otherShapeType = b2Shape_GetType(otherShapeId);
        b2AABB otherAABB;

        if (otherShapeType == b2_circleShape) {
            b2Circle otherCircle = b2Shape_GetCircle(otherShapeId);
            float radius = otherCircle.radius;
            // Account for circle center offset
            float centerX = other_pos.x + otherCircle.center.x;
            float centerY = other_pos.y + otherCircle.center.y;
            otherAABB.lowerBound = {centerX - radius, centerY - radius};
            otherAABB.upperBound = {centerX + radius, centerY + radius};
        } else if (otherShapeType == b2_polygonShape) {
            b2Polygon otherPolygon = b2Shape_GetPolygon(otherShapeId);
            float minX = other_pos.x + otherPolygon.vertices[0].x;
            float maxX = minX;
            float minY = other_pos.y + otherPolygon.vertices[0].y;
            float maxY = minY;

            for (int i = 1; i < otherPolygon.count; ++i) {
                float x = other_pos.x + otherPolygon.vertices[i].x;
                float y = other_pos.y + otherPolygon.vertices[i].y;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }

            otherAABB.lowerBound = {minX, minY};
            otherAABB.upperBound = {maxX, maxY};
        } else {
            // Default to a 1x1 box centered at position
            otherAABB.lowerBound = {other_pos.x - 0.5f, other_pos.y - 0.5f};
            otherAABB.upperBound = {other_pos.x + 0.5f, other_pos.y + 0.5f};
        }

        // Test AABB overlap with small tolerance to avoid floating point precision issues
        const float tolerance = 0.001f;
        bool overlap = (bodyAABB.lowerBound.x <= otherAABB.upperBound.x + tolerance) &&
                      (bodyAABB.upperBound.x >= otherAABB.lowerBound.x - tolerance) &&
                      (bodyAABB.lowerBound.y <= otherAABB.upperBound.y + tolerance) &&
                      (bodyAABB.upperBound.y >= otherAABB.lowerBound.y - tolerance);

        if (overlap) {
            overlapping_bodies.push_back(other_body.get());
        }
    }

    return !overlapping_bodies.empty();
}

void PhysicsManager::UpdatePhysicsBodyTransform(PhysicsBody2D* body, const glm::vec2& position, float rotation) {
    if (!body || !b2Body_IsValid(body->GetBox2DBodyId())) {
        return;
    }

    b2BodyId bodyId = body->GetBox2DBodyId();
    b2BodyType bodyType = b2Body_GetType(bodyId);

    // Only update kinematic and static bodies manually
    // Dynamic bodies should be controlled by physics simulation
    if (bodyType == b2_kinematicBody || bodyType == b2_staticBody) {
        b2Vec2 pos = {position.x, position.y};
        b2Rot rot = b2MakeRot(rotation);
        b2Body_SetTransform(bodyId, pos, rot);
    }
}

void PhysicsManager::RenderDebug2D(const glm::mat4& camera_matrix) {
    if (!s_initialized || !s_debug_rendering_enabled) {
        return;
    }

    (void)camera_matrix; // TODO: Use camera matrix for proper rendering

    // Debug rendering would typically use OpenGL or the engine's renderer
    // For now, we'll just provide a framework for debug visualization

    for (const auto& body : s_bodies_2d) {
        if (!body || !body->GetOwner()) {
            continue;
        }

        b2BodyId bodyId = body->GetBox2DBodyId();
        if (!b2Body_IsValid(bodyId)) {
            continue;
        }

        // Get body transform
        b2Vec2 position = b2Body_GetPosition(bodyId);
        b2Rot rotation = b2Body_GetRotation(bodyId);
        float angle = b2Rot_GetAngle(rotation);

        // Get body type for color coding
        b2BodyType bodyType = b2Body_GetType(bodyId);

        // TODO: Render collision shapes based on their type
        // This would require integration with the engine's rendering system
        // For now, we'll just log debug information

        if (s_debug_rendering_enabled) {
            const char* typeStr = "Unknown";
            switch (bodyType) {
                case b2_staticBody: typeStr = "Static"; break;
                case b2_kinematicBody: typeStr = "Kinematic"; break;
                case b2_dynamicBody: typeStr = "Dynamic"; break;
            }

            // In a real implementation, this would render wireframe shapes
            std::cout << "Debug: " << typeStr << " body at ("
                      << position.x << ", " << position.y << ") angle: " << angle << std::endl;
        }
    }
}

PhysicsBody3D* PhysicsManager::CreatePhysicsBody3D(Node3D* node, PhysicsBodyType body_type,
                                                   CollisionShapeType shape_type, const glm::vec3& size,
                                                   const PhysicsMaterial& material) {
    if (!s_initialized || !s_world_3d || !node) {
        std::cerr << "CreatePhysicsBody3D: Invalid parameters - initialized: " << s_initialized
                  << ", world: " << (s_world_3d ? "valid" : "null")
                  << ", node: " << (node ? "valid" : "null") << std::endl;
        return nullptr;
    }

    std::cout << "PhysicsManager: Creating 3D physics body for node '" << node->GetName()
              << "' (type: " << static_cast<int>(body_type)
              << ", shape: " << static_cast<int>(shape_type) << ")" << std::endl;

    // Create collision shape
    std::unique_ptr<btCollisionShape> shape;

    switch (shape_type) {
        case CollisionShapeType::Box: {
            shape = std::make_unique<btBoxShape>(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
            break;
        }
        case CollisionShapeType::Circle:
        case CollisionShapeType::Sphere: {
            shape = std::make_unique<btSphereShape>(size.x); // Use x component as radius
            break;
        }
        case CollisionShapeType::Capsule: {
            shape = std::make_unique<btCapsuleShape>(size.x, size.y); // radius, height
            break;
        }
        case CollisionShapeType::Cylinder: {
            shape = std::make_unique<btCylinderShape>(btVector3(size.x, size.y * 0.5f, size.z)); // radius_x, half_height, radius_z
            break;
        }
        case CollisionShapeType::Mesh: {
            std::cerr << "Mesh collision shapes require CollisionMesh3D component. Use CreatePhysicsBody3D with CollisionMesh3D parameter." << std::endl;
            return nullptr;
        }
        default:
            std::cerr << "Unsupported 3D collision shape type!" << std::endl;
            return nullptr;
    }

    // Safety check for collision shape
    if (!shape) {
        std::cerr << "Failed to create collision shape for 3D physics body!" << std::endl;
        return nullptr;
    }

    // Calculate mass and inertia
    btScalar mass = (body_type == PhysicsBodyType::Dynamic) ? material.density : 0.0f;
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f) {
        shape->calculateLocalInertia(mass, localInertia);
    }

    // Create motion state
    glm::vec3 position = node->GetPosition();
    glm::quat rotation = node->GetRotation();
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));
    startTransform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));

    auto motionState = std::make_unique<btDefaultMotionState>(startTransform);
    btMotionState* motionStatePtr = motionState.get();

    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionStatePtr, shape.get(), localInertia);
    rbInfo.m_friction = material.friction;
    rbInfo.m_restitution = material.restitution;
    rbInfo.m_linearDamping = material.linear_damping;
    rbInfo.m_angularDamping = material.angular_damping;

    auto btbody = std::make_unique<btRigidBody>(rbInfo);

    // Set body type
    switch (body_type) {
        case PhysicsBodyType::Static:
            btbody->setCollisionFlags(btbody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
            break;
        case PhysicsBodyType::Kinematic:
            btbody->setCollisionFlags(btbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            break;
        case PhysicsBodyType::Dynamic:
            // Default dynamic body
            break;
    }

    // Store the shape and motion state to keep them alive
    s_mesh_shapes.push_back(std::move(shape));
    s_motion_states.push_back(std::move(motionState));

    // Add to world
    s_world_3d->addRigidBody(btbody.get());

    // Create physics body wrapper
    auto physics_body = std::make_unique<PhysicsBody3D>(btbody.release(), node);
    PhysicsBody3D* result = physics_body.get();

    // Set user pointer for collision detection
    result->GetBulletBody()->setUserPointer(result);

    s_bodies_3d.push_back(std::move(physics_body));

    std::cout << "PhysicsManager: Successfully created 3D physics body for '" << node->GetName()
              << "' (mass: " << (body_type == PhysicsBodyType::Dynamic ? material.density : 0.0f) << ")" << std::endl;

    return result;
}

PhysicsBody3D* PhysicsManager::CreatePhysicsBody3D(Node3D* node, PhysicsBodyType body_type,
                                                   CollisionMesh3D* collision_mesh,
                                                   const PhysicsMaterial& material) {
    if (!s_initialized || !s_world_3d || !node || !collision_mesh) {
        return nullptr;
    }

    // Create collision shape from mesh
    std::unique_ptr<btCollisionShape> shape = CreateMeshCollisionShape(collision_mesh);
    if (!shape) {
        std::cerr << "Failed to create mesh collision shape!" << std::endl;
        return nullptr;
    }

    // Set node transform
    glm::vec3 position = node->GetPosition();
    glm::quat rotation = node->GetRotation();

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));
    startTransform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));

    // Calculate mass and inertia
    btScalar mass = (body_type == PhysicsBodyType::Dynamic) ? material.density : 0.0f;
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f) {
        shape->calculateLocalInertia(mass, localInertia);
    }

    // Create motion state
    auto motionState = std::make_unique<btDefaultMotionState>(startTransform);
    btMotionState* motionStatePtr = motionState.get();

    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionStatePtr, shape.get(), localInertia);
    rbInfo.m_friction = material.friction;
    rbInfo.m_restitution = material.restitution;
    rbInfo.m_linearDamping = material.linear_damping;
    rbInfo.m_angularDamping = material.angular_damping;

    auto btbody = std::make_unique<btRigidBody>(rbInfo);

    // Set body type
    switch (body_type) {
        case PhysicsBodyType::Static:
            btbody->setCollisionFlags(btbody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
            break;
        case PhysicsBodyType::Kinematic:
            btbody->setCollisionFlags(btbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            break;
        case PhysicsBodyType::Dynamic:
            // Default dynamic body
            break;
    }

    // Store the shape and motion state to keep them alive
    s_mesh_shapes.push_back(std::move(shape));
    s_motion_states.push_back(std::move(motionState));

    // Add to world
    s_world_3d->addRigidBody(btbody.get());

    // Create physics body wrapper
    auto physics_body = std::make_unique<PhysicsBody3D>(btbody.release(), node);
    PhysicsBody3D* result = physics_body.get();

    s_bodies_3d.push_back(std::move(physics_body));
    return result;
}

void PhysicsManager::RemovePhysicsBody3D(PhysicsBody3D* body) {
    if (!body || !s_world_3d) {
        return;
    }

    // Remove from world
    if (body->GetBulletBody()) {
        s_world_3d->removeRigidBody(body->GetBulletBody());
    }

    // Remove from our list
    auto it = std::find_if(s_bodies_3d.begin(), s_bodies_3d.end(),
        [body](const std::unique_ptr<PhysicsBody3D>& ptr) {
            return ptr.get() == body;
        });

    if (it != s_bodies_3d.end()) {
        s_bodies_3d.erase(it);
    }
}



// === PhysicsBody3D Implementation ===

PhysicsBody3D::PhysicsBody3D(btRigidBody* body, Node3D* owner)
    : m_body(body), m_owner(owner) {
}

PhysicsBody3D::~PhysicsBody3D() {
    // Body cleanup is handled by PhysicsManager
}

void PhysicsBody3D::SetPosition(const glm::vec3& position) {
    if (m_body) {
        btTransform transform;
        m_body->getMotionState()->getWorldTransform(transform);
        transform.setOrigin(btVector3(position.x, position.y, position.z));
        m_body->setWorldTransform(transform);
    }
}

glm::vec3 PhysicsBody3D::GetPosition() const {
    if (m_body) {
        btTransform transform;
        m_body->getMotionState()->getWorldTransform(transform);
        btVector3 pos = transform.getOrigin();
        return glm::vec3(pos.x(), pos.y(), pos.z());
    }
    return glm::vec3(0.0f);
}

void PhysicsBody3D::SetRotation(const glm::quat& rotation) {
    if (m_body) {
        btTransform transform;
        m_body->getMotionState()->getWorldTransform(transform);
        transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        m_body->setWorldTransform(transform);
    }
}

glm::quat PhysicsBody3D::GetRotation() const {
    if (m_body) {
        btTransform transform;
        m_body->getMotionState()->getWorldTransform(transform);
        btQuaternion rot = transform.getRotation();
        return glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
    }
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void PhysicsBody3D::SetLinearVelocity(const glm::vec3& velocity) {
    if (m_body) {
        m_body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
}

glm::vec3 PhysicsBody3D::GetLinearVelocity() const {
    if (m_body) {
        btVector3 vel = m_body->getLinearVelocity();
        return glm::vec3(vel.x(), vel.y(), vel.z());
    }
    return glm::vec3(0.0f);
}

void PhysicsBody3D::SetAngularVelocity(const glm::vec3& velocity) {
    if (m_body) {
        m_body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
}

glm::vec3 PhysicsBody3D::GetAngularVelocity() const {
    if (m_body) {
        btVector3 vel = m_body->getAngularVelocity();
        return glm::vec3(vel.x(), vel.y(), vel.z());
    }
    return glm::vec3(0.0f);
}

void PhysicsBody3D::ApplyForce(const glm::vec3& force, const glm::vec3& point) {
    if (m_body) {
        btVector3 btForce(force.x, force.y, force.z);
        if (point != glm::vec3(0.0f)) {
            btVector3 btPoint(point.x, point.y, point.z);
            m_body->applyForce(btForce, btPoint);
        } else {
            m_body->applyCentralForce(btForce);
        }
    }
}

void PhysicsBody3D::ApplyImpulse(const glm::vec3& impulse, const glm::vec3& point) {
    if (m_body) {
        btVector3 btImpulse(impulse.x, impulse.y, impulse.z);
        if (point != glm::vec3(0.0f)) {
            btVector3 btPoint(point.x, point.y, point.z);
            m_body->applyImpulse(btImpulse, btPoint);
        } else {
            m_body->applyCentralImpulse(btImpulse);
        }
    }
}

void PhysicsBody3D::ApplyTorque(const glm::vec3& torque) {
    if (m_body) {
        m_body->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }
}

void PhysicsBody3D::SetMass(float mass) {
    if (m_body && mass > 0.0f) {
        btVector3 inertia;
        m_body->getCollisionShape()->calculateLocalInertia(mass, inertia);
        m_body->setMassProps(mass, inertia);
    }
}

float PhysicsBody3D::GetMass() const {
    if (m_body) {
        return 1.0f / m_body->getInvMass();
    }
    return 0.0f;
}

void PhysicsBody3D::SetGravityScale(float scale) {
    if (m_body) {
        glm::vec3 gravity = PhysicsManager::GetGravity3D() * scale;
        m_body->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }
}

float PhysicsBody3D::GetGravityScale() const {
    // This is an approximation since Bullet doesn't have a direct gravity scale
    return 1.0f;
}

void PhysicsBody3D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;
}

// === Mesh Collision Utility Functions ===

std::unique_ptr<btCollisionShape> PhysicsManager::CreateMeshCollisionShape(CollisionMesh3D* collision_mesh) {
    if (!collision_mesh) {
        return nullptr;
    }

    const std::string& mesh_path = collision_mesh->GetMeshPath();

    if (mesh_path.empty()) {
        // Create primitive mesh shape based on owner node's PrimitiveMesh component
        return CreatePrimitiveMeshShape(collision_mesh);
    } else {
        // Create shape from external mesh file
        return CreateExternalMeshShape(mesh_path, static_cast<int>(collision_mesh->GetMeshType()));
    }
}

std::unique_ptr<btCollisionShape> PhysicsManager::CreatePrimitiveMeshShape(CollisionMesh3D* collision_mesh) {
    if (!collision_mesh) {
        return nullptr;
    }

    Node3D* owner = dynamic_cast<Node3D*>(collision_mesh->GetOwner());
    if (!owner) {
        return nullptr;
    }

    // Look for PrimitiveMesh component on the same node
    PrimitiveMesh* primitive_mesh = owner->GetComponent<PrimitiveMesh>();
    if (!primitive_mesh) {
        std::cerr << "CollisionMesh3D: No PrimitiveMesh component found, creating default box shape" << std::endl;
        // Create a default box shape using the collision mesh scale
        glm::vec3 scale = collision_mesh->GetScale();
        return std::make_unique<btBoxShape>(btVector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));
    }

    // Get mesh properties
    glm::vec3 size = primitive_mesh->GetSize();
    glm::vec3 scale = collision_mesh->GetScale();
    size *= scale; // Apply collision mesh scale

    // Create collision shape based on primitive type
    switch (primitive_mesh->GetMeshType()) {
        case PrimitiveMesh::MeshType::Cube: {
            return std::make_unique<btBoxShape>(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        }
        case PrimitiveMesh::MeshType::Sphere: {
            float radius = std::max({size.x, size.y, size.z}) * 0.5f;
            return std::make_unique<btSphereShape>(radius);
        }
        case PrimitiveMesh::MeshType::Cylinder: {
            return std::make_unique<btCylinderShape>(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        }
        case PrimitiveMesh::MeshType::Cone: {
            // Use a cylinder shape as approximation for cone
            return std::make_unique<btCylinderShape>(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        }
        case PrimitiveMesh::MeshType::Plane: {
            // Use a thin box for plane
            return std::make_unique<btBoxShape>(btVector3(size.x * 0.5f, 0.01f, size.z * 0.5f));
        }
        default: {
            std::cerr << "CollisionMesh3D: Unsupported primitive mesh type for collision shape" << std::endl;
            return std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f)); // Default box
        }
    }
}

std::unique_ptr<btCollisionShape> PhysicsManager::CreateExternalMeshShape(const std::string& mesh_path,
                                                                         int mesh_type_int) {
    // Convert int to MeshType
    CollisionMesh3D::MeshType mesh_type = static_cast<CollisionMesh3D::MeshType>(mesh_type_int);
    // Load mesh using MeshLoader
    auto model = MeshLoader::LoadModel(mesh_path);
    if (!model || !model->IsLoaded()) {
        std::cerr << "CollisionMesh3D: Failed to load mesh from " << mesh_path << ", creating default box shape" << std::endl;
        return std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f));
    }

    const auto& meshes = model->GetMeshes();
    if (meshes.empty()) {
        std::cerr << "CollisionMesh3D: No meshes found in " << mesh_path << ", creating default box shape" << std::endl;
        return std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f));
    }

    // Use the first mesh for collision
    const auto& mesh = meshes[0];

    // Extract vertices
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    for (const auto& vertex : mesh.vertices) {
        vertices.push_back(vertex.position);
    }

    indices = mesh.indices;

    // Create collision shape based on mesh type
    std::unique_ptr<btCollisionShape> result;
    switch (mesh_type) {
        case CollisionMesh3D::MeshType::Convex: {
            result = CreateConvexHullFromMesh(vertices);
            break;
        }
        case CollisionMesh3D::MeshType::Trimesh: {
            result = CreateTriangleMeshFromMesh(vertices, indices);
            break;
        }
        case CollisionMesh3D::MeshType::Simplified: {
            // For simplified, create a convex hull but with reduced vertex count
            // TODO: Implement vertex reduction algorithm
            result = CreateConvexHullFromMesh(vertices);
            break;
        }
        default: {
            std::cerr << "CollisionMesh3D: Unknown mesh type" << std::endl;
            result = CreateConvexHullFromMesh(vertices);
            break;
        }
    }

    // If mesh creation failed, create a default box shape
    if (!result) {
        std::cerr << "CollisionMesh3D: Mesh collision shape creation failed, using default box shape" << std::endl;
        result = std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f));
    }

    return result;
}

std::unique_ptr<btConvexHullShape> PhysicsManager::CreateConvexHullFromMesh(const std::vector<glm::vec3>& vertices) {
    if (vertices.empty()) {
        std::cerr << "CreateConvexHullFromMesh: Empty vertices, returning nullptr" << std::endl;
        return nullptr;
    }

    auto convex_shape = std::make_unique<btConvexHullShape>();

    for (const auto& vertex : vertices) {
        convex_shape->addPoint(btVector3(vertex.x, vertex.y, vertex.z));
    }

    // Optimize the convex hull
    convex_shape->optimizeConvexHull();

    return convex_shape;
}

std::unique_ptr<btBvhTriangleMeshShape> PhysicsManager::CreateTriangleMeshFromMesh(const std::vector<glm::vec3>& vertices,
                                                                                  const std::vector<unsigned int>& indices) {
    if (vertices.empty() || indices.empty()) {
        std::cerr << "CreateTriangleMeshFromMesh: Empty vertices or indices, creating default box shape" << std::endl;
        return nullptr;
    }

    // Create triangle mesh
    auto triangle_mesh = std::make_unique<btTriangleMesh>();

    // Add triangles to the mesh
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 < indices.size() &&
            indices[i] < vertices.size() &&
            indices[i + 1] < vertices.size() &&
            indices[i + 2] < vertices.size()) {
            const glm::vec3& v0 = vertices[indices[i]];
            const glm::vec3& v1 = vertices[indices[i + 1]];
            const glm::vec3& v2 = vertices[indices[i + 2]];

            triangle_mesh->addTriangle(
                btVector3(v0.x, v0.y, v0.z),
                btVector3(v1.x, v1.y, v1.z),
                btVector3(v2.x, v2.y, v2.z)
            );
        }
    }

    // Store the triangle mesh to keep it alive
    btTriangleMesh* mesh_ptr = triangle_mesh.get();
    s_triangle_meshes.push_back(std::move(triangle_mesh));

    // Create BVH triangle mesh shape (for static bodies only)
    auto mesh_shape = std::make_unique<btBvhTriangleMeshShape>(mesh_ptr, true);

    return mesh_shape;
}

bool PhysicsManager::ConvexSweepTest3D(PhysicsBody3D* body, const glm::vec3& start_pos, const glm::vec3& end_pos,
                                      PhysicsBody3D*& hit_body, glm::vec3& hit_point, glm::vec3& hit_normal, float& hit_fraction) {
    if (!s_initialized || !s_world_3d || !body) {
        std::cerr << "ConvexSweepTest3D: Invalid parameters" << std::endl;
        return false;
    }

    hit_body = nullptr;
    hit_fraction = 1.0f;

    btRigidBody* btbody = body->GetBulletBody();
    if (!btbody || !btbody->getCollisionShape()) {
        std::cerr << "ConvexSweepTest3D: Invalid body or shape" << std::endl;
        return false;
    }

    // Get the collision shape from the body
    btCollisionShape* shape = btbody->getCollisionShape();

    // Only convex shapes can be used for sweep tests
    btConvexShape* convex_shape = dynamic_cast<btConvexShape*>(shape);
    if (!convex_shape) {
        std::cerr << "ConvexSweepTest3D: Shape is not convex, falling back to overlap test" << std::endl;
        // For non-convex shapes, fall back to AABB overlap test
        std::vector<PhysicsBody3D*> overlapping_bodies;
        return TestBodyOverlap3D(body, end_pos, overlapping_bodies);
    }

    // Create transforms for start and end positions
    btTransform start_transform;
    start_transform.setIdentity();
    start_transform.setOrigin(btVector3(start_pos.x, start_pos.y, start_pos.z));

    btTransform end_transform;
    end_transform.setIdentity();
    end_transform.setOrigin(btVector3(end_pos.x, end_pos.y, end_pos.z));

    // Perform convex sweep test
    btCollisionWorld::ClosestConvexResultCallback callback(
        btVector3(start_pos.x, start_pos.y, start_pos.z),
        btVector3(end_pos.x, end_pos.y, end_pos.z)
    );

    // Exclude the moving body from the test
    if (btbody->getBroadphaseHandle()) {
        callback.m_collisionFilterGroup = btbody->getBroadphaseHandle()->m_collisionFilterGroup;
        callback.m_collisionFilterMask = btbody->getBroadphaseHandle()->m_collisionFilterMask;
    }

    try {
        s_world_3d->convexSweepTest(convex_shape, start_transform, end_transform, callback);
    } catch (...) {
        std::cerr << "ConvexSweepTest3D: Exception during sweep test" << std::endl;
        return false;
    }

    if (callback.hasHit()) {
        // Find the PhysicsBody3D that was hit
        const btCollisionObject* hit_object = callback.m_hitCollisionObject;
        if (hit_object && hit_object->getUserPointer()) {
            PhysicsBody3D* hit_physics_body = static_cast<PhysicsBody3D*>(hit_object->getUserPointer());

            // Make sure we didn't hit ourselves
            if (hit_physics_body != body) {
                hit_body = hit_physics_body;

                // Convert hit point and normal to glm
                const btVector3& bt_hit_point = callback.m_hitPointWorld;
                const btVector3& bt_hit_normal = callback.m_hitNormalWorld;

                hit_point = glm::vec3(bt_hit_point.x(), bt_hit_point.y(), bt_hit_point.z());
                hit_normal = glm::vec3(bt_hit_normal.x(), bt_hit_normal.y(), bt_hit_normal.z());
                hit_fraction = callback.m_closestHitFraction;

                return true;
            }
        }
    }

    return false;
}

bool PhysicsManager::TestBodyOverlap3D(PhysicsBody3D* body, const glm::vec3& position, std::vector<PhysicsBody3D*>& overlapping_bodies) {
    overlapping_bodies.clear();

    if (!s_initialized || !s_world_3d || !body) {
        return false;
    }

    btRigidBody* btbody = body->GetBulletBody();
    if (!btbody || !btbody->getCollisionShape()) {
        return false;
    }

    // Create a transform at the test position
    btTransform test_transform;
    test_transform.setIdentity();
    test_transform.setOrigin(btVector3(position.x, position.y, position.z));

    // Create a collision object for the test
    btCollisionObject test_object;
    test_object.setCollisionShape(btbody->getCollisionShape());
    test_object.setWorldTransform(test_transform);

    // Perform overlap test
    struct OverlapCallback : public btCollisionWorld::ContactResultCallback {
        std::vector<PhysicsBody3D*>* bodies;
        PhysicsBody3D* self;

        OverlapCallback(std::vector<PhysicsBody3D*>* b, PhysicsBody3D* s) : bodies(b), self(s) {}

        btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
                               const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override {
            (void)cp; (void)partId0; (void)index0; (void)partId1; (void)index1;

            // Check both collision objects to find the one that's not our test object
            const btCollisionObject* hit_object = nullptr;
            if (colObj0Wrap->getCollisionObject() != &test_object) {
                hit_object = colObj0Wrap->getCollisionObject();
            } else if (colObj1Wrap->getCollisionObject() != &test_object) {
                hit_object = colObj1Wrap->getCollisionObject();
            }

            if (hit_object && hit_object->getUserPointer()) {
                PhysicsBody3D* hit_body = static_cast<PhysicsBody3D*>(hit_object->getUserPointer());
                if (hit_body != self) {
                    bodies->push_back(hit_body);
                }
            }

            return 0; // Continue testing
        }

        btCollisionObject test_object;
    } callback(&overlapping_bodies, body);

    callback.test_object = test_object;
    s_world_3d->contactTest(&test_object, callback);

    return !overlapping_bodies.empty();
}

bool PhysicsManager::Raycast3D(const glm::vec3& start, const glm::vec3& end,
                               Node3D*& hit_node, glm::vec3& hit_point, glm::vec3& hit_normal) {
    if (!s_initialized || !s_world_3d) {
        hit_node = nullptr;
        return false;
    }

    hit_node = nullptr;

    // Convert glm vectors to Bullet vectors
    btVector3 from(start.x, start.y, start.z);
    btVector3 to(end.x, end.y, end.z);

    // Perform raycast
    btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
    s_world_3d->rayTest(from, to, rayCallback);

    if (rayCallback.hasHit()) {
        // Convert hit point and normal back to glm
        hit_point = glm::vec3(rayCallback.m_hitPointWorld.getX(),
                             rayCallback.m_hitPointWorld.getY(),
                             rayCallback.m_hitPointWorld.getZ());

        hit_normal = glm::vec3(rayCallback.m_hitNormalWorld.getX(),
                              rayCallback.m_hitNormalWorld.getY(),
                              rayCallback.m_hitNormalWorld.getZ());

        // Try to find the Node3D associated with the hit body
        const btRigidBody* hit_body = btRigidBody::upcast(rayCallback.m_collisionObject);
        if (hit_body) {
            // Search through all 3D physics bodies to find the matching one
            for (const auto& body : s_bodies_3d) {
                if (body->GetBulletBody() == hit_body) {
                    hit_node = body->GetOwner();
                    break;
                }
            }
        }

        return true;
    }

    return false;
}

bool PhysicsManager::Raycast2D(const glm::vec2& start, const glm::vec2& end,
                               Node2D*& hit_node, glm::vec2& hit_point, glm::vec2& hit_normal) {
    if (!s_initialized || !b2World_IsValid(s_world_2d)) {
        hit_node = nullptr;
        return false;
    }

    hit_node = nullptr;

    // Convert glm vectors to Box2D vectors
    b2Vec2 point1 = {start.x, start.y};
    b2Vec2 point2 = {end.x, end.y};

    // Create a raycast input
    b2RayCastInput input;
    input.origin = point1;
    input.translation = {point2.x - point1.x, point2.y - point1.y};
    input.maxFraction = 1.0f;

    // For now, return false as a simplified implementation
    // A full implementation would use Box2D's world query functions
    // and iterate through bodies to find hits
    return false;
}

} // namespace Lupine
