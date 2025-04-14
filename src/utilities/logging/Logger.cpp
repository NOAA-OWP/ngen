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

const std::string MODULE_NAME      = "ngen";
const std::string LOG_DIR_NGENCERF = "/ngencerf/data";  // ngenCERF log directory string if environement var empty.
const std::string LOG_DIR_DEFAULT  = "run-logs";        // Default parent log directory string if env var empty  & ngencerf dosn't exist
const std::string LOG_FILE_EXT     = "log";             // Log file name extension
const std::string DS               = "/";               // Directory separator

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
 * If openedOnce false, open the file in truncate mode
 * so only the last run of ngen in a calibration is saved.
 * 
 * return bool true if open and good, false otherwise
 */
bool Logger::LogFileReady(void) {

    if (openedOnce && logFile.is_open() && logFile.good()) {
        logFile.seekp(0, std::ios::end); // Ensure write pointer is at the actual file end
        return true;
    }
    else {
        if (openedOnce) {
            // Somehow the logfile was closed. Open in append mode so  
            // previosly logged messages are not lost
            logFile.open(logFilePath, ios::out | ios::app); // This will silently fail if already open.
            if (logFile.good()) return true;
        }
        else {
            // Attempt until opened once (initially and on each attempt to write to the log)
            if (!logFilePath.empty()) {
                logFile.open(logFilePath, ios::out | ios::trunc); // Truncating ensures keeping only the last calibration iteration.
                if (logFile.good()) {
                    setenv("NGEN_LOG_FILE_PATH", (char *)logFilePath.c_str(), 1);
                    openedOnce = true;
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * Check the log level envinroment variable and update it if it changed.
 * @return true if environment variable exists otherwise false. 
 *
*/
bool Logger::CheckLogLevelEv(void) {
    const char* envLogLevel = std::getenv("NGEN_LOGLEVEL");
    if (envLogLevel != nullptr && envLogLevel[0] != '\0') {
        LogLevel envll = ConvertStringToLogLevel(envLogLevel);
        if (envll != logLevel) {
            logLevel = envll;
            std::string llMsg = "INFO: " + MODULE_NAME + " log level set to found NGEN_LOGLEVEL (" + TrimString(ConvertLogLevelToString(logLevel)) + ")\n";
            // This is an INFO message that always should be in the log but the
            // logLevel could be different than INFO. herefore use logLevel to 
            // ensure the message is recorded in the log
            Log(llMsg, logLevel);
            envLogLevelLogged = true;
        }
        return true;
    }
    return false;
}

/**
* Configure Logger Preferences and open log file
* @param level: LogLevel::ERROR by Default
* @return void
*/
void Logger::SetLogPreferences(LogLevel level) {

    // Make sure the module name used for logging is all uppercase and 8 characters wide.
    moduleName = MODULE_NAME;
    std::string upperName = moduleName.substr(0, 8);  // Truncate to 8 chars max
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    std::ostringstream oss;
    oss << std::left << std::setw(8) << std::setfill(' ') << upperName;
    moduleName = oss.str();

	// Determine the log file directory and log file name.
    // Use name from environment variable if set, otherwise use a default
    std::string logFileDir = "";
    logFilePath = "";
    const char* envVar = std::getenv("NGEN_RESULTS_DIR");  // Currently set by ngen-cal but envision set for WCOSS at some point
    if (envVar != nullptr && envVar[0] != '\0') {
        logFileDir = envVar + DS + "logs";
        logFilePath = logFileDir + DS + MODULE_NAME + "." + LOG_FILE_EXT;
    }
    else { 
        // Get parent log directory
        std::string dir;
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

    openedOnce = false; // Ensure this is false before calling LogFileReady
    if (LogFileReady()) {
        std::cout << "Program " << MODULE_NAME << " Log File: " << logFilePath << std::endl;
        // This is an INFO message that always should be in the log but the
        // logLevel could be different than INFO. Therefore use logLevel to 
        // ensure the message is recorded in the log
        std::string lMsg = "INFO: Log File: " + logFilePath + "\n";
        Log(lMsg, logLevel); // Calls CheckLogLevelEv
    }
    else {
        std::cout << "Unable to create log directory ";
        if (!logFileDir.empty()) {
            std::cout << logFileDir;
            std::cout << " (Perhaps check permissions)" << std::endl;
        }
        std::cout << "Log entries will be written to stdout" << std::endl;
    }

    // Set the logger log level if environment var not found
    envLogLevelLogged = false; // Ensure this is false before calling LogFileReady
    if (!CheckLogLevelEv()) logLevel = level;

    // Ensure the initial log level is logged. This flag is only checked here during startup in 
    // case the environment log level is the same as the default log level. If so it wouldn't
    // get logged in CheckLogLevelEv
    if (!envLogLevelLogged) {
        std::string llMsg = "INFO: log level set to " + ConvertLogLevelToString(logLevel) + "\n";
        // This is an INFO message that always should be in the log but the
        // logLevel could be different than INFO. Therefore use logLevel to 
        // ensure the message is recorded in the log
        Log(llMsg, logLevel);
        envLogLevelLogged = true;
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

    // Check for change in log level environment variable
    (void) CheckLogLevelEv();

    // don't log if messageLevel < logLevel 
    if (messageLevel >= logLevel) {
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

std::string Logger::ConvertLogLevelToString(LogLevel level) {
    auto it = logLevelToStringMap.find(level);
    if (it != logLevelToStringMap.end()) {
        return it->second;  // Found valid named or numeric log level
    }
    return "NONE";
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
