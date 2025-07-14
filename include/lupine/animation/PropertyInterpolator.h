#pragma once

#include <functional>
#include <memory>
#include <variant>

namespace lupine {

/**
 * @brief Interpolation types for property animation
 */
enum class InterpolationType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Bounce,
    Elastic,
    Back,
    Custom
};

/**
 * @brief Property interpolator for smooth animation transitions
 */
class PropertyInterpolator {
public:
    using CustomEasingFunction = std::function<float(float)>;
    using PropertyValue = std::variant<float, int, bool>;

    PropertyInterpolator();
    ~PropertyInterpolator();

    /**
     * @brief Set the interpolation type
     * @param type The interpolation type to use
     */
    void SetInterpolationType(InterpolationType type);

    /**
     * @brief Set a custom easing function
     * @param function Custom easing function (takes normalized time [0,1], returns eased value [0,1])
     */
    void SetCustomEasing(CustomEasingFunction function);

    /**
     * @brief Interpolate between two values
     * @param start Starting value
     * @param end Ending value
     * @param t Normalized time [0,1]
     * @return Interpolated value
     */
    PropertyValue Interpolate(const PropertyValue& start, const PropertyValue& end, float t);

    /**
     * @brief Apply easing to normalized time
     * @param t Normalized time [0,1]
     * @return Eased time value
     */
    float ApplyEasing(float t);

private:
    InterpolationType m_interpolationType;
    CustomEasingFunction m_customEasing;

    // Built-in easing functions
    float LinearEasing(float t);
    float EaseInEasing(float t);
    float EaseOutEasing(float t);
    float EaseInOutEasing(float t);
    float BounceEasing(float t);
    float ElasticEasing(float t);
    float BackEasing(float t);
};

} // namespace lupine
