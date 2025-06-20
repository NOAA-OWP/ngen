#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <unordered_map>

enum class LogLevel {
	NONE    = 0,
	DEBUG   = 1,
	INFO    = 2,
	WARNING = 3,
	SEVERE  = 4,
	FATAL   = 5,
};

/**
* Logger Class Used to Output Details of Current Application Flow
*/
class Logger {
  public:
//    Logger(void);
//    ~Logger(void);

    // Methods
    static void Log(std::string message, LogLevel messageLevel=LogLevel::INFO);
    static void Log(LogLevel messageLevel, const char* message, ...);
    static void Log(LogLevel messageLevel, std::string message);
    static bool IsLoggingEnabled(void);
    static LogLevel GetLogLevel(void);
    static void SetLogPreferences(LogLevel level=LogLevel::INFO);

	
    static __always_inline void logMsgAndThrowError(const std::string& message) {
		Log(message, LogLevel::SEVERE);
		throw std::runtime_error(message);
	};

  private:
    // Methods
    static std::string ConvertLogLevelToString(LogLevel level);
    static LogLevel    ConvertStringToLogLevel(const std::string& logLevel);
    static std::string CreateDateString(void);
    static bool        CreateDirectory(const std::string& path);
    static std::string CreateTimestamp(bool appendMS=true, bool iso=true);
    static bool        DirectoryExists(const std::string& path);
    static std::string ExtractFirstNDirs(const std::string& path, int numDirs);
    static bool        FileExists(const std::string& path);
    static bool        FindAndOpenLogConfigFile(std::string path, std::ifstream& configFileStream);
    static std::string GetLogFilePath(void);
    static std::string GetParentDirName(const std::string& path);
    static bool        JsonFileValid(std::ifstream& jsonFile);
    static bool        LogFileReady(void);
    static bool        ParseLoggerConfigFile(std::ifstream& jsonFile);
    static void        ReadConfigFile(std::string searchPath);
    static void        SetupLogFile(void);
    static void        ManageLoggingEnvVars(bool set=true);
    static std::string ToUpper(const std::string& str);
    static std::string TrimString(const std::string& str);

    // Variables
    static bool         loggerInitialized;
    static bool         loggingEnabled;
    static std::fstream logFile;
    static std::string  logFileDir;
    static std::string  logFilePath;
    static LogLevel     logLevel;
    static std::string  moduleName;
    static std::string  ngenResultsDir;
    static bool         openedOnce;
    
    static std::unordered_map<std::string, LogLevel> moduleLogLevels;

};

// Placed here to ensure the class is declared before setting this preprocessor symbol
#define LOG Logger::Log

#endif
