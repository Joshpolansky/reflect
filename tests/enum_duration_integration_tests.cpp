#include "doctest.h"
#include "reflection.hpp"
#include <chrono>

using namespace reflection;

// Test enum types
enum class LogLevel { DEBUG, INFO, WARN, ERROR };
enum class Priority { LOW = 1, MEDIUM = 5, HIGH = 10 };

// Register the enums
REGISTER_ENUM(LogLevel,
    {LogLevel::DEBUG, "debug"},
    {LogLevel::INFO, "info"}, 
    {LogLevel::WARN, "warn"},
    {LogLevel::ERROR, "error"}
);

REGISTER_ENUM(Priority,
    {Priority::LOW, "low"},
    {Priority::MEDIUM, "medium"},
    {Priority::HIGH, "high"}
);

struct Config {
    std::string name;
    LogLevel level;
    std::chrono::seconds timeout;
    std::chrono::minutes interval;
    Priority priority;
};

TEST_CASE("Enum Duration Integration Tests") {
    Config config;
    config.name = "test_config";
    config.level = LogLevel::INFO;
    config.timeout = std::chrono::seconds(30);
    config.interval = std::chrono::minutes(1);
    config.priority = Priority::MEDIUM;

    SUBCASE("Enum string conversions") {
        // Test valid enum conversions
        CHECK(set_field(config, "level", nlohmann::json("debug")));
        CHECK(config.level == LogLevel::DEBUG);
        
        CHECK(set_field(config, "level", nlohmann::json("error")));
        CHECK(config.level == LogLevel::ERROR);
        
        CHECK(set_field(config, "priority", nlohmann::json("high")));
        CHECK(config.priority == Priority::HIGH);
    }
    
    SUBCASE("Enum case insensitive conversions") {
        CHECK(set_field(config, "level", nlohmann::json("DEBUG")));
        CHECK(config.level == LogLevel::DEBUG);
        
        CHECK(set_field(config, "level", nlohmann::json("Error")));
        CHECK(config.level == LogLevel::ERROR);
        
        CHECK(set_field(config, "priority", nlohmann::json("LOW")));
        CHECK(config.priority == Priority::LOW);
    }
    
    SUBCASE("Enum integer conversions") {
        CHECK(set_field(config, "level", nlohmann::json(0)));
        CHECK(config.level == LogLevel::DEBUG);
        
        CHECK(set_field(config, "level", nlohmann::json(3)));
        CHECK(config.level == LogLevel::ERROR);
        
        // Test with non-zero based enum values
        CHECK(set_field(config, "priority", nlohmann::json(1)));
        CHECK(config.priority == Priority::LOW);
        
        CHECK(set_field(config, "priority", nlohmann::json(10)));
        CHECK(config.priority == Priority::HIGH);
    }
    
    SUBCASE("Duration string conversions") {
        // Seconds
        CHECK(set_field(config, "timeout", nlohmann::json("45s")));
        CHECK(config.timeout.count() == 45);
        
        CHECK(set_field(config, "timeout", nlohmann::json("2m")));
        CHECK(config.timeout.count() == 120); // 2 minutes = 120 seconds
        
        // Minutes
        CHECK(set_field(config, "interval", nlohmann::json("5m")));
        CHECK(config.interval.count() == 5);
        
        // Test unit conversion from hours to minutes
        CHECK(set_field(config, "interval", nlohmann::json("2h")));
        CHECK(config.interval.count() == 120); // 2 hours = 120 minutes
    }
    
    SUBCASE("Duration numeric conversions") {
        // Numbers should be interpreted in the target duration's unit
        CHECK(set_field(config, "timeout", nlohmann::json(60)));
        CHECK(config.timeout.count() == 60); // 60 seconds
        
        CHECK(set_field(config, "interval", nlohmann::json(10)));
        CHECK(config.interval.count() == 10); // 10 minutes
    }
    
    SUBCASE("Duration whitespace handling") {
        CHECK(set_field(config, "timeout", nlohmann::json(" 30s ")));
        CHECK(config.timeout.count() == 30);
        
        CHECK(set_field(config, "interval", nlohmann::json("  5m  ")));
        CHECK(config.interval.count() == 5);
    }
    
    SUBCASE("Error handling - invalid enum values") {
        LogLevel original_level = config.level;
        
        CHECK_FALSE(set_field(config, "level", nlohmann::json("invalid")));
        CHECK(config.level == original_level); // Should be unchanged
        
        CHECK_FALSE(set_field(config, "level", nlohmann::json("unknown_level")));
        CHECK(config.level == original_level); // Should be unchanged
    }
    
    SUBCASE("Error handling - invalid durations") {
        auto original_timeout = config.timeout;
        
        CHECK_FALSE(set_field(config, "timeout", nlohmann::json("invalid_duration")));
        CHECK(config.timeout == original_timeout); // Should be unchanged
        
        CHECK_FALSE(set_field(config, "timeout", nlohmann::json("30x")));
        CHECK(config.timeout == original_timeout); // Should be unchanged
        
        CHECK_FALSE(set_field(config, "timeout", nlohmann::json("s30")));
        CHECK(config.timeout == original_timeout); // Should be unchanged
    }
    
    SUBCASE("Mixed conversion scenarios") {
        // Test realistic configuration scenarios
        CHECK(set_field(config, "name", nlohmann::json("production_config")));
        CHECK(set_field(config, "level", nlohmann::json("warn")));
        CHECK(set_field(config, "timeout", nlohmann::json("30s")));
        CHECK(set_field(config, "interval", nlohmann::json(5)));
        CHECK(set_field(config, "priority", nlohmann::json("HIGH")));
        
        CHECK(config.name == "production_config");
        CHECK(config.level == LogLevel::WARN);
        CHECK(config.timeout.count() == 30);
        CHECK(config.interval.count() == 5);
        CHECK(config.priority == Priority::HIGH);
    }
}