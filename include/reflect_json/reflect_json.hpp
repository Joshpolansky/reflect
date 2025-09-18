#pragma once

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>  // For field name support
#include <nlohmann/json.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <string_view>
#include <sstream>
#include <algorithm>

namespace reflect_json {

/**
 * @brief JSON Reflection Utility using Boost.PFR
 * 
 * This utility provides automatic JSON serialization/deserialization for structs
 * using Boost.PFR (Precise and Flat Reflection) without requiring manual field mapping.
 */
class reflection {
private:
    // Helper function to recursively serialize fields
    template<typename T>
    static nlohmann::json serialize_field(const T& field) {
        if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
            return field;
        } else if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
            // Recursively serialize nested aggregates
            return to_json(field);
        } else {
            return field; // Let nlohmann::json handle it
        }
    }
    
    // Helper function to recursively deserialize fields  
    template<typename T>
    static T deserialize_field(const nlohmann::json& j) {
        if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
            return j.get<T>();
        } else if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
            // Recursively deserialize nested aggregates
            return from_json<T>(j);
        } else {
            return j.get<T>(); // Let nlohmann::json handle it
        }
    }

    template<class T, std::size_t... I>
    static nlohmann::json to_json_impl(const T& t, std::index_sequence<I...>) {
        // Ensure T is a proper aggregate type, not an array
        static_assert(!std::is_array_v<T>, "Cannot reflect array types, use std::array instead");
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate");
        
        nlohmann::json j;
        
        // Try to use PFR's native field names first, fallback to custom names
        if constexpr (has_pfr_names<T>()) {
            // Use PFR's native field name support
            auto names = boost::pfr::names_as_array<T>();
            ((j[std::string(names[I])] = serialize_field(boost::pfr::get<I>(t))), ...);
        } else {
            // Fallback to custom field names or indices
            auto field_names = get_field_names<T>();
            if (field_names.size() == sizeof...(I)) {
                ((j[field_names[I]] = serialize_field(boost::pfr::get<I>(t))), ...);
            } else {
                // Fallback to field indices if no names available
                ((j["field_" + std::to_string(I)] = serialize_field(boost::pfr::get<I>(t))), ...);
            }
        }
        
        return j;
    }

    template<class T, std::size_t... I>
    static void from_json_impl(const nlohmann::json& j, T& t, std::index_sequence<I...>) {
        // Ensure T is a proper aggregate type, not an array
        static_assert(!std::is_array_v<T>, "Cannot reflect array types, use std::array instead");
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate");
        
        if constexpr (has_pfr_names<T>()) {
            // Use PFR's native field names
            auto names = boost::pfr::names_as_array<T>();
            ((boost::pfr::get<I>(t) = deserialize_field<std::decay_t<decltype(boost::pfr::get<I>(t))>>(j.at(std::string(names[I])))), ...);
        } else {
            // Fallback to custom field names or indices
            auto field_names = get_field_names<T>();
            if (field_names.size() == sizeof...(I)) {
                ((boost::pfr::get<I>(t) = deserialize_field<std::decay_t<decltype(boost::pfr::get<I>(t))>>(j.at(field_names[I]))), ...);
            } else {
                // Fallback to field indices
                ((boost::pfr::get<I>(t) = deserialize_field<std::decay_t<decltype(boost::pfr::get<I>(t))>>(j.at("field_" + std::to_string(I)))), ...);
            }
        }
    }

    // Check if PFR can provide field names (requires C++20 and support)
    template<typename T>
    static constexpr bool has_pfr_names() {
        #if __cplusplus >= 202002L
            return true;  // PFR names are available in C++20
        #else
            return false;
        #endif
    }

    template<typename T>
    static std::vector<std::string> get_field_names() {
        // This is where you can customize field names for specific types
        // PFR can now extract actual field names from source code automatically!
        // This method is now primarily for backwards compatibility or custom overrides
        // Options:
        // 1. Return empty vector to use PFR's native names or default naming
        // 2. Use DEFINE_FIELD_NAMES macro for custom names  
        // 3. Override this method for specific types
        return {};
    }

    /**
     * @brief Get field names using PFR's native support
     * @tparam T Struct type
     * @return Vector of field names
     */
    template<typename T>
    static std::vector<std::string> get_pfr_field_names() {
        std::vector<std::string> names;
        if constexpr (has_pfr_names<T>()) {
            auto pfr_names = boost::pfr::names_as_array<T>();
            names.reserve(pfr_names.size());
            for (const auto& name : pfr_names) {
                names.emplace_back(name);
            }
        }
        return names;
    }

    /**
     * @brief Get field information that PFR can provide
     * @tparam T Struct type
     * @return JSON object with field count, types, and indices
     */
    template<typename T>
    static nlohmann::json get_field_info() {
        nlohmann::json info;
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        
        info["field_count"] = field_count;
        info["fields"] = nlohmann::json::array();
        
        get_field_info_impl<T>(info, std::make_index_sequence<field_count>{});
        
        return info;
    }

