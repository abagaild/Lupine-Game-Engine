#include "lupine/editor/TileBuilder.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateMesh(const PrimitiveMeshParams& params) {
    switch (params.type) {
        case TilePrimitiveType::Cube:
            return GenerateCube(params);
        case TilePrimitiveType::Rectangle:
            return GenerateRectangle(params);
        case TilePrimitiveType::TriangularPyramid:
            return GenerateTriangularPyramid(params);
        case TilePrimitiveType::Pyramid:
            return GeneratePyramid(params);
        case TilePrimitiveType::Cone:
            return GenerateCone(params);
        case TilePrimitiveType::Sphere:
            return GenerateSphere(params);
        case TilePrimitiveType::CylinderOpen:
        case TilePrimitiveType::CylinderClosed:
            return GenerateCylinder(params);
        default:
            return GenerateCube(params);
    }
}

std::vector<MeshFace> TilePrimitiveMeshGenerator::GetAvailableFaces(TilePrimitiveType type) {
    switch (type) {
        case TilePrimitiveType::Cube:
        case TilePrimitiveType::Rectangle:
            return {MeshFace::Front, MeshFace::Back, MeshFace::Left, 
                   MeshFace::Right, MeshFace::Top, MeshFace::Bottom};
        
        case TilePrimitiveType::TriangularPyramid:
        case TilePrimitiveType::Pyramid:
            return {MeshFace::Front, MeshFace::Back, MeshFace::Left, 
                   MeshFace::Right, MeshFace::Base};
        
        case TilePrimitiveType::Cone:
            return {MeshFace::Side, MeshFace::Base};
        
        case TilePrimitiveType::Sphere:
            return {MeshFace::North, MeshFace::South, MeshFace::East, MeshFace::West};
        
        case TilePrimitiveType::CylinderOpen:
            return {MeshFace::Side};
        
        case TilePrimitiveType::CylinderClosed:
            return {MeshFace::Side, MeshFace::Top, MeshFace::Bottom};
        
        default:
            return {MeshFace::Front};
    }
}

