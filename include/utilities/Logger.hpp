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

enum class LoggingModule {
	NGEN = 0,
	NOAHOWP, 
	SNOW17, 
	UEB, 
	CFE, 
	SACSMA, 
	LASAM, 
	SMP, 
	SFT, 
	TROUTE, 
	SCHISM, 
	SFINCS, 
	GC2D, 
	TOPOFLOW,
	MODULE_COUNT
};

/**
* Logger Class Used to Output Details of Current Application Flow
*/
class Logger {
  public:
	static std::shared_ptr<Logger> GetInstance();
	void SetLogPreferences(LogLevel level);
	void Log(std::string message, LogLevel messageLevel);
	LogLevel GetLogLevel(const std::string& logLevel);
	std::string createTimestamp();
	std::string getLogFilePath();
	static __always_inline void logMsgAndThrowError(const std::string& message) {
		(Logger::GetInstance())->Log(message, LogLevel::ERROR);
		throw std::runtime_error(message);
	};

  private:
	LogLevel logLevel;
	std::fstream logFile;
	std::string logFilePath;
	static std::shared_ptr<Logger> loggerInstance;
};



#endif
