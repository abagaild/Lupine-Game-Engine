#pragma once

#include <glm/glm.hpp>

namespace Lupine {

class Project;

/**
 * @brief Manages viewport calculations and screen space bounds
 * 
 * ViewportManager handles the calculation of screen space bounds based on
 * project settings and provides utilities for converting between different
 * coordinate systems.
 */
class ViewportManager {
public:
    /**
     * @brief Screen space bounds information
     */
    struct ScreenBounds {
        float width;
        float height;
        float aspect_ratio;
        glm::vec2 center;
        glm::vec4 bounds; // left, top, right, bottom
    };

    /**
     * @brief Get screen bounds from project settings
     * @param project Project to get settings from (can be nullptr for defaults)
     * @return Screen bounds information
     */
    static ScreenBounds GetScreenBounds(const Project* project = nullptr);
    
    /**
     * @brief Get screen bounds with custom dimensions
     * @param width Screen width
     * @param height Screen height
     * @return Screen bounds information
     */
    static ScreenBounds GetScreenBounds(float width, float height);
    
    /**
     * @brief Get default screen bounds (1920x1080, 16:9)
     * @return Default screen bounds
     */
    static ScreenBounds GetDefaultScreenBounds();
    
    /**
     * @brief Create orthographic projection matrix for screen space
     * @param bounds Screen bounds to use
     * @return Orthographic projection matrix (0,0 = top-left)
     */
    static glm::mat4 CreateScreenSpaceProjection(const ScreenBounds& bounds);
    
    /**
     * @brief Create orthographic projection matrix for 2D world space
     * @param bounds Screen bounds to use
     * @param zoom Zoom level (1.0 = normal)
     * @param center_offset Offset from center
     * @return Orthographic projection matrix for 2D world
     */
    static glm::mat4 Create2DWorldProjection(const ScreenBounds& bounds, float zoom = 1.0f, const glm::vec2& center_offset = glm::vec2(0.0f));
    
    /**
     * @brief Convert screen coordinates to world coordinates
     * @param screen_pos Screen position (0,0 = top-left)
     * @param bounds Screen bounds
     * @param zoom Zoom level
     * @param center_offset Center offset
     * @return World position
     */
    static glm::vec2 ScreenToWorld(const glm::vec2& screen_pos, const ScreenBounds& bounds, float zoom = 1.0f, const glm::vec2& center_offset = glm::vec2(0.0f));
    
    /**
     * @brief Convert world coordinates to screen coordinates
     * @param world_pos World position
     * @param bounds Screen bounds
     * @param zoom Zoom level
     * @param center_offset Center offset
     * @return Screen position (0,0 = top-left)
     */
    static glm::vec2 WorldToScreen(const glm::vec2& world_pos, const ScreenBounds& bounds, float zoom = 1.0f, const glm::vec2& center_offset = glm::vec2(0.0f));
    
    /**
     * @brief Get the current active screen bounds
     * @return Current screen bounds
     */
    static const ScreenBounds& GetCurrentBounds() { return s_current_bounds; }
    
    /**
     * @brief Set the current active screen bounds
     * @param bounds New screen bounds
     */
    static void SetCurrentBounds(const ScreenBounds& bounds) { s_current_bounds = bounds; }
    
    /**
     * @brief Update current bounds from project
     * @param project Project to get settings from
     */
    static void UpdateFromProject(const Project* project);

private:
    static ScreenBounds s_current_bounds;
};

} // namespace Lupine
