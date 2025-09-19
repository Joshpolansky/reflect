#pragma once

#define REFLECT_JSON_VERSION_MAJOR 1
#define REFLECT_JSON_VERSION_MINOR 0
#define REFLECT_JSON_VERSION_PATCH 0
#define REFLECT_JSON_VERSION_STRING "1.0.0"

#define REFLECT_JSON_VERSION \
    (REFLECT_JSON_VERSION_MAJOR * 10000 + \
     REFLECT_JSON_VERSION_MINOR * 100 + \
     REFLECT_JSON_VERSION_PATCH)

namespace utilities {
namespace reflect_json {

/**
 * @brief Get the version string of the reflect-json library
 * @return Version string in format "major.minor.patch"
 */
constexpr const char* version_string() noexcept {
    return REFLECT_JSON_VERSION_STRING;
}

/**
 * @brief Get the major version number
 */
constexpr int version_major() noexcept {
    return REFLECT_JSON_VERSION_MAJOR;
}

/**
 * @brief Get the minor version number
 */
constexpr int version_minor() noexcept {
    return REFLECT_JSON_VERSION_MINOR;
}

/**
 * @brief Get the patch version number
 */
constexpr int version_patch() noexcept {
    return REFLECT_JSON_VERSION_PATCH;
}

/**
 * @brief Get the version as a single integer
 * @return Version as integer (major * 10000 + minor * 100 + patch)
 */
constexpr int version() noexcept {
    return REFLECT_JSON_VERSION;
}

} // namespace reflect_json
} // namespace utilities