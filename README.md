# Reflect-JSON

[![CI](https://github.com/your-username/reflect-json/workflows/CI/badge.svg)](https://github.com/your-username/reflect-json/actions)
[![codecov](https://codecov.io/gh/your-username/reflect-json/branch/main/graph/badge.svg)](https://codecov.io/gh/your-username/reflect-json)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

A powerful, modern C++ library that provides automatic JSON serialization and deserialization for structs using **Boost.PFR** (Precise and Flat Reflection) and **nlohmann/json**.

## ✨ Features

- 🔄 **Zero-boilerplate JSON serialization** - No manual field mapping required
- 🏷️ **Automatic field names** - Uses PFR's native field name extraction (C++20+)
- 📊 **Schema generation** - Automatic JSON schema creation for API documentation
- 🎯 **Type-safe** - Compile-time type checking and validation
- 🔧 **Customizable field names** - Override automatic names when needed
- 🪝 **nlohmann/json integration** - Works with existing JSON workflows
- 📁 **File I/O support** - Direct save/load to/from files
- 📦 **Container support** - Works with vectors, arrays, and other containers
- 🔍 **Field iteration** - Iterate over fields with their names

## 📦 Installation

### Using vcpkg (Recommended)

```bash
# Install dependencies
vcpkg install boost-pfr nlohmann-json

# Add to your CMakeLists.txt
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(reflect-json REQUIRED)

target_link_libraries(your_target 
    PRIVATE 
        reflect-json::reflect-json
)
```

### Building from Source

```bash
git clone https://github.com/your-username/reflect-json.git
cd reflect-json
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)
make install
```

## 🚀 Quick Start

```cpp
#include <reflect_json/json_reflection.hpp>
#include <iostream>

struct Person {
    std::string name;
    int age;
    bool is_active;
    double salary;
};

int main() {
    Person person{"Alice", 30, true, 75000.0};
    
    // 🔄 Convert to JSON (automatic field names!)
    auto json = utilities::to_json(person);
    std::cout << json.dump(2) << std::endl;
    // Output: {"name": "Alice", "age": 30, "is_active": true, "salary": 75000.0}
    
    // 🔄 Convert back from JSON  
    auto restored = utilities::from_json<Person>(json);
    
    return 0;
}
```

**Compile with PFR field names enabled:**
```bash
g++ -std=c++20 -DBOOST_PFR_CORE_NAME_ENABLED -g example.cpp
```

## 🔧 Requirements

- C++20 or later
- Boost.PFR (boost-pfr package via vcpkg)
- nlohmann/json (nlohmann-json package via vcpkg)

## 📖 Usage Examples

### Basic Serialization

```cpp
struct Point3D {
    double x, y, z;
};

Point3D point{1.5, 2.7, 3.9};
auto json = utilities::to_json(point);
// Results in: {"x": 1.5, "y": 2.7, "z": 3.9}

// Deserialize back
auto restored = utilities::from_json<Point3D>(json);
```

### Custom Field Names

```cpp
struct Person {
    std::string name;
    int age;
    bool is_active;
};

// Override automatic field names if needed
DEFINE_FIELD_NAMES(Person, "full_name", "years_old", "active_status")

Person person{"John", 30, true};
auto json = utilities::to_json(person);
// Results in: {"full_name": "John", "years_old": 30, "active_status": true}
```

### Nested Structures

```cpp
struct Address {
    std::string street;
    std::string city;
    int zip_code;
};

struct Employee {
    std::string name;
    Address home_address;
    Address work_address;
    double salary;
};

Employee emp{"Alice", {"123 Main St", "Springfield", 12345}, 
                      {"456 Work Ave", "Downtown", 67890}, 
                      75000.0};

auto json = utilities::to_json(emp);
// Automatically handles nested serialization
```

### Collections

```cpp
std::vector<Person> people = {
    {"Alice", 25, true},
    {"Bob", 35, false}
};

// Automatic serialization of collections
nlohmann::json json = people;
auto restored = json.get<std::vector<Person>>();
```

### File I/O

```cpp
// Save to file
auto json = utilities::to_json(person);
std::ofstream file("person.json");
file << json.dump(2);

// Load from file
std::ifstream input("person.json");
nlohmann::json loaded_json;
input >> loaded_json;
auto person = utilities::from_json<Person>(loaded_json);
```

## 🔍 API Reference

### Core Functions

```cpp
// Convert struct to JSON
template<typename T>
nlohmann::json to_json(const T& obj);

// Convert JSON to struct
template<typename T>
T from_json(const nlohmann::json& j);

// Convert JSON to existing struct (in-place)
template<typename T>
void from_json(const nlohmann::json& j, T& obj);
```

### JsonReflection Class

```cpp
class JsonReflection {
public:
    // Static methods for conversion
    template<typename T>
    static nlohmann::json to_json(const T& obj);
    
    template<typename T>
    static T from_json(const nlohmann::json& j);
    
    template<typename T>
    static void from_json(const nlohmann::json& j, T& obj);
    
    // Generate JSON schema
    template<typename T>
    static nlohmann::json get_schema();
    
    // Field iteration with names
    template<typename T, typename F>
    static void for_each_field_with_name(const T& obj, F&& func);
};
```

### Field Name Customization

```cpp
// Define custom field names for better JSON output
DEFINE_FIELD_NAMES(StructName, "field1", "field2", "field3")
```

## 📊 Schema Generation

Generate JSON schemas for API documentation:

```cpp
struct Config {
    std::string name;
    int port;
    bool enabled;
};

auto schema = utilities::JsonReflection::get_schema<Config>();
// Produces:
// {
//   "type": "object",
//   "properties": {
//     "name": {"type": "string"},
//     "port": {"type": "integer"},
//     "enabled": {"type": "boolean"}
//   }
// }
```

## 🏗️ Building

### Quick Start with Build Script

The easiest way to build the project:

```bash
# Quick build with vcpkg (recommended)
./scripts/build.sh

# Build release version and run tests
./scripts/build.sh -p vcpkg-release --test

# Build with coverage report
./scripts/build.sh --coverage

# Clean build
./scripts/build.sh --clean
```

### Using CMake Presets

Modern CMake preset support for easy configuration:

```bash
# Configure with preset
cmake --preset vcpkg-debug

# Build
cmake --build build/vcpkg-debug

# Test
ctest --preset debug
```

**Available presets:**
- `vcpkg-debug` - Debug build with vcpkg (recommended for development)
- `vcpkg-release` - Release build with vcpkg
- `coverage` - Debug build with code coverage
- `sanitizer` - Debug build with AddressSanitizer and UBSan
- `clang` / `gcc` - Compiler-specific builds

### Manual CMake Build

```bash
# With vcpkg
export VCPKG_ROOT=/path/to/vcpkg
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)

# Without vcpkg (requires system-installed dependencies)
cmake -B build -DREFLECT_JSON_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

### CMake Integration in Your Project

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_project)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(reflect-json REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE reflect-json::reflect-json)
target_compile_features(my_app PRIVATE cxx_std_20)
```

### Build Options

- `REFLECT_JSON_BUILD_TESTS` - Build test suite (default: ON)
- `REFLECT_JSON_BUILD_EXAMPLES` - Build examples (default: ON)
- `REFLECT_JSON_INSTALL` - Generate install target (default: ON)

## 🧪 Testing

```bash
# Using build script (recommended)
./scripts/build.sh --test

# Using CMake presets
ctest --preset debug

# Manual testing
cd build && ctest --output-on-failure --parallel
```

## ⚠️ Limitations

1. **Aggregate types only**: Structs must be aggregate types (no constructors, private members, etc.)
2. **No inheritance**: Inheritance hierarchies are not supported
3. **Plain data focus**: Works best with POD (Plain Old Data) types
4. **Field names**: Without compilation flags, automatic field names may not work

## 🔧 Supported Types

- **Primitive types**: `int`, `float`, `double`, `bool`, `char`
- **Strings**: `std::string`
- **Containers**: `std::vector`, `std::array` (with automatic element conversion)
- **Nested structs**: Any aggregate struct
- **Optional**: `std::optional<T>` (with proper JSON handling)

## 🐛 Error Handling

The utility uses nlohmann/json's exception system:

```cpp
try {
    auto obj = utilities::from_json<MyStruct>(json);
} catch (const nlohmann::json::exception& e) {
    std::cerr << "JSON error: " << e.what() << std::endl;
}
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgements

- [Boost.PFR](https://github.com/boostorg/pfr) - For powerful reflection capabilities
- [nlohmann/json](https://github.com/nlohmann/json) - For excellent JSON handling
- All contributors and users of this library

## 📈 Performance

The library is designed for ease of use rather than maximum performance. For high-performance scenarios, consider:

- Pre-compiled serialization functions for hot paths
- Bulk operations for collections
- Custom serializers for performance-critical types

## 🔗 Related Projects

- [Boost.PFR](https://github.com/boostorg/pfr) - Reflection library
- [nlohmann/json](https://github.com/nlohmann/json) - JSON library
- [glaze](https://github.com/stephenberry/glaze) - High-performance JSON library
- [cereal](https://github.com/USCiLab/cereal) - Serialization library

---

<div align="center">
  <strong>⭐ If this project helped you, please consider giving it a star! ⭐</strong>
</div>