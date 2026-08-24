#include "Logger.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstdlib> // For getenv()
#include <cstdio>
#include <cstring>
#include <fstream> // For file handling
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string> // For std::string
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;

const std::string  MODULE_NAME         = "ngen";
const std::string  LOG_DIR_NGENCERF    = "/ngencerf/data";       // ngenCERF log directory string if environement var empty.
const std::string  LOG_DIR_DEFAULT     = "run-logs";             // Default parent log directory string if env var empty  & ngencerf dosn't exist
const std::string  LOG_FILE_EXT        = "log";                  // Log file name extension
const std::string  DS                  = "/";                    // Directory separator
const unsigned int LOG_MODULE_NAME_LEN = 8;                      // Width of module name for log entries

const std::string  EV_EWTS_LOGGING     = "NGEN_EWTS_LOGGING";    // Enable/disable of Error Warning and Trapping System  
const std::string  EV_NGEN_LOGFILEPATH = "NGEN_LOG_FILE_PATH";   // ngen log file 

const std::string  CONFIG_FILENAME     = "ngen_logging.json";    // ngen logging config file 

// String to LogLevel map
static const std::unordered_map<std::string, LogLevel> logLevelMap = {
    {"NONE", LogLevel::NONE}, {"0", LogLevel::NONE},
    {"DEBUG", LogLevel::DEBUG}, {"1", LogLevel::DEBUG},
    {"INFO", LogLevel::INFO}, {"2", LogLevel::INFO},
    {"WARNING", LogLevel::WARNING}, {"3", LogLevel::WARNING},
    {"SEVERE", LogLevel::SEVERE}, {"4", LogLevel::SEVERE},
    {"FATAL", LogLevel::FATAL}, {"5", LogLevel::FATAL},
};

// Reverse map: LogLevel to String
static const std::unordered_map<LogLevel, std::string> logLevelToStringMap = {
    {LogLevel::NONE,    "NONE   "},
    {LogLevel::DEBUG,   "DEBUG  "},
    {LogLevel::INFO,    "INFO   "},
    {LogLevel::WARNING, "WARNING"},
    {LogLevel::SEVERE,  "SEVERE "},
    {LogLevel::FATAL,   "FATAL  "},
};

const std::unordered_map<std::string, std::string> moduleNamesMap = {
    {"NGEN", "NGEN"},
    {"CFE-S", "CFE"},
    {"CFE-X", "CFE"},
    {"LASAM", "LASAM"},
    {"NOAH-OWP-MODULAR", "NOAHOWP"},
    {"PET", "PET"},
    {"SAC-SMA", "SACSMA"},
    {"SFT", "SFT"},
    {"SMP", "SMP"},
    {"SNOW-17", "SNOW17"},
    {"TOPMODEL", "TOPMODEL"},
    {"TOPOFLOW-GLACIER", "TFGLACR"},
    {"T-ROUTE", "TROUTE"},
    {"UEB", "UEB_BMI"},
    {"LSTM", "LSTM"},
    {"FORCING", "FORCING"}
};

bool Logger::DirectoryExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // Cannot access path
    }
    return (info.st_mode & S_IFDIR) != 0;
}

/**
 * Create the directory checking both the call
 * to execute the command and the result of the command
 */
bool Logger::CreateDirectory(const std::string& path) {

    if (!DirectoryExists(path)) {
        std::string mkdir_cmd = "mkdir -p " + path;
        int status = system(mkdir_cmd.c_str());

        if (status == -1) {
            std::cerr << "[CRITICAL] " << MODULE_NAME << " system() failed to run mkdir.\n";
            return false;
        } else if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0) {
                std::cerr << "[CRITICAL] " << MODULE_NAME << " mkdir command failed with exit code: " << exitCode << "\n";
                return false;
            }
        } else {
            std::cerr << "[CRITICAL] " << MODULE_NAME << " mkdir terminated abnormally.\n";
            return false;
        }
    }
    return true;
}