std::string TilePrimitiveMeshGenerator::GetFaceName(MeshFace face) {
    switch (face) {
        case MeshFace::Front: return "Front";
        case MeshFace::Back: return "Back";
        case MeshFace::Left: return "Left";
        case MeshFace::Right: return "Right";
        case MeshFace::Top: return "Top";
        case MeshFace::Bottom: return "Bottom";
        case MeshFace::Side: return "Side";
        case MeshFace::Base: return "Base";
        case MeshFace::North: return "North";
        case MeshFace::South: return "South";
        case MeshFace::East: return "East";
        case MeshFace::West: return "West";
        default: return "Unknown";
    }
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateCube(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;
    
    float w = params.dimensions.x * 0.5f;
    float h = params.dimensions.y * 0.5f;
    float d = params.dimensions.z * 0.5f;
    
    // Cube vertices with UV coordinates for texture atlas layout
    // Format: position(3), normal(3), uv(2) = 8 floats per vertex
    // UV Layout: [Left] [Front] [Right] / [Back] [Bottom] [Top]
    float face_width = 1.0f / 3.0f;
    float face_height = 1.0f / 2.0f;

    std::vector<float> vertices = {
        // Front face (Z+) - Middle top
        -w, -h,  d,  0.0f,  0.0f,  1.0f,  face_width, 0.0f,
         w, -h,  d,  0.0f,  0.0f,  1.0f,  2.0f * face_width, 0.0f,
         w,  h,  d,  0.0f,  0.0f,  1.0f,  2.0f * face_width, face_height,
        -w,  h,  d,  0.0f,  0.0f,  1.0f,  face_width, face_height,

        // Back face (Z-) - Left bottom
        -w, -h, -d,  0.0f,  0.0f, -1.0f,  face_width, face_height,
        -w,  h, -d,  0.0f,  0.0f, -1.0f,  face_width, 1.0f,
         w,  h, -d,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         w, -h, -d,  0.0f,  0.0f, -1.0f,  0.0f, face_height,

        // Left face (X-) - Left top
        -w, -h, -d, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -w, -h,  d, -1.0f,  0.0f,  0.0f,  face_width, 0.0f,
        -w,  h,  d, -1.0f,  0.0f,  0.0f,  face_width, face_height,
        -w,  h, -d, -1.0f,  0.0f,  0.0f,  0.0f, face_height,

        // Right face (X+) - Right top
         w, -h, -d,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         w,  h, -d,  1.0f,  0.0f,  0.0f,  1.0f, face_height,
         w,  h,  d,  1.0f,  0.0f,  0.0f,  2.0f * face_width, face_height,
         w, -h,  d,  1.0f,  0.0f,  0.0f,  2.0f * face_width, 0.0f,

        // Top face (Y+) - Right bottom
        -w,  h, -d,  0.0f,  1.0f,  0.0f,  2.0f * face_width, 1.0f,
        -w,  h,  d,  0.0f,  1.0f,  0.0f,  2.0f * face_width, face_height,
         w,  h,  d,  0.0f,  1.0f,  0.0f,  1.0f, face_height,
         w,  h, -d,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,

        // Bottom face (Y-) - Middle bottom
        -w, -h, -d,  0.0f, -1.0f,  0.0f,  2.0f * face_width, face_height,
         w, -h, -d,  0.0f, -1.0f,  0.0f,  face_width, face_height,
         w, -h,  d,  0.0f, -1.0f,  0.0f,  face_width, 1.0f,
        -w, -h,  d,  0.0f, -1.0f,  0.0f,  2.0f * face_width, 1.0f
    };
    
    // Cube indices
    std::vector<unsigned int> indices = {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Back face
        4, 5, 6,  6, 7, 4,
        // Left face
        8, 9, 10,  10, 11, 8,
        // Right face
        12, 13, 14,  14, 15, 12,
        // Top face
        16, 17, 18,  18, 19, 16,
        // Bottom face
        20, 21, 22,  22, 23, 20
    };
    
    mesh_data.vertices = vertices;
    mesh_data.indices = indices;
    
    // Set up face vertex indices for texture mapping
    mesh_data.face_vertex_indices[MeshFace::Front] = {0, 1, 2, 3};
    mesh_data.face_vertex_indices[MeshFace::Back] = {4, 5, 6, 7};
    mesh_data.face_vertex_indices[MeshFace::Left] = {8, 9, 10, 11};
    mesh_data.face_vertex_indices[MeshFace::Right] = {12, 13, 14, 15};
    mesh_data.face_vertex_indices[MeshFace::Top] = {16, 17, 18, 19};
    mesh_data.face_vertex_indices[MeshFace::Bottom] = {20, 21, 22, 23};
    
    // UV bounds will be calculated by CalculateFaceUVBounds()
    
    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);
    
    return mesh_data;
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateRectangle(const PrimitiveMeshParams& params) {
    // Rectangle is essentially a cube with custom dimensions
    return GenerateCube(params);
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateTriangularPyramid(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;

    float w = params.dimensions.x * 0.5f;
    float h = params.dimensions.y;
    float d = params.dimensions.z * 0.5f;

    // Triangular pyramid vertices
    std::vector<float> vertices = {
        // Base triangle (Y = 0)
        -w, 0.0f, -d,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  // 0
         w, 0.0f, -d,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  // 1
         0.0f, 0.0f,  d,  0.0f, -1.0f,  0.0f,  0.5f, 1.0f,  // 2

        // Apex
         0.0f, h, 0.0f,  0.0f,  1.0f,  0.0f,  0.5f, 0.5f,  // 3

        // Front face vertices (duplicated for proper normals)
        -w, 0.0f, -d,  -0.577f,  0.577f, -0.577f,  0.0f, 0.0f,  // 4
         w, 0.0f, -d,   0.577f,  0.577f, -0.577f,  1.0f, 0.0f,  // 5
         0.0f, h, 0.0f,  0.0f,  0.577f, -0.577f,  0.5f, 1.0f,  // 6

        // Left face vertices
         0.0f, 0.0f,  d,  0.577f,  0.577f,  0.577f,  0.0f, 0.0f,  // 7
        -w, 0.0f, -d,  -0.577f,  0.577f,  0.577f,  1.0f, 0.0f,  // 8
         0.0f, h, 0.0f, -0.577f,  0.577f,  0.577f,  0.5f, 1.0f,  // 9

        // Right face vertices
         w, 0.0f, -d,   0.577f,  0.577f,  0.577f,  0.0f, 0.0f,  // 10
         0.0f, 0.0f,  d, -0.577f,  0.577f,  0.577f,  1.0f, 0.0f,  // 11
         0.0f, h, 0.0f,  0.577f,  0.577f,  0.577f,  0.5f, 1.0f   // 12
    };

    std::vector<unsigned int> indices = {
        // Base triangle
        0, 2, 1,
        // Front face
        4, 6, 5,
        // Left face
        7, 9, 8,
        // Right face
        10, 12, 11
    };

    mesh_data.vertices = vertices;
    mesh_data.indices = indices;

    // Set up face vertex indices
    mesh_data.face_vertex_indices[MeshFace::Base] = {0, 1, 2};
    mesh_data.face_vertex_indices[MeshFace::Front] = {4, 5, 6};
    mesh_data.face_vertex_indices[MeshFace::Left] = {7, 8, 9};
    mesh_data.face_vertex_indices[MeshFace::Right] = {10, 11, 12};

    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);

    return mesh_data;
}

GeneratedMeshData TilePrimitiveMeshGenerator::GeneratePyramid(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;

    float w = params.dimensions.x * 0.5f;
    float h = params.dimensions.y;
    float d = params.dimensions.z * 0.5f;

    // Square pyramid vertices
    std::vector<float> vertices = {
        // Base square (Y = 0)
        -w, 0.0f, -d,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  // 0
         w, 0.0f, -d,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  // 1
         w, 0.0f,  d,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,  // 2
        -w, 0.0f,  d,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  // 3

        // Apex
         0.0f, h, 0.0f,  0.0f,  1.0f,  0.0f,  0.5f, 0.5f,  // 4

        // Front face vertices (duplicated for proper normals)
        -w, 0.0f, -d,  0.0f,  0.707f, -0.707f,  0.0f, 0.0f,  // 5
         w, 0.0f, -d,  0.0f,  0.707f, -0.707f,  1.0f, 0.0f,  // 6
         0.0f, h, 0.0f,  0.0f,  0.707f, -0.707f,  0.5f, 1.0f,  // 7

        // Back face vertices
         w, 0.0f,  d,  0.0f,  0.707f,  0.707f,  0.0f, 0.0f,  // 8
        -w, 0.0f,  d,  0.0f,  0.707f,  0.707f,  1.0f, 0.0f,  // 9
         0.0f, h, 0.0f,  0.0f,  0.707f,  0.707f,  0.5f, 1.0f,  // 10

        // Left face vertices
        -w, 0.0f,  d, -0.707f,  0.707f,  0.0f,  0.0f, 0.0f,  // 11
        -w, 0.0f, -d, -0.707f,  0.707f,  0.0f,  1.0f, 0.0f,  // 12
         0.0f, h, 0.0f, -0.707f,  0.707f,  0.0f,  0.5f, 1.0f,  // 13

        // Right face vertices
         w, 0.0f, -d,  0.707f,  0.707f,  0.0f,  0.0f, 0.0f,  // 14
         w, 0.0f,  d,  0.707f,  0.707f,  0.0f,  1.0f, 0.0f,  // 15
         0.0f, h, 0.0f,  0.707f,  0.707f,  0.0f,  0.5f, 1.0f   // 16
    };

    std::vector<unsigned int> indices = {
        // Base square
        0, 1, 2,  2, 3, 0,
        // Front face
        5, 7, 6,
        // Back face
        8, 10, 9,
        // Left face
        11, 13, 12,
        // Right face
        14, 16, 15
    };

    mesh_data.vertices = vertices;
    mesh_data.indices = indices;

    // Set up face vertex indices
    mesh_data.face_vertex_indices[MeshFace::Base] = {0, 1, 2, 3};
    mesh_data.face_vertex_indices[MeshFace::Front] = {5, 6, 7};
    mesh_data.face_vertex_indices[MeshFace::Back] = {8, 9, 10};
    mesh_data.face_vertex_indices[MeshFace::Left] = {11, 12, 13};
    mesh_data.face_vertex_indices[MeshFace::Right] = {14, 15, 16};

    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);

    return mesh_data;
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateCone(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;

    int sides = std::max(3, params.subdivisions);
    float radius = params.radius;
    float height = params.height;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Apex vertex
    vertices.insert(vertices.end(), {0.0f, height, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f});

    // Base center vertex
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});

    // Generate base vertices and side vertices
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Calculate normal for side face
        glm::vec3 side_normal = glm::normalize(glm::vec3(x, radius, z));

        // Side vertex (for side faces)
        vertices.insert(vertices.end(), {x, 0.0f, z, side_normal.x, side_normal.y, side_normal.z,
                                       (float)i / sides, 0.0f});

        // Base vertex (for base face)
        vertices.insert(vertices.end(), {x, 0.0f, z, 0.0f, -1.0f, 0.0f,
                                       0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});
    }

    // Generate indices
    for (int i = 0; i < sides; ++i) {
        int next = (i + 1) % sides;

        // Side triangle (apex to base edge)
        indices.insert(indices.end(), {0u, static_cast<unsigned int>(2 + i * 2), static_cast<unsigned int>(2 + next * 2)});

        // Base triangle (center to base edge)
        indices.insert(indices.end(), {1u, static_cast<unsigned int>(3 + next * 2), static_cast<unsigned int>(3 + i * 2)});
    }

    mesh_data.vertices = vertices;
    mesh_data.indices = indices;

    // Set up face vertex indices
    std::vector<int> side_vertices, base_vertices;
    for (int i = 0; i < sides; ++i) {
        side_vertices.push_back(2 + i * 2);
        base_vertices.push_back(3 + i * 2);
    }
    side_vertices.push_back(0); // Add apex
    base_vertices.push_back(1); // Add base center

    mesh_data.face_vertex_indices[MeshFace::Side] = side_vertices;
    mesh_data.face_vertex_indices[MeshFace::Base] = base_vertices;

    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);

    return mesh_data;
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateSphere(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;

    int rings = std::max(3, params.subdivisions / 2);
    int sectors = std::max(3, params.subdivisions);
    float radius = params.radius;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int r = 0; r <= rings; ++r) {
        float phi = M_PI * r / rings; // 0 to PI
        float y = radius * cos(phi);
        float ring_radius = radius * sin(phi);

        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.0f * M_PI * s / sectors; // 0 to 2*PI
            float x = ring_radius * cos(theta);
            float z = ring_radius * sin(theta);

            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal (same as position for unit sphere)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            // Texture coordinates
            vertices.push_back((float)s / sectors);
            vertices.push_back((float)r / rings);
        }
    }

    // Generate indices
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            int current = r * (sectors + 1) + s;
            int next = current + sectors + 1;

            // First triangle
            indices.insert(indices.end(), {static_cast<unsigned int>(current), static_cast<unsigned int>(next), static_cast<unsigned int>(current + 1)});
            // Second triangle
            indices.insert(indices.end(), {static_cast<unsigned int>(current + 1), static_cast<unsigned int>(next), static_cast<unsigned int>(next + 1)});
        }
    }

    mesh_data.vertices = vertices;
    mesh_data.indices = indices;

    // For sphere, we'll use hemisphere regions as faces
    int total_vertices = (rings + 1) * (sectors + 1);
    std::vector<int> all_vertices;
    for (int i = 0; i < total_vertices; ++i) {
        all_vertices.push_back(i);
    }

    mesh_data.face_vertex_indices[MeshFace::North] = all_vertices; // Top hemisphere
    mesh_data.face_vertex_indices[MeshFace::South] = all_vertices; // Bottom hemisphere
    mesh_data.face_vertex_indices[MeshFace::East] = all_vertices;  // Right hemisphere
    mesh_data.face_vertex_indices[MeshFace::West] = all_vertices;  // Left hemisphere

    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);

    return mesh_data;
}

