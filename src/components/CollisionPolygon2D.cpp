#include "lupine/components/CollisionPolygon2D.h"
#include "lupine/nodes/Node2D.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Lupine {

CollisionPolygon2D::CollisionPolygon2D()
    : Component("CollisionPolygon2D")
    , m_is_convex(true)
    , m_is_trigger(false)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_needs_update(false) {
    
    // Create a default square polygon
    m_vertices = {
        {-0.5f, -0.5f},
        { 0.5f, -0.5f},
        { 0.5f,  0.5f},
        {-0.5f,  0.5f}
    };
    
    InitializeExportVariables();
}

void CollisionPolygon2D::SetVertices(const std::vector<glm::vec2>& vertices) {
    if (m_vertices != vertices) {
        m_vertices = vertices;
        m_is_convex = CheckConvexity();
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionPolygon2D::AddVertex(const glm::vec2& vertex) {
    m_vertices.push_back(vertex);
    m_is_convex = CheckConvexity();
    m_needs_update = true;
    UpdateExportVariables();
}

void CollisionPolygon2D::RemoveVertex(size_t index) {
    if (index < m_vertices.size()) {
        m_vertices.erase(m_vertices.begin() + index);
        m_is_convex = CheckConvexity();
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionPolygon2D::ClearVertices() {
    m_vertices.clear();
    m_is_convex = true;
    m_needs_update = true;
    UpdateExportVariables();
}

void CollisionPolygon2D::SetConvex(bool convex) {
    if (m_is_convex != convex) {
        m_is_convex = convex;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionPolygon2D::SetTrigger(bool trigger) {
    if (m_is_trigger != trigger) {
        m_is_trigger = trigger;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionPolygon2D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionPolygon2D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

bool CollisionPolygon2D::CollidesWithLayer(int layer) const {
    return (m_collision_mask & (1 << layer)) != 0;
}

bool CollisionPolygon2D::IsValidPolygon() const {
    return m_vertices.size() >= 3;
}

void CollisionPolygon2D::OnReady() {
    UpdateFromExportVariables();
}

void CollisionPolygon2D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Update from export variables if they changed
    UpdateFromExportVariables();
    
    // Note: This component defines collision shapes but doesn't create physics bodies itself.
    // Physics bodies (RigidBody2D, KinematicBody2D, Area2D) will use this component's data.
}

void CollisionPolygon2D::InitializeExportVariables() {
    // Polygon properties
    AddExportVariable("vertices", VerticesToString(), "Polygon vertices as comma-separated coordinates", ExportVariableType::String);
    AddExportVariable("is_convex", m_is_convex, "Whether polygon is convex (for optimization)", ExportVariableType::Bool);
    
    // Collision properties
    AddExportVariable("is_trigger", m_is_trigger, "Whether shape is a trigger (no collision response)", ExportVariableType::Bool);
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer (0-31)", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask (which layers to collide with)", ExportVariableType::Int);
}

void CollisionPolygon2D::UpdateExportVariables() {
    SetExportVariable("vertices", VerticesToString());
    SetExportVariable("is_convex", m_is_convex);
    SetExportVariable("is_trigger", m_is_trigger);
    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
}

void CollisionPolygon2D::UpdateFromExportVariables() {
    // Get values from export variables
    std::string new_vertices_str = GetExportVariableValue<std::string>("vertices", VerticesToString());
    bool new_is_convex = GetExportVariableValue<bool>("is_convex", m_is_convex);
    bool new_is_trigger = GetExportVariableValue<bool>("is_trigger", m_is_trigger);
    int new_collision_layer = GetExportVariableValue<int>("collision_layer", m_collision_layer);
    int new_collision_mask = GetExportVariableValue<int>("collision_mask", m_collision_mask);

    // Check if vertices changed
    std::string current_vertices_str = VerticesToString();
    if (current_vertices_str != new_vertices_str) {
        StringToVertices(new_vertices_str);
        m_needs_update = true;
    }

    // Check other values
    bool changed = false;
    if (m_is_convex != new_is_convex) {
        m_is_convex = new_is_convex;
        changed = true;
    }
    if (m_is_trigger != new_is_trigger) {
        m_is_trigger = new_is_trigger;
        changed = true;
    }
    if (m_collision_layer != new_collision_layer) {
        m_collision_layer = new_collision_layer;
        changed = true;
    }
    if (m_collision_mask != new_collision_mask) {
        m_collision_mask = new_collision_mask;
        changed = true;
    }

    if (changed) {
        m_needs_update = true;
    }
}

std::string CollisionPolygon2D::VerticesToString() const {
    std::stringstream ss;
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        if (i > 0) ss << ",";
        ss << m_vertices[i].x << "," << m_vertices[i].y;
    }
    return ss.str();
}

void CollisionPolygon2D::StringToVertices(const std::string& str) {
    m_vertices.clear();

    if (str.empty()) return;

    std::stringstream ss(str);
    std::string token;
    std::vector<float> values;

    while (std::getline(ss, token, ',')) {
        try {
            values.push_back(std::stof(token));
        } catch (const std::exception&) {
            // Skip invalid values
        }
    }

    // Convert pairs of values to vertices
    for (size_t i = 0; i + 1 < values.size(); i += 2) {
        m_vertices.emplace_back(values[i], values[i + 1]);
    }

    m_is_convex = CheckConvexity();
}

bool CollisionPolygon2D::CheckConvexity() const {
    if (m_vertices.size() < 3) return true;

    // Simple convexity check using cross products
    bool sign = false;
    bool first = true;

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        size_t j = (i + 1) % m_vertices.size();
        size_t k = (i + 2) % m_vertices.size();

        glm::vec2 v1 = m_vertices[j] - m_vertices[i];
        glm::vec2 v2 = m_vertices[k] - m_vertices[j];

        float cross = v1.x * v2.y - v1.y * v2.x;

        if (first) {
            sign = cross > 0;
            first = false;
        } else if ((cross > 0) != sign) {
            return false; // Not convex
        }
    }

    return true;
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(CollisionPolygon2D, "Physics", "2D collision polygon for custom shapes")
