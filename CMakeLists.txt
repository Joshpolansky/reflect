cmake_minimum_required(VERSION 3.14)

# Set policy for Boost finding
cmake_policy(SET CMP0167 NEW)

project(reflection
    VERSION 2.0.0
    DESCRIPTION "A modern C++ library for reflection and JSON serialization"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(REFLECTION_BUILD_TESTS "Build tests" ON)
option(REFLECTION_BUILD_EXAMPLES "Build examples" ON)
option(REFLECTION_INSTALL "Generate install target" ON)

# Add compile flags for PFR field names support
add_compile_definitions(BOOST_PFR_CORE_NAME_ENABLED)
add_compile_options(-g)

# Find dependencies
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Header-only library target
add_library(reflection INTERFACE)
add_library(reflection::reflection ALIAS reflection)

# Set target properties
target_include_directories(reflection
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(reflection
    INTERFACE
        Boost::boost
        nlohmann_json::nlohmann_json
)

target_compile_features(reflection
    INTERFACE
        cxx_std_20
)

# Compiler-specific options
target_compile_options(reflection
    INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Build tests
if(REFLECTION_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples
if(REFLECTION_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
if(REFLECTION_INSTALL)
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
        TARGETS reflection
        EXPORT reflection-targets
        INCLUDES DESTINATION include
    )

    # Install export
    install(
        EXPORT reflection-targets
        FILE reflection-targets.cmake
        NAMESPACE reflection::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflection
    )

    # Create config file
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/reflection-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/reflection-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflection
    )

    # Create version file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/reflection-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install config files
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/reflection-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/reflection-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/reflection
    )
endif()

# Export targets for build tree
export(
    TARGETS reflection
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reflection-targets.cmake
    NAMESPACE reflection::
)

# Package configuration
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE reflection)