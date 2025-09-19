#pragma once

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <chrono>
#include <map>
#include <concepts>

namespace reflection {

// =============================================================================
// CUSTOM CONVERTER SYSTEM
// =============================================================================

// Generic custom converter template - users can specialize this
template<typename T>
struct custom_converter {
    // No default implementation - this forces specialization
    // static std::string to_string(const T& value);
    // static T from_string(const std::string& str);
};

// Macro to register enum conversions
#define REGISTER_ENUM(EnumType, ...) \
    template<> \
    struct custom_converter<EnumType> { \
        static std::string to_string(const EnumType& value) { \
            static const std::map<EnumType, std::string> enum_map{__VA_ARGS__}; \
            auto it = enum_map.find(value); \
            if (it != enum_map.end()) { \
                return it->second; \
            } \
            return std::to_string(static_cast<int>(value)); \
        } \
        \
        static EnumType from_string(const std::string& str) { \
            static const std::map<EnumType, std::string> enum_map{__VA_ARGS__}; \
            \
            /* Case-insensitive string comparison */ \
            auto to_lower = [](const std::string& s) { \
                std::string lower = s; \
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower); \
                return lower; \
            }; \
            \
            std::string str_lower = to_lower(str); \
            \
            for (const auto& [enum_val, enum_str] : enum_map) { \
                if (to_lower(enum_str) == str_lower) { \
                    return enum_val; \
                } \
            } \
            \
            /* Try to parse as integer */ \
            try { \
                int int_val = std::stoi(str); \
                return static_cast<EnumType>(int_val); \
            } catch (...) { \
                throw std::invalid_argument("Invalid enum string: " + str); \
            } \
        } \
    };

// Specialization for std::chrono::duration types
template<typename Rep, typename Period>
struct custom_converter<std::chrono::duration<Rep, Period>> {
    using duration_type = std::chrono::duration<Rep, Period>;
    
    static std::string to_string(const duration_type& value) {
        auto count = value.count();
        
        // Convert to most appropriate unit
        if constexpr (std::is_same_v<Period, std::ratio<1>>) {
            return std::to_string(count) + "s";
        } else if constexpr (std::is_same_v<Period, std::ratio<60>>) {
            return std::to_string(count) + "m";
        } else if constexpr (std::is_same_v<Period, std::ratio<3600>>) {
            return std::to_string(count) + "h";
        } else if constexpr (std::is_same_v<Period, std::milli>) {
            return std::to_string(count) + "ms";
        } else {
            return std::to_string(count) + "s";  // Default to seconds
        }
    }
    
    static duration_type from_string(const std::string& str) {
        if (str.empty()) {
            throw std::invalid_argument("Empty duration string");
        }
        
        // Remove whitespace
        std::string trimmed = str;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        
        if (trimmed.empty()) {
            throw std::invalid_argument("Empty duration string after trimming");
        }
        
        // Check if it's just a number (assume seconds)
        bool is_numeric = std::all_of(trimmed.begin(), trimmed.end(), 
            [](char c) { return std::isdigit(c) || c == '.' || c == '-' || c == '+'; });
        
        if (is_numeric) {
            try {
                Rep count = static_cast<Rep>(std::stod(trimmed));
                return duration_type(count);
            } catch (...) {
                throw std::invalid_argument("Invalid numeric duration: " + trimmed);
            }
        }
        
        // Parse with unit suffix
        size_t i = 0;
        while (i < trimmed.length() && (std::isdigit(trimmed[i]) || trimmed[i] == '.' || trimmed[i] == '-' || trimmed[i] == '+')) {
            ++i;
        }
        
        if (i == 0) {
            throw std::invalid_argument("No numeric part found in duration: " + trimmed);
        }
        
        std::string numeric_part = trimmed.substr(0, i);
        std::string unit_part = trimmed.substr(i);
        
        // Remove whitespace from unit part
        unit_part.erase(0, unit_part.find_first_not_of(" \t"));
        unit_part.erase(unit_part.find_last_not_of(" \t") + 1);
        
        Rep count;
        try {
            count = static_cast<Rep>(std::stod(numeric_part));
        } catch (...) {
            throw std::invalid_argument("Invalid numeric part in duration: " + numeric_part);
        }
        
        // Convert based on unit
        if (unit_part == "s" || unit_part == "sec" || unit_part == "seconds") {
            return std::chrono::duration_cast<duration_type>(std::chrono::duration<double>(count));
        } else if (unit_part == "m" || unit_part == "min" || unit_part == "minutes") {
            auto minutes = std::chrono::duration<double, std::ratio<60>>(count);
            return std::chrono::duration_cast<duration_type>(minutes);
        } else if (unit_part == "h" || unit_part == "hour" || unit_part == "hours") {
            auto hours = std::chrono::duration<double, std::ratio<3600>>(count);
            return std::chrono::duration_cast<duration_type>(hours);
        } else if (unit_part == "d" || unit_part == "day" || unit_part == "days") {
            auto days = std::chrono::duration<double, std::ratio<86400>>(count);
            return std::chrono::duration_cast<duration_type>(days);
        } else if (unit_part == "ms" || unit_part == "milliseconds") {
            auto millis = std::chrono::duration<double, std::milli>(count);
            return std::chrono::duration_cast<duration_type>(millis);
        } else if (unit_part.empty()) {
            // No unit specified, assume the target unit
            return duration_type(count);
        } else {
            throw std::invalid_argument("Unknown time unit: " + unit_part);
        }
    }
};

