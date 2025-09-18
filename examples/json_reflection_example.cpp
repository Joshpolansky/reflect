#include <reflect_json/reflect_json.hpp>
#include <fstream>
#include <iostream>

/**
 * @brief Practical example demonstrating JSON reflection utility
 * 
 * This example shows how to use the reflection utility for real-world scenarios
 * such as configuration files, data serialization, and API communication.
 */

// Example: Robot configuration
struct MotorConfig {
    double max_speed;
    double acceleration;
    double deceleration;
    bool enabled;
};

struct SensorConfig {
    std::string type;
    double sampling_rate;
    double threshold;
    int port;
};

struct RobotConfig {
    std::string robot_name;
    MotorConfig motor;
    SensorConfig sensor;
    double control_frequency;
    bool debug_mode;
};

// Example: Data logging structure
struct LogEntry {
    double timestamp;
    std::string level;
    std::string message;
    std::string component;
};

class ConfigManager {
public:
    /**
     * @brief Save configuration to JSON file
     */
    static bool save_config(const RobotConfig& config, const std::string& filename) {
        try {
            auto json_config = reflect_json::to_json(config);
            
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                return false;
            }
            
            file << json_config.dump(2);
            file.close();
            
            std::cout << "Configuration saved to: " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving config: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Load configuration from JSON file
     */
    static bool load_config(RobotConfig& config, const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for reading: " << filename << std::endl;
                return false;
            }
            
            nlohmann::json json_config;
            file >> json_config;
            file.close();
            
            config = reflect_json::from_json<RobotConfig>(json_config);
            
            std::cout << "Configuration loaded from: " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error loading config: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Get default configuration
     */
    static RobotConfig get_default_config() {
        return RobotConfig{
            "DefaultRobot",           // robot_name
            {                         // motor
                100.0,                // max_speed
                50.0,                 // acceleration
                75.0,                 // deceleration
                true                  // enabled
            },
            {                         // sensor
                "lidar",              // type
                10.0,                 // sampling_rate
                0.1,                  // threshold
                8080                  // port
            },
            100.0,                    // control_frequency
            false                     // debug_mode
        };
    }
};

class Logger {
private:
    std::vector<LogEntry> log_entries;

public:
    void log(const std::string& level, const std::string& message, const std::string& component = "system") {
        LogEntry entry{
            std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(),
            level,
            message,
            component
        };
        log_entries.push_back(entry);
    }

    bool save_log(const std::string& filename) {
        try {
            nlohmann::json log_json;
            // Serialize each log entry individually to ensure proper conversion
            for(auto entry : log_entries){
                log_json.push_back(reflect_json::to_json(entry));
            }
            
            std::ofstream file(filename);
            file << log_json.dump(2);
            file.close();
            
            std::cout << "Log saved to: " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving log: " << e.what() << std::endl;
            return false;
        }
    }

    bool load_log(const std::string& filename) {
        try {
            std::ifstream file(filename);
            nlohmann::json log_json;
            file >> log_json;
            file.close();
            
            log_entries.clear();
            for (const auto& entry_json : log_json) {
                log_entries.push_back(reflect_json::from_json<LogEntry>(entry_json));
            }
            
            std::cout << "Log loaded from: " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error loading log: " << e.what() << std::endl;
            return false;
        }
    }

    void print_log() const {
        std::cout << "\n=== Log Entries ===" << std::endl;
        for (const auto& entry : log_entries) {
            std::cout << "[" << entry.timestamp << "] " 
                      << "[" << entry.level << "] "
                      << "[" << entry.component << "] "
                      << entry.message << std::endl;
        }
    }
};

int main() {
    std::cout << "JSON Reflection Utility - Practical Example" << std::endl;
    std::cout << "===========================================" << std::endl;

    // 1. Configuration Management Example
    std::cout << "\n1. Configuration Management:" << std::endl;
    
    // Create and save default configuration
    auto config = ConfigManager::get_default_config();
    ConfigManager::save_config(config, "robot_config.json");
    
    // Modify and save again
    config.robot_name = "ProductionRobot";
    config.motor.max_speed = 150.0;
    config.sensor.type = "camera";
    config.debug_mode = true;
    
    ConfigManager::save_config(config, "robot_config_modified.json");
    
    // Load configuration
    RobotConfig loaded_config;
    if (ConfigManager::load_config(loaded_config, "robot_config_modified.json")) {
        std::cout << "Loaded robot name: " << loaded_config.robot_name << std::endl;
        std::cout << "Motor max speed: " << loaded_config.motor.max_speed << std::endl;
        std::cout << "Sensor type: " << loaded_config.sensor.type << std::endl;
    }

    // 2. Data Logging Example
    std::cout << "\n2. Data Logging:" << std::endl;
    
    Logger logger;
    logger.log("INFO", "System initialized", "main");
    logger.log("DEBUG", "Motor configuration loaded", "motor");
    logger.log("WARN", "Sensor threshold exceeded", "sensor");
    logger.log("ERROR", "Connection timeout", "network");
    logger.log("INFO", "System shutdown", "main");
    
    logger.print_log();
    logger.save_log("system.log");
    
    // Load log from file
    Logger logger2;
    if (logger2.load_log("system.log")) {
        std::cout << "\nLoaded log entries:" << std::endl;
        logger2.print_log();
    }

    // 3. Schema Generation for API Documentation
    std::cout << "\n3. Schema Generation:" << std::endl;
    
    auto config_schema = reflect_json::reflection::get_schema<RobotConfig>();
    auto log_schema = reflect_json::reflection::get_schema<LogEntry>();
    
    std::ofstream schema_file("api_schemas.json");
    nlohmann::json schemas;
    schemas["RobotConfig"] = config_schema;
    schemas["LogEntry"] = log_schema;
    schema_file << schemas.dump(2);
    schema_file.close();
    
    std::cout << "API schemas saved to api_schemas.json" << std::endl;

    // Clean up files (optional)
    std::cout << "\nExample completed successfully!" << std::endl;
    std::cout << "Files created:" << std::endl;
    std::cout << "- robot_config.json" << std::endl;
    std::cout << "- robot_config_modified.json" << std::endl;
    std::cout << "- system.log" << std::endl;
    std::cout << "- api_schemas.json" << std::endl;

    return 0;
}
