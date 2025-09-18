add_executable(reflect-json-tests
    json_reflection_test.cpp
)

target_link_libraries(reflect-json-tests
    PRIVATE
        reflect-json::reflect-json
)

target_compile_features(reflect-json-tests
    PRIVATE
        cxx_std_20
)