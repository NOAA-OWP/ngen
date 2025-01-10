#include <stdlib.h>
#include "Logger.hpp"
#include <cstring>
#include <string>
#include <chrono>

std::shared_ptr<Logger> Logger::loggerInstance;

using namespace std;

std::string module_name[static_cast<int>(LoggingModule::MODULE_COUNT)] 
{
	"NGEN    ",
	"NOAHOWP ", 
	"SNOW17  ", 
	"UEB     ", 
	"CFE     ", 
	"SACSMA  ", 
	"LASAM   ", 
	"SMP     ", 
	"SFT     ", 
	"TROUTE  ", 
	"SCHISM  ", 
	"SFINCS  ", 
	"GC2D    ", 
	"TOPOFLOW",
};

/**
* Configure Logger Preferences
* @param logFile
* @param level: LogLevel::ERROR by Default
* @param output: LogOutput::CONSOLE by Default
* @return void
*/
void Logger::SetLogPreferences(LogLevel level = LogLevel::ERROR) {
	logLevel = level;
	std::stringstream ss("");

	// get the log file path
	ss << getenv("NGEN_RESULTS_DIR");
    std::string fwd_slash = "/";
 	std::string logFileDir;
	if (ss.str() != "")
		logFileDir = ss.str() + fwd_slash + "logs" + fwd_slash;
	else
		logFileDir = "/ngencerf/data/run-logs/ngen_" + Logger::createDateString() + fwd_slash;

	ss.str("");
    std::string logFileName = "ngen.log";
    std::string stdout_logFileName = "ngen.stdout";
    std::string stderr_logFileName = "ngen.stderr";

    // creating the directory
   	int status;
	std::string mkdir_cmd = "mkdir -p " + logFileDir;
	const char *cstr = mkdir_cmd.c_str();
   	status = system(cstr);
   	if (status == -1)
   	   	std::cerr << "Error(" << (errno) << ") creating log file directory for NGEN: " << logFileDir << std::endl;
   	else {
   	   	std::cout << "Log directory: " << logFileDir <<std::endl;
#if 0
		// creating the stdout/stderr log files
		std::string stdout_logFilePath = logFileDir+stdout_logFileName;
		std::string stderr_logFilePath = logFileDir+stderr_logFileName;
    	// Open stdout log file for writing
    	stdout_logFile = freopen(stdout_logFilePath.c_str(), "a", stdout);
    	if (stdout_logFile == NULL) {
    	    std::cerr << "Error opening ngen.stdout log file." << std::endl;
    	}
		else {
			std::cout << "Log file path for STDOUT:" << stdout_logFilePath << std::endl;
		}

    	// Open stderr log file for writing
    	stderr_logFile = freopen(stderr_logFilePath.c_str(), "a", stderr);
    	if (stderr_logFile == NULL) {
    	    std::cerr << "Error opening ngen.stderr log file." << std::endl;
    	}
		else {
			std::cerr << "Log file path for STDERR:" << stderr_logFilePath << std::endl;
		}
#endif

		// creating the log file
		logFilePath = logFileDir+logFileName;
		logFile.open(logFilePath, ios::out | ios::app);
		if (!logFile.good()) {
			std::cerr << "Warning: Can't Open Log File for NGEN: " << logFilePath << std::endl;
			// try logging to a file in local directory
    		std::string logFileDir = "./run-logs/ngen_" + Logger::createTimestamp() + fwd_slash;
			mkdir_cmd = "mkdir -p " + logFileDir;
			cstr = mkdir_cmd.c_str();
			status = system(cstr);
   			if (status == -1)
   	   			std::cerr << "Error(" << (errno) << ") creating log file directory: " << logFileDir << std::endl;
   			else {
				logFilePath = logFileDir+logFileName;
				logFile.open(logFilePath, ios::out | ios::app);
				if (!logFile.good()) {
					std::cerr << "Can't Open local directory Log File for NGEN:" << logFilePath <<std::endl;			
				}
				else {
					std::cout << "ngen is logging instead into: " << logFilePath << std::endl;
					setenv("NGEN_LOG_FILE_PATH", (char *)logFilePath.c_str(), 1);
				}
			}
		}
		else {
			std::cout << "NGEN Log File Path:" << logFilePath << std::endl;
			setenv("NGEN_LOG_FILE_PATH", (char *)logFilePath.c_str(), 1);
		}
	}
	
	std::cout << "NGEN Log Level is set at: " << Logger::getLogLevelString(logLevel) << std::endl;
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
* Convert LogLevel to String Representation of Log Level 
* @param logLevel : LogLevel
* @return String log level
*/
std::string Logger::getLogLevelString(LogLevel level) {
	std::string logType;
	//Set Log Level Name
	switch (level) {
		case LogLevel::FATAL:
			logType = "FATAL ";
			break;
		case LogLevel::DEBUG:
			logType = "DEBUG ";
			break;
		case LogLevel::INFO:
			logType = "INFO  ";
			break;
		case LogLevel::WARN:
			logType = "WARN  ";
			break;
		case LogLevel::ERROR:
			logType = "ERROR ";
			break;
		default:
			logType = "NONE  ";
			break;
	}
	return logType;
}

/**
* Log given message with defined parameters and generate message to pass on Console or File
* @param message: Log Message
* @param messageLevel: Log Level, LogLevel::DEBUG by default
*/
void Logger::Log(std::string message, LogLevel messageLevel = LogLevel::DEBUG) {
	LoggingModule module=LoggingModule::NGEN;

	// don't log if messageLevel < logLevel 
	if (messageLevel >= logLevel) {
		std::string logType;
		logType = Logger::getLogLevelString(messageLevel);

		std::string final_message;
		std::string mod_name;
		mod_name = module_name[static_cast<int>(module)];
		std::string separator = " ";

		// make sure message has endl at the end
   	 	if (message.find("\n", message.length()-1) == std::string::npos) {
			message = message + "\n";
    	}

		final_message = createTimestamp() + separator + mod_name + separator + logType + message;
		logFile << final_message;
		logFile.flush();
	}
}


/**
* Convert String Representation of Log Level to LogLevel Type
* @param logLevel : String log level
* @return LogLevel
*/
LogLevel Logger::GetLogLevel(const std::string& logLevel) {
	if (logLevel == "DEBUG") {
		return LogLevel::DEBUG;
	}
	else if (logLevel == "INFO") {
		return LogLevel::INFO;
	}
	else if (logLevel == "WARN") {
		return LogLevel::ERROR;
	}
	else if (logLevel == "ERROR") {
		return LogLevel::ERROR;
	}
	else if (logLevel == "FATAL") {
		return LogLevel::ERROR;
	}

	return LogLevel::NONE;
}

using std::chrono::system_clock;

std::string Logger::createTimestamp() {
    std::chrono::_V2::system_clock::time_point currentTime = std::chrono::system_clock::now();
    char buffer1[80];
    char buffer2[80];
	std::stringstream ss;
    
    long transformed = currentTime.time_since_epoch().count() / 1000000;
    
    long millis = transformed % 1000;
    
    std::time_t tt;
    tt = std::time(0);
    tm *timeinfo = std::gmtime(&tt);
    strftime (buffer1,100,"%FT%H:%M:%S",timeinfo);
    sprintf(buffer2, ".%03d", (int)millis);
	ss << buffer1 << buffer2;
    
    return ss.str();
}

std::string Logger::createDateString() {
    char buffer1[80];
    std::time_t tt;
	std::stringstream ss;

    tt = std::time(0);
    tm *timeinfo = std::gmtime(&tt);
    strftime (buffer1,100,"%F",timeinfo);
	ss << buffer1;

    return ss.str();
}

std::string Logger::getLogFilePath() {
	return logFilePath;
}
