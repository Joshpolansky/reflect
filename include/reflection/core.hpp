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

namespace reflection {

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
std::vector<std::string> get_field_names() {
	// This is where you can customize field names for specific types
	// PFR can now extract actual field names from source code automatically!
	return {};
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

// Helper to try setting a field value from JSON with comprehensive type conversion
template<typename T>
bool try_set_field(T& field, const nlohmann::json& value) {
	try {
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
			// For basic types, let nlohmann::json handle it
			field = value.get<T>();
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
			return nlohmann::json(obj);
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
				return nlohmann::json(element);
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

// Stub implementations - now properly implemented by calling json module functions
template<typename T>
nlohmann::json serialize_field_stub(const T& field) {
	// Always use the JSON module's serialize_field function
	return json::detail::serialize_field(field);
}

template<typename T>
T deserialize_field_stub(const nlohmann::json& j) {
	// Use the JSON module's deserialize_field function  
	return json::detail::deserialize_field<T>(j);
}

template<typename T>
nlohmann::json to_json_stub(const T& obj) {
	// Use the JSON module's serialize_field function for all types
	if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) {
		return obj;
	} else {
		return json::detail::serialize_field(obj);  // serialize_field handles both vectors and aggregates
	}
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