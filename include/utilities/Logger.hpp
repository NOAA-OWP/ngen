#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <ctime>

enum class LogLevel {
	NONE = 0,
	INFO = 1,
	ERROR = 2,
	WARN = 3,
	DEBUG = 4,
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
	void Log(std::string message, LogLevel messageLevel, LoggingModule module);
	LogLevel GetLogLevel(const std::string& logLevel);
	std::string createTimestamp();

  private:
	LogLevel logLevel;
	std::fstream logFile;
	static std::shared_ptr<Logger> loggerInstance;
};



#endif
