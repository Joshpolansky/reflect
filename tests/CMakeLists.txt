add_executable(reflect-json-tests
    test_main.cpp
    basic_reflection_tests.cpp
    path_navigation_tests.cpp
    array_support_tests.cpp
    vector_json_tests.cpp
)

target_link_libraries(reflect-json-tests
    PRIVATE
        reflection::reflection
)

target_include_directories(reflect-json-tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
)

target_compile_features(reflect-json-tests
    PRIVATE
        cxx_std_20
)

# Add test discovery for CTest
include(CTest)
add_test(NAME reflect-json-unit-tests COMMAND reflect-json-tests -r junit -o test_results.xml)