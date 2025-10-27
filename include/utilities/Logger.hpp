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
    Logger();
    ~Logger() = default;

    // Methods
    static void Log(std::string message, LogLevel messageLevel=LogLevel::INFO);
    static void Log(LogLevel messageLevel, const char* message, ...);
    static void Log(LogLevel messageLevel, std::string message);
    bool IsLoggingEnabled(void);
    LogLevel GetLogLevel(void);
    void SetLogPreferences(LogLevel level=LogLevel::INFO);
	
    static __always_inline void logMsgAndThrowError(const std::string& message) {
		Log(message, LogLevel::SEVERE);
		throw std::runtime_error(message);
	};

    static Logger* GetLogger();

  private:
    // Methods
    static std::string ConvertLogLevelToString(LogLevel level);
    static LogLevel    ConvertStringToLogLevel(const std::string& logLevel);
    std::string CreateDateString(void);
    bool        CreateDirectory(const std::string& path);
    static std::string CreateTimestamp(bool appendMS=true, bool iso=true);
    bool        DirectoryExists(const std::string& path);
    std::string ExtractFirstNDirs(const std::string& path, int numDirs);
    bool        FileExists(const std::string& path);
    bool        FindAndOpenLogConfigFile(std::string path, std::ifstream& configFileStream);
    std::string GetLogFilePath(void);
    std::string GetParentDirName(const std::string& path);
    bool        JsonFileValid(std::ifstream& jsonFile);
    bool        LogFileReady(void);
    bool        ParseLoggerConfigFile(std::ifstream& jsonFile);
    void        ReadConfigFile(std::string searchPath);
    void        SetupLogFile(void);
    void        ManageLoggingEnvVars(bool set=true);
    static std::string ToUpper(const std::string& str);
    static std::string TrimString(const std::string& str);

    // Variables
    bool         loggerInitialized = false;
    bool         loggingEnabled = true;
    std::fstream logFile;
    std::string  logFileDir = "";
    std::string  logFilePath = "";
    LogLevel     logLevel = LogLevel::INFO;
    std::string  moduleName = "";
    std::string  ngenResultsDir = "";
    bool         openedOnce = false;
    
    std::unordered_map<std::string, LogLevel> moduleLogLevels;
};

// Placed here to ensure the class is declared before setting this preprocessor symbol
#define LOG Logger::Log

#endif
