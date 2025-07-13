#pragma once

namespace Lupine {

/**
 * @brief Initialize the component registry with all available components
 * 
 * This function manually registers all components to ensure they are available
 * even if static initialization doesn't work properly.
 */
void InitializeComponentRegistry();

} // namespace Lupine
