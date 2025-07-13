#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace Lupine {

/**
 * @brief UUID wrapper class for unique identification of engine objects
 * 
 * This class provides a simple interface for generating and managing UUIDs
 * for nodes, components, scenes, and other engine objects.
 */
class UUID {
public:
    /**
     * @brief Default constructor - generates a new random UUID
     */
    UUID();
    
    /**
     * @brief Constructor from string representation
     * @param uuid_string String representation of UUID
     */
    explicit UUID(const std::string& uuid_string);
    
    /**
     * @brief Copy constructor
     */
    UUID(const UUID& other) = default;

    /**
     * @brief Assignment operator
     */
    UUID& operator=(const UUID& other) = default;

    /**
     * @brief Move constructor
     */
    UUID(UUID&& other) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    UUID& operator=(UUID&& other) noexcept = default;

    /**
     * @brief Destructor
     */
    ~UUID() = default;

    /**
     * @brief Get string representation of UUID
     * @return String representation
     */
    std::string ToString() const;
    
    /**
     * @brief Check if UUID is nil (empty)
     * @return True if UUID is nil
     */
    bool IsNil() const;
    
    /**
     * @brief Generate a new random UUID
     * @return New UUID instance
     */
    static UUID Generate();

    /**
     * @brief Create UUID from string representation
     * @param uuid_string String representation of UUID
     * @return UUID instance
     */
    static UUID FromString(const std::string& uuid_string);
    
    /**
     * @brief Create a nil UUID
     * @return Nil UUID instance
     */
    static UUID Nil();
    
    /**
     * @brief Equality operator
     */
    bool operator==(const UUID& other) const;
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const UUID& other) const;
    
    /**
     * @brief Less than operator (for use in containers)
     */
    bool operator<(const UUID& other) const;
    
    /**
     * @brief Hash function for use in unordered containers
     */
    struct Hash {
        std::size_t operator()(const UUID& uuid) const;
    };

private:
    std::string m_uuid_string;
    static std::random_device s_random_device;
    static std::mt19937 s_generator;

    /**
     * @brief Generate a new random UUID string
     */
    void GenerateUUID();
};

} // namespace Lupine

// Hash specialization for std::hash
namespace std {
    template<>
    struct hash<Lupine::UUID> {
        std::size_t operator()(const Lupine::UUID& uuid) const {
            return Lupine::UUID::Hash{}(uuid);
        }
    };
}
