#pragma once

#include <cstdint>

namespace Lupine {

/**
 * @brief Blend factors for blending operations
 */
enum class BlendFactor {
    Zero,                   // 0
    One,                    // 1
    SrcColor,              // Source color
    OneMinusSrcColor,      // 1 - Source color
    DstColor,              // Destination color
    OneMinusDstColor,      // 1 - Destination color
    SrcAlpha,              // Source alpha
    OneMinusSrcAlpha,      // 1 - Source alpha
    DstAlpha,              // Destination alpha
    OneMinusDstAlpha,      // 1 - Destination alpha
    ConstantColor,         // Constant color
    OneMinusConstantColor, // 1 - Constant color
    ConstantAlpha,         // Constant alpha
    OneMinusConstantAlpha  // 1 - Constant alpha
};

/**
 * @brief Blend equations for blending operations
 */
enum class BlendEquation {
    Add,                // Source + Destination
    Subtract,           // Source - Destination
    ReverseSubtract,    // Destination - Source
    Min,                // min(Source, Destination)
    Max                 // max(Source, Destination)
};

/**
 * @brief Depth comparison functions
 */
enum class DepthFunc {
    Never,      // Never pass
    Less,       // Pass if incoming < stored
    Equal,      // Pass if incoming == stored
    LessEqual,  // Pass if incoming <= stored
    Greater,    // Pass if incoming > stored
    NotEqual,   // Pass if incoming != stored
    GreaterEqual, // Pass if incoming >= stored
    Always      // Always pass
};

/**
 * @brief Face culling modes
 */
enum class CullFace {
    None,       // No culling
    Front,      // Cull front faces
    Back,       // Cull back faces
    FrontAndBack // Cull both front and back faces
};

/**
 * @brief Front face winding order
 */
enum class FrontFace {
    Clockwise,          // Clockwise winding
    CounterClockwise    // Counter-clockwise winding
};

/**
 * @brief Polygon fill modes
 */
enum class PolygonMode {
    Fill,       // Fill polygons
    Line,       // Draw polygon outlines
    Point       // Draw polygon vertices as points
};

/**
 * @brief Stencil operations
 */
enum class StencilOp {
    Keep,       // Keep current value
    Zero,       // Set to zero
    Replace,    // Replace with reference value
    Increment,  // Increment (clamp to max)
    IncrementWrap, // Increment (wrap to zero)
    Decrement,  // Decrement (clamp to zero)
    DecrementWrap, // Decrement (wrap to max)
    Invert      // Bitwise invert
};

/**
 * @brief Blend state configuration
 */
struct BlendState {
    bool enabled = false;
    BlendFactor src_color = BlendFactor::SrcAlpha;
    BlendFactor dst_color = BlendFactor::OneMinusSrcAlpha;
    BlendFactor src_alpha = BlendFactor::One;
    BlendFactor dst_alpha = BlendFactor::OneMinusSrcAlpha;
    BlendEquation color_equation = BlendEquation::Add;
    BlendEquation alpha_equation = BlendEquation::Add;
    float constant_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    // Color write mask (RGBA)
    bool color_write_mask[4] = {true, true, true, true};
    
    BlendState() = default;
    
    // Common blend state presets
    static BlendState Disabled() {
        BlendState state;
        state.enabled = false;
        return state;
    }
    
    static BlendState AlphaBlend() {
        BlendState state;
        state.enabled = true;
        state.src_color = BlendFactor::SrcAlpha;
        state.dst_color = BlendFactor::OneMinusSrcAlpha;
        state.src_alpha = BlendFactor::One;
        state.dst_alpha = BlendFactor::OneMinusSrcAlpha;
        return state;
    }
    
    static BlendState Additive() {
        BlendState state;
        state.enabled = true;
        state.src_color = BlendFactor::SrcAlpha;
        state.dst_color = BlendFactor::One;
        state.src_alpha = BlendFactor::One;
        state.dst_alpha = BlendFactor::One;
        return state;
    }
    