/**
 * Open log file and return open status. If already open,
 * ensure the write pointer is at the end of the file. 
 * 
 * return bool true if open and good, false otherwise
 */
bool Logger::LogFileReady(void) {

    if (openedOnce && logFile.is_open() && logFile.good()) {
        logFile.seekp(0, std::ios::end); // Ensure write pointer is at the actual file end
        return true;
    }
    else if (openedOnce) {
        // Somehow the logfile was closed. Open in append mode so  
        // previosly logged messages are not lost
        logFile.open(logFilePath, ios::out | ios::app); // This will silently fail if already open.
        if (logFile.good()) return true;
    }
    return false;
}

void Logger::SetupLogFile(void) {

	// Determine the log file directory and log file name.
    // Use name from environment variable if set, otherwise use a default
    if (!ngenResultsDir.empty()) {
        logFileDir = ngenResultsDir + DS + "logs";
        if (CreateDirectory(logFileDir))
            logFilePath = logFileDir + DS + MODULE_NAME + "." + LOG_FILE_EXT;
    }
    if (logFilePath.empty()) { 
        // Get parent log directory
        if (DirectoryExists(LOG_DIR_NGENCERF)) {
            logFileDir = LOG_DIR_NGENCERF + DS + LOG_DIR_DEFAULT;
        }
        else {
            const char *home = getenv("HOME"); // Get users home directory pathname
            std::string dir = (home) ? home : "~";
            logFileDir = dir + DS + LOG_DIR_DEFAULT;
        }

        // Ensure parent log direcotry exists
        if (CreateDirectory(logFileDir)) {
            // Get full log directory path
            const char* envUsername = std::getenv("USER");
            std::string dirName = (envUsername) ? envUsername : CreateDateString();
            logFileDir = logFileDir +  DS + dirName;

            // Set the full path if log directory exists/created
            if (CreateDirectory(logFileDir)) 
                logFilePath = logFileDir + DS + MODULE_NAME + "_" + CreateTimestamp(false,false) + "." + LOG_FILE_EXT;
        }
    }

    // Attempt to open log file
    if (!logFilePath.empty()) {
        logFile.open(logFilePath, ios::out | ios::trunc); // Truncating ensures keeping only the last calibration iteration.
        if (logFile.is_open()) {
            openedOnce = true;
            std::cout << "[DEBUG] " << MODULE_NAME << " Log File: " << logFilePath << std::endl;
            return;
        }
    }
    std::cout << "[WARNING] " << MODULE_NAME << " Unable to create log file ";
    if (!logFilePath.empty()) {
        std::cout << logFilePath;
    }
    else if (!logFileDir.empty()) {
        std::cout << logFileDir;
    }
    std::cout << " (Perhaps check permissions)" << std::endl;
    std::cout << "[WARNING] " << MODULE_NAME << " Log entries will be written to stdout" << std::endl;
}

std::string Logger::ToUpper(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return result;
}

std::string Logger::ExtractFirstNDirs(const std::string& path, int numDirs) {
    size_t pos = 0;
    int slashCount = 0;

    while (pos < path.length() && slashCount < numDirs) {
        if (path[pos] == '/') {
            ++slashCount;
        }
        ++pos;
    }

    // If the path starts with '/', keep it as is; otherwise return substring
    return path.substr(0, pos);
}

std::string CleanJsonToken(const std::string& token) {
    std::string s = token;
    if (!s.empty() && s.front() == '"') s.erase(0, 1);
    if (!s.empty() && s.back() == ',') s.pop_back();
    if (!s.empty() && s.back() == '"') s.pop_back();
    return s;
}

/*
    JSON file format exmaple:
    {
        "logging_enabled": true,
        "modules": {
            "ngen": "INFO",
            "CFE-S": "INFO",
            "UEB": "INFO",
            "Noah-OWP-Modular": "DEBUG",
            "T-Route": "INFO"
        }
    }
*/