// =============================================================================
// TYPE HELPERS AND UTILITIES
// =============================================================================

namespace detail {
// Vector detection helper
template<typename T>
struct is_vector : std::false_type {};

template<typename T, typename A>
struct is_vector<std::vector<T, A> > : std::true_type {};

template<typename T>
static constexpr bool is_vector_v = is_vector<std::decay_t<T> >::value;

// Check if PFR can provide field names (requires C++20 and support)
template<typename T>
constexpr bool has_pfr_names() {
	#if __cplusplus >= 202002L
	return true;      // PFR names are available in C++20
	#else
	return false;
	#endif
}

template<typename T>
struct field_names_traits {
	static std::vector<std::string> get() {
		// This is where you can customize field names for specific types
		// PFR can now extract actual field names from source code automatically!
		return {};
	}
};

template<typename T>
std::vector<std::string> get_field_names() {
	return field_names_traits<T>::get();
}

// Forward declarations for functions we'll need from json module
template<typename T>
nlohmann::json serialize_field_stub(const T& field);

template<typename T>
T deserialize_field_stub(const nlohmann::json& j);

template<typename T>
nlohmann::json to_json_stub(const T& obj);
}//namespace detail

// =============================================================================
// PATH NAVIGATION TYPES
// =============================================================================

/**
 * @brief Represents a single part of a path - either a field name or array index
 */
struct PathPart {
	std::string field_name; ///< Field name (empty if this is an array index)
	std::optional<std::size_t> array_index; ///< Array index (nullopt if this is a field)

	PathPart(const std::string& name) : field_name(name) {
	}
	PathPart(std::size_t index) : array_index(index) {
	}

	bool is_array_access() const {
		return array_index.has_value();
	}
	bool is_field_access() const {
		return !field_name.empty() && !array_index.has_value();
	}

	std::string to_string() const {
		if (is_array_access()) {
			return "[" + std::to_string(*array_index) + "]";
		}
		return field_name;
	}
};

// =============================================================================
// PATH PARSING AND VALIDATION
// =============================================================================

/**
 * @brief Parse a path string into PathPart objects, supporting both dot notation and array access
 * @param path Path string (e.g., "field.items[0].name", "data[2]")
 * @return Vector of PathPart objects
 *
 * Examples:
 * - "name" -> [PathPart("name")]
 * - "items[0]" -> [PathPart("items"), PathPart(0)]
 * - "person.addresses[1].street" -> [PathPart("person"), PathPart("addresses"), PathPart(1), PathPart("street")]
 */
