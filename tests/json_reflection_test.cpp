#include <iostream>
#include <cassert>
#include <vector>
#include <fstream>
#include <reflect_json/json_reflection.hpp>

// Example structs to demonstrate the reflection utility

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
class RobotStateClassBase { 
    // int PrivateField = 42;
public:
    int PublicField = 31415;
};

class RobotStateClass{
public:
    Point3D position;
    Point3D velocity;
    std::string status;
    double battery_level;
    bool is_connected;
};

void test_basic_reflection() {
    std::cout << "=== Testing Basic Reflection ===" << std::endl;
    
    // Create test objects
    Point3D point{1.5, 2.7, 3.9};
    PersonInfo person{"John Doe", 30, true, 75000.50};
    
    // Test basic serialization
    auto point_json = reflect_json::to_json(point);
    auto person_json = reflect_json::to_json(person);
    
    std::cout << "Point3D as JSON: " << point_json.dump(2) << std::endl;
    std::cout << "PersonInfo as JSON: " << person_json.dump(2) << std::endl;
    
    // Convert back from JSON
    auto point_restored = reflect_json::from_json<Point3D>(point_json);
    auto person_restored = reflect_json::from_json<PersonInfo>(person_json);
    
    // Verify data integrity
    assert(point_restored.x == point.x);
    assert(point_restored.y == point.y);
    assert(point_restored.z == point.z);
    
    assert(person_restored.name == person.name);
    assert(person_restored.age == person.age);
    assert(person_restored.is_active == person.is_active);
    assert(person_restored.salary == person.salary);
    
    std::cout << "✓ Basic reflection tests passed!" << std::endl << std::endl;
}

void test_nested_structs() {
    std::cout << "=== Testing Nested Structs ===" << std::endl;
    
    RobotState robot{
        {10.0, 20.0, 30.0},  // position
        {1.0, 2.0, 3.0},     // velocity
        "OPERATIONAL",        // status
        85.5,                // battery_level
        true                 // is_connected
    };
    
    // Convert to JSON
    auto robot_json = reflect_json::to_json(robot);
    std::cout << "RobotState as JSON: " << robot_json.dump(2) << std::endl;
    
    // Convert back
    auto robot_restored = reflect_json::from_json<RobotState>(robot_json);
    
    // Verify nested data
    assert(robot_restored.position.x == robot.position.x);
    assert(robot_restored.position.y == robot.position.y);
    assert(robot_restored.position.z == robot.position.z);
    assert(robot_restored.velocity.x == robot.velocity.x);
    assert(robot_restored.status == robot.status);
    assert(robot_restored.battery_level == robot.battery_level);
    assert(robot_restored.is_connected == robot.is_connected);
    
    std::cout << "✓ Nested struct tests passed!" << std::endl << std::endl;
}

void test_schema_generation() {
    std::cout << "=== Testing Schema Generation ===" << std::endl;
    
    auto point_schema = reflect_json::reflection::get_schema<Point3D>();
    auto person_schema = reflect_json::reflection::get_schema<PersonInfo>();
    auto robot_schema = reflect_json::reflection::get_schema<RobotState>();
    
    std::cout << "Point3D schema: " << point_schema.dump(2) << std::endl;
    std::cout << "PersonInfo schema: " << person_schema.dump(2) << std::endl;
    std::cout << "RobotState schema: " << robot_schema.dump(2) << std::endl;
    
    std::cout << "✓ Schema generation tests passed!" << std::endl << std::endl;
}

void test_pfr_capabilities() {
    std::cout << "=== Testing PFR Capabilities ===" << std::endl;
    
    // Test what PFR can tell us about our structs
    auto point_info = reflect_json::reflection::get_reflection_info<Point3D>();
    auto person_info = reflect_json::reflection::get_reflection_info<PersonInfo>();
    auto robot_info = reflect_json::reflection::get_reflection_info<RobotState>();
    
    std::cout << "Point3D reflection info: " << point_info.dump(2) << std::endl;
    std::cout << "PersonInfo reflection info: " << person_info.dump(2) << std::endl;
    std::cout << "RobotState reflection info: " << robot_info.dump(2) << std::endl;
    
    // Demonstrate field access without names
    Point3D point{1.0, 2.0, 3.0};
    
    std::cout << "\nDirect field access using PFR:" << std::endl;
    std::cout << "point field 0: " << boost::pfr::get<0>(point) << std::endl;
    std::cout << "point field 1: " << boost::pfr::get<1>(point) << std::endl;  
    std::cout << "point field 2: " << boost::pfr::get<2>(point) << std::endl;
    
    // Show field iteration
    std::cout << "\nIterating over fields:" << std::endl;
    int index = 0;
    boost::pfr::for_each_field(point, [&index](const auto& field) {
        std::cout << "Field " << index++ << ": " << field << std::endl;
    });
    
    std::cout << "✓ PFR capabilities demonstration completed!" << std::endl << std::endl;
}

