#include <stdlib.h>
#include "Logger.hpp"
#include <iostream>
#include <sstream>
#include <fstream>      // For file handling
#include <cstdlib>      // For getenv()
#include <cstring>
#include <string>       // For std::string
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include <chrono>
#include <sys/wait.h>
#include <sys/stat.h>
#include <vector>


const std::string  MODULE_NAME         = "ngen";
const std::string  LOG_DIR_NGENCERF    = "/ngencerf/data";  // ngenCERF log directory string if environement var empty.
const std::string  LOG_DIR_DEFAULT     = "run-logs";        // Default parent log directory string if env var empty  & ngencerf dosn't exist
const std::string  LOG_FILE_EXT        = "log";             // Log file name extension
const std::string  DS                  = "/";               // Directory separator
const unsigned int LOG_MODULE_NAME_LEN = 8;                 // Width of module name for log entries

const std::string  EV_EWTS_LOGGING     = "NGEN_EWTS_LOGGING";    // Enable/disable of Error Warning and Trapping System  
const std::string  EV_NGEN_LOGFILEPATH = "NGEN_LOG_FILE_PATH";   // ngen log file 

std::shared_ptr<Logger> Logger::loggerInstance;

using namespace std;

// String to LogLevel map
static const std::unordered_map<std::string, LogLevel> logLevelMap = {
    {"NONE", LogLevel::NONE}, {"0", LogLevel::NONE},
    {"DEBUG", LogLevel::DEBUG}, {"1", LogLevel::DEBUG},
    {"INFO", LogLevel::INFO}, {"2", LogLevel::INFO},
    {"ERROR", LogLevel::ERROR}, {"3", LogLevel::ERROR},
    {"WARN", LogLevel::WARN}, {"4", LogLevel::WARN},
    {"FATAL", LogLevel::FATAL}, {"5", LogLevel::FATAL},
};

// Reverse map: LogLevel to String
static const std::unordered_map<LogLevel, std::string> logLevelToStringMap = {
    {LogLevel::NONE,  "NONE "},
    {LogLevel::DEBUG, "DEBUG"},
    {LogLevel::INFO,  "INFO "},
    {LogLevel::ERROR, "ERROR"},
    {LogLevel::WARN,  "WARN "},
    {LogLevel::FATAL, "FATAL"},
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
            logFileDir = "~" + DS + LOG_DIR_DEFAULT;
        }

        // Ensure parent log direcotry exists
        if (CreateDirectory(logFileDir)) {
            // Get full log directory path
            const char* envUsername = std::getenv("USER");
            if (envUsername) { 
                std::string username = envUsername;
                logFileDir = logFileDir +  DS + username;
            }
            else {
                logFileDir = logFileDir + DS + CreateDateString();
            }
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
            setenv("NGEN_LOG_FILE_PATH", (char *)logFilePath.c_str(), 1);
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

            std::string lMsg = "Log File: " + logFilePath + "\n";
            LogLevel saveLevel = logLevel;
            logLevel = LogLevel::INFO; // Ensure this INFO message is always logged
            Log(lMsg, logLevel);
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

void Logger::ReadConfigFile(void) {

    loggingEnabled = true;

    moduleLogLevels.clear();
    for (const auto& hydroModule : allModules) {
        moduleLogLevels[hydroModule] = LogLevel::INFO;
    }

    std::ifstream jsonFile;
    if (ngenResultsDir.empty()) {
        std::cout << "WARNING: NGEN_RESULTS_DIR environment variable not set.";
        std::cout << " Using default logging configuration of enabled and log level INFO" << std::endl;
    } else {
        std::string configDir = ExtractFirstNDirs(ngenResultsDir,6);
        std::string configFilePath = configDir + "logging-config.json";
        std::cout << "Logger config: Opening logger config file " << configFilePath << std::endl;
        jsonFile.open(configFilePath.c_str());
        if (jsonFile.is_open() && (jsonFile.peek() != std::ifstream::traits_type::eof())) {
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
        } else {
            std::cout << "WARNING: Unable to open logging config file " << configFilePath << ".";
            std::cout << " Using default logging configuration of enabled and log level INFO" << std::endl;    
        }
    }

    // Set the environment variables for the module loggers
    SetLogLevelEnvVars();
}

void Logger::SetLogLevelEnvVars(void) {
    cout << "Logger setup: Setting Modules Environment Variables" << endl;
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
* @param level: LogLevel::ERROR by Default
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

        ReadConfigFile();
        
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

/**
* Get Single Logger Instance or Create new Object if Not Created
* @return std::shared_ptr<Logger>
*/
std::shared_ptr<Logger> Logger::GetInstance() {
	if (loggerInstance == nullptr) {
		loggerInstance = std::shared_ptr<Logger>(new Logger());
	}
	return loggerInstance;
}

/**
* Log given message with defined parameters and generate message to pass on Console or File
* @param message: Log Message
* @param messageLevel: Log Level, LogLevel::INFO by default
*/
void Logger::Log(std::string message, LogLevel messageLevel) {

    if (!loggerInitialized) SetLogPreferences();

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
* @param logLevel : String log level
* @return LogLevel
*/
LogLevel Logger::ConvertStringToLogLevel(const std::string& logLevel) {
    std::string trimmed = TrimString(logLevel);
    if (!trimmed.empty()) {
        // Convert string to LogLevel (supports both names and numbers)
        auto it = logLevelMap.find(logLevel);
        if (it != logLevelMap.end()) {
            return it->second;  // Found valid named or numeric log level
        }

        // Try parsing as an integer (for cases where an invalid numeric value is given)
        try {
            int levelNum = std::stoi(logLevel);
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