private:
    template<typename T, std::size_t... I>
    static void get_field_info_impl(nlohmann::json& info, std::index_sequence<I...>) {
        // Get field names using the same logic as JSON serialization
        if constexpr (has_pfr_names<T>()) {
            auto names = boost::pfr::names_as_array<T>();
            ((info["fields"].push_back({
                {"index", I},
                {"name", std::string(names[I])},
                {"type", typeid(boost::pfr::tuple_element_t<I, T>).name()}
            })), ...);
        } else {
            auto field_names = get_field_names<T>();
            if (field_names.size() == sizeof...(I)) {
                ((info["fields"].push_back({
                    {"index", I},
                    {"name", field_names[I]},
                    {"type", typeid(boost::pfr::tuple_element_t<I, T>).name()}
                })), ...);
            } else {
                ((info["fields"].push_back({
                    {"index", I},
                    {"name", "field_" + std::to_string(I)},
                    {"type", typeid(boost::pfr::tuple_element_t<I, T>).name()}
                })), ...);
            }
        }
    }

public:
    /**
     * @brief Convert a struct to JSON
     * @tparam T Struct type (must be aggregate/plain old data)
     * @param obj Object to serialize
     * @return JSON representation
     */
    template<typename T>
    static nlohmann::json to_json(const T& obj) {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
        return to_json_impl(obj, std::make_index_sequence<fields_count>{});
    }

    /**
     * @brief Create struct from JSON
     * @tparam T Struct type (must be aggregate/plain old data)
     * @param j JSON object
     * @return Deserialized object
     */
    template<typename T>
    static T from_json(const nlohmann::json& j) {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        T obj{};
        from_json(j, obj);
        return obj;
    }

    /**
     * @brief Populate struct from JSON (in-place)
     * @tparam T Struct type
     * @param j JSON object
     * @param obj Object to populate
     */
    template<typename T>
    static void from_json(const nlohmann::json& j, T& obj) {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
        from_json_impl(j, obj, std::make_index_sequence<fields_count>{});
    }

    /**
     * @brief Get field names and types as JSON schema
     * @tparam T Struct type
     * @return JSON schema describing the struct
     */
    template<typename T>
    static nlohmann::json get_schema() {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        nlohmann::json schema;
        schema["type"] = "object";
        schema["properties"] = nlohmann::json::object();
        
        constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
        get_schema_impl<T>(schema, std::make_index_sequence<fields_count>{});
        
        return schema;
    }

    /**
     * @brief Get detailed field information that PFR can provide
     * @tparam T Struct type  
     * @return JSON with field count, types, and reflection capabilities
     */
    template<typename T>
    static nlohmann::json get_reflection_info() {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        
        nlohmann::json info;
        info["struct_name"] = typeid(T).name();
        info["is_aggregate"] = std::is_aggregate_v<T>;
        info["field_info"] = get_field_info<T>();
        
        // Check PFR native field name support
        info["pfr_names_enabled"] = has_pfr_names<T>();
        
        if constexpr (has_pfr_names<T>()) {
            auto pfr_names = get_pfr_field_names<T>();
            info["pfr_field_names"] = pfr_names;
        }
        
        // Check if custom field names are defined
        auto field_names = get_field_names<T>();
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        
        info["has_custom_field_names"] = (field_names.size() == field_count);
        if (info["has_custom_field_names"]) {
            info["custom_field_names"] = field_names;
        }
        
        // Show which names will be used for JSON
        if constexpr (has_pfr_names<T>()) {
            info["json_field_names"] = get_pfr_field_names<T>();
            info["name_source"] = "PFR_NATIVE";
        } else if (field_names.size() == field_count) {
            info["json_field_names"] = field_names;
            info["name_source"] = "CUSTOM_DEFINED";
        } else {
            std::vector<std::string> default_names;
            for (size_t i = 0; i < field_count; ++i) {
                default_names.push_back("field_" + std::to_string(i));
            }
            info["json_field_names"] = default_names;
            info["name_source"] = "DEFAULT_INDICES";
        }
        
        return info;
    }

    /**
     * @brief Iterate over fields with their names
     * @tparam T Struct type
     * @tparam F Function type
     * @param obj Object to iterate over
     * @param func Function to call for each field
     */
    template<typename T, typename F>
    static void for_each_field_with_name(const T& obj, F&& func) {
        static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
        
        if constexpr (has_pfr_names<T>()) {
            // Use PFR's native for_each_field_with_name if available
            boost::pfr::for_each_field_with_name(obj, std::forward<F>(func));
        } else {
            // Fallback implementation
            auto field_names = get_field_names<T>();
            constexpr auto field_count = boost::pfr::tuple_size_v<T>;
            
            if (field_names.size() == field_count) {
                for_each_field_with_custom_names(obj, field_names, std::forward<F>(func), std::make_index_sequence<field_count>{});
            } else {
                for_each_field_with_default_names(obj, std::forward<F>(func), std::make_index_sequence<field_count>{});
            }
        }
    }

