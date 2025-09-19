#include "doctest.h"
#include "reflection.hpp"

using namespace reflection;

// Test structures
struct Address {
    std::string street;
    std::string city;
    int zip_code;
};

struct Person {
    std::string name;
    int age;
    Address address;
    bool active;
};

TEST_CASE("Path Navigation - Basic Field Access") {
    Person person{"John Doe", 30, {"123 Main St", "Anytown", 12345}, true};
    
    SUBCASE("Get field values") {
        auto name_result = get_field(person, "name");
        REQUIRE(name_result.has_value());
        CHECK(name_result->get<std::string>() == "John Doe");
        
        auto age_result = get_field(person, "age");
        REQUIRE(age_result.has_value());
        CHECK(age_result->get<int>() == 30);
        
        auto active_result = get_field(person, "active");
        REQUIRE(active_result.has_value());
        CHECK(active_result->get<bool>() == true);
    }
    
    SUBCASE("Set field values") {
        CHECK(set_field(person, "name", nlohmann::json("Jane Smith")));
        CHECK(person.name == "Jane Smith");
        
        CHECK(set_field(person, "age", nlohmann::json(35)));
        CHECK(person.age == 35);
        
        CHECK(set_field(person, "active", nlohmann::json(false)));
        CHECK(person.active == false);
    }
}

TEST_CASE("Path Navigation - Nested Struct Access") {
    Person person{"Alice", 25, {"456 Oak Ave", "Springfield", 67890}, false};
    
    SUBCASE("Get nested field values") {
        auto street_result = get_field(person, "address.street");
        REQUIRE(street_result.has_value());
        CHECK(street_result->get<std::string>() == "456 Oak Ave");
        
        auto city_result = get_field(person, "address.city");
        REQUIRE(city_result.has_value());
        CHECK(city_result->get<std::string>() == "Springfield");
        
        auto zip_result = get_field(person, "address.zip_code");
        REQUIRE(zip_result.has_value());
        CHECK(zip_result->get<int>() == 67890);
    }
    
    SUBCASE("Set nested field values") {
        CHECK(set_field(person, "address.street", nlohmann::json("789 Pine St")));
        CHECK(person.address.street == "789 Pine St");
        
        CHECK(set_field(person, "address.city", nlohmann::json("New York")));
        CHECK(person.address.city == "New York");
        
        CHECK(set_field(person, "address.zip_code", nlohmann::json(10001)));
        CHECK(person.address.zip_code == 10001);
    }
}

TEST_CASE("Path Navigation - Type Conversion") {
    Person person{"Bob", 40, {"111 Elm St", "Boston", 02101}, true};
    
    SUBCASE("String to number conversion") {
        CHECK(set_field(person, "age", nlohmann::json("42")));
        CHECK(person.age == 42);
        
        CHECK(set_field(person, "address.zip_code", nlohmann::json("90210")));
        CHECK(person.address.zip_code == 90210);
    }
    
    SUBCASE("String to boolean conversion") {
        CHECK(set_field(person, "active", nlohmann::json("false")));
        CHECK(person.active == false);
        
        CHECK(set_field(person, "active", nlohmann::json("true")));
        CHECK(person.active == true);
        
        CHECK(set_field(person, "active", nlohmann::json("1")));
        CHECK(person.active == true);
        
        CHECK(set_field(person, "active", nlohmann::json("0")));
        CHECK(person.active == false);
    }
    
    SUBCASE("Number to boolean conversion") {
        CHECK(set_field(person, "active", nlohmann::json(1)));
        CHECK(person.active == true);
        
        CHECK(set_field(person, "active", nlohmann::json(0)));
        CHECK(person.active == false);
    }
}

TEST_CASE("Path Navigation - Path Validation") {
    SUBCASE("Valid paths") {
        CHECK(is_valid_path<Person>("name"));
        CHECK(is_valid_path<Person>("age"));
        CHECK(is_valid_path<Person>("address"));
        CHECK(is_valid_path<Person>("active"));
        CHECK(is_valid_path<Person>("address.street"));
        CHECK(is_valid_path<Person>("address.city"));
        CHECK(is_valid_path<Person>("address.zip_code"));
    }
    
    SUBCASE("Invalid paths") {
        CHECK_FALSE(is_valid_path<Person>("nonexistent"));
        CHECK_FALSE(is_valid_path<Person>("address.nonexistent"));
        CHECK_FALSE(is_valid_path<Person>("name.invalid"));
        CHECK_FALSE(is_valid_path<Person>("age.invalid"));
    }
}

TEST_CASE("Path Navigation - Path Discovery") {
    auto paths = get_all_paths<Person>();
    
    SUBCASE("Contains all expected paths") {
        std::vector<std::string> expected_paths = {
            "name", "age", "address", "active",
            "address.street", "address.city", "address.zip_code"
        };
        
        for (const auto& expected : expected_paths) {
            CHECK(std::find(paths.begin(), paths.end(), expected) != paths.end());
        }
    }
    
    SUBCASE("Correct number of paths") {
        CHECK(paths.size() == 7);
    }
}

TEST_CASE("Path Navigation - Error Handling") {
    Person person{"Carol", 28, {"222 Maple Dr", "Seattle", 98101}, false};
    
    SUBCASE("Invalid field names return nullopt/false") {
        auto invalid_result = get_field(person, "invalid_field");
        CHECK_FALSE(invalid_result.has_value());
        
        CHECK_FALSE(set_field(person, "invalid_field", nlohmann::json("value")));
    }
    
    SUBCASE("Invalid nested paths return nullopt/false") {
        auto invalid_nested = get_field(person, "address.invalid");
        CHECK_FALSE(invalid_nested.has_value());
        
        CHECK_FALSE(set_field(person, "address.invalid", nlohmann::json("value")));
    }
    
    SUBCASE("Empty paths are invalid") {
        auto empty_result = get_field(person, "");
        CHECK_FALSE(empty_result.has_value());
        
        CHECK_FALSE(set_field(person, "", nlohmann::json("value")));
        CHECK_FALSE(is_valid_path<Person>(""));
    }
}

TEST_CASE("Path Navigation - Path Parsing") {
    SUBCASE("Single field path") {
        auto parts = parse_path("name");
        REQUIRE(parts.size() == 1);
        CHECK(parts[0] == "name");
    }
    
    SUBCASE("Nested path") {
        auto parts = parse_path("address.street");
        REQUIRE(parts.size() == 2);
        CHECK(parts[0] == "address");
        CHECK(parts[1] == "street");
    }
    
    SUBCASE("Deep nested path") {
        auto parts = parse_path("a.b.c.d");
        REQUIRE(parts.size() == 4);
        CHECK(parts[0] == "a");
        CHECK(parts[1] == "b");
        CHECK(parts[2] == "c");
        CHECK(parts[3] == "d");
    }
    
    SUBCASE("Empty path") {
        auto parts = parse_path("");
        CHECK(parts.empty());
    }
}