bool Logger::ParseLoggerConfigFile(std::ifstream& jsonFile) 
{
    // Rewind file in case it's been partially read
    jsonFile.clear();
    jsonFile.seekg(0, std::ios::beg);

    try {
        // Read the JSON into a property tree
        boost::property_tree::ptree config;
        boost::property_tree::read_json(jsonFile, config);

        // Read logging_enabled flag
        try {
            loggingEnabled = config.get<bool>("logging_enabled", true);  // default true if missing
            std::cout << "[DEBUG] " << MODULE_NAME << " Logging " 
                      << (loggingEnabled ? "ENABLED" : "DISABLED") << std::endl;
        }
        catch (const boost::property_tree::ptree_bad_data& e) {
            std::cout << "[ERROR] " << MODULE_NAME << " JSON data error: " << e.what() << std::endl;
            return false;
        }

        // Read modules subtree only if logging enabled
        if (loggingEnabled) {
            bool atLeastOneModuleFound = false;
            if (auto modulesOpt = config.get_child_optional("modules")) {
                for (const auto& kv : *modulesOpt) {
                    std::string moduleName = ToUpper(kv.first);
                    std::string levelStr = ToUpper(kv.second.get_value<std::string>());

                    auto it = moduleNamesMap.find(moduleName);
                    if (it != moduleNamesMap.end()) {
                        atLeastOneModuleFound = true;
                        moduleLogLevels[moduleName] = ConvertStringToLogLevel(levelStr);
                        std::cout << "[DEBUG] " << MODULE_NAME << " Found Log level "
                                << kv.first << "="
                                << ConvertLogLevelToString(moduleLogLevels[moduleName])
                                << std::endl;
                        if (moduleName == "NGEN") logLevel = moduleLogLevels[moduleName];
                    } else {
                        std::cout << "[ERROR] " << MODULE_NAME << " Ignoring unknown module " << moduleName << std::endl;
                    }
                }
            } else {
                 std::cout << "[ERROR] " << MODULE_NAME << " Missing 'modules' section in logging.json." << std::endl;
            }
            return atLeastOneModuleFound;
        }
        return true;
    }
    catch (const boost::property_tree::json_parser_error& e) {
        std::cout << "[ERROR] " << MODULE_NAME << " JSON parse error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "[ERROR] " << MODULE_NAME << " Exception while parsing config: " << e.what() << std::endl;
    }
    return false;
}

void Logger::ReadConfigFile(std::string searchPath) {

    bool success = false;
    std::ifstream jsonFile;

    // Set logger defaults
    moduleLogLevels.clear();
    loggingEnabled = true;

    // Open and Parse config file
    if (searchPath.empty()) {
        std::cout << "[WARNING] " << MODULE_NAME << " Logging config file cannot be read from NGEN_RESULTS_DIR environment variable because not set or empty." << std::endl;
        std::cout << "[WARNING] " << MODULE_NAME << " Using defaults for logging." << std::endl;
    } else {
        if (FindAndOpenLogConfigFile(searchPath, jsonFile)) {
            if (jsonFile.peek() != std::ifstream::traits_type::eof()) {
                std::cout << "[DEBUG] " << MODULE_NAME << " parsing logging config file " << searchPath << std::endl;
                success = ParseLoggerConfigFile(jsonFile);
            }
        }
    }
    if (loggingEnabled && !success) {
        std::cout << "[WARNING] " << MODULE_NAME << " Issue with logging config file " << CONFIG_FILENAME << " in " << ((searchPath.empty())?"undefined path":searchPath) << "." << std::endl;
        std::cout << "[WARNING] " << MODULE_NAME << " Using default logging configuration of enabled and log level INFO for all known modules" << std::endl;  
        for (const auto kv : moduleNamesMap) {
            std::string moduleName = ToUpper(kv.first);
            moduleLogLevels[moduleName] = LogLevel::INFO;
        }
    }
}

