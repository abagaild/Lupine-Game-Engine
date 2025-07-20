#include "lupine/animation/PropertySystem.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

// PropertyInterpolator implementation
EnhancedAnimationValue PropertyInterpolator::Interpolate(const EnhancedAnimationValue& a, const EnhancedAnimationValue& b, 
                                                       float t, InterpolationType interpolation) const {
    if (!a.IsValid() || !b.IsValid() || a.type != b.type) {
        return a; // Return first value if types don't match or invalid
    }
    
    // Clamp t to [0, 1]
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Apply easing
    float easedT = ApplyEasing(t, interpolation);
    
    switch (a.type) {
        case ExportVariableType::Float:
            return InterpolateFloat(a.GetValue<float>(), b.GetValue<float>(), easedT, interpolation);
        case ExportVariableType::Vec2:
            return InterpolateVec2(a.GetValue<glm::vec2>(), b.GetValue<glm::vec2>(), easedT, interpolation);
        case ExportVariableType::Vec3:
            return InterpolateVec3(a.GetValue<glm::vec3>(), b.GetValue<glm::vec3>(), easedT, interpolation);
        case ExportVariableType::Vec4:
            return InterpolateVec4(a.GetValue<glm::vec4>(), b.GetValue<glm::vec4>(), easedT, interpolation);
        case ExportVariableType::Bool:
            return InterpolateBool(a.GetValue<bool>(), b.GetValue<bool>(), easedT, interpolation);
        case ExportVariableType::Int:
            return InterpolateInt(a.GetValue<int>(), b.GetValue<int>(), easedT, interpolation);
        case ExportVariableType::String:
            return InterpolateString(a.GetValue<std::string>(), b.GetValue<std::string>(), easedT, interpolation);
        default:
            return a; // For non-interpolatable types, return the first value
    }
}

EnhancedAnimationValue PropertyInterpolator::InterpolateFloat(float a, float b, float t, InterpolationType /*interpolation*/) const {
    float result = a + (b - a) * t;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Float);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateVec2(const glm::vec2& a, const glm::vec2& b, float t, InterpolationType /*interpolation*/) const {
    glm::vec2 result = a + (b - a) * t;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Vec2);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateVec3(const glm::vec3& a, const glm::vec3& b, float t, InterpolationType /*interpolation*/) const {
    glm::vec3 result = a + (b - a) * t;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Vec3);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateVec4(const glm::vec4& a, const glm::vec4& b, float t, InterpolationType /*interpolation*/) const {
    glm::vec4 result = a + (b - a) * t;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Vec4);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateQuaternion(const glm::quat& a, const glm::quat& b, float t, InterpolationType /*interpolation*/) const {
    glm::quat result = glm::slerp(a, b, t);
    // Convert quaternion to vec4 for storage
    glm::vec4 quatAsVec4(result.x, result.y, result.z, result.w);
    return EnhancedAnimationValue(ExportValue(quatAsVec4), ExportVariableType::Vec4);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateBool(bool a, bool b, float t, InterpolationType /*interpolation*/) const {
    // For boolean values, use step interpolation at t = 0.5
    bool result = (t < 0.5f) ? a : b;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Bool);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateInt(int a, int b, float t, InterpolationType /*interpolation*/) const {
    int result = static_cast<int>(a + (b - a) * t);
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::Int);
}

EnhancedAnimationValue PropertyInterpolator::InterpolateString(const std::string& a, const std::string& b, float t, InterpolationType /*interpolation*/) const {
    // For strings, use step interpolation at t = 0.5
    std::string result = (t < 0.5f) ? a : b;
    return EnhancedAnimationValue(ExportValue(result), ExportVariableType::String);
}

float PropertyInterpolator::ApplyEasing(float t, InterpolationType interpolation) const {
    switch (interpolation) {
        case InterpolationType::Linear:
            return EaseLinear(t);
        case InterpolationType::Ease:
            return EaseInOutQuad(t);
        case InterpolationType::EaseIn:
            return EaseInQuad(t);
        case InterpolationType::EaseOut:
            return EaseOutQuad(t);
        case InterpolationType::EaseInOut:
            return EaseInOutQuad(t);
        default:
            return EaseLinear(t);
    }
}

