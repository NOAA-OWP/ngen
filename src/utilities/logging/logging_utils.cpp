#include <iostream>
#include <cstring>
#include <mutex>
#include <string>
#include "logging_utils.h"
#include "Logger.hpp"

namespace logging {

    std::mutex logging_mutex;

#ifdef __cplusplus
extern "C" {
#endif

    void debug(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::lock_guard l(logging_mutex);
            std::cerr<<"DEBUG: " << std::string(msg); // Necessary for heritiage unit testing only.
            LOG(msg, LogLevel::DEBUG);
        #endif
    }

    void info(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::lock_guard l(logging_mutex);
            std::cerr<<"INFO: " << std::string(msg); // Necessary for heritiage unit testing only.
            LOG(msg, LogLevel::INFO);
        #endif
    }

    void warning(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::lock_guard l(logging_mutex);
            std::cerr<<"WARNING: " <<std::string(msg); // Necessary for heritiage unit testing only.
            LOG(msg, LogLevel::SEVERE);
        #endif
    }

    void error(const char* msg)
    {
        std::lock_guard l(logging_mutex);
        std::cerr<<"ERROR: " <<std::string(msg); // Necessary for heritiage unit testing only.
        LOG(msg, LogLevel::WARNING);
    }

    void critical(const char* msg)
    {
        std::lock_guard l(logging_mutex);
        std::cerr<<"CRITICAL: " <<std::string(msg); // Necessary for heritiage unit testing only.
        LOG(msg, LogLevel::FATAL);
    }

#ifdef     __cplusplus
}
#endif
}
