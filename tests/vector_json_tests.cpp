#include "../third_party/doctest.h"
#include "../include/reflect_json/reflect_json.hpp"
#include <vector>
#include <string>

using namespace reflect_json;

// Test structures with vectors
struct Point {
    double x, y;
};

struct Shape {
    std::string name;
    std::vector<Point> points;
    int id;
};

struct Matrix {
    std::vector<std::vector<int>> data;
    std::string name;
};

struct NumberList {
    std::vector<int> numbers;
    std::string description;
};

TEST_CASE("Vector JSON Reflection - Basic Operations") {
    SUBCASE("Struct with vector to JSON") {
        Shape triangle;
        triangle.name = "Triangle";
        triangle.id = 123;
        triangle.points = {
            {0.0, 0.0},
            {1.0, 0.0},
            {0.5, 1.0}
        };
        
        // Test serialization
        auto json = reflection::to_json(triangle);
        
        CHECK(json["name"].get<std::string>() == "Triangle");
        CHECK(json["id"].get<int>() == 123);
        CHECK(json["points"].is_array());
        CHECK(json["points"].size() == 3);
        
        // Check individual points
        CHECK(json["points"][0]["x"].get<double>() == 0.0);
        CHECK(json["points"][0]["y"].get<double>() == 0.0);
        CHECK(json["points"][1]["x"].get<double>() == 1.0);
        CHECK(json["points"][2]["y"].get<double>() == 1.0);
    }
    
    SUBCASE("JSON to struct with vector") {
        nlohmann::json json = {
            {"name", "Square"},
            {"id", 456},
            {"points", {
                {{"x", 0.0}, {"y", 0.0}},
                {{"x", 1.0}, {"y", 0.0}},
                {{"x", 1.0}, {"y", 1.0}},
                {{"x", 0.0}, {"y", 1.0}}
            }}
        };
        
        // Test deserialization
        auto shape = reflection::from_json<Shape>(json);
        
        CHECK(shape.name == "Square");
        CHECK(shape.id == 456);
        CHECK(shape.points.size() == 4);
        
        CHECK(shape.points[0].x == 0.0);
        CHECK(shape.points[0].y == 0.0);
        CHECK(shape.points[1].x == 1.0);
        CHECK(shape.points[1].y == 0.0);
        CHECK(shape.points[2].x == 1.0);
        CHECK(shape.points[2].y == 1.0);
        CHECK(shape.points[3].x == 0.0);
        CHECK(shape.points[3].y == 1.0);
    }
    
    SUBCASE("Round trip test") {
        Shape original;
        original.name = "Pentagon";
        original.id = 789;
        original.points = {
            {1.0, 0.0},
            {0.309, 0.951},
            {-0.809, 0.588},
            {-0.809, -0.588},
            {0.309, -0.951}
        };
        
        // Convert to JSON and back
        auto json = reflection::to_json(original);
        auto restored = reflection::from_json<Shape>(json);
        
        CHECK(restored.name == original.name);
        CHECK(restored.id == original.id);
        CHECK(restored.points.size() == original.points.size());
        
        for (size_t i = 0; i < original.points.size(); ++i) {
            CHECK(restored.points[i].x == original.points[i].x);
            CHECK(restored.points[i].y == original.points[i].y);
        }
    }
}

TEST_CASE("Vector JSON Reflection - Nested Vectors") {
    SUBCASE("2D vector serialization") {
        Matrix matrix;
        matrix.name = "Identity";
        matrix.data = {
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 1}
        };
        
        auto json = reflection::to_json(matrix);
        
        CHECK(json["name"].get<std::string>() == "Identity");
        CHECK(json["data"].is_array());
        CHECK(json["data"].size() == 3);
        CHECK(json["data"][0].is_array());
        CHECK(json["data"][0].size() == 3);
        CHECK(json["data"][0][0].get<int>() == 1);
        CHECK(json["data"][1][1].get<int>() == 1);
        CHECK(json["data"][2][2].get<int>() == 1);
        CHECK(json["data"][0][1].get<int>() == 0);
    }
    
    SUBCASE("2D vector deserialization") {
        nlohmann::json json = {
            {"name", "Test Matrix"},
            {"data", {
                {1, 2, 3},
                {4, 5, 6}
            }}
        };
        
        auto matrix = reflection::from_json<Matrix>(json);
        
        CHECK(matrix.name == "Test Matrix");
        CHECK(matrix.data.size() == 2);
        CHECK(matrix.data[0].size() == 3);
        CHECK(matrix.data[1].size() == 3);
        CHECK(matrix.data[0][0] == 1);
        CHECK(matrix.data[0][1] == 2);
        CHECK(matrix.data[0][2] == 3);
        CHECK(matrix.data[1][0] == 4);
        CHECK(matrix.data[1][1] == 5);
        CHECK(matrix.data[1][2] == 6);
    }
}

TEST_CASE("Vector JSON Reflection - Edge Cases") {
    SUBCASE("Empty vector") {
        Shape empty_shape;
        empty_shape.name = "Empty";
        empty_shape.id = 0;
        empty_shape.points = {};  // Empty vector
        
        auto json = reflection::to_json(empty_shape);
        CHECK(json["points"].is_array());
        CHECK(json["points"].empty());
        
        auto restored = reflection::from_json<Shape>(json);
        CHECK(restored.points.empty());
    }
    
    SUBCASE("Vector of primitives") {        
        NumberList list;
        list.description = "Prime numbers";
        list.numbers = {2, 3, 5, 7, 11, 13};
        
        auto json = reflection::to_json(list);
        CHECK(json["numbers"].is_array());
        CHECK(json["numbers"].size() == 6);
        CHECK(json["numbers"][0].get<int>() == 2);
        CHECK(json["numbers"][5].get<int>() == 13);
        
        auto restored = reflection::from_json<NumberList>(json);
        CHECK(restored.numbers.size() == 6);
        CHECK(restored.numbers[0] == 2);
        CHECK(restored.numbers[5] == 13);
    }
}