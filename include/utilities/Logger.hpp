#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <ctime>
#include <sstream>
#include <unordered_map>

#define LOG (Logger::GetInstance())->Log

enum class LogLevel {
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	WARNING = 3,
	SEVERE = 4,
	FATAL = 5,
};

/**
* Logger Class Used to Output Details of Current Application Flow
*/
class Logger {
  public:
//    Logger(void);
//    ~Logger(void);

    // Methods
    void  SetLogPreferences(LogLevel level=LogLevel::INFO);
    void  Log(std::string message, LogLevel messageLevel=LogLevel::INFO);
	
	static __always_inline void logMsgAndThrowError(const std::string& message) {
		(Logger::GetInstance())->Log(message, LogLevel::INFO);
		throw std::runtime_error(message);
	};

    // Variables
	static std::shared_ptr<Logger> GetInstance();

  private:
    // Methods
    std::string ConvertLogLevelToString(LogLevel level);
    LogLevel    ConvertStringToLogLevel(const std::string& logLevel);
    std::string CreateDateString(void);
    bool        CreateDirectory(const std::string& path);
    std::string CreateTimestamp(bool appendMS=true, bool iso=true);
    bool        DirectoryExists(const std::string& path);
    std::string ExtractFirstNDirs(const std::string& path, int numDirs);
    std::string GetLogFilePath(void);
    LogLevel    GetLogLevel(void);
    bool        LogFileReady(void);
    void        ReadConfigFile(void);
    void        SetupLogFile(void);
    void        SetLoggingEnvVars(void);
    std::string ToUpper(const std::string& str);
    std::string TrimString(const std::string& str);

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

	static std::shared_ptr<Logger> loggerInstance;
};



#endif
