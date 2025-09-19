#pragma once

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <sstream>
#include <fstream>

namespace reflection {
namespace json {

// Forward declarations for functions that need mutual recursion
template<typename T>
T from_json(const nlohmann::json& j);

template<typename T>
void from_json(const nlohmann::json& j, T& obj);

template<typename T>
nlohmann::json get_schema();

// =============================================================================
// TYPE HELPERS AND UTILITIES
// =============================================================================

namespace detail {
    // Vector detection helper
    template<typename T>
    struct is_vector : std::false_type {};

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : std::true_type {};

    template<typename T>
    static constexpr bool is_vector_v = is_vector<std::decay_t<T>>::value;

    // Check if PFR can provide field names (requires C++20 and support)
    template<typename T>
    constexpr bool has_pfr_names() {
        #if __cplusplus >= 202002L
            return true;  // PFR names are available in C++20
        #else
            return false;
        #endif
    }

    template<typename T>
    std::vector<std::string> get_field_names() {
        // This is where you can customize field names for specific types
        // PFR can now extract actual field names from source code automatically!
        // This method is now primarily for backwards compatibility or custom overrides
        return {};
    }

    // Forward declaration for to_json_impl
    template<class T, std::size_t... I>
    nlohmann::json to_json_impl(const T& t, std::index_sequence<I...>);

    // Helper function to recursively serialize fields
    template<typename T>
    nlohmann::json serialize_field(const T& field) {
        // First, try custom converter to_string for JSON serialization
        if constexpr (std::is_enum_v<T>) {
            // For enums, try to get string representation first
            auto string_repr = custom_converter<T>::to_string(field);
            if (!string_repr.empty()) {
                return nlohmann::json(string_repr);
            }
            // Fall back to integer representation
            return nlohmann::json(static_cast<std::underlying_type_t<T>>(field));
        } else if constexpr (std::is_same_v<T, std::chrono::seconds> || 
                            std::is_same_v<T, std::chrono::minutes> ||
                            std::is_same_v<T, std::chrono::hours> ||
                            std::is_same_v<T, std::chrono::duration<double>> ||
                            std::is_same_v<T, std::chrono::duration<float>> ||
                            std::is_same_v<T, std::chrono::duration<long long>> ||
                            std::is_same_v<T, std::chrono::duration<long, std::ratio<60>>> ||
                            std::is_same_v<T, std::chrono::duration<int, std::ratio<3600>>>) {
            // For duration types, use custom converter to get string representation
            auto string_repr = custom_converter<T>::to_string(field);
            if (!string_repr.empty()) {
                return nlohmann::json(string_repr);
            }
            // Fall back to count as number
            return nlohmann::json(field.count());
        } else if constexpr (is_vector_v<T>) {
            // Handle vectors by serializing each element
            nlohmann::json array = nlohmann::json::array();
            for (const auto& element : field) {
                array.push_back(serialize_field(element));
            }
            return array;
        } else if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
            // Recursively serialize nested aggregates - use to_json_impl directly to avoid circular dependency
            constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
            return to_json_impl(field, std::make_index_sequence<fields_count>{});
        } else {
            return field; // Let nlohmann::json handle it
        }
    }
    
