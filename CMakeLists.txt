cmake_minimum_required(VERSION 3.14)

# Set policy for Boost finding
cmake_policy(SET CMP0167 NEW)

project(reflect-json
    VERSION 1.0.0
    DESCRIPTION "A modern C++ library for JSON serialization using reflection"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(REFLECT_JSON_BUILD_TESTS "Build tests" ON)
option(REFLECT_JSON_BUILD_EXAMPLES "Build examples" ON)
option(REFLECT_JSON_INSTALL "Generate install target" ON)

# Add compile flags for PFR field names support
add_compile_definitions(BOOST_PFR_CORE_NAME_ENABLED)
add_compile_options(-g)

# Find dependencies
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Header-only library target
add_library(reflect-json INTERFACE)
add_library(reflect-json::reflect-json ALIAS reflect-json)

# Set target properties
target_include_directories(reflect-json
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(reflect-json
    INTERFACE
        Boost::boost
        nlohmann_json::nlohmann_json
)

target_compile_features(reflect-json
    INTERFACE
        cxx_std_20
)

# Compiler-specific options
target_compile_options(reflect-json
    INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Build tests
if(REFLECT_JSON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples
if(REFLECT_JSON_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
if(REFLECT_JSON_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install headers
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    # Install targets
    install(
        TARGETS reflect-json
        EXPORT reflect-json-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install export
    install(
        EXPORT reflect-json-targets
        FILE reflect-json-targets.cmake
        NAMESPACE reflect-json::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflect-json
    )

    # Create config file
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/reflect-json-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/reflect-json-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflect-json
    )

    # Create version file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/reflect-json-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install config files
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/reflect-json-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/reflect-json-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflect-json
    )
endif()

# Export targets for build tree
export(
    TARGETS reflect-json
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reflect-json-targets.cmake
    NAMESPACE reflect-json::
)

# Package configuration
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE reflect-json)