/**
 * @file logger.h
 * @author lc
 * @brief Interface for the Logger class: Logging warnings, errors, and info messages to LOG file in the plugin folder during runtime
 * @version 0.1
 * @date 2023-09-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef XPROTECTION_BASE_LOGGER_H
#define XPROTECTION_BASE_LOGGER_H

#include <ctime>
#include <chrono>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>

#include <Windows.h>
#include <XPLMUtilities.h>


#include "defs.h"

namespace XPlaneChatBot {
namespace Base {

/**
 * @brief Verbosity level for logging (integer from 0 to 6):
 * 0: no logging
 * 1: only unspecified 
 * 2: unspecified + failures
 * 3: unspecified + failures + errors
 * 4: unspecified + failures + errors + warnings
 * 5: unspecified + failures + errors + warnings + debug
 * 6: unspecified + failures + errors + warnings + debug + info
 */
#define VERBOSE 6 

/// @brief Enumeration for log levels
enum LogLevel {
    UNSPEC = 1, ///< Unspecified    
    FAIL,   ///< Failure (should make this force application quit)
    ERR,    ///< Error (application can continue)
    WARN,   ///< Warning (something unexpected happened)
    DEBUG,  ///< Debug information (only for debugging)
    INFO,   ///< Information (something expected happened)
};


/// @brief Class for logging warnings, errors, and info messages to LOG file in the XPlane folder during runtime
class Logger
{
public:
    /**
     * @brief Logs a message with log level and function
     *
     * @param log_string String to be logged
     * @param log_level Level of logging, one of: UNSPEC, FAIL, ERROR, WARN, INFO (optional)
     * @param function_name Name of function calling log function: __FUNCTION__ (optional)
     */
    static void log(const std::string& log_string, LogLevel log_level = LogLevel::UNSPEC, const char* function_name = "");

    /**
     * @brief Opens the log file if it's not already open. Must be called at XPluginStart()
     * 
     * @param logs_path Path where to save log files: $(PluginDir)/Logs/
     */
    static void openLogFile(const std::string& logs_path);

    /**
     * @brief Closes the log file if it's open. Must be called at XPluginStop()
     */
    static void closeLogFile();

private:
    /**
     * @brief Get the current time as a string.
     * *
     * @return The current time as a string.
     */
    static std::string getCurrentTime();

    /**
     * @brief Get the log level string.
     *
     * @param log_level The log level.
     * @return The log level string.
     */
    static std::string getLogLevelString(LogLevel log_level);

    /**
     * @brief Get the function name or just XPlaneChatBot if function name is ""
     * 
     * @param function_name Name of function if log was called with __FUNCTION__
     */
    static std::string getFunctionString(const char* function_name); 

    static std::mutex log_mutex; ///< Mutex for logging
    static std::ofstream log_file; ///< Log file stream object
};

} // namespace Base
} // namespace XPlaneChatBot

#endif // XPROTECTION_BASE_LOGGER_H