inline std::vector<PathPart> parse_path_enhanced(const std::string& path) {
	std::vector<PathPart> parts;
	if (path.empty()) return parts;

	std::string current_field;
	bool in_brackets = false;
	std::string index_str;

	for (char c : path) {
		if (c == '[') {
			// Start of array index
			if (!current_field.empty()) {
				parts.emplace_back(current_field);
				current_field.clear();
			}
			in_brackets = true;
			index_str.clear();
		} else if (c == ']') {
			// End of array index
			if (in_brackets && !index_str.empty()) {
				try {
					std::size_t index = std::stoull(index_str);
					parts.emplace_back(index);
				} catch (const std::exception&) {
					// Invalid index format - just ignore the index but continue parsing
					// The field name was already added before the '[' 
				}
			}
			in_brackets = false;
			index_str.clear();
		} else if (c == '.') {
			// Field separator (only outside brackets)
			if (!in_brackets) {
				if (!current_field.empty()) {
					parts.emplace_back(current_field);
					current_field.clear();
				}
			} else {
				// Dots inside brackets are part of the index string
				index_str += c;
			}
		} else {
			// Regular character
			if (in_brackets) {
				index_str += c;
			} else {
				current_field += c;
			}
		}
	}

	// Add final field if any
	if (!current_field.empty()) {
		parts.emplace_back(current_field);
	}

	return parts;
}

/**
 * @brief Parse a simple dot notation path (legacy support)
 * @param path Dot notation path string
 * @return Vector of field names
 */
inline std::vector<std::string> parse_path(const std::string& path) {
	std::vector<std::string> parts;
	std::stringstream ss(path);
	std::string part;

	while (std::getline(ss, part, '.')) {
		if (!part.empty()) {
			parts.push_back(part);
		}
	}

	return parts;
}

// =============================================================================
// FIELD INDEX RESOLUTION
// =============================================================================

// Helper to find field index by name
template<typename T>
std::optional<std::size_t> get_field_index(const std::string& field_name) {
	if constexpr (detail::has_pfr_names<T>()) {
		auto names = boost::pfr::names_as_array<T>();
		for (std::size_t i = 0; i < names.size(); ++i) {
			if (std::string(names[i]) == field_name) {
				return i;
			}
		}
	} else {
		auto field_names = get_field_names<T>();
		constexpr auto field_count = boost::pfr::tuple_size_v<T>;

		if (field_names.size() == field_count) {
			auto it = std::find(field_names.begin(), field_names.end(), field_name);
			if (it != field_names.end()) {
				return std::distance(field_names.begin(), it);
			}
		} else {
			// Try default field names
			for (std::size_t i = 0; i < field_count; ++i) {
				if (field_name == "field_" + std::to_string(i)) {
					return i;
				}
			}
		}
	}

	return std::nullopt;
}

// Helper concept to detect duration types
template<typename T>
concept is_duration = requires {
    typename T::rep;
    typename T::period;
    std::is_same_v<T, std::chrono::duration<typename T::rep, typename T::period>>;
};

// Helper to detect if a type has a custom converter
template<typename T>
concept has_custom_converter = requires(const T& value, const std::string& str) {
    { custom_converter<T>::to_string(value) } -> std::convertible_to<std::string>;
    { custom_converter<T>::from_string(str) } -> std::convertible_to<T>;
};