bool Logger::IsValidEnvVarName(const std::string& name) {
    if (name.empty()) return false;

    // First character must be a letter or underscore
    if (!std::isalpha(name[0]) && name[0] != '_') return false;

    // All other characters must be alphanumeric or underscore
    for (size_t i = 1; i < name.size(); ++i) {
        if (!std::isalnum(name[i]) && name[i] != '_') return false;
    }

    return true;
}

void Logger::ManageLoggingEnvVars(bool set) {

    // Set logger env vars common to all modules 
    if (set) {
        Log("ngen Logger setup: Setting Module Logger Environment Variables", LogLevel::DEBUG);
        if (!logFilePath.empty()) {
            // Set the log file env var
            setenv("NGEN_LOG_FILE_PATH", logFilePath.c_str(), 1);
            std::cout << "[DEBUG] " << MODULE_NAME << " Set env var NGEN_LOG_FILE_PATH=" << logFilePath << std::endl;
        }
        else {
            std::cout << "[WARNING] " << MODULE_NAME << " NGEN_LOG_FILE_PATH env var not set. Modules writing to their default logs." << std::endl;          
        }

        // Set the logging enabled/disabled env var
        setenv((EV_EWTS_LOGGING).c_str(), ((loggingEnabled)?"ENABLED":"DISABLED"), 1);
        std::string logMsg = std::string("Set Logging ") + ((loggingEnabled)?"ENABLED":"DISABLED");
        std::cout << logMsg << "(envVar=" << EV_EWTS_LOGGING << ")" << std::endl;
        if (!logFilePath.empty()) {
            LogLevel saveLevel = logLevel;
            logLevel = LogLevel::INFO; // Ensure this INFO message is always logged
            Log(logMsg, logLevel);
            logLevel = saveLevel;
        }
    }
    else {
        Log("Logger setup: Unset existing Module Logger Environment Variables", LogLevel::DEBUG);
    }

    // Set logger env vars unique to each module in the formulation 
    // Note: moduleLogLevels is populated from the logging config file which
    //       only contains the log levels for module in the formulation
    for (const auto& modulePair : moduleLogLevels) {
        std::string envVar;
        std::string moduleNameForEnvVar;

        const std::string& moduleName = modulePair.first;
        LogLevel           level = modulePair.second;

        // Look up the module env var name in moduleNamesMap
        auto it = moduleNamesMap.find(moduleName); 
        if (it != moduleNamesMap.end()) {
            moduleNameForEnvVar = it->second;
            if (!IsValidEnvVarName(moduleNameForEnvVar)) {
                std::string logMsg = std::string("Invalid env var name ") + moduleNameForEnvVar +
                                     std::string(" for module ") + moduleName;
                Log(logMsg, LogLevel::WARNING);
                continue;
            }
        } 
        else {
            std::string logMsg = std::string("Unknown module in logLevels: ") + moduleName;
            Log(logMsg, LogLevel::WARNING);
            continue;
        }

        if (set) {
            // Sets the log level envirnoment variable 
            envVar = moduleNameForEnvVar + "_LOGLEVEL";
            std::string ll = ConvertLogLevelToString(level);
            setenv(envVar.c_str(), ll.c_str(), 1);
            std::string logMsg = std::string("Set ") + moduleName 
                + ((moduleName != "NGEN")?" Log Level env var to ":" Log Level to ") 
                + TrimString(ll);
            std::cout << logMsg;
            if (moduleName != "NGEN") std::cout << " (" << envVar << ")";
            std::cout << std::endl;
            if (!logFilePath.empty()) {
                LogLevel saveLevel = logLevel;
                logLevel = LogLevel::INFO; // Ensure this INFO message is always logged
                Log(logMsg, logLevel);
                logLevel = saveLevel;
            }
        }
        else {
            if (moduleName != "NGEN") {
                // It is possible that individual submodules may be writing to their own
                // logs if there was an issue accessing the ngen log file. The log file used
                // by each module is stored in its own environment variable. Unsetting this
                // environment variable will cause the modules to truncate their logs.
                // This is important when running calibrations since iterative runs of ngen
                // can number in the 1000's and it is only necessary to retain the last ngen run
                envVar = moduleNameForEnvVar + "_LOGFILEPATH";
                unsetenv(envVar.c_str());
                envVar = moduleNameForEnvVar + "_LOGLEVEL";
                unsetenv(envVar.c_str());
            }
        }
    }
}

