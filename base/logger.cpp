/**
 * @file logger.cpp
 * @author lc
 * @brief  Implementation of the Logger class
 * @see logger.h
 * @version 0.1
 * @date 2023-09-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "logger.h"
#include <iostream>

namespace XPlaneChatBot {
namespace Base {

std::ofstream Logger::log_file;
std::mutex Logger::log_mutex;

void Logger::log(const std::string& log_string, LogLevel log_level, const char* function_name)
{
    if (log_level > VERBOSE) {
		return; // Don't log if the log level is higher than the verbosity level
	}
    std::string log_entry;
    log_entry.reserve(100); // Reserve an estimated amount of space to avoid multiple allocations

    log_entry.append("[")
             .append(getCurrentTime())
             .append("][")
             .append(getLogLevelString(log_level))
             .append("][")
             .append(getFunctionString(function_name))
             .append("] ")
             .append(log_string)
             .append("\n");

    if (log_level == DEBUG) {
        OutputDebugStringA(log_entry.c_str());
    }

    if (log_file.is_open()) {
        std::lock_guard<std::mutex> lock(log_mutex); // Ensure thread safety
        log_file << log_entry; // Write to the log file
        // We'll want to decide whether to flush the log file based on the log level: if (log_level <= LogLevel::DEBUG) 
        log_file.flush(); // Flush the log file to ensure the message is written
    }
}

void Logger::openLogFile(const std::string& logs_path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open()) {
        // Create directory if it doesn't exist
        namespace fs = std::filesystem;
        fs::path dirPath(logs_path);
        if (!fs::exists(dirPath)) {
            fs::create_directories(dirPath);
        }
        
        // Add timestamp to filename
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::string filename = "XProtection_" + std::to_string(milliseconds) + ".log";

        // Open file
		log_file.open(logs_path + filename, std::ios::app);
	}
}

void Logger::closeLogFile() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.flush();
		log_file.close();
	}
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now(); // Get the current time as a time_point
    std::time_t now_c = std::chrono::system_clock::to_time_t(now); // Convert to time_t
    char buffer[20]; // Create a buffer large enough to hold the datetime string

    struct tm timeinfo;
    localtime_s(&timeinfo, &now_c); // Use localtime_s for safer conversion

    // Format the time directly into the buffer
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", &timeinfo);

    return std::string(buffer); // Construct a string from the buffer and return it
}


std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
    case LogLevel::FAIL: return "FAILURE";
    case LogLevel::ERR: return "ERROR";
    case LogLevel::WARN: return "WARNING";
    case LogLevel::INFO: return "INFO";
    case LogLevel::DEBUG: return "DEBUG";
    default: return "UNSPECIFIED";
    }
}

std::string Logger::getFunctionString(const char* function_name) {
    return function_name ? function_name : "XPlaneChatBot";
}

} // namespace Base
} // namespace XPlaneChatBot
