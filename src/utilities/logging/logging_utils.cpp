#include <iostream>
#include <cstring>
#include <string>
#include "logging_utils.h"
#include "Logger.hpp"

namespace logging {

#ifdef __cplusplus
extern "C" {
#endif

    void debug(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"DEBUG: " << std::string(msg);
            LOG(msg, LogLevel::DEBUG);
        #endif
    }

    void info(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"INFO: " << std::string(msg);
            LOG(msg, LogLevel::INFO);
        #endif
    }

    void warning(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"WARNING: " <<std::string(msg);
            LOG(msg, LogLevel::WARN);
        #endif
    }

    void error(const char* msg)
    {
        std::cerr<<"ERROR: " <<std::string(msg);
        LOG(msg, LogLevel::ERROR);
    }

    void critical(const char* msg)
    {
        std::cerr<<"CRITICAL: " <<std::string(msg);
        LOG(msg, LogLevel::FATAL);
    }

#ifdef     __cplusplus
}
#endif
}