// Helper to try setting a field value from JSON with comprehensive type conversion
template<typename T>
bool try_set_field(T& field, const nlohmann::json& value) {
	try {
		// First check for types with custom converters
		if constexpr (has_custom_converter<T>) {
			if (value.is_string()) {
				field = custom_converter<T>::from_string(value.get<std::string>());
				return true;
			} else if (value.is_number()) {
				// For enum types, try converting from number
				if constexpr (std::is_enum_v<T>) {
					field = static_cast<T>(value.get<std::underlying_type_t<T>>());
					return true;
				}
				// For duration types, handle numeric values according to their unit
				else if constexpr (is_duration<T>) {
					// For duration types, treat numbers as units of that duration's period
					// e.g., if T is std::chrono::minutes, treat number as minutes
					typename T::rep count = static_cast<typename T::rep>(value.get<double>());
					field = T(count);
					return true;
				} else {
					// Fallback: try converting number as seconds string
					field = custom_converter<T>::from_string(std::to_string(value.get<double>()) + "s");
					return true;
				}
			}
		}
		
		if constexpr (std::is_same_v<T, std::string>) {
			if (value.is_string()) {
				field = value.get<std::string>();
				return true;
			} else {
				// Convert other types to string
				field = value.dump();
				return true;
			}
		} else if constexpr (std::is_same_v<T, bool>) {
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
		} else if constexpr (std::is_arithmetic_v<T>) {
			if (value.is_number()) {
				field = value.get<T>();
				return true;
			} else if (value.is_string()) {
				// Try to parse string as number
				try {
					if constexpr (std::is_integral_v<T>) {
						field = static_cast<T>(std::stoll(value.get<std::string>()));
					} else {
						field = static_cast<T>(std::stod(value.get<std::string>()));
					}
					return true;
				} catch (...) {
					return false;
				}
			}
		} else if constexpr (detail::is_vector_v<T>) {
			// For vectors, deserialize from JSON array
			if (value.is_array()) {
				field = detail::deserialize_field_stub<T>(value);
				return true;
			}
		} else if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
			// For nested structs, deserialize from JSON
			if (value.is_object()) {
				field = detail::deserialize_field_stub<T>(value);
				return true;
			}
		} else {
			// Last resort: try deserialize_field_stub for other types
			field = detail::deserialize_field_stub<T>(value);
			return true;
		}
	} catch (const std::exception& e) {
		return false;
	}
	return false;
}

// Forward declaration 
template<typename T, std::size_t... I>
bool validate_path_at_index(std::size_t target_index, const std::vector<std::string>& path_parts, std::size_t depth, std::index_sequence<I...>);

// Validation helper for recursive path checking
template<typename T>
bool validate_path_recursive(const std::vector<std::string>& path_parts, std::size_t depth) {
	if (depth >= path_parts.size()) {
		return true;
	}

	if constexpr (!std::is_aggregate_v<T> || std::is_array_v<T>) {
		return false; // Cannot navigate into non-aggregate types
	} else {
		auto field_index = get_field_index<T>(path_parts[depth]);
		if (!field_index) {
			return false;
		}

		constexpr auto field_count = boost::pfr::tuple_size_v<T>;
		return validate_path_at_index<T>(*field_index, path_parts, depth + 1, std::make_index_sequence<field_count>{});
	}
}

template<typename T, std::size_t... I>
bool validate_path_at_index(std::size_t target_index, const std::vector<std::string>& path_parts, std::size_t depth, std::index_sequence<I...>) {
	return ((I == target_index && validate_path_recursive<std::decay_t<decltype(boost::pfr::get<I>(std::declval<T>()))> >(path_parts, depth)) || ...);
}

// Forward declaration for collect_all_paths
template<typename T>
void collect_all_paths(std::vector<std::string>& paths, const std::string& prefix);

