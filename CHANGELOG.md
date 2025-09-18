# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Nothing yet

### Changed
- Nothing yet

### Fixed
- Nothing yet

## [1.0.0] - 2025-01-17

### Added
- Initial release of reflect-json library
- Automatic JSON serialization/deserialization using Boost.PFR
- Support for C++20 native field name extraction
- Zero-boilerplate JSON conversion for aggregate types
- Custom field name overrides via `DEFINE_FIELD_NAMES` macro
- JSON schema generation for structs
- Support for nested structures and containers
- File I/O utilities for JSON persistence
- Comprehensive test suite with multiple test cases
- CMake integration with modern target exports
- Cross-platform CI/CD with GitHub Actions
- Code coverage reporting
- Documentation and examples
- MIT license for open source distribution

### Technical Features
- Header-only library design
- Integration with nlohmann/json library
- Boost.PFR for reflection capabilities
- Support for std::vector, std::array, and other containers
- Type-safe compile-time validation
- Error handling through nlohmann/json exceptions
- Field iteration with automatic name resolution

### Documentation
- Comprehensive README with usage examples
- API reference documentation
- Building and installation instructions
- Contributing guidelines
- Code of conduct and issue templates

### Build System
- Modern CMake (3.14+) configuration
- vcpkg integration for dependency management
- Support for find_package installation
- Cross-platform builds (Linux, macOS, Windows)
- Multiple compiler support (GCC, Clang, MSVC)
- Automated testing and coverage reporting

[Unreleased]: https://github.com/your-username/reflect-json/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/your-username/reflect-json/releases/tag/v1.0.0