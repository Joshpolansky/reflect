#include "reflection.hpp"
#include <iostream>

using namespace reflection;

// Define some example enums and register them
enum class Priority { LOW, MEDIUM, HIGH };
enum class Status { ACTIVE, INACTIVE, PENDING };

// Register the enums with string mappings
REGISTER_ENUM(Priority,
    {Priority::LOW, "low"},
    {Priority::MEDIUM, "medium"},
    {Priority::HIGH, "high"}
);

REGISTER_ENUM(Status,
    {Status::ACTIVE, "active"},
    {Status::INACTIVE, "inactive"},
    {Status::PENDING, "pending"}
);

// Example configuration structure
struct TaskConfig {
    std::string name;
    Status status;
    Priority priority;
    std::chrono::seconds timeout;
    std::chrono::minutes interval;
};

int main() {
    TaskConfig config;
    config.name = "example task";
    
    std::cout << "=== Enum and Duration Conversion Demo ===" << std::endl;
    
    // Test enum conversion from string
    std::cout << "\n1. Setting enum from string:" << std::endl;
    bool success = set_field(config, "status", nlohmann::json("active"));
    std::cout << "   set_field(config, \"status\", \"active\") = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test enum conversion - case insensitive
    success = set_field(config, "priority", nlohmann::json("HIGH")); 
    std::cout << "   set_field(config, \"priority\", \"HIGH\") = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test duration conversion from string
    std::cout << "\n2. Setting duration from string:" << std::endl;
    success = set_field(config, "timeout", nlohmann::json("30s"));
    std::cout << "   set_field(config, \"timeout\", \"30s\") = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   timeout value: " << config.timeout.count() << " seconds" << std::endl;
    
    success = set_field(config, "interval", nlohmann::json("5m"));
    std::cout << "   set_field(config, \"interval\", \"5m\") = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   interval value: " << config.interval.count() << " minutes" << std::endl;
    
    // Test duration from numeric JSON (interpreted in target unit)
    std::cout << "\n3. Setting duration from numeric JSON:" << std::endl;
    success = set_field(config, "interval", nlohmann::json(10));
    std::cout << "   set_field(config, \"interval\", 10) = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   interval value: " << config.interval.count() << " minutes" << std::endl;
    
    // Test whitespace handling
    std::cout << "\n4. Whitespace handling in durations:" << std::endl;
    success = set_field(config, "timeout", nlohmann::json(" 45s "));
    std::cout << "   set_field(config, \"timeout\", \" 45s \") = " << (success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "   timeout value: " << config.timeout.count() << " seconds" << std::endl;
    
    // Test getting values back (basic field retrieval)
    std::cout << "\n5. Getting field values:" << std::endl;
    auto result = get_field(config, "name");
    if (result && result->is_string()) {
        std::cout << "   get_field(config, \"name\") = \"" << result->get<std::string>() << "\"" << std::endl;
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    
    return 0;
}