// Helper for collecting all paths in a struct
template<typename T, std::size_t I>
void collect_path_for_field(std::vector<std::string>& paths, const std::string& prefix) {
	std::string field_name;

	if constexpr (detail::has_pfr_names<T>()) {
		auto names = boost::pfr::names_as_array<T>();
		field_name = std::string(names[I]);
	} else {
		auto field_names = get_field_names<T>();
		constexpr auto field_count = boost::pfr::tuple_size_v<T>;

		if (field_names.size() == field_count) {
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

template<typename T>
void collect_all_paths(std::vector<std::string>& paths, const std::string& prefix) {
	if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T>) {
		constexpr auto field_count = boost::pfr::tuple_size_v<T>;
		[&paths, &prefix]<std::size_t... I>(std::index_sequence<I...>) {
			(collect_path_for_field<T, I>(paths, prefix), ...);
		} (std::make_index_sequence<field_count>{});
	}
}


namespace detail {
// =============================================================================
// ENHANCED ARRAY-AWARE RECURSIVE FUNCTIONS
// =============================================================================

// Forward declarations to resolve circular dependencies
template<typename T>
std::optional<nlohmann::json> get_array_element(const T& container, std::size_t index, const std::vector<::reflection::PathPart>& path_parts, std::size_t depth);

template<typename T, std::size_t... I>
std::optional<nlohmann::json> get_field_enhanced_at_index(const T& obj, std::size_t target_index, const std::vector<::reflection::PathPart>& path_parts, std::size_t depth, std::index_sequence<I...>);

template<typename T>
bool set_array_element(T& container, std::size_t index, const std::vector<::reflection::PathPart>& path_parts, const nlohmann::json& value, std::size_t depth);

template<typename T, std::size_t... I>
bool set_field_at_index_enhanced(T& obj, std::size_t target_index, const nlohmann::json& value, std::index_sequence<I...>);

template<typename T, std::size_t... I>
bool set_field_at_index_enhanced_recursive(T& obj, std::size_t target_index, const std::vector<::reflection::PathPart>& path_parts, const nlohmann::json& value, std::size_t depth, std::index_sequence<I...>);

// Enhanced recursive getter with array support
template<typename T>
std::optional<nlohmann::json> get_field_enhanced_recursive(const T& obj, const std::vector<::reflection::PathPart>& path_parts, std::size_t depth) {
	if (depth >= path_parts.size()) {
		// Base case: return current object as JSON
		if constexpr (::reflection::detail::is_vector_v<T>) {
			return serialize_field_stub(obj); // Use our serialize_field which handles vectors
		} else if constexpr (std::is_aggregate_v<T> && !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>) {
			return to_json_stub(obj);
		} else {
			// For primitive types, check if there's a custom converter first
			if constexpr (has_custom_converter<T>) {
				return nlohmann::json(custom_converter<T>::to_string(obj));
			} else {
				return nlohmann::json(obj);
			}
		}
	}

	const auto& current_part = path_parts[depth];

	// Handle array access
	if (current_part.is_array_access()) {
		return get_array_element(obj, *current_part.array_index, path_parts, depth + 1);
	}

	// Handle field access - only valid for aggregate types
	if constexpr (::reflection::detail::is_vector_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
		return std::nullopt; // Cannot navigate into non-aggregate types
	} else if constexpr (!std::is_aggregate_v<T>) {
		return std::nullopt;
	} else {
		constexpr auto field_count = boost::pfr::tuple_size_v<T>;
		auto field_index = get_field_index<T>(current_part.field_name);

		if (!field_index) {
			return std::nullopt;
		}

		return get_field_enhanced_at_index(obj, *field_index, path_parts, depth + 1, std::make_index_sequence<field_count>{});
	}
}

// Enhanced recursive setter with array support
template<typename T>
bool set_field_enhanced_recursive(T& obj, const std::vector<::reflection::PathPart>& path_parts, const nlohmann::json& value, std::size_t depth) {
	if (depth >= path_parts.size()) {
		return false; // Invalid path
	}

	const auto& current_part = path_parts[depth];

	// Handle array access
	if (current_part.is_array_access()) {
		return set_array_element(obj, *current_part.array_index, path_parts, value, depth + 1);
	}

	// Handle field access - only valid for aggregate types
	if constexpr (::reflection::detail::is_vector_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
		return false; // Cannot navigate into non-aggregate types
	} else if constexpr (!std::is_aggregate_v<T>) {
		return false;
	} else {
		constexpr auto field_count = boost::pfr::tuple_size_v<T>;
		auto field_index = get_field_index<T>(current_part.field_name);

		if (!field_index) {
			return false;
		}

		if (depth == path_parts.size() - 1) {
			// Final field - set value
			return set_field_at_index_enhanced(obj, *field_index, value, std::make_index_sequence<field_count>{});
		} else {
			// Navigate deeper
			return set_field_at_index_enhanced_recursive(obj, *field_index, path_parts, value, depth + 1, std::make_index_sequence<field_count>{});
		}
	}
}

// Array element getter
template<typename T>
std::optional<nlohmann::json> get_array_element(const T& container, std::size_t index, const std::vector<::reflection::PathPart>& path_parts, std::size_t depth) {
	// Check if T is a vector/array-like container
	if constexpr (::reflection::detail::is_vector_v<T>) {
		if (index >= container.size()) {
			return std::nullopt; // Index out of bounds
		}

		const auto& element = container[index];

		if (depth >= path_parts.size()) {
			// Return the array element directly
			if constexpr (std::is_aggregate_v<std::decay_t<decltype(element)> > &&
			              !std::is_arithmetic_v<std::decay_t<decltype(element)> > &&
			              !std::is_same_v<std::decay_t<decltype(element)>, std::string>) {
				return to_json_stub(element);
			} else {
				// For primitive types, check if there's a custom converter first
				if constexpr (has_custom_converter<std::decay_t<decltype(element)>>) {
					return nlohmann::json(custom_converter<std::decay_t<decltype(element)>>::to_string(element));
				} else {
					return nlohmann::json(element);
				}
			}
		} else {
			// Continue navigation into the element
			return get_field_enhanced_recursive(element, path_parts, depth);
		}
	}

	return std::nullopt; // Not an array-like container
}

// Array element setter
template<typename T>
bool set_array_element(T& container, std::size_t index, const std::vector<::reflection::PathPart>& path_parts, const nlohmann::json& value, std::size_t depth) {
	// Check if T is a vector/array-like container
	if constexpr (::reflection::detail::is_vector_v<T>) {
		if (index >= container.size()) {
			return false; // Index out of bounds
		}

		auto& element = container[index];

		if (depth >= path_parts.size()) {
			// Set the array element directly
			return try_set_field(element, value);
		} else {
			// Continue navigation into the element
			return set_field_enhanced_recursive(element, path_parts, value, depth);
		}
	}

	return false; // Not an array-like container
}

// Enhanced field setter helpers
template<typename T, std::size_t... I>
std::optional<nlohmann::json> get_field_enhanced_at_index(const T& obj, std::size_t target_index, const std::vector<::reflection::PathPart>& path_parts, std::size_t depth, std::index_sequence<I...>) {
	std::optional<nlohmann::json> result;
	((I == target_index && (result = get_field_enhanced_recursive(boost::pfr::get<I>(obj), path_parts, depth))) || ...);
	return result;
}

template<typename T, std::size_t... I>
bool set_field_at_index_enhanced(T& obj, std::size_t target_index, const nlohmann::json& value, std::index_sequence<I...>) {
	return ((I == target_index && try_set_field(boost::pfr::get<I>(obj), value)) || ...);
}

template<typename T, std::size_t... I>
bool set_field_at_index_enhanced_recursive(T& obj, std::size_t target_index, const std::vector<::reflection::PathPart>& path_parts, const nlohmann::json& value, std::size_t depth, std::index_sequence<I...>) {
	return ((I == target_index && set_field_enhanced_recursive(boost::pfr::get<I>(obj), path_parts, value, depth)) || ...);
}

// =============================================================================
// SIMPLE DOT-NOTATION RECURSIVE FUNCTIONS
// =============================================================================

// Forward declarations
template<typename T, std::size_t... I>
std::optional<nlohmann::json> get_field_at_index(const T& obj, std::size_t target_index,
                                                 const std::vector<std::string>& path_parts, std::size_t depth,
                                                 std::index_sequence<I...>);

template<typename T, std::size_t... I>
bool set_field_at_index(T& obj, std::size_t target_index, const nlohmann::json& value, std::index_sequence<I...>);

template<typename T, std::size_t... I>
bool set_field_at_index_recursive(T& obj, std::size_t target_index, const std::vector<std::string>& path_parts, 
                                  const nlohmann::json& value, std::size_t depth, std::index_sequence<I...>);

// Helper function for recursive path navigation (getter)
template<typename T>
std::optional<nlohmann::json> get_field_recursive(const T& obj, const std::vector<std::string>& path_parts, std::size_t depth) {
	if (depth >= path_parts.size()) {
		// Base case: we've reached the end, return the current object as JSON
		if constexpr (std::is_aggregate_v<T> && !std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !std::is_array_v<T>) {
			return to_json_stub(obj);
		} else {
			// For primitive types, check if there's a custom converter first
			if constexpr (has_custom_converter<T>) {
				return nlohmann::json(custom_converter<T>::to_string(obj));
			} else {
				// For primitive types, use nlohmann::json constructor directly
				return nlohmann::json(obj);
			}
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
std::optional<nlohmann::json> get_field_at_index(const T& obj, std::size_t target_index,
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
bool set_field_recursive(T& obj, const std::vector<std::string>& path_parts, const nlohmann::json& value, std::size_t depth) {
	if (depth >= path_parts.size()) {
		return false; // Invalid path
	}

	if constexpr (!std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || std::is_array_v<T>) {
		return false; // Cannot navigate into primitive/array types
	} else {
		const std::string& field_name = path_parts[depth];
		auto field_index = get_field_index<T>(field_name);

		if (!field_index) {
			return false; // Field not found
		}

		constexpr auto field_count = boost::pfr::tuple_size_v<T>;

		if (depth == path_parts.size() - 1) {
			// Final field - set value
			return set_field_at_index(obj, *field_index, value, std::make_index_sequence<field_count>{});
		} else {
			// Navigate deeper
			return set_field_at_index_recursive(obj, *field_index, path_parts, value, depth + 1, std::make_index_sequence<field_count>{});
		}
	}
}

// Helper to set field at specific index
template<typename T, std::size_t... I>
bool set_field_at_index(T& obj, std::size_t target_index, const nlohmann::json& value, std::index_sequence<I...>) {
	return ((I == target_index && try_set_field(boost::pfr::get<I>(obj), value)) || ...);
}

// Helper to set field at specific index and continue recursion
template<typename T, std::size_t... I>
bool set_field_at_index_recursive(T& obj, std::size_t target_index, const std::vector<std::string>& path_parts, const nlohmann::json& value, std::size_t depth, std::index_sequence<I...>) {
	return ((I == target_index && set_field_recursive(boost::pfr::get<I>(obj), path_parts, value, depth)) || ...);
}

// Stub implementations - provide basic serialization functionality
template<typename T>
nlohmann::json serialize_field_stub(const T& field) {
    // For basic types, just convert directly
    if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
        return field;
    }
    // For types with custom converters, use them
    else if constexpr (has_custom_converter<T>) {
        return custom_converter<T>::to_string(field);
    }
    // For vectors, serialize elements
    else if constexpr (is_vector_v<T>) {
        nlohmann::json result = nlohmann::json::array();
        for (const auto& item : field) {
            result.push_back(serialize_field_stub(item));
        }
        return result;
    }
    // For aggregates, serialize fields using PFR
    else if constexpr (boost::pfr::tuple_size_v<T> > 0) {
        nlohmann::json result = nlohmann::json::object();
        constexpr auto field_count = boost::pfr::tuple_size_v<T>;
        
        // Try to use PFR's native field names first, fallback to custom names
        if constexpr (has_pfr_names<T>()) {
            // Use PFR's native field name support
            auto names = boost::pfr::names_as_array<T>();
            size_t field_index = 0;
            boost::pfr::for_each_field(field, [&result, &names, &field_index](const auto& field_value) {
                result[std::string(names[field_index])] = serialize_field_stub(field_value);
                ++field_index;
            });
        } else {
            // Fallback to custom field names or indices
            auto field_names = get_field_names<T>();
            size_t field_index = 0;
            boost::pfr::for_each_field(field, [&result, &field_names, &field_index, field_count](const auto& field_value) {
                std::string field_name;
                if (field_names.size() == field_count) {
                    field_name = field_names[field_index];
                } else {
                    field_name = "field_" + std::to_string(field_index);
                }
                result[field_name] = serialize_field_stub(field_value);
                ++field_index;
            });
        }
        return result;
    }
    // For other types, return null as fallback
    else {
        return nlohmann::json(nullptr);
    }
}

template<typename T>
T deserialize_field_stub(const nlohmann::json& j) {
    // For basic types, just convert directly
    if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
        return j.get<T>();
    }
    // For types with custom converters, use them
    else if constexpr (has_custom_converter<T>) {
        if (j.is_string()) {
            return custom_converter<T>::from_string(j.get<std::string>());
        } else if (j.is_number()) {
            // For enum types, try converting from number
            if constexpr (std::is_enum_v<T>) {
                return static_cast<T>(j.get<std::underlying_type_t<T>>());
            }
            // For duration types, handle numeric values according to their unit
            else if constexpr (is_duration<T>) {
                // For duration types, treat numbers as units of that duration's period
                // e.g., if T is std::chrono::minutes, treat number as minutes
                typename T::rep count = static_cast<typename T::rep>(j.get<double>());
                return T(count);
            } else {
                // Fallback: try converting number as seconds string
                return custom_converter<T>::from_string(std::to_string(j.get<double>()) + "s");
            }
        }
    }
    // For other types, construct default and try basic conversion
    if constexpr (std::is_default_constructible_v<T>) {
        return T{};
    } else {
        // This will likely fail, but it's the best we can do
        return j.get<T>();
    }
}

template<typename T>
nlohmann::json to_json_stub(const T& obj) {
    return serialize_field_stub(obj);
}

} // namespace detail

// =============================================================================
// PUBLIC API - ENHANCED (Array Support)
// =============================================================================

/**
 * @brief Get field value by path with array support
 * @tparam T Struct type
 * @param obj Object to get field from
 * @param path Path with array support (e.g., "items[0].name", "data[2]")
 * @return Optional JSON value if path exists
 */
template<typename T>
std::optional<nlohmann::json> get_field_enhanced(const T& obj, const std::string& path) {
	auto path_parts = ::reflection::parse_path_enhanced(path);
	if (path_parts.empty()) {
		return std::nullopt;
	}

	return detail::get_field_enhanced_recursive(obj, path_parts, 0);
}

/**
 * @brief Set field value by path with array support
 * @tparam T Struct type
 * @param obj Object to set field in
 * @param path Path with array support (e.g., "items[0].name", "data[2]")
 * @param value JSON value to set
 * @return True if successfully set, false if path invalid or type mismatch
 */
template<typename T>
bool set_field_enhanced(T& obj, const std::string& path, const nlohmann::json& value) {
	auto path_parts = ::reflection::parse_path_enhanced(path);
	if (path_parts.empty()) {
		return false;
	}

	return detail::set_field_enhanced_recursive(obj, path_parts, value, 0);
}

// =============================================================================
// PUBLIC API - SIMPLE (Dot notation only)
// =============================================================================

/**
 * @brief Get field value by dot notation path
 * @tparam T Struct type
 * @param obj Object to get field from
 * @param path Dot notation path
 * @return Optional JSON value if path exists
 */
template<typename T>
std::optional<nlohmann::json> get_field(const T& obj, const std::string& path) {
	auto path_parts = ::reflection::parse_path(path);
	if (path_parts.empty()) {
		return std::nullopt;
	}

	return detail::get_field_recursive(obj, path_parts, 0);
}

/**
 * @brief Set field value by dot notation path
 * @tparam T Struct type
 * @param obj Object to set field in
 * @param path Dot notation path
 * @param value JSON value to set
 * @return True if successfully set, false if path invalid
 */
template<typename T>
bool set_field(T& obj, const std::string& path, const nlohmann::json& value) {
	auto path_parts = ::reflection::parse_path(path);
	if (path_parts.empty()) {
		return false;
	}

	return detail::set_field_recursive(obj, path_parts, value, 0);
}

/**
 * @brief Check if a dot notation path is valid for a struct type
 * @tparam T Struct type
 * @param path Dot notation path to validate
 * @return True if path is valid
 */
template<typename T>
bool is_valid_path(const std::string& path) {
	auto path_parts = ::reflection::parse_path(path);
	if (path_parts.empty()) {
		return false;
	}

	// Note: validate_path_recursive is in main reflection namespace, not detail
	return validate_path_recursive<T>(path_parts, 0);
}

/**
 * @brief Get all possible dot notation paths for a struct type
 * @tparam T Struct type
 * @param prefix Current path prefix (for internal recursion)
 * @return Vector of all valid paths
 */
template<typename T>
std::vector<std::string> get_all_paths(const std::string& prefix = "") {
	std::vector<std::string> paths;
	// Note: collect_all_paths is in main reflection namespace, not detail
	collect_all_paths<T>(paths, prefix);
	return paths;
}

} // namespace reflection