// Easing function implementations
float PropertyInterpolator::EaseInSine(float t) const {
    return 1.0f - static_cast<float>(std::cos((t * M_PI) / 2.0f));
}

float PropertyInterpolator::EaseOutSine(float t) const {
    return static_cast<float>(std::sin((t * M_PI) / 2.0f));
}

float PropertyInterpolator::EaseInOutSine(float t) const {
    return static_cast<float>(-(std::cos(M_PI * t) - 1.0f) / 2.0f);
}

float PropertyInterpolator::EaseInQuad(float t) const {
    return t * t;
}

float PropertyInterpolator::EaseOutQuad(float t) const {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float PropertyInterpolator::EaseInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float PropertyInterpolator::EaseInCubic(float t) const {
    return t * t * t;
}

float PropertyInterpolator::EaseOutCubic(float t) const {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

float PropertyInterpolator::EaseInOutCubic(float t) const {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float PropertyInterpolator::EaseInQuart(float t) const {
    return t * t * t * t;
}

float PropertyInterpolator::EaseOutQuart(float t) const {
    return 1.0f - std::pow(1.0f - t, 4.0f);
}

float PropertyInterpolator::EaseInOutQuart(float t) const {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

float PropertyInterpolator::EaseInQuint(float t) const {
    return t * t * t * t * t;
}

float PropertyInterpolator::EaseOutQuint(float t) const {
    return 1.0f - std::pow(1.0f - t, 5.0f);
}

float PropertyInterpolator::EaseInOutQuint(float t) const {
    return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}

float PropertyInterpolator::EaseInExpo(float t) const {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}

float PropertyInterpolator::EaseOutExpo(float t) const {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

float PropertyInterpolator::EaseInOutExpo(float t) const {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

float PropertyInterpolator::EaseInCirc(float t) const {
    return 1.0f - std::sqrt(1.0f - std::pow(t, 2.0f));
}

float PropertyInterpolator::EaseOutCirc(float t) const {
    return std::sqrt(1.0f - std::pow(t - 1.0f, 2.0f));
}

float PropertyInterpolator::EaseInOutCirc(float t) const {
    return t < 0.5f ? (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f 
                    : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

float PropertyInterpolator::EaseInBack(float t) const {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

float PropertyInterpolator::EaseOutBack(float t) const {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * static_cast<float>(std::pow(t - 1.0f, 3.0f)) + c1 * static_cast<float>(std::pow(t - 1.0f, 2.0f));
}

float PropertyInterpolator::EaseInOutBack(float t) const {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;
    return t < 0.5f ? static_cast<float>((std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f)
                    : static_cast<float>((std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f);
}

float PropertyInterpolator::EaseInElastic(float t) const {
    const float c4 = (2.0f * M_PI) / 3.0f;
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.0f) * c4);
}

float PropertyInterpolator::EaseOutElastic(float t) const {
    const float c4 = (2.0f * M_PI) / 3.0f;
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin(t * c4) + 1.0f;
}

float PropertyInterpolator::EaseInOutElastic(float t) const {
    const float c5 = (2.0f * M_PI) / 4.5f;
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
                    : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

float PropertyInterpolator::EaseInBounce(float t) const {
    return 1.0f - EaseOutBounce(1.0f - t);
}

float PropertyInterpolator::EaseOutBounce(float t) const {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        return n1 * (t -= 1.5f / d1) * t + 0.75f;
    } else if (t < 2.5f / d1) {
        return n1 * (t -= 2.25f / d1) * t + 0.9375f;
    } else {
        return n1 * (t -= 2.625f / d1) * t + 0.984375f;
    }
}

float PropertyInterpolator::EaseInOutBounce(float t) const {
    return t < 0.5f ? (1.0f - EaseOutBounce(1.0f - 2.0f * t)) / 2.0f
                    : (1.0f + EaseOutBounce(2.0f * t - 1.0f)) / 2.0f;
}

} // namespace Lupine
