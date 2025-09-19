#include "doctest.h"
#include "reflection.hpp"
#include <vector>
#include <fstream>

using namespace reflection;

// Test structs
struct Point3D {
    double x;
    double y;
    double z;
};

struct PersonInfo {
    std::string name;
    int age;
    bool is_active;
    double salary;
};

struct RobotState {
    Point3D position;
    Point3D velocity;
    std::string status;
    double battery_level;
    bool is_connected;
};

class RobotStateClass {
public:
    Point3D position;
    Point3D velocity;
    std::string status;
    double battery_level;
    bool is_connected;
};

// Define custom field names for PersonInfo to satisfy the reflection test
DEFINE_FIELD_NAMES(PersonInfo, "name", "age", "is_active", "salary")

TEST_CASE("Basic Reflection - Struct to JSON") {
    Point3D point{1.5, 2.7, 3.9};
    auto json = json::to_json(point);
    
    CHECK(json["x"].get<double>() == 1.5);
    CHECK(json["y"].get<double>() == 2.7);
    CHECK(json["z"].get<double>() == 3.9);
}

TEST_CASE("Basic Reflection - JSON to Struct") {
    nlohmann::json json = {{"x", 4.2}, {"y", 5.8}, {"z", 6.1}};
    auto point = json::from_json<Point3D>(json);
    
    CHECK(point.x == 4.2);
    CHECK(point.y == 5.8);
    CHECK(point.z == 6.1);
}

TEST_CASE("Basic Reflection - Complex Struct") {
    PersonInfo person{"John Doe", 30, true, 75000.50};
    auto json = json::to_json(person);
    
    CHECK(json["name"].get<std::string>() == "John Doe");
    CHECK(json["age"].get<int>() == 30);
    CHECK(json["is_active"].get<bool>() == true);
    CHECK(json["salary"].get<double>() == 75000.50);
}

TEST_CASE("Nested Structs") {
    RobotState robot{
        {10.0, 20.0, 30.0},     // position
        {1.0, 2.0, 3.0},        // velocity
        "OPERATIONAL",          // status
        85.5,                   // battery_level
        true                    // is_connected
    };
    
    auto json = reflection::json::to_json(robot);
    
    SUBCASE("Position fields") {
        CHECK(json["position"]["x"].get<double>() == 10.0);
        CHECK(json["position"]["y"].get<double>() == 20.0);
        CHECK(json["position"]["z"].get<double>() == 30.0);
    }
    
    SUBCASE("Velocity fields") {
        CHECK(json["velocity"]["x"].get<double>() == 1.0);
        CHECK(json["velocity"]["y"].get<double>() == 2.0);
        CHECK(json["velocity"]["z"].get<double>() == 3.0);
    }
    
    SUBCASE("Other fields") {
        CHECK(json["status"].get<std::string>() == "OPERATIONAL");
        CHECK(json["battery_level"].get<double>() == 85.5);
        CHECK(json["is_connected"].get<bool>() == true);
    }
}

TEST_CASE("Class Reflection") {
    RobotStateClass robot;
    robot.position = {50.0, 60.0, 70.0};
    robot.velocity = {5.0, 6.0, 7.0};
    robot.status = "MOVING";
    robot.battery_level = 45.3;
    robot.is_connected = true;
    
    auto json = json::to_json(robot);
    
    CHECK(json["position"]["x"].get<double>() == 50.0);
    CHECK(json["status"].get<std::string>() == "MOVING");
    CHECK(json["battery_level"].get<double>() == 45.3);
    CHECK(json["is_connected"].get<bool>() == true);
}

TEST_CASE("Schema Generation") {
    SUBCASE("Point3D schema") {
        auto schema = json::get_schema<Point3D>();
        
        CHECK(schema["type"].get<std::string>() == "object");
        CHECK(schema["properties"].contains("x"));
        CHECK(schema["properties"].contains("y"));
        CHECK(schema["properties"].contains("z"));
        CHECK(schema["properties"]["x"]["type"].get<std::string>() == "number");
    }
    
    SUBCASE("PersonInfo schema") {
        auto schema = json::get_schema<PersonInfo>();
        
        CHECK(schema["type"].get<std::string>() == "object");
        CHECK(schema["properties"].contains("name"));
        CHECK(schema["properties"].contains("age"));
        CHECK(schema["properties"].contains("is_active"));
        CHECK(schema["properties"].contains("salary"));
        
        CHECK(schema["properties"]["name"]["type"].get<std::string>() == "string");
        CHECK(schema["properties"]["age"]["type"].get<std::string>() == "integer");
        CHECK(schema["properties"]["is_active"]["type"].get<std::string>() == "boolean");
        CHECK(schema["properties"]["salary"]["type"].get<std::string>() == "number");
    }
}

TEST_CASE("PFR Capabilities") {
    Point3D point{1, 2, 3};
    
    SUBCASE("Field count") {
        constexpr auto field_count = boost::pfr::tuple_size_v<Point3D>;
        CHECK(field_count == 3);
    }
    
    SUBCASE("Field access") {
        CHECK(boost::pfr::get<0>(point) == 1.0);
        CHECK(boost::pfr::get<1>(point) == 2.0);
        CHECK(boost::pfr::get<2>(point) == 3.0);
    }
}

TEST_CASE("Reflection Info") {
    SUBCASE("Point3D info") {
        auto info = json::get_reflection_info<Point3D>();
        
        CHECK(info["is_aggregate"].get<bool>() == true);
        CHECK(info["field_info"]["field_count"].get<int>() == 3);
        CHECK(info["pfr_names_enabled"].get<bool>() == true);
    }
    
    SUBCASE("PersonInfo info") {
        auto info = json::get_reflection_info<PersonInfo>();
        
        CHECK(info["is_aggregate"].get<bool>() == true);
        CHECK(info["field_info"]["field_count"].get<int>() == 4);
        CHECK(info["has_custom_field_names"].get<bool>() == true);
    }
}

TEST_CASE("Vector of Structs") {
    std::vector<Point3D> points = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0},
        {7.0, 8.0, 9.0}
    };
    
    // Convert each point to JSON manually since we don't have vector serialization
    nlohmann::json json = nlohmann::json::array();
    for (const auto& point : points) {
        json.push_back(json::to_json(point));
    }
    
    CHECK(json.is_array());
    CHECK(json.size() == 3);
    CHECK(json[0]["x"].get<double>() == 1.0);
    CHECK(json[1]["y"].get<double>() == 5.0);
    CHECK(json[2]["z"].get<double>() == 9.0);
}

TEST_CASE("File I/O") {
    PersonInfo person{"Alice", 25, false, 60000.0};
    
    SUBCASE("Write and read JSON file") {
        // Write to file
        auto json = json::to_json(person);
        std::ofstream file("test_output.json");
        file << json.dump(2);
        file.close();
        
        // Read from file
        std::ifstream infile("test_output.json");
        nlohmann::json loaded_json;
        infile >> loaded_json;
        infile.close();
        
        auto loaded_person = json::from_json<PersonInfo>(loaded_json);
        
        CHECK(loaded_person.name == person.name);
        CHECK(loaded_person.age == person.age);
        CHECK(loaded_person.is_active == person.is_active);
        CHECK(loaded_person.salary == person.salary);
        
        // Clean up
        std::remove("test_output.json");
    }
}