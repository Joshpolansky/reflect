add_executable(basic-example
    json_reflection_example.cpp
)

target_link_libraries(basic-example
    PRIVATE
        reflect-json::reflect-json
)

target_compile_features(basic-example
    PRIVATE
        cxx_std_20
)