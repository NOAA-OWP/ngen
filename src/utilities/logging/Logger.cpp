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
    std::string fwd_slash = "/";
    std::string logFileName = "ngen_log.txt";
    std::string logFileDir = "/ngencerf/data/run-logs/ngen_" + Logger::createTimestamp() + fwd_slash;
    std::cout << "Log File Directory:" << logFileDir << std::endl;

	if (!logFileName.empty()) {
    	// creating the directory
   		int status;
		std::string mkdir_cmd = "mkdir -p " + logFileDir;
		const char *cstr = mkdir_cmd.c_str();
   		status = system(cstr);
   		if (status == -1)
   		   std::cerr << "Error : " << strerror(errno) << std::endl;
   		else
   		   std::cout << "Directories are created" << std::endl;
		// creating the file
		logFile.open(logFileDir+logFileName, ios::out);
		if (!logFile.good()) {
			std::cerr << "Can't Open Log File" << std::endl;
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
* @param codeFile: __FILE__
* @param codeLine: __LINE__
* @param message: Log Message
* @param messageLevel: Log Level, LogLevel::DEBUG by default
*/
void Logger::Log(std::string message, LogLevel messageLevel = LogLevel::DEBUG, LoggingModule module=LoggingModule::NGEN) {
	// don't log if messageLevel < logLevel 
	if (messageLevel >= logLevel) {
		std::string logType;
		//Set Log Level Name
		switch (messageLevel) {
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
    char buffer[80];
    
    long transformed = currentTime.time_since_epoch().count() / 1000000;
    
    long millis = transformed % 1000;
    
    std::time_t tt;
    tt = std::time(0);
    tm *timeinfo = std::gmtime(&tt);
    strftime (buffer,80,"%FT%H:%M:%S",timeinfo);
    sprintf(buffer, "%s:%03d",buffer,(int)millis);
    
    return std::string(buffer);
}