void test_json_integration() {
    std::cout << "=== Testing JSON Reflection Integration ===" << std::endl;
    
    PersonInfo person{"Alice", 25, false, 60000.0};
    
    // Using explicit JSON reflection calls
    nlohmann::json j = reflect_json::reflection::to_json(person);
    std::cout << "Explicit to_json conversion: " << j.dump(2) << std::endl;
    
    PersonInfo person_restored = reflect_json::reflection::from_json<PersonInfo>(j);
    
    assert(person_restored.name == person.name);
    assert(person_restored.age == person.age);
    assert(person_restored.is_active == person.is_active);
    assert(person_restored.salary == person.salary);
    
    std::cout << "✓ nlohmann::json integration tests passed!" << std::endl << std::endl;
}

void test_vector_of_structs() {
    std::cout << "=== Testing Vector of Structs ===" << std::endl;
    
    std::vector<Point3D> points = {
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0},
        {7.0, 8.0, 9.0}
    };
    
    // Convert vector to JSON manually
    nlohmann::json points_json = nlohmann::json::array();
    for (const auto& point : points) {
        points_json.push_back(reflect_json::reflection::to_json(point));
    }
    std::cout << "Vector of Point3D as JSON: " << points_json.dump(2) << std::endl;
    
    // Convert back
    std::vector<Point3D> points_restored;
    for (const auto& json_point : points_json) {
        points_restored.push_back(reflect_json::reflection::from_json<Point3D>(json_point));
    }
    
    assert(points_restored.size() == points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        assert(points_restored[i].x == points[i].x);
        assert(points_restored[i].y == points[i].y);
        assert(points_restored[i].z == points[i].z);
    }
    
    std::cout << "✓ Vector of structs tests passed!" << std::endl << std::endl;
}

void test_file_io() {
    std::cout << "=== Testing File I/O ===" << std::endl;
    
    RobotState robot{
        {100.0, 200.0, 300.0},  // position
        {10.0, 20.0, 30.0},     // velocity
        "IDLE",                  // status
        95.7,                   // battery_level
        false                   // is_connected
    };
    
    // Save to file
    auto robot_json = reflect_json::to_json(robot);
    std::ofstream file("test_robot_state.json");
    file << robot_json.dump(2);
    file.close();
    
    // Load from file
    std::ifstream input_file("test_robot_state.json");
    nlohmann::json loaded_json;
    input_file >> loaded_json;
    input_file.close();
    
    auto robot_loaded = reflect_json::from_json<RobotState>(loaded_json);
    
    assert(robot_loaded.position.x == robot.position.x);
    assert(robot_loaded.status == robot.status);
    assert(robot_loaded.battery_level == robot.battery_level);
    
    std::cout << "✓ File I/O tests passed!" << std::endl << std::endl;
    
    // Clean up
    std::remove("test_robot_state.json");
}

void test_class(){

    std::cout << "=== Testing Class Reflection ===" << std::endl;
    
    RobotStateClass robot{
        {50.0, 60.0, 70.0},  // position
        {5.0, 6.0, 7.0},     // velocity
        "MOVING",            // status
        45.3,               // battery_level
        true                // is_connected
    };
    
    // Convert to JSON
    auto robot_json = reflect_json::to_json(robot);
    std::cout << "RobotStateClass as JSON: " << robot_json.dump(2) << std::endl;
    
    // Convert back
    auto robot_restored = reflect_json::from_json<RobotStateClass>(robot_json);
    
    // Verify nested data
    assert(robot_restored.position.x == robot.position.x);
    assert(robot_restored.velocity.y == robot.velocity.y);
    assert(robot_restored.status == robot.status);
    assert(robot_restored.battery_level == robot.battery_level);
    assert(robot_restored.is_connected == robot.is_connected);
    
    std::cout << "✓ Class reflection tests passed!" << std::endl << std::endl;
}

int main() {
    try {
        std::cout << "JSON Reflection Utility Test Suite" << std::endl;
        std::cout << "===================================" << std::endl << std::endl;
        
        test_basic_reflection();
        test_nested_structs();
        test_class();
        test_schema_generation();
        test_pfr_capabilities();
        test_json_integration();
        test_vector_of_structs();
        test_file_io();
        
        std::cout << "🎉 All tests passed! The JSON reflection utility is working correctly." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