private:
    template<typename T, typename F, std::size_t... I>
    static void for_each_field_with_custom_names(const T& obj, const std::vector<std::string>& names, F&& func, std::index_sequence<I...>) {
        ((func(names[I], boost::pfr::get<I>(obj))), ...);
    }

    template<typename T, typename F, std::size_t... I>
    static void for_each_field_with_default_names(const T& obj, F&& func, std::index_sequence<I...>) {
        ((func("field_" + std::to_string(I), boost::pfr::get<I>(obj))), ...);
    }

private:
    template<typename T, std::size_t... I>
    static void get_schema_impl(nlohmann::json& schema, std::index_sequence<I...>) {
        // Use the same field naming logic as JSON serialization for consistency
        if constexpr (has_pfr_names<T>()) {
            // Use PFR's native field names
            auto names = boost::pfr::names_as_array<T>();
            ((schema["properties"][std::string(names[I])] = get_type_info<std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))>>()), ...);
        } else {
            // Fallback to custom field names or indices
            auto field_names = get_field_names<T>();
            if (field_names.size() == sizeof...(I)) {
                ((schema["properties"][field_names[I]] = get_type_info<std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))>>()), ...);
            } else {
                // Final fallback to field indices if no names available
                ((schema["properties"]["field_" + std::to_string(I)] = get_type_info<std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))>>()), ...);
            }
        }
    }

    template<typename T>
    static nlohmann::json get_type_info() {
        nlohmann::json type_info;
        
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>) {
            type_info["type"] = "integer";
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            type_info["type"] = "number";
        } else if constexpr (std::is_same_v<T, std::string>) {
            type_info["type"] = "string";
        } else if constexpr (std::is_same_v<T, bool>) {
            type_info["type"] = "boolean";
        } else {
            type_info["type"] = "object";
        }
        
        return type_info;
    }