GeneratedMeshData TilePrimitiveMeshGenerator::GenerateCylinder(const PrimitiveMeshParams& params) {
    GeneratedMeshData mesh_data;

    int sides = std::max(3, params.subdivisions);
    float radius = params.radius;
    float height = params.height;
    float half_height = height * 0.5f;
    bool closed = (params.type == TilePrimitiveType::CylinderClosed);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate side vertices
    for (int i = 0; i <= sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Bottom vertex
        vertices.insert(vertices.end(), {x, -half_height, z, x/radius, 0.0f, z/radius, (float)i/sides, 0.0f});

        // Top vertex
        vertices.insert(vertices.end(), {x, half_height, z, x/radius, 0.0f, z/radius, (float)i/sides, 1.0f});
    }

    // Generate side indices
    for (int i = 0; i < sides; ++i) {
        int bottom1 = i * 2;
        int top1 = i * 2 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = (i + 1) * 2 + 1;

        // First triangle
        indices.insert(indices.end(), {static_cast<unsigned int>(bottom1), static_cast<unsigned int>(top1), static_cast<unsigned int>(bottom2)});
        // Second triangle
        indices.insert(indices.end(), {static_cast<unsigned int>(bottom2), static_cast<unsigned int>(top1), static_cast<unsigned int>(top2)});
    }

    std::vector<int> side_vertices;
    for (int i = 0; i <= sides; ++i) {
        side_vertices.push_back(i * 2);     // Bottom vertices
        side_vertices.push_back(i * 2 + 1); // Top vertices
    }
    mesh_data.face_vertex_indices[MeshFace::Side] = side_vertices;

    if (closed) {
        int vertex_offset = vertices.size() / 8; // Current vertex count

        // Add top and bottom center vertices
        vertices.insert(vertices.end(), {0.0f, half_height, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f}); // Top center
        vertices.insert(vertices.end(), {0.0f, -half_height, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f}); // Bottom center

        int top_center = vertex_offset;
        int bottom_center = vertex_offset + 1;

        // Add top and bottom rim vertices
        std::vector<int> top_vertices, bottom_vertices;
        for (int i = 0; i < sides; ++i) {
            float angle = 2.0f * M_PI * i / sides;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            // Top rim vertex
            vertices.insert(vertices.end(), {x, half_height, z, 0.0f, 1.0f, 0.0f,
                                           0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});
            top_vertices.push_back(vertex_offset + 2 + i);

            // Bottom rim vertex
            vertices.insert(vertices.end(), {x, -half_height, z, 0.0f, -1.0f, 0.0f,
                                           0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});
            bottom_vertices.push_back(vertex_offset + 2 + sides + i);
        }

        // Generate top and bottom indices
        for (int i = 0; i < sides; ++i) {
            int next = (i + 1) % sides;

            // Top face triangle
            indices.insert(indices.end(), {static_cast<unsigned int>(top_center), static_cast<unsigned int>(top_vertices[i]), static_cast<unsigned int>(top_vertices[next])});

            // Bottom face triangle
            indices.insert(indices.end(), {static_cast<unsigned int>(bottom_center), static_cast<unsigned int>(bottom_vertices[next]), static_cast<unsigned int>(bottom_vertices[i])});
        }

        // Set up face vertex indices for caps
        top_vertices.push_back(top_center);
        bottom_vertices.push_back(bottom_center);
        mesh_data.face_vertex_indices[MeshFace::Top] = top_vertices;
        mesh_data.face_vertex_indices[MeshFace::Bottom] = bottom_vertices;
    }

    mesh_data.vertices = vertices;
    mesh_data.indices = indices;

    CalculateBounds(mesh_data);
    CalculateFaceUVBounds(mesh_data);

    return mesh_data;
}

