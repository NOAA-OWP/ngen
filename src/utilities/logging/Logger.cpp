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

bool         Logger::loggerInitialized = false;
bool         Logger::loggingEnabled = true;
std::fstream Logger::logFile;
std::string  Logger::logFileDir = "";
std::string  Logger::logFilePath = "";
LogLevel     Logger::logLevel = LogLevel::INFO;
std::string  Logger::moduleName = "";
std::string  Logger::ngenResultsDir = "";
bool         Logger::openedOnce = false;

std::unordered_map<std::string, LogLevel> Logger::moduleLogLevels;

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

const std::vector<std::string> allModules = {
    "NGEN",
    "CFE-S",
    "CFE-X",
    "LASAM",
    "NOAH-OWP-MODULAR",
    "PET",
    "SAC-SMA",
    "SFT",
    "SMP",
    "SNOW-17",
    "TOPMODEL",
    "TOPOFLOW",
    "T-ROUTE",
    "UEB",
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
            std::cerr << "system() failed to run mkdir.\n";
            return false;
        } else if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0) {
                std::cerr << "mkdir command failed with exit code: " << exitCode << "\n";
                return false;
            }
        } else {
            std::cerr << "mkdir terminated abnormally.\n";
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

    // Attempt until log file
    if (!logFilePath.empty()) {
        logFile.open(logFilePath, ios::out | ios::trunc); // Truncating ensures keeping only the last calibration iteration.
        if (logFile.is_open()) {
            openedOnce = true;
            setenv("NGEN_LOG_FILE_PATH", logFilePath.c_str(), 1);
            std::cout << "  NGEN_LOG_FILE_PATH=" << logFilePath << std::endl;
            // Make sure individual submodule logger files pathnames environment vars are cleared
            unsetenv("CFE_LOGFILEPATH");
            unsetenv("LASAM_LOGFILEPATH");
            unsetenv("NOAHOWP_LOGFILEPATH");
            unsetenv("PET_LOGFILEPATH");  // Not required yet
            unsetenv("SACSMA_LOGFILEPATH");
            unsetenv("SNOW17_LOGFILEPATH");
            unsetenv("SFT_LOGFILEPATH");
            unsetenv("SMP_LOGFILEPATH"); // Not required yet
            unsetenv("TOPMODEL_LOGFILEPATH");
            unsetenv("TROUTE_LOGFILEPATH");
            unsetenv("UEB_BMI_LOGFILEPATH");
            std::cout << "Program " << MODULE_NAME << " Log File: " << logFilePath << std::endl;

            std::string logMsg = "Log File: " + logFilePath + "\n";
            LogLevel saveLevel = logLevel;
            logLevel = LogLevel::INFO; // Ensure this INFO message is always logged
            Log(logMsg, logLevel);
            logLevel = saveLevel;
            return;
        }
    }
    std::cout << "Unable to create log file ";
    if (!logFilePath.empty()) {
        std::cout << logFilePath;
    }
    else if (!logFileDir.empty()) {
        std::cout << logFileDir;
    }
    std::cout << " (Perhaps check permissions)" << std::endl;
    std::cout << "Log entries will be written to stdout" << std::endl;
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
bool Logger::JsonFileValid(std::ifstream& jsonFile)
{
    // Move file pointer to beginning of file
    jsonFile.clear();                 // Clear any error bits. Does not clear the contents of the file
    jsonFile.seekg(0, std::ios::beg); // Move file pointer to beginning of the file

    std::stringstream buffer;
    buffer << jsonFile.rdbuf();
    std::string contents = buffer.str();

    // Trim leading whitespace
    size_t start = contents.find_first_not_of(" \t\n\r");
    // Trim trailing whitespace
    size_t end   = contents.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos) {
        std::cerr << MODULE_NAME << " ERROR: Issue in JSON file. File is empty or whitespace only." << std::endl;
        return false;
    }

    // Extract the trimmed content
    contents = contents.substr(start, end - start + 1);

    // Now check that it starts and ends with braces
    if (contents.front() != '{' || contents.back() != '}') {
        std::cerr << MODULE_NAME << " ERROR: Issue in JSON file. Does not begin/end with {}" << std::endl;
        return false;
    }

    // Check some basics. Not an exhaustive checker.
    unsigned int numOpenBraces  = std::count(contents.begin(), contents.end(), '{');
    unsigned int numCloseBraces = std::count(contents.begin(), contents.end(), '}');
    unsigned int numQuotes      = std::count(contents.begin(), contents.end(), '"');
    unsigned int numColons      = std::count(contents.begin(), contents.end(), ':');
    if ( (numOpenBraces && numCloseBraces && numQuotes && numColons) &&  // Not equal 0
         (numOpenBraces == numCloseBraces) &&                            // Matching open and close braces
         ((numQuotes % 2) == 0) )                                        // Even number of quotes
    {
        return true;
    }
    std::cerr << MODULE_NAME << " ERROR: Issue in JSON file. Open Braces=" << numOpenBraces << ", Close Braces=" << numCloseBraces
              <<  ", quotes=" << numQuotes << ", and colons=" << numColons << std::endl;
    return false;
}

