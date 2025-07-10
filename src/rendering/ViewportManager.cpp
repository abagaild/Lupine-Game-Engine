#include "lupine/rendering/ViewportManager.h"
#include "lupine/core/Project.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Lupine {

// Static member definition
ViewportManager::ScreenBounds ViewportManager::s_current_bounds = ViewportManager::GetDefaultScreenBounds();

ViewportManager::ScreenBounds ViewportManager::GetScreenBounds(const Project* project) {
    if (!project) {
        return GetDefaultScreenBounds();
    }

    // Get render resolution from project settings (separate from window size)
    int width = project->GetSettingValue<int>("display/render_width", 1920);
    int height = project->GetSettingValue<int>("display/render_height", 1080);

    // Check if we're in debug mode and should use debug render resolution
    float debug_scale = project->GetSettingValue<float>("display/debug_window_scale", 1.0f);
    if (debug_scale != 1.0f) {
        int debug_render_width = project->GetSettingValue<int>("display/debug_render_width", width);
        int debug_render_height = project->GetSettingValue<int>("display/debug_render_height", height);
        if (debug_render_width != width || debug_render_height != height) {
            width = debug_render_width;
            height = debug_render_height;
        }
    }

    return GetScreenBounds(static_cast<float>(width), static_cast<float>(height));
}

ViewportManager::ScreenBounds ViewportManager::GetScreenBounds(float width, float height) {
    ScreenBounds bounds;
    bounds.width = width;
    bounds.height = height;
    bounds.aspect_ratio = width / height;
    bounds.center = glm::vec2(width * 0.5f, height * 0.5f);
    bounds.bounds = glm::vec4(0.0f, 0.0f, width, height); // left, top, right, bottom
    
    return bounds;
}

ViewportManager::ScreenBounds ViewportManager::GetDefaultScreenBounds() {
    // Default to high DPI resolution (1920x1080 render resolution)
    return GetScreenBounds(1920.0f, 1080.0f);
}

glm::mat4 ViewportManager::CreateScreenSpaceProjection(const ScreenBounds& bounds) {
    // Create orthographic projection for screen space (0,0 = top-left)
    return glm::ortho(0.0f, bounds.width, bounds.height, 0.0f, -1.0f, 1.0f);
}

glm::mat4 ViewportManager::Create2DWorldProjection(const ScreenBounds& bounds, float zoom, const glm::vec2& center_offset) {
    // Calculate world space bounds based on zoom and aspect ratio
    float half_height = (bounds.height * 0.5f) / zoom;
    float half_width = (bounds.width * 0.5f) / zoom;
    
    // Apply center offset
    float left = -half_width + center_offset.x;
    float right = half_width + center_offset.x;
    float bottom = -half_height + center_offset.y;
    float top = half_height + center_offset.y;
    
    return glm::ortho(left, right, bottom, top, -1000.0f, 1000.0f);
}

glm::vec2 ViewportManager::ScreenToWorld(const glm::vec2& screen_pos, const ScreenBounds& bounds, float zoom, const glm::vec2& center_offset) {
    // Convert screen coordinates (0,0 = top-left) to normalized coordinates (-1 to 1)
    glm::vec2 normalized;
    normalized.x = (screen_pos.x / bounds.width) * 2.0f - 1.0f;
    normalized.y = 1.0f - (screen_pos.y / bounds.height) * 2.0f;
    
    // Apply zoom and center offset
    float half_height = (bounds.height * 0.5f) / zoom;
    float half_width = (bounds.width * 0.5f) / zoom;
    
    glm::vec2 world_pos;
    world_pos.x = normalized.x * half_width + center_offset.x;
    world_pos.y = normalized.y * half_height + center_offset.y;
    
    return world_pos;
}

glm::vec2 ViewportManager::WorldToScreen(const glm::vec2& world_pos, const ScreenBounds& bounds, float zoom, const glm::vec2& center_offset) {
    // Apply zoom and center offset
    float half_height = (bounds.height * 0.5f) / zoom;
    float half_width = (bounds.width * 0.5f) / zoom;
    
    // Convert to normalized coordinates (-1 to 1)
    glm::vec2 normalized;
    normalized.x = (world_pos.x - center_offset.x) / half_width;
    normalized.y = (world_pos.y - center_offset.y) / half_height;
    
    // Convert to screen coordinates (0,0 = top-left)
    glm::vec2 screen_pos;
    screen_pos.x = (normalized.x + 1.0f) * 0.5f * bounds.width;
    screen_pos.y = (1.0f - normalized.y) * 0.5f * bounds.height;
    
    return screen_pos;
}

void ViewportManager::UpdateFromProject(const Project* project) {
    s_current_bounds = GetScreenBounds(project);
}

} // namespace Lupine