public:
    // =============================================================================
    // PATH NAVIGATION API (Phase 1)
    // =============================================================================
    
    /**
     * @brief Parse a dot notation path into individual field names
     * @param path Dot notation path (e.g., "field.nested.value")
     * @return Vector of field names
     */
    static std::vector<std::string> parse_path(const std::string& path) {
        std::vector<std::string> parts;
        if (path.empty()) return parts;
        
        std::stringstream ss(path);
        std::string part;
        
        while (std::getline(ss, part, '.')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        return parts;
    }

    /**
     * @brief Get field index by name for a struct type
     * @tparam T Struct type
     * @param field_name Field name to look up
     * @return Optional field index if found
     */
    template<typename T>
    static std::optional<std::size_t> get_field_index(const std::string& field_name) {
        // Only aggregate types have fields
        if constexpr (!std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || std::is_array_v<T>) {
            return std::nullopt; // Primitive types have no fields
        } else {
            constexpr auto field_count = boost::pfr::tuple_size_v<T>;
            
            if constexpr (has_pfr_names<T>()) {
                auto names = boost::pfr::names_as_array<T>();
                for (std::size_t i = 0; i < names.size(); ++i) {
                    if (std::string(names[i]) == field_name) {
                        return i;
                    }
                }
            } else {
                auto field_names = get_field_names<T>();
                if (field_names.size() == field_count) {
                    for (std::size_t i = 0; i < field_names.size(); ++i) {
                        if (field_names[i] == field_name) {
                            return i;
                        }
                    }
                } else {
                    // Check if it's a generic field name like "field_0"
                    if (field_name.substr(0, 6) == "field_") {
                        try {
                            std::size_t index = std::stoull(field_name.substr(6));
                            if (index < field_count) {
                                return index;
                            }
                        } catch (...) {
                            // Invalid field name format
                        }
                    }
                }
            }
            return std::nullopt;
        }
    }

    /**
     * @brief Get field value by dot notation path
     * @tparam T Struct type
     * @param obj Object to get field from
     * @param path Dot notation path (e.g., "field.nested.value")
     * @return Optional JSON value if path exists
     */
    template<typename T>
    static std::optional<nlohmann::json> get_field(const T& obj, const std::string& path) {
        auto path_parts = parse_path(path);
        if (path_parts.empty()) {
            return std::nullopt;
        }
        
        return get_field_recursive(obj, path_parts, 0);
    }

    /**
     * @brief Set field value by dot notation path
     * @tparam T Struct type
     * @param obj Object to set field in
     * @param path Dot notation path (e.g., "field.nested.value")
     * @param value JSON value to set
     * @return True if successfully set, false if path invalid or type mismatch
     */
    template<typename T>
    static bool set_field(T& obj, const std::string& path, const nlohmann::json& value) {
        auto path_parts = parse_path(path);
        if (path_parts.empty()) {
            return false;
        }
        
        return set_field_recursive(obj, path_parts, value, 0);
    }

    /**
     * @brief Check if a dot notation path is valid for a struct type
     * @tparam T Struct type
     * @param path Dot notation path to validate
     * @return True if path is valid
     */
    template<typename T>
    static bool is_valid_path(const std::string& path) {
        auto path_parts = parse_path(path);
        if (path_parts.empty()) {
            return false;
        }
        
        return validate_path_recursive<T>(path_parts, 0);
    }

    /**
     * @brief Get all possible dot notation paths for a struct type
     * @tparam T Struct type
     * @param prefix Current path prefix (for internal recursion)
     * @return Vector of all valid paths
     */
    template<typename T>
    static std::vector<std::string> get_all_paths(const std::string& prefix = "") {
        std::vector<std::string> paths;
        collect_all_paths<T>(paths, prefix);
        return paths;
    }

private:
    // Helper function for recursive path navigation (getter)
    template<typename T>
    static std::optional<nlohmann::json> get_field_recursive(const T& obj, const std::vector<std::string>& path_parts, std::size_t depth) {
        if (depth >= path_parts.size()) {
            // Base case: we've reached the end, return the current object as JSON
            if constexpr (std::is_aggregate_v<T> && !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !std::is_array_v<T>) {
                return to_json(obj);
            } else {
                // For primitive types, use nlohmann::json constructor directly
                return nlohmann::json(obj);
            }
        }
        
        // Only aggregate types can have fields accessed via path navigation
        if constexpr (!std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || std::is_array_v<T>) {
            return std::nullopt; // Cannot navigate further into primitive types
        } else {
            const std::string& field_name = path_parts[depth];
            auto field_index = get_field_index<T>(field_name);
            
            if (!field_index) {
                return std::nullopt; // Field not found
            }
            
            constexpr auto field_count = boost::pfr::tuple_size_v<T>;
            return get_field_at_index(obj, *field_index, path_parts, depth + 1, std::make_index_sequence<field_count>{});
        }
    }

    // Helper to get field at specific index and continue recursion
    template<typename T, std::size_t... I>
    static std::optional<nlohmann::json> get_field_at_index(const T& obj, std::size_t target_index, 
                                                           const std::vector<std::string>& path_parts, std::size_t depth,
                                                           std::index_sequence<I...>) {
        std::optional<nlohmann::json> result;
        
        ((I == target_index ? 
            (result = get_field_recursive(boost::pfr::get<I>(obj), path_parts, depth)) : 
            false), ...);
            
        return result;
    }

    // Helper function for recursive path navigation (setter)
    template<typename T>
    static bool set_field_recursive(T& obj, const std::vector<std::string>& path_parts, const nlohmann::json& value, std::size_t depth) {
        if (depth >= path_parts.size()) {
            return false; // Invalid path
        }
        
        // Only aggregate types can have fields accessed via path navigation
        if constexpr (!std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || std::is_array_v<T>) {
            return false; // Cannot navigate into primitive types
        } else {
            const std::string& field_name = path_parts[depth];
            auto field_index = get_field_index<T>(field_name);
            
            if (!field_index) {
                return false; // Field not found
            }
            
            constexpr auto field_count = boost::pfr::tuple_size_v<T>;
            
            if (depth == path_parts.size() - 1) {
                // Base case: this is the final field to set
                return set_field_at_index(obj, *field_index, value, std::make_index_sequence<field_count>{});
            } else {
                // Recursive case: navigate deeper
                return set_field_at_index_recursive(obj, *field_index, path_parts, value, depth + 1, std::make_index_sequence<field_count>{});
            }
        }
    }

    // Helper to set field at specific index (final field) - simplified approach
    template<typename T, std::size_t... I>
    static bool set_field_at_index(T& obj, std::size_t target_index, const nlohmann::json& value, std::index_sequence<I...>) {
        constexpr auto field_count = sizeof...(I);
        
        // Simple switch approach for small number of fields
        if constexpr (field_count >= 1) {
            if (target_index == 0) {
                return try_set_field(boost::pfr::get<0>(obj), value);
            }
        }
        if constexpr (field_count >= 2) {
            if (target_index == 1) {
                return try_set_field(boost::pfr::get<1>(obj), value);
            }
        }
        if constexpr (field_count >= 3) {
            if (target_index == 2) {
                return try_set_field(boost::pfr::get<2>(obj), value);
            }
        }
        if constexpr (field_count >= 4) {
            if (target_index == 3) {
                return try_set_field(boost::pfr::get<3>(obj), value);
            }
        }
        if constexpr (field_count >= 5) {
            if (target_index == 4) {
                return try_set_field(boost::pfr::get<4>(obj), value);
            }
        }
        if constexpr (field_count >= 6) {
            if (target_index == 5) {
                return try_set_field(boost::pfr::get<5>(obj), value);
            }
        }
        if constexpr (field_count >= 7) {
            if (target_index == 6) {
                return try_set_field(boost::pfr::get<6>(obj), value);
            }
        }
        if constexpr (field_count >= 8) {
            if (target_index == 7) {
                return try_set_field(boost::pfr::get<7>(obj), value);
            }
        }
        
        return false; // Index out of range
    }

    // Helper to set field at specific index and continue recursion
    template<typename T, std::size_t... I>
    static bool set_field_at_index_recursive(T& obj, std::size_t target_index, const std::vector<std::string>& path_parts,
                                            const nlohmann::json& value, std::size_t depth, std::index_sequence<I...>) {
        bool success = false;
        
        ((I == target_index ? 
            (success = set_field_recursive(boost::pfr::get<I>(obj), path_parts, value, depth)) : 
            false), ...);
            
        return success;
    }

    // Helper to safely set a field value with type conversion
    template<typename FieldType>
    static bool try_set_field(FieldType& field, const nlohmann::json& value) {
        try {
            if constexpr (std::is_same_v<FieldType, std::string>) {
                if (value.is_string()) {
                    field = value.get<std::string>();
                    return true;
                } else {
                    // Convert other types to string
                    field = value.dump();
                    return true;
                }
            } else if constexpr (std::is_same_v<FieldType, bool>) {
                if (value.is_boolean()) {
                    field = value.get<bool>();
                    return true;
                } else if (value.is_string()) {
                    std::string str = value.get<std::string>();
                    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                    if (str == "true" || str == "1" || str == "yes") {
                        field = true;
                        return true;
                    } else if (str == "false" || str == "0" || str == "no") {
                        field = false;
                        return true;
                    }
                    return false;
                } else if (value.is_number()) {
                    field = value.get<int>() != 0;
                    return true;
                }
                return false;
            } else if constexpr (std::is_arithmetic_v<FieldType>) {
                if (value.is_number()) {
                    field = value.get<FieldType>();
                    return true;
                } else if (value.is_string()) {
                    // Try to parse string as number
                    try {
                        if constexpr (std::is_integral_v<FieldType>) {
                            field = static_cast<FieldType>(std::stoll(value.get<std::string>()));
                        } else {
                            field = static_cast<FieldType>(std::stod(value.get<std::string>()));
                        }
                        return true;
                    } catch (...) {
                        return false;
                    }
                }
            } else if constexpr (std::is_aggregate_v<FieldType> && !std::is_array_v<FieldType>) {
                // For nested structs, deserialize from JSON
                if (value.is_object()) {
                    field = from_json<FieldType>(value);
                    return true;
                }
            } else {
                // Fallback: let nlohmann::json handle it
                field = value.get<FieldType>();
                return true;
            }
        } catch (const std::exception& e) {
            return false;
        }
        return false;
    }

    // Helper for path validation
    template<typename T>
    static bool validate_path_recursive(const std::vector<std::string>& path_parts, std::size_t depth) {
        if (depth >= path_parts.size()) {
            return true; // Reached the end successfully
        }
        
        const std::string& field_name = path_parts[depth];
        auto field_index = get_field_index<T>(field_name);
        
        if (!field_index) {
            return false; // Field not found
        }
        
        if (depth == path_parts.size() - 1) {
            return true; // This is the final field and it exists
        }
        
        // Need to validate the next level - check the field type
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        return validate_path_at_index<T>(*field_index, path_parts, depth + 1, std::make_index_sequence<field_count>{});
    }

    // Helper to validate path at specific field index
    template<typename T, std::size_t... I>
    static bool validate_path_at_index(std::size_t target_index, const std::vector<std::string>& path_parts, std::size_t depth, std::index_sequence<I...>) {
        bool result = false;
        
        ((I == target_index ? 
            (result = validate_field_type_for_path<std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))>>(path_parts, depth)) : 
            false), ...);
            
        return result;
    }

    // Helper to validate if a field type can continue the path
    template<typename FieldType>
    static bool validate_field_type_for_path(const std::vector<std::string>& path_parts, std::size_t depth) {
        if constexpr (std::is_aggregate_v<FieldType> && !std::is_array_v<FieldType>) {
            // This field is a struct, so we can continue the path
            return validate_path_recursive<FieldType>(path_parts, depth);
        } else {
            // This field is a primitive type, so the path should end here
            return depth >= path_parts.size();
        }
    }

    // Helper to collect all paths for a struct type
    template<typename T>
    static void collect_all_paths(std::vector<std::string>& paths, const std::string& prefix) {
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        collect_paths_for_fields<T>(paths, prefix, std::make_index_sequence<field_count>{});
    }

    // Helper to collect paths for all fields
    template<typename T, std::size_t... I>
    static void collect_paths_for_fields(std::vector<std::string>& paths, const std::string& prefix, std::index_sequence<I...>) {
        ((collect_path_for_field<T, I>(paths, prefix)), ...);
    }

    // Helper to collect path for a specific field
    template<typename T, std::size_t I>
    static void collect_path_for_field(std::vector<std::string>& paths, const std::string& prefix) {
        std::string field_name;
        
        if constexpr (has_pfr_names<T>()) {
            auto names = boost::pfr::names_as_array<T>();
            field_name = std::string(names[I]);
        } else {
            auto field_names = get_field_names<T>();
            if (field_names.size() > I) {
                field_name = field_names[I];
            } else {
                field_name = "field_" + std::to_string(I);
            }
        }
        
        std::string full_path = prefix.empty() ? field_name : prefix + "." + field_name;
        paths.push_back(full_path);
        
        // If this field is a nested struct, recursively collect its paths
        using FieldType = std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))>;
        if constexpr (std::is_aggregate_v<FieldType> && !std::is_array_v<FieldType>) {
            collect_all_paths<FieldType>(paths, full_path);
        }
    }
};

