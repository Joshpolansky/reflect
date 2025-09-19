#include "doctest.h"
#include "reflection.hpp"
#include <chrono>

using namespace reflection;

// Test enum types
enum class Status { ACTIVE, INACTIVE, PENDING };
enum class Priority { LOW = 1, MEDIUM = 5, HIGH = 10 };
enum OldStyleEnum { OPTION_A, OPTION_B, OPTION_C };

// Register enum mappings
REGISTER_ENUM(Status,
    {Status::ACTIVE, "active"},
    {Status::INACTIVE, "inactive"}, 
    {Status::PENDING, "pending"}
);

REGISTER_ENUM(Priority,
    {Priority::LOW, "low"},
    {Priority::MEDIUM, "medium"},
    {Priority::HIGH, "high"}
);

REGISTER_ENUM(OldStyleEnum,
    {OPTION_A, "option_a"},
    {OPTION_B, "option_b"},
    {OPTION_C, "option_c"}
);

// Test structures
struct TaskConfig {
    std::string name;
    Status status;
    Priority priority;
    OldStyleEnum option;
    std::chrono::seconds timeout;
    std::chrono::minutes interval;
    std::chrono::duration<double> precise_duration;
};

struct ServerConfig {
    std::string host;
    int port;
    std::chrono::seconds connection_timeout;
    std::chrono::minutes health_check_interval;
    Status server_status;
};

TEST_CASE("Type Conversions - Enum Support") {
    TaskConfig config;
    config.name = "test";
    config.status = Status::ACTIVE;
    config.priority = Priority::HIGH;
    config.option = OPTION_B;
    
    SUBCASE("Enum from string") {
        CHECK(set_field(config, "status", nlohmann::json("inactive")));
        CHECK(config.status == Status::INACTIVE);
        
        CHECK(set_field(config, "priority", nlohmann::json("low")));
        CHECK(config.priority == Priority::LOW);
        
        CHECK(set_field(config, "option", nlohmann::json("option_c")));
        CHECK(config.option == OPTION_C);
    }
    
    SUBCASE("Enum from integer") {
        CHECK(set_field(config, "status", nlohmann::json(2))); // PENDING
        CHECK(config.status == Status::PENDING);
        
        CHECK(set_field(config, "priority", nlohmann::json(5))); // MEDIUM
        CHECK(config.priority == Priority::MEDIUM);
        
        CHECK(set_field(config, "option", nlohmann::json(0))); // OPTION_A
        CHECK(config.option == OPTION_A);
    }
    
    SUBCASE("Invalid enum conversion") {
        Status original_status = config.status;
        CHECK_FALSE(set_field(config, "status", nlohmann::json("invalid_status")));
        CHECK(config.status == original_status); // Should remain unchanged
        
        CHECK_FALSE(set_field(config, "priority", nlohmann::json("not_a_priority")));
    }
    
    SUBCASE("Get enum as JSON") {
        config.status = Status::PENDING;
        auto result = get_field(config, "status");
        REQUIRE(result.has_value());
        // Note: get_field returns the raw enum value as JSON, conversion to string 
        // would happen in a higher-level API
    }
}

