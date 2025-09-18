#include "../third_party/doctest.h"
#include "../include/reflect_json/reflect_json.hpp"
#include <vector>
#include <string>

using namespace reflect_json;

// Test structures for array support
struct Item {
    std::string name;
    int value;
};

struct ItemList {
    std::vector<Item> items;
    std::string description;
};

struct NestedArrayExample {
    std::string title;
    std::vector<ItemList> lists;
    int count;
};

TEST_CASE("Array Support - Path Parsing") {
    SUBCASE("Simple array access") {
        auto parts = reflection::parse_path_enhanced("items[0]");
        REQUIRE(parts.size() == 2);
        
        CHECK(parts[0].is_field_access());
        CHECK(parts[0].field_name == "items");
        
        CHECK(parts[1].is_array_access());
        CHECK(*parts[1].array_index == 0);
    }
    
    SUBCASE("Nested field after array") {
        auto parts = reflection::parse_path_enhanced("items[2].name");
        REQUIRE(parts.size() == 3);
        
        CHECK(parts[0].is_field_access());
        CHECK(parts[0].field_name == "items");
        
        CHECK(parts[1].is_array_access());
        CHECK(*parts[1].array_index == 2);
        
        CHECK(parts[2].is_field_access());
        CHECK(parts[2].field_name == "name");
    }
    
    SUBCASE("Multiple array accesses") {
        auto parts = reflection::parse_path_enhanced("lists[1].items[3].value");
        REQUIRE(parts.size() == 5);
        
        CHECK(parts[0].field_name == "lists");
        CHECK(*parts[1].array_index == 1);
        CHECK(parts[2].field_name == "items");
        CHECK(*parts[3].array_index == 3);
        CHECK(parts[4].field_name == "value");
    }
    
    SUBCASE("Invalid index handling") {
        auto parts = reflection::parse_path_enhanced("items[abc]");
        // Should only have the field name, invalid index should be ignored
        CHECK(parts.size() == 1);
        CHECK(parts[0].field_name == "items");
    }
}

TEST_CASE("Array Support - Get Array Elements") {
    ItemList list;
    list.description = "Test List";
    list.items = {
        {"Item A", 10},
        {"Item B", 20},
        {"Item C", 30}
    };
    
    SUBCASE("Get entire array") {
        auto result = reflection::get_field_enhanced(list, "items");
        REQUIRE(result.has_value());
        CHECK(result->is_array());
        CHECK(result->size() == 3);
    }
    
    SUBCASE("Get array element") {
        auto result = reflection::get_field_enhanced(list, "items[1]");
        REQUIRE(result.has_value());
        CHECK((*result)["name"].get<std::string>() == "Item B");
        CHECK((*result)["value"].get<int>() == 20);
    }
    
    SUBCASE("Get field from array element") {
        auto result = reflection::get_field_enhanced(list, "items[0].name");
        REQUIRE(result.has_value());
        CHECK(result->get<std::string>() == "Item A");
        
        auto result2 = reflection::get_field_enhanced(list, "items[2].value");
        REQUIRE(result2.has_value());
        CHECK(result2->get<int>() == 30);
    }
    
    SUBCASE("Out of bounds access") {
        auto result = reflection::get_field_enhanced(list, "items[5]");
        CHECK_FALSE(result.has_value());
        
        auto result2 = reflection::get_field_enhanced(list, "items[10].name");
        CHECK_FALSE(result2.has_value());
    }
}

TEST_CASE("Array Support - Set Array Elements") {
    ItemList list;
    list.description = "Test List";
    list.items = {
        {"Item A", 10},
        {"Item B", 20},
        {"Item C", 30}
    };
    
    SUBCASE("Set field in array element") {
        bool success = reflection::set_field_enhanced(list, "items[1].name", nlohmann::json("Modified Item"));
        CHECK(success);
        CHECK(list.items[1].name == "Modified Item");
        CHECK(list.items[1].value == 20); // Should be unchanged
    }
    
    SUBCASE("Set value in array element") {
        bool success = reflection::set_field_enhanced(list, "items[0].value", nlohmann::json(99));
        CHECK(success);
        CHECK(list.items[0].value == 99);
        CHECK(list.items[0].name == "Item A"); // Should be unchanged
    }
    
    SUBCASE("Set with type conversion") {
        bool success = reflection::set_field_enhanced(list, "items[2].value", nlohmann::json("42"));
        CHECK(success);
        CHECK(list.items[2].value == 42);
    }
    
    SUBCASE("Out of bounds setting") {
        bool success = reflection::set_field_enhanced(list, "items[5].name", nlohmann::json("Should Fail"));
        CHECK_FALSE(success);
        
        // Original data should be unchanged
        CHECK(list.items.size() == 3);
        CHECK(list.items[0].name == "Item A");
    }
}

TEST_CASE("Array Support - Nested Arrays") {
    NestedArrayExample data;
    data.title = "Complex Example";
    data.count = 42;
    data.lists = {
        {
            {{"Alpha", 1}, {"Beta", 2}},
            "First List"
        },
        {
            {{"Gamma", 3}, {"Delta", 4}, {"Epsilon", 5}},
            "Second List"
        }
    };
    
    SUBCASE("Access nested array elements") {
        // Access first list's description
        auto result1 = reflection::get_field_enhanced(data, "lists[0].description");
        REQUIRE(result1.has_value());
        CHECK(result1->get<std::string>() == "First List");
        
        // Access item in second list
        auto result2 = reflection::get_field_enhanced(data, "lists[1].items[2].name");
        REQUIRE(result2.has_value());
        CHECK(result2->get<std::string>() == "Epsilon");
        
        // Access item value in first list
        auto result3 = reflection::get_field_enhanced(data, "lists[0].items[1].value");
        REQUIRE(result3.has_value());
        CHECK(result3->get<int>() == 2);
    }
    
    SUBCASE("Modify nested array elements") {
        // Modify description in second list
        bool success1 = reflection::set_field_enhanced(data, "lists[1].description", nlohmann::json("Modified List"));
        CHECK(success1);
        CHECK(data.lists[1].description == "Modified List");
        
        // Modify item name in first list
        bool success2 = reflection::set_field_enhanced(data, "lists[0].items[0].name", nlohmann::json("Modified Alpha"));
        CHECK(success2);
        CHECK(data.lists[0].items[0].name == "Modified Alpha");
        CHECK(data.lists[0].items[0].value == 1); // Should be unchanged
    }
}

TEST_CASE("Array Support - Edge Cases") {
    ItemList list;
    list.items = {{"Single", 100}};
    list.description = "Single Item";
    
    SUBCASE("Empty path") {
        auto result = reflection::get_field_enhanced(list, "");
        CHECK_FALSE(result.has_value());
        
        bool success = reflection::set_field_enhanced(list, "", nlohmann::json("value"));
        CHECK_FALSE(success);
    }
    
    SUBCASE("Array access on non-array field") {
        auto result = reflection::get_field_enhanced(list, "description[0]");
        CHECK_FALSE(result.has_value());
        
        bool success = reflection::set_field_enhanced(list, "description[0]", nlohmann::json("value"));
        CHECK_FALSE(success);
    }
    
    SUBCASE("Field access on array without index") {
        // This should work - returns the entire array as JSON
        auto result = reflection::get_field_enhanced(list, "items");
        REQUIRE(result.has_value());
        CHECK(result->is_array());
        CHECK(result->size() == 1);
    }
}