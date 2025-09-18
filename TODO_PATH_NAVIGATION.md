# Path Navigation & Advanced Features - TODO

This document tracks the development of advanced path-based field access, configuration management, and web API framework features for the reflect-json library.

## 🎯 Primary Use Cases
- **Configuration Management**: Environment variables, JSON config files, validation
- **Web API Framework**: Form processing, partial updates, field filtering, schema generation

## 📋 Development Roadmap

### ✅ Completed
- [x] Basic JSON reflection (serialization/deserialization)
- [x] Schema generation with proper field names
- [x] Reflection info with field names and types
- [x] Consistent naming (reflect_json namespace, reflection class)
- [x] Comprehensive test suite
- [x] Build system and CI/CD
- [x] **Phase 1: Core Path Navigation - COMPLETED!**
  - [x] `get_field(obj, "path.to.field")` - Get field value by dot notation path
  - [x] `set_field(obj, "path.to.field", value)` - Set field value by path
  - [x] Path parsing utility (`parse_path("a.b.c")` -> `["a", "b", "c"]`)
  - [x] `is_valid_path<T>(path)` - Check if path exists in struct
  - [x] `get_all_paths<T>()` - List all possible paths for a struct type
  - [x] Recursive struct traversal using boost::pfr
  - [x] Type conversion system (string↔number, string↔boolean)
  - [x] Boolean field type ordering fix
  - [x] doctest test infrastructure (17 test cases, 133 assertions)
  - [x] Comprehensive edge case testing and validation

### 🚧 Phase 2: Configuration Management
**Status: Not Started**

#### Environment Variable Processing
- [ ] `env_mapping` struct for env var to config path mapping
- [ ] `apply_environment_variables()` with mapping vector
- [ ] `apply_environment_variables()` with simple map
- [ ] Default value handling for missing env vars
- [ ] Type conversion from string env vars

#### JSON Configuration Files
- [ ] `apply_json_file()` - Load and apply JSON config file
- [ ] `apply_json_overrides()` - Apply JSON overrides to existing config
- [ ] `config_result` struct with success/error reporting
- [ ] Hierarchical config merging (defaults -> file -> env -> CLI args)

#### Configuration Validation
- [ ] `validate_config()` - Validate configuration values
- [ ] `validation_error` struct with detailed error info
- [ ] Required field validation
- [ ] Type constraint validation
- [ ] Custom validation rules support

#### Default Value Management
- [ ] `get_defaults<T>()` - Extract default values from default-constructed struct
- [ ] `reset_to_defaults()` - Reset specific paths to defaults
- [ ] Default value schema generation

#### Configuration Introspection
- [ ] `get_config_schema()` - Generate configuration schema
- [ ] `get_current_values()` - Get current config as path->value map
- [ ] Configuration diff/comparison utilities

#### Tests
- [ ] Environment variable mapping tests
- [ ] JSON config file processing tests
- [ ] Configuration validation tests
- [ ] Default value handling tests
- [ ] Integration tests with real config scenarios

### 🌐 Phase 3: Web API Framework Support
**Status: Not Started**

#### Form Data Processing
- [ ] `form_result` struct with validation errors
- [ ] `process_form_data()` - Process HTML form data
- [ ] `process_multipart_form()` - Handle file uploads and multipart data
- [ ] Field-level validation with detailed error messages
- [ ] Unknown field handling

#### REST API Partial Updates
- [ ] `patch_result` struct for PATCH operation results
- [ ] `apply_json_patch()` - Apply JSON patch operations
- [ ] `apply_partial_update()` - Apply partial JSON updates
- [ ] Change tracking (which fields were modified)
- [ ] Atomic update support

#### API Response Filtering
- [ ] `extract_fields()` - Extract specific fields by path list
- [ ] `filter_response()` - Parse and apply field query strings
- [ ] Query string parsing ("name,email,address.city")
- [ ] Nested field filtering support

#### API Schema Generation
- [ ] `generate_openapi_schema()` - Generate OpenAPI 3.0 schema
- [ ] `generate_json_schema()` - Generate JSON Schema
- [ ] `field_metadata` struct with API field information
- [ ] `get_api_fields()` - Get field metadata for documentation

#### Request Validation
- [ ] `validation_result` struct for request validation
- [ ] `validate_request_data()` - Validate incoming JSON requests
- [ ] Required field validation
- [ ] Type validation
- [ ] Format validation (email, phone, etc.)

#### Tests
- [ ] Form data processing tests
- [ ] PATCH operation tests  
- [ ] Field filtering tests
- [ ] Schema generation tests
- [ ] Request validation tests
- [ ] Integration tests with web framework examples

### 🚀 Phase 4: Advanced Features
**Status: Not Started**

#### Array/Vector Support
- [ ] Array index notation (`items[0].field`)
- [ ] Vector element access and modification
- [ ] Array bounds checking

#### Performance Optimizations
- [ ] Field name -> index mapping cache
- [ ] Template specialization for common patterns
- [ ] Compile-time path validation where possible

#### Custom Type Conversion
- [ ] `string_converter<T>` specialization system
- [ ] Custom format parsers (duration, URL, etc.)
- [ ] Pluggable conversion system

#### Field Attributes (Future)
- [ ] Field metadata annotation system
- [ ] Validation constraints
- [ ] Documentation strings
- [ ] API generation hints

#### Extended Path Syntax
- [ ] Wildcard support (`users.*.name`)
- [ ] Conditional paths (`user.address?city`)
- [ ] Path expressions and queries

## 🏗️ Implementation Strategy

### Code Organization
```
include/reflect_json/
├── reflect_json.hpp          # Main header (current functionality)
├── path_navigation.hpp       # Core path navigation
├── config/
│   ├── environment.hpp       # Environment variable processing
│   ├── json_config.hpp       # JSON configuration file handling
│   └── validation.hpp        # Configuration validation
└── web/
    ├── forms.hpp            # Form data processing
    ├── api_updates.hpp      # REST API partial updates
    ├── filtering.hpp        # Response field filtering
    └── schema.hpp           # API schema generation
```

### Testing Strategy
- Unit tests for each component
- Integration tests for real-world scenarios
- Performance benchmarks
- Example applications demonstrating usage

### Documentation Plan
- API reference documentation
- Configuration management tutorial
- Web API framework integration guide
- Best practices and patterns

## 🎯 Success Criteria

### Performance Targets
- Path lookup: < 1μs for typical nested structs
- Configuration loading: < 10ms for typical config files
- Form processing: < 100μs for typical web forms

### API Quality Goals
- Type-safe field access with compile-time validation where possible
- Clear error messages with actionable information
- Intuitive API that follows C++ conventions
- Header-only library (no runtime dependencies beyond existing ones)

### Real-World Validation
- Successfully process configuration for a web server application
- Handle form processing for a typical CRUD web application
- Generate valid OpenAPI schemas for REST APIs
- Support at least 5 levels of nesting in practical scenarios

## 📝 Notes

### Technical Considerations
- Maintain header-only library structure
- Preserve C++20 requirement
- Keep boost::pfr and nlohmann::json as only dependencies
- Ensure compatibility with existing reflection functionality

### Breaking Changes
- This is a major feature addition, not a breaking change
- All existing APIs should continue to work unchanged
- New functionality should be additive and opt-in

### Future Considerations  
- Potential for code generation tools
- Integration with popular web frameworks
- Plugin system for custom validation rules
- Performance profiling and optimization tools

---

**Last Updated**: September 17, 2025
**Current Phase**: Planning and API Design Complete
**Next Milestone**: Begin Phase 1 Implementation (Core Path Navigation)