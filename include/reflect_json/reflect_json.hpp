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