    static BlendState Multiply() {
        BlendState state;
        state.enabled = true;
        state.src_color = BlendFactor::DstColor;
        state.dst_color = BlendFactor::Zero;
        state.src_alpha = BlendFactor::DstAlpha;
        state.dst_alpha = BlendFactor::Zero;
        return state;
    }
};

/**
 * @brief Depth state configuration
 */
struct DepthState {
    bool test_enabled = true;
    bool write_enabled = true;
    DepthFunc function = DepthFunc::Less;
    float near_plane = 0.0f;
    float far_plane = 1.0f;
    
    DepthState() = default;
    
    static DepthState Disabled() {
        DepthState state;
        state.test_enabled = false;
        state.write_enabled = false;
        return state;
    }
    
    static DepthState ReadOnly() {
        DepthState state;
        state.test_enabled = true;
        state.write_enabled = false;
        return state;
    }
    
    static DepthState Default() {
        return DepthState();
    }
};

/**
 * @brief Stencil state configuration
 */
struct StencilState {
    bool enabled = false;
    DepthFunc function = DepthFunc::Always;
    int reference_value = 0;
    uint32_t read_mask = 0xFFFFFFFF;
    uint32_t write_mask = 0xFFFFFFFF;
    StencilOp stencil_fail = StencilOp::Keep;
    StencilOp depth_fail = StencilOp::Keep;
    StencilOp pass = StencilOp::Keep;
    
    StencilState() = default;
    
    static StencilState Disabled() {
        return StencilState();
    }
};

/**
 * @brief Rasterizer state configuration
 */
struct RasterizerState {
    CullFace cull_mode = CullFace::Back;
    FrontFace front_face = FrontFace::CounterClockwise;
    PolygonMode fill_mode = PolygonMode::Fill;
    bool scissor_test = false;
    bool depth_clamp = false;
    float line_width = 1.0f;
    float point_size = 1.0f;
    
    // Polygon offset
    bool polygon_offset_fill = false;
    bool polygon_offset_line = false;
    bool polygon_offset_point = false;
    float polygon_offset_factor = 0.0f;
    float polygon_offset_units = 0.0f;
    
    RasterizerState() = default;
    
    static RasterizerState Default() {
        return RasterizerState();
    }
    
    static RasterizerState NoCulling() {
        RasterizerState state;
        state.cull_mode = CullFace::None;
        return state;
    }
    
    static RasterizerState Wireframe() {
        RasterizerState state;
        state.fill_mode = PolygonMode::Line;
        state.cull_mode = CullFace::None;
        return state;
    }
};

/**
 * @brief Complete render state configuration
 */
struct RenderState {
    BlendState blend;
    DepthState depth;
    StencilState stencil;
    RasterizerState rasterizer;
    
    RenderState() = default;
    
    // Common render state presets
    static RenderState Default() {
        RenderState state;
        state.blend = BlendState::Disabled();
        state.depth = DepthState::Default();
        state.stencil = StencilState::Disabled();
        state.rasterizer = RasterizerState::Default();
        return state;
    }
    
    static RenderState Transparent() {
        RenderState state;
        state.blend = BlendState::AlphaBlend();
        state.depth = DepthState::ReadOnly();
        state.stencil = StencilState::Disabled();
        state.rasterizer = RasterizerState::Default();
        return state;
    }
    
    static RenderState Opaque() {
        RenderState state;
        state.blend = BlendState::Disabled();
        state.depth = DepthState::Default();
        state.stencil = StencilState::Disabled();
        state.rasterizer = RasterizerState::Default();
        return state;
    }
    
    static RenderState UI() {
        RenderState state;
        state.blend = BlendState::AlphaBlend();
        state.depth = DepthState::Disabled();
        state.stencil = StencilState::Disabled();
        state.rasterizer = RasterizerState::NoCulling();
        return state;
    }
};

} // namespace Lupine
