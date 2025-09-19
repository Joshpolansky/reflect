#pragma once

/**
 * @file reflection.hpp
 * @brief Modern C++20 ref// Export path navigation functions from core module
using reflection::get_field;
using reflection::set_field;
using reflection::get_field_enhanced;  
using reflection::set_field_enhanced;
using reflection::is_valid_path;
using reflection::get_all_paths;
using reflection::parse_path;
using reflection::parse_path_enhanced;ibrary with JSON serialization and path navigation
 * @version 2.0.0
 * 
 * This header provides the complete reflection API including:
 * - JSON serialization/deserialization (reflection::json namespace)
 * - Path navigation with dot notation and array access (reflection namespace)
 * - Schema generation and type introspection
 * - Automatic field name detection using Boost.PFR
 * 
 * Key Features:
 * - Zero-boilerplate JSON serialization
 * - Automatic field names with C++20 + Boost.PFR
 * - Advanced path navigation: "field.items[0].name"
 * - Type-safe operations with compile-time validation
 * - Extensible architecture for multiple serialization formats
 * 
 * Architecture:
 * - reflection::json - JSON-specific operations (to_json, from_json, schema)
 * - reflection:: - General reflection and path navigation
 * 
 * Usage Examples:
 * 
 * @code{.cpp}
 * // JSON serialization
 * using namespace reflection;
 * auto j = json::to_json(person);
 * auto loaded = json::from_json<Person>(j);
 * 
 * // Path navigation  
 * auto name = get_field(person, "name");
 * auto street = get_field_enhanced(person, "address.streets[0].name");
 * set_field_enhanced(person, "contacts[1].phone", "555-0123");
 * 
 * // Schema generation
 * auto schema = json::get_schema<Person>();
 * @endcode
 */

// Include JSON module
#include "reflection/json.hpp"

// Include core reflection and path navigation
#include "reflection/core.hpp"

namespace reflection {

/**
 * @brief Library version information
 */
struct version {
    static constexpr int major = 2;
    static constexpr int minor = 0;
    static constexpr int patch = 0;
    static constexpr const char* string = "2.0.0";
};

/**
 * @brief Get version string
 * @return Version string in format "major.minor.patch"
 */
constexpr const char* get_version() {
    return version::string;
}

// Path navigation functions are already available in the reflection namespace
// from the core.hpp include - no need to re-export them

} // namespace reflection

/**
 * @brief Convenience macro for defining custom field names (optional)
 * @param StructType The struct type
 * @param ... Field names as string literals
 * 
 * Example:
 * @code{.cpp}
 * struct Person { std::string name; int age; };
 * DEFINE_FIELD_NAMES(Person, "full_name", "years_old");
 * @endcode
 * 
 * Note: With C++20 and Boost.PFR, this is often unnecessary as field names
 * can be automatically detected from source code.
 */
#define DEFINE_FIELD_NAMES(StructType, ...) \
    template<> \
    std::vector<std::string> reflection::json::detail::get_field_names<StructType>() { \
        return {__VA_ARGS__}; \
    }