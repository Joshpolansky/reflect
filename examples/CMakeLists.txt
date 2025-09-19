add_executable(basic-example
    json_reflection_example.cpp
)

target_link_libraries(basic-example
    PRIVATE
        reflection::reflection
)

target_compile_features(basic-example
    PRIVATE
        cxx_std_20
)

add_executable(user-test 
    json_reflection_test.cpp
)

target_link_libraries(user-test
    PRIVATE
        reflection::reflection
)

target_compile_features(user-test
    PRIVATE
        cxx_std_20
)