    // Helper function to recursively deserialize fields  
    template<typename T>
    T deserialize_field(const nlohmann::json& j) {
        if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
            return j.get<T>();
        } else if constexpr (is_vector_v<T>) {
            // Handle vectors by deserializing each element
            T result;
            if (j.is_array()) {
                result.reserve(j.size());
                for (const auto& element : j) {
                    using ElementType = typename T::value_type;
                    result.push_back(deserialize_field<ElementType>(element));
                }
            }
            return result;
        } else if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
            // Recursively deserialize nested aggregates
            return from_json<T>(j);
        } else {
            return j.get<T>(); // Let nlohmann::json handle it
        }
    }

    template<class T, std::size_t... I>
    nlohmann::json to_json_impl(const T& t, std::index_sequence<I...>) {
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
    void from_json_impl(const nlohmann::json& j, T& t, std::index_sequence<I...>) {
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

    /**
     * @brief Get field names using PFR's native support
     * @tparam T Struct type
     * @return Vector of field names
     */
    template<typename T>
    std::vector<std::string> get_pfr_field_names() {
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
    template<typename T, std::size_t... I>
    void get_field_info_impl(nlohmann::json& info, std::index_sequence<I...>) {
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

    /**
     * @brief Get field information that PFR can provide
     * @tparam T Struct type
     * @return JSON object with field count, types, and indices
     */
    template<typename T>
    nlohmann::json get_field_info() {
        nlohmann::json info;
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        
        info["field_count"] = field_count;
        info["fields"] = nlohmann::json::array();
        
        get_field_info_impl<T>(info, std::make_index_sequence<field_count>{});
        
        return info;
    }

    template<typename T>
    nlohmann::json get_type_schema() {
        nlohmann::json type_info;
        if constexpr (std::is_same_v<T, bool>) {
            type_info["type"] = "boolean";
        } else if constexpr (std::is_integral_v<T>) {
            type_info["type"] = "integer";
        } else if constexpr (std::is_floating_point_v<T>) {
            type_info["type"] = "number";
        } else if constexpr (std::is_same_v<T, std::string>) {
            type_info["type"] = "string";
        } else if constexpr (is_vector_v<T>) {
            type_info["type"] = "array";
            using ElementType = typename T::value_type;
            type_info["items"] = get_type_schema<ElementType>();
        } else if constexpr (std::is_aggregate_v<T>) {
            type_info = get_schema<T>();
        } else {
            type_info["type"] = "object";
            type_info["description"] = "Unknown type: " + std::string(typeid(T).name());
        }
        return type_info;
    }


    template<typename T, std::size_t... I>
    void get_schema_impl(nlohmann::json& schema, std::index_sequence<I...>) {
        if constexpr (detail::has_pfr_names<T>()) {
            auto names = boost::pfr::names_as_array<T>();
            ((schema["properties"][std::string(names[I])] = get_type_schema<boost::pfr::tuple_element_t<I, T>>()), ...);
        } else {
            auto field_names = detail::get_field_names<T>();
            if (field_names.size() == sizeof...(I)) {
                ((schema["properties"][field_names[I]] = get_type_schema<boost::pfr::tuple_element_t<I, T>>()), ...);
            } else {
                ((schema["properties"]["field_" + std::to_string(I)] = get_type_schema<boost::pfr::tuple_element_t<I, T>>()), ...);
            }
        }
    }

    template<typename T, typename F, std::size_t... I>
    void for_each_field_with_custom_names(const T& obj, const std::vector<std::string>& field_names, F&& func, std::index_sequence<I...>) {
        ((func(field_names[I], boost::pfr::get<I>(obj))), ...);
    }

    template<typename T, typename F, std::size_t... I>
    void for_each_field_with_default_names(const T& obj, F&& func, std::index_sequence<I...>) {
        ((func("field_" + std::to_string(I), boost::pfr::get<I>(obj))), ...);
    }

} // namespace detail

// =============================================================================
// PUBLIC JSON API
// =============================================================================

/**
 * @brief Convert a struct to JSON
 * @tparam T Struct type (must be aggregate/plain old data)
 * @param obj Object to serialize
 * @return JSON representation
 */
template<typename T>
nlohmann::json to_json(const T& obj) {
    static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
    constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
    return detail::to_json_impl(obj, std::make_index_sequence<fields_count>{});
}

/**
 * @brief Create struct from JSON
 * @tparam T Struct type (must be aggregate/plain old data)
 * @param j JSON object
 * @return Deserialized object
 */
template<typename T>
T from_json(const nlohmann::json& j) {
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
void from_json(const nlohmann::json& j, T& obj) {
    static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
    constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
    detail::from_json_impl(j, obj, std::make_index_sequence<fields_count>{});
}

/**
 * @brief Get field names and types as JSON schema
 * @tparam T Struct type
 * @return JSON schema describing the struct
 */
template<typename T>
nlohmann::json get_schema() {
    static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
    nlohmann::json schema;
    schema["type"] = "object";
    schema["properties"] = nlohmann::json::object();
    
    constexpr auto fields_count = boost::pfr::tuple_size_v<T>;
    detail::get_schema_impl<T>(schema, std::make_index_sequence<fields_count>{});
    
    return schema;
}

/**
 * @brief Get detailed field information that PFR can provide
 * @tparam T Struct type  
 * @return JSON with field count, types, and reflection capabilities
 */
template<typename T>
nlohmann::json get_reflection_info() {
    static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
    
    nlohmann::json info;
    info["struct_name"] = typeid(T).name();
    info["is_aggregate"] = std::is_aggregate_v<T>;
    info["field_info"] = detail::get_field_info<T>();
    
    // Check PFR native field name support
    info["pfr_names_enabled"] = detail::has_pfr_names<T>();
    
    if constexpr (detail::has_pfr_names<T>()) {
        auto pfr_names = detail::get_pfr_field_names<T>();
        info["pfr_field_names"] = pfr_names;
    }
    
    // Check if custom field names are defined
    auto field_names = detail::get_field_names<T>();
    constexpr auto field_count = boost::pfr::tuple_size_v<T>;
    
    info["has_custom_field_names"] = (field_names.size() == field_count);
    if (info["has_custom_field_names"]) {
        info["custom_field_names"] = field_names;
    }
    
    // Show which names will be used for JSON
    if constexpr (detail::has_pfr_names<T>()) {
        info["json_field_names"] = detail::get_pfr_field_names<T>();
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
void for_each_field_with_name(const T& obj, F&& func) {
    static_assert(std::is_aggregate_v<T>, "Type must be an aggregate type");
    
    if constexpr (detail::has_pfr_names<T>()) {
        // Use PFR's native for_each_field_with_name if available
        boost::pfr::for_each_field_with_name(obj, std::forward<F>(func));
    } else {
        // Fallback implementation
        auto field_names = detail::get_field_names<T>();
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        
        if (field_names.size() == field_count) {
            detail::for_each_field_with_custom_names(obj, field_names, std::forward<F>(func), std::make_index_sequence<field_count>{});
        } else {
            detail::for_each_field_with_default_names(obj, std::forward<F>(func), std::make_index_sequence<field_count>{});
        }
    }
}

/**
 * @brief Save object to JSON file
 * @tparam T Struct type
 * @param obj Object to save
 * @param filename File path
 */
template<typename T>
void save_to_file(const T& obj, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    auto json_obj = ::reflection::json::to_json(obj);
    file << json_obj.dump(4);  // Pretty print with 4-space indentation
}

/**
 * @brief Load object from JSON file
 * @tparam T Struct type
 * @param filename File path
 * @return Loaded object
 */
template<typename T>
T load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for reading: " + filename);
    }
    
    nlohmann::json j;
    file >> j;
    return from_json<T>(j);
}

} // namespace json
} // namespace reflection

// =============================================================================
// NLOHMANN JSON INTEGRATION (OPTIONAL CONVENIENCE) - DISABLED DUE TO ADL CONFLICTS
// =============================================================================

/*
namespace nlohmann {

template<typename T>
void to_json(nlohmann::json& j, const T& obj) {
    if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T> && 
                  !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>) {
        j = reflection::json::to_json(obj);
    }
}

template<typename T>
void from_json(const nlohmann::json& j, T& obj) {
    if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T> && 
                  !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>) {
        reflection::json::from_json(j, obj);
    }
}

} // namespace nlohmann
*/