/**
* Configure Logger Preferences and open log file
* @param level: LogLevel::WARNING by Default
* @return void
*/
void Logger::SetLogPreferences(LogLevel level) {

    if (!loggerInitialized) {
        loggerInitialized = true; // Only call this once

        // Unset any existing related environment vars
        ManageLoggingEnvVars(false); 

        // Determine the log file directory and log file name.
        // Use name from environment variable if set, otherwise use a default
        const char* envVar = std::getenv("NGEN_RESULTS_DIR");  // Currently set by ngen-cal but envision set for WCOSS at some point
        if (envVar != nullptr && envVar[0] != '\0') {
            ngenResultsDir = envVar;
            std::cout << "[DEBUG] " << MODULE_NAME << " Found envVar NGEN_RESULTS_DIR = " << ngenResultsDir << std::endl;
        }

        ReadConfigFile(ngenResultsDir);

        if (loggingEnabled) {

            // Make sure the module name used for logging is all uppercase and LOG_MODULE_NAME_LEN characters wide.
            moduleName = MODULE_NAME;
            std::string upperName = moduleName.substr(0, LOG_MODULE_NAME_LEN);  // Truncate to LOG_MODULE_NAME_LEN chars max
            std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

            std::ostringstream oss;
            oss << std::left << std::setw(LOG_MODULE_NAME_LEN) << std::setfill(' ') << upperName;
            moduleName = oss.str();

            SetupLogFile();

            // Set the environment variables for the module loggers
            ManageLoggingEnvVars(true);
            }
    }
}

void Logger::Log(LogLevel messageLevel, const char* message, ...) {
    va_list args;
    va_start(args, message);

    // Make a copy to calculate required size
    va_list args_copy;
    va_copy(args_copy, args);
    int requiredLen = vsnprintf(nullptr, 0, message, args_copy);
    va_end(args_copy);

    if (requiredLen > 0) {
        std::vector<char> buffer(requiredLen + 1);  // +1 for null terminator
        vsnprintf(buffer.data(), buffer.size(), message, args);

        va_end(args);

        Log(std::string(buffer.data()), messageLevel);
    } else {
        va_end(args);  // still need to clean up
    }
}

/**
 * Log given message with defined parameters and generate message to pass on Console or File
 * @param message: Log Message
 * @param messageLevel: Log Level, LogLevel::INFO by default
 */
void Logger::Log(LogLevel messageLevel, std::string message) {
    Log(message, messageLevel);
}
/**
* Log given message with defined parameters and generate message to pass on Console or File
* @param message: Log Message
* @param messageLevel: Log Level, LogLevel::INFO by default
*/
void Logger::Log(std::string message, LogLevel messageLevel) {
    Logger *logger = GetLogger();

    // Log only when appropriate 
    if  ((logger->loggingEnabled) && (messageLevel >= logger->logLevel)) {
        std::string   logType   = ConvertLogLevelToString(messageLevel);
        std::string   logPrefix = CreateTimestamp() + " " +  logger->moduleName + " " + logType;

        // Log message, creating individual entries for a multi-line message
        std::istringstream logMsg(message);
        std::string line;
        if (logger->LogFileReady()) {
            while (std::getline(logMsg, line)) {
                logger->logFile << logPrefix + " " + line << std::endl;
            }
            logger->logFile.flush();
        }
        else {
            // Log file not found. Write to stdout.
            while (std::getline(logMsg, line)) {
                std::cout << logPrefix + " " + line << std::endl;
            }
            std::cout << std::flush;
        }
    }
}

