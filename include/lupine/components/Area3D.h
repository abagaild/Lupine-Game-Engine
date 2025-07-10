#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>
#include <functional>

namespace Lupine {

class Node3D;

/**
 * @brief 3D Area Component
 * 
 * Defines trigger areas for 3D physics detection.
 * Areas can detect when other physics bodies enter, stay, or exit their bounds.
 */
class Area3D : public Component {
public:
    /**
     * @brief Area event callbacks
     */
    using AreaCallback = std::function<void(Node3D* other_node)>;
    
    /**
     * @brief Constructor
     */
    Area3D();
    
    /**
     * @brief Destructor
     */
    virtual ~Area3D();
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Area3D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Physics"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnDestroy() override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;

private:
    // TODO: Implement Area3D functionality similar to Area2D but for 3D
};

} // namespace Lupine