bool Logger::ParseLoggerConfigFile(std::ifstream& jsonFile) 
{
    if (JsonFileValid(jsonFile)) {
        // Move file pointer to beginning of file
        jsonFile.clear();                 // Clear any error bits. Does not clear the contents of the file
        jsonFile.seekg(0, std::ios::beg); // Move file pointer to beginning of the file

        // Parse the json file
        std::string line;
        bool inModules = false;
        while (std::getline(jsonFile, line)) {
            std::string trimmed = TrimString(line);
            if (trimmed.find("\"logging_enabled\"") != std::string::npos) {
                size_t colon = trimmed.find(":");
                if (colon != std::string::npos) {
                    std::string rawValue = TrimString(trimmed.substr(colon + 1));
                    std::string value = CleanJsonToken(rawValue);
                    loggingEnabled = (value == "true");
                    std::cout << "  Found logging_enabled=" << (loggingEnabled?"true":"false") << std::endl;
                }
            } else if (trimmed.find("\"modules\"") != std::string::npos) {
                inModules = true;
            } else if (inModules && trimmed.find("}") != std::string::npos) {
                break; // end of modules section
            } else if (inModules) {
                size_t colon = trimmed.find(":");
                if (colon != std::string::npos) {
                    std::string rawModule = TrimString(trimmed.substr(0, colon));                
                    std::string moduleTok = CleanJsonToken(rawModule);                
                    std::string moduleStr = ToUpper(moduleTok);                
                    std::string rawLevel = TrimString(trimmed.substr(colon + 1));
                    std::string levelStr = ToUpper(CleanJsonToken(rawLevel)); 
                    if (std::find(allModules.begin(), allModules.end(), moduleStr) != allModules.end()) {
                        moduleLogLevels[moduleStr] = ConvertStringToLogLevel(levelStr);
                        std::cout << "  Found Log level " << moduleTok << "=" << ConvertLogLevelToString(moduleLogLevels[moduleStr]) << std::endl;
                    } else {
                        std::cout << "  ERROR: Ignoring unknown module " << moduleStr <<  std::endl;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

void Logger::ReadConfigFile(std::string searchPath) {

    // Set logger defaults
    loggingEnabled = true;
    moduleLogLevels.clear();
    for (const auto& hydroModule : allModules) {
        moduleLogLevels[hydroModule] = LogLevel::INFO;
    }

    // Open and Parse config file
    std::ifstream jsonFile;
    if (searchPath.empty()) {
        std::cout << "WARNING: NGEN_RESULTS_DIR environment variable not set or empty.";
        std::cout << " Using default logging configuration of enabled and log level INFO" << std::endl;
    } else {
        bool configFileFound = false;
        if (FindAndOpenLogConfigFile(searchPath, jsonFile)) {
            if (jsonFile.peek() != std::ifstream::traits_type::eof()) {
                if (ParseLoggerConfigFile(jsonFile)) configFileFound = true;
            }
        }
        if (!configFileFound) {
            std::cout << MODULE_NAME << " WARNING: Issue with logging config file " << CONFIG_FILENAME << " in " << searchPath << ".";
            std::cout << " Using default logging configuration of enabled and log level INFO" << std::endl;    
        }
    }

    // Set the environment variables for the module loggers
    SetLoggingEnvVars();
}

void Logger::SetLoggingEnvVars(void) {
    cout << "Logger setup: Setting Logger Environment Variables" << endl;
    setenv((EV_EWTS_LOGGING).c_str(), ((loggingEnabled)?"ENABLED":"DISABLED"), 1);
    cout << "  " << EV_EWTS_LOGGING << "=" << ((loggingEnabled)?"ENABLED":"DISABLED") << endl;

    for (const auto& hydroModule : allModules) {
        std::string moduleEnv = "";
        if (hydroModule == "NGEN") {
            logLevel = moduleLogLevels[hydroModule];
            moduleEnv = "NGEN_LOGLEVEL";
        }
        else if (hydroModule == "CFE-S" || hydroModule == "CFE-X") moduleEnv = "CFE_LOGLEVEL";
        else if (hydroModule == "NOAH-OWP-MODULAR") moduleEnv = "NOAHOWP_LOGLEVEL";
        else if (hydroModule == "SAC-SMA") moduleEnv = "SACSMA_LOGLEVEL";
        else if (hydroModule == "SNOW-17") moduleEnv = "SNOW17_LOGLEVEL";
        else if (hydroModule == "T-ROUTE") moduleEnv = "TROUTE_LOGLEVEL";
        else if (hydroModule == "UEB") moduleEnv = "UEB_BMI_LOGLEVEL";
        else moduleEnv = hydroModule + "_LOGLEVEL";
        std::string ll = ConvertLogLevelToString(moduleLogLevels[hydroModule]);
        setenv(moduleEnv.c_str(), ll.c_str(), 1);
        cout << "  " << hydroModule << ": " << moduleEnv << "=" << ll << endl;
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

        // Determine the log file directory and log file name.
        // Use name from environment variable if set, otherwise use a default
        std::string logFileDir = "";
        logFilePath = "";
        const char* envVar = std::getenv("NGEN_RESULTS_DIR");  // Currently set by ngen-cal but envision set for WCOSS at some point
        if (envVar != nullptr && envVar[0] != '\0') {
            ngenResultsDir = envVar;
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

            std::string llMsg = "Log level set to " + ConvertLogLevelToString(logLevel) + "\n";
            LogLevel saveLevel = logLevel;
            logLevel = LogLevel::INFO; // Ensure this INFO message is always logged
            Log(llMsg, logLevel);
            logLevel = saveLevel;
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

    if (!loggerInitialized) SetLogPreferences(); // Cover case where Log is called before setup done

    // Log only when appropriate 
    if  ((loggingEnabled) && (messageLevel >= logLevel)) {
        std::string   logType   = ConvertLogLevelToString(messageLevel);
        std::string   logPrefix = CreateTimestamp() + " " +  moduleName + " " + logType;

        // Log message, creating individual entries for a multi-line message
        std::istringstream logMsg(message);
        std::string line;
        if (LogFileReady()) {
            while (std::getline(logMsg, line)) {
                logFile << logPrefix + " " + line << std::endl;
            }
            logFile.flush();
        }
        else {
            // Log file not found. Write to stdout.
            while (std::getline(logMsg, line)) {
                cout << logPrefix + " " + line << std::endl;
            }
            cout << std::flush;
        }
    }
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
            std::cout << "Logger config: Opening logger config file " << candidate << std::endl;
            configFileStream.open(candidate);
            return configFileStream.is_open();
        }
        path = GetParentDirName(path);
    }
    return false;
}