/**
 * @brief Macro to define custom field names for a struct
 * Usage: DEFINE_FIELD_NAMES(MyStruct, "field1", "field2", ...)
 */
#define DEFINE_FIELD_NAMES(StructType, ...) \
    template<> \
    std::vector<std::string> reflect_json::reflection::get_field_names<StructType>() { \
        return {__VA_ARGS__}; \
    }

/**
 * @brief Convenience functions for common use cases
 */
template<typename T>
nlohmann::json to_json(const T& obj) {
    return reflection::to_json(obj);
}

template<typename T>
T from_json(const nlohmann::json& j) {
    return reflection::from_json<T>(j);
}

template<typename T>
void from_json(const nlohmann::json& j, T& obj) {
    reflection::from_json(j, obj);
}

} // namespace reflect_json

/**
 * @brief Integration with nlohmann::json ADL
 * These functions enable automatic conversion using nlohmann::json's to_json/from_json mechanism
 */
namespace nlohmann {

/*
 * ADL overloads disabled to prevent conflicts with string literals and other types
 * Use reflect_json::reflection::to_json() and from_json() explicitly instead
 */

/*
template<typename T>
void to_json(json& j, const T& obj) {
    if constexpr (std::is_aggregate_v<T> && !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !std::is_array_v<T>) {
        j = reflect_json::reflection::to_json(obj);
    }
}

template<typename T>
void from_json(const json& j, T& obj) {
    if constexpr (std::is_aggregate_v<T> && !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !std::is_array_v<T>) {
        reflect_json::reflection::from_json(j, obj);
    }
*/

} // namespace nlohmann