Logger* Logger::GetLogger()
{
    static Logger* logger = nullptr;
    if (logger == nullptr) {
        logger = new Logger;
        logger->SetLogPreferences();
    }
    return logger;
}

Logger::Logger()
{

}

// Function to trim leading and trailing spaces
std::string Logger::TrimString(const std::string& str) {
    // Trim leading spaces
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        return "";  // No non-whitespace characters
    }

    // Trim trailing spaces
    size_t last = str.find_last_not_of(" \t\n\r\f\v");

    // Return the trimmed string
    return str.substr(first, last - first + 1);
}

std::string Logger::ConvertLogLevelToString(LogLevel level) {
    auto it = logLevelToStringMap.find(level);
    if (it != logLevelToStringMap.end()) {
        return it->second;  // Found valid named or numeric log level
    }
    return "NONE";
}

/**
* Convert String Representation of Log Level to LogLevel Type
* @param levelStr : String log level
* @return LogLevel
*/
LogLevel Logger::ConvertStringToLogLevel(const std::string& levelStr) {
    std::string level = TrimString(levelStr);
    if (!level.empty()) {
        // Convert string to LogLevel (supports both names and numbers)
        auto it = logLevelMap.find(level);
        if (it != logLevelMap.end()) {
            return it->second;  // Found valid named or numeric log level
        }

        // Try parsing as an integer (for cases where an invalid numeric value is given)
        try {
            int levelNum = std::stoi(level);
            if (levelNum >= 0 && levelNum <= 5) {
                return static_cast<LogLevel>(levelNum);
            }
        } catch (...) {
            // Ignore errors (e.g., if std::stoi fails for non-numeric input)
        }
    }
	return LogLevel::NONE;
}

std::string Logger::CreateTimestamp(bool appendMS, bool iso) {
    using namespace std::chrono;

    // Get current time point
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);

    // Get milliseconds
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to UTC time
    std::tm utc_tm;
    gmtime_r(&now_time_t, &utc_tm);

    // Format date/time with strftime
    char buffer[32];
    if (iso) {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &utc_tm);
    }
    else {
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%S", &utc_tm);
    }

    if (appendMS) {
        // Combine with milliseconds
        std::ostringstream oss;
        oss << buffer << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }
    return std::string(buffer);
}

std::string Logger::CreateDateString(void) {
    std::time_t tt = std::time(0);
    std::tm* timeinfo = std::gmtime(&tt); // Use std::localtime(&tt) if you want local time

    char buffer[11]; // Enough for "YYYY-MM-DD" + null terminator
    std::strftime(buffer, sizeof(buffer), "%F", timeinfo); // %F == %Y-%m-%d

    std::stringstream ss;
    ss << buffer;

    return ss.str();
}

std::string Logger::GetLogFilePath(void) {
    return logFilePath;
}

LogLevel Logger::GetLogLevel(void) {
	return logLevel;
}

bool Logger::IsLoggingEnabled(void) {
    return loggingEnabled;
}

bool Logger::FileExists(const std::string& path) {
    struct stat statbuf{};
    return stat(path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode);
}

std::string Logger::GetParentDirName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos || pos == 0) return "/";
    return path.substr(0, pos);
}

bool Logger::FindAndOpenLogConfigFile(std::string path, std::ifstream& configFileStream) {
    while (!path.empty() && path != "/") {
        std::string candidate = path + DS + CONFIG_FILENAME;
        if (FileExists(candidate)) {
            std::cout << "[DEBUG] " << MODULE_NAME << " Opening logger config file " << candidate << std::endl;
            configFileStream.open(candidate);
            return configFileStream.is_open();
        }
        path = GetParentDirName(path);
    }
    return false;
}