void TilePrimitiveMeshGenerator::CalculateBounds(GeneratedMeshData& mesh_data) {
    if (mesh_data.vertices.empty()) {
        mesh_data.min_bounds = glm::vec3(0.0f);
        mesh_data.max_bounds = glm::vec3(0.0f);
        return;
    }
    
    mesh_data.min_bounds = glm::vec3(FLT_MAX);
    mesh_data.max_bounds = glm::vec3(-FLT_MAX);
    
    // Iterate through vertices (8 floats per vertex: pos(3), normal(3), uv(2))
    for (size_t i = 0; i < mesh_data.vertices.size(); i += 8) {
        glm::vec3 pos(mesh_data.vertices[i], mesh_data.vertices[i + 1], mesh_data.vertices[i + 2]);
        mesh_data.min_bounds = glm::min(mesh_data.min_bounds, pos);
        mesh_data.max_bounds = glm::max(mesh_data.max_bounds, pos);
    }
}

void TilePrimitiveMeshGenerator::CalculateFaceUVBounds(GeneratedMeshData& mesh_data) {
    // Calculate proper UV bounds for each face to avoid overlapping
    // Use texture atlas layout where each face gets its own UV region

    auto face_count = mesh_data.face_vertex_indices.size();
    if (face_count == 0) return;

    // For cube-like meshes with 6 faces, use a 3x2 layout
    if (face_count == 6) {
        // Standard cube unwrapping layout:
        //   [Left] [Front] [Right]
        //   [Back] [Bottom] [Top]
        float face_width = 1.0f / 3.0f;
        float face_height = 1.0f / 2.0f;

        mesh_data.face_uv_bounds[MeshFace::Left] = glm::vec4(0.0f, 0.0f, face_width, face_height);
        mesh_data.face_uv_bounds[MeshFace::Front] = glm::vec4(face_width, 0.0f, 2.0f * face_width, face_height);
        mesh_data.face_uv_bounds[MeshFace::Right] = glm::vec4(2.0f * face_width, 0.0f, 1.0f, face_height);
        mesh_data.face_uv_bounds[MeshFace::Back] = glm::vec4(0.0f, face_height, face_width, 1.0f);
        mesh_data.face_uv_bounds[MeshFace::Bottom] = glm::vec4(face_width, face_height, 2.0f * face_width, 1.0f);
        mesh_data.face_uv_bounds[MeshFace::Top] = glm::vec4(2.0f * face_width, face_height, 1.0f, 1.0f);
    }
    // For pyramid-like meshes with 5 faces, use a cross layout
    else if (face_count == 5) {
        float face_size = 1.0f / 3.0f;

        mesh_data.face_uv_bounds[MeshFace::Base] = glm::vec4(face_size, 0.0f, 2.0f * face_size, face_size);
        mesh_data.face_uv_bounds[MeshFace::Front] = glm::vec4(face_size, face_size, 2.0f * face_size, 2.0f * face_size);
        mesh_data.face_uv_bounds[MeshFace::Left] = glm::vec4(0.0f, face_size, face_size, 2.0f * face_size);
        mesh_data.face_uv_bounds[MeshFace::Right] = glm::vec4(2.0f * face_size, face_size, 1.0f, 2.0f * face_size);
        mesh_data.face_uv_bounds[MeshFace::Back] = glm::vec4(face_size, 2.0f * face_size, 2.0f * face_size, 1.0f);
    }
    // For triangular pyramid with 4 faces, use a 2x2 layout
    else if (face_count == 4) {
        float face_size = 0.5f;

        mesh_data.face_uv_bounds[MeshFace::Base] = glm::vec4(0.0f, 0.0f, face_size, face_size);
        mesh_data.face_uv_bounds[MeshFace::Front] = glm::vec4(face_size, 0.0f, 1.0f, face_size);
        mesh_data.face_uv_bounds[MeshFace::Left] = glm::vec4(0.0f, face_size, face_size, 1.0f);
        mesh_data.face_uv_bounds[MeshFace::Right] = glm::vec4(face_size, face_size, 1.0f, 1.0f);
    }
    // For cone and cylinder with 2 faces (side + base), use vertical split
    else if (face_count == 2) {
        mesh_data.face_uv_bounds[MeshFace::Side] = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
        mesh_data.face_uv_bounds[MeshFace::Base] = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
    }
    // For cylinder with 3 faces (side + top + bottom), use horizontal split
    else if (face_count == 3) {
        float face_width = 1.0f / 3.0f;

        mesh_data.face_uv_bounds[MeshFace::Side] = glm::vec4(0.0f, 0.0f, 2.0f * face_width, 1.0f);
        mesh_data.face_uv_bounds[MeshFace::Top] = glm::vec4(2.0f * face_width, 0.0f, 1.0f, 0.5f);
        mesh_data.face_uv_bounds[MeshFace::Bottom] = glm::vec4(2.0f * face_width, 0.5f, 1.0f, 1.0f);
    }
    // For sphere (single face), use full UV space
    else {
        for (auto& pair : mesh_data.face_vertex_indices) {
            mesh_data.face_uv_bounds[pair.first] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }
    }
}

} // namespace Lupine