TEST_CASE("Type Conversions - Duration Support") {
    TaskConfig config;
    
    SUBCASE("Duration from string - seconds") {
        CHECK(set_field(config, "timeout", nlohmann::json("30s")));
        CHECK(config.timeout.count() == 30);
        
        CHECK(set_field(config, "timeout", nlohmann::json("45")));  // Default unit is seconds
        CHECK(config.timeout.count() == 45);
    }
    
    SUBCASE("Duration from string - minutes") {
        CHECK(set_field(config, "interval", nlohmann::json("5m")));
        CHECK(config.interval.count() == 5);
        
        // Setting seconds in a minutes field
        CHECK(set_field(config, "interval", nlohmann::json("120s")));
        CHECK(config.interval.count() == 2); // 120 seconds = 2 minutes
    }
    
    SUBCASE("Duration from string - hours") {
        CHECK(set_field(config, "precise_duration", nlohmann::json("2h")));
        auto hours_in_seconds = std::chrono::duration_cast<std::chrono::seconds>(config.precise_duration);
        CHECK(hours_in_seconds.count() == 7200); // 2 hours = 7200 seconds
    }
    
    SUBCASE("Duration from string - days") {
        CHECK(set_field(config, "precise_duration", nlohmann::json("1d")));
        auto days_in_seconds = std::chrono::duration_cast<std::chrono::seconds>(config.precise_duration);
        CHECK(days_in_seconds.count() == 86400); // 1 day = 86400 seconds
    }
    
    SUBCASE("Duration from number (seconds)") {
        CHECK(set_field(config, "timeout", nlohmann::json(60)));
        CHECK(config.timeout.count() == 60);
        
        CHECK(set_field(config, "precise_duration", nlohmann::json(3.5)));
        CHECK(config.precise_duration.count() == 3.5);
    }
    
    SUBCASE("Invalid duration format") {
        auto original_timeout = config.timeout;
        CHECK_FALSE(set_field(config, "timeout", nlohmann::json("invalid_duration")));
        CHECK(config.timeout == original_timeout);
        
        CHECK_FALSE(set_field(config, "timeout", nlohmann::json("30x"))); // Invalid unit
    }
    
    SUBCASE("Decimal durations") {
        CHECK(set_field(config, "precise_duration", nlohmann::json("2.5s")));
        CHECK(config.precise_duration.count() == 2.5);
        
        CHECK(set_field(config, "precise_duration", nlohmann::json("1.5m")));
        CHECK(config.precise_duration.count() == 90.0); // 1.5 minutes = 90 seconds
    }
}

TEST_CASE("Type Conversions - Complex Configuration") {
    ServerConfig server;
    server.host = "localhost";
    server.port = 8080;
    
    SUBCASE("Complete configuration from JSON-like values") {
        CHECK(set_field(server, "host", nlohmann::json("production.example.com")));
        CHECK(set_field(server, "port", nlohmann::json("443")));  // String to int
        CHECK(set_field(server, "connection_timeout", nlohmann::json("30s")));
        CHECK(set_field(server, "health_check_interval", nlohmann::json("5m")));
        CHECK(set_field(server, "server_status", nlohmann::json("active")));
        
        CHECK(server.host == "production.example.com");
        CHECK(server.port == 443);
        CHECK(server.connection_timeout.count() == 30);
        CHECK(server.health_check_interval.count() == 5);
        CHECK(server.server_status == Status::ACTIVE);
    }
    
    SUBCASE("Mixed value types") {
        // Demonstrate that the system handles different JSON value types correctly
        CHECK(set_field(server, "port", nlohmann::json(9000)));           // JSON number
        CHECK(set_field(server, "connection_timeout", nlohmann::json("45s"))); // String duration  
        CHECK(set_field(server, "health_check_interval", nlohmann::json(10)));  // Number as minutes
        CHECK(set_field(server, "server_status", nlohmann::json(0)));     // Integer as enum
        
        CHECK(server.port == 9000);
        CHECK(server.connection_timeout.count() == 45);
        CHECK(server.health_check_interval.count() == 10);
        CHECK(server.server_status == Status::ACTIVE); // 0 maps to first enum value
    }
}

TEST_CASE("Type Conversions - Edge Cases") {
    TaskConfig config;
    
    SUBCASE("Case sensitivity in enum strings") {
        CHECK(set_field(config, "status", nlohmann::json("ACTIVE"))); // Should work
        CHECK(set_field(config, "status", nlohmann::json("Active"))); // Mixed case should now work too (case-insensitive)
        CHECK_FALSE(set_field(config, "status", nlohmann::json("INVALID"))); // Invalid value should fail
    }
    
    SUBCASE("Whitespace handling in durations") {
        CHECK(set_field(config, "timeout", nlohmann::json(" 30s "))); // With spaces
        CHECK(config.timeout.count() == 30);
    }
    
    SUBCASE("Zero values") {
        CHECK(set_field(config, "timeout", nlohmann::json("0s")));
        CHECK(config.timeout.count() == 0);
        
        CHECK(set_field(config, "timeout", nlohmann::json(0)));
        CHECK(config.timeout.count() == 0);
    }
}