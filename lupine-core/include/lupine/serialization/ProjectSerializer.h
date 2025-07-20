#pragma once

#include "lupine/core/Project.h"
#include <string>
#include <memory>

namespace Lupine {

/**
 * @brief Project serialization and deserialization
 */
class ProjectSerializer {
public:
    /**
     * @brief Serialize project to file
     * @param project Project to serialize
     * @param file_path Output file path
     * @return True if successful
     */
    static bool SerializeToFile(const Project* project, const std::string& file_path);

    /**
     * @brief Deserialize project from file
     * @param file_path Input file path
     * @return Loaded project or nullptr if failed
     */
    static std::unique_ptr<Project> DeserializeFromFile(const std::string& file_path);

    /**
     * @brief Serialize project to string
     * @param project Project to serialize
     * @return Serialized string
     */
    static std::string SerializeToString(const Project* project);

    /**
     * @brief Deserialize project from string
     * @param content Serialized content
     * @return Loaded project or nullptr if failed
     */
    static std::unique_ptr<Project> DeserializeFromString(const std::string& content);
};

} // namespace Lupine
