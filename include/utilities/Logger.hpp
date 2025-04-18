#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <ctime>
#include <sstream>

#define LOG (Logger::GetInstance())->Log

enum class LogLevel {
	NONE = 0,
	DEBUG = 1,
	INFO = 2,
	ERROR = 3,
	WARN = 4,
	FATAL = 5,
};

/**
* Logger Class Used to Output Details of Current Application Flow
*/
class Logger {
  public:
	static std::shared_ptr<Logger> GetInstance();

    bool        CheckLogLevelEv(void);
    std::string ConvertLogLevelToString(LogLevel level);
    LogLevel    ConvertStringToLogLevel(const std::string& logLevel);
	std::string CreateDateString(void);
    bool        CreateDirectory(const std::string& path);
	std::string CreateTimestamp(bool appendMS=true, bool iso=true);
    bool        DirectoryExists(const std::string& path);
    std::string  FormatModuleName(const std::string& moduleName);
    std::string GetLogFilePath(void);
	LogLevel    GetLogLevel(void);
    void        Log(std::string message, LogLevel messageLevel=LogLevel::INFO);
    bool        LogFileReady(void);
	void        SetLogPreferences(LogLevel level=LogLevel::INFO);
    std::string TrimString(const std::string& str);

	
	static __always_inline void logMsgAndThrowError(const std::string& message) {
		(Logger::GetInstance())->Log(message, LogLevel::INFO);
		throw std::runtime_error(message);
	};

  private:
	std::fstream logFile;
	std::string  logFilePath = "";
	LogLevel     logLevel = LogLevel::INFO;
    std::string  moduleName = "";
    bool         openedOnce = false;
    bool         envLogLevelLogged = false;

	static std::shared_ptr<Logger> loggerInstance;
};



#endif
