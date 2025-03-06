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
            LOG(msg, LogLevel::DEBUG);
        #endif
    }

    void info(const char* msg)
    {
        #ifndef NGEN_QUIET
            LOG(msg, LogLevel::INFO);
        #endif
    }

    void warning(const char* msg)
    {
        #ifndef NGEN_QUIET
            LOG(msg, LogLevel::WARN);
        #endif
    }

    void error(const char* msg)
    {
        LOG(msg, LogLevel::ERROR);
    }

    void critical(const char* msg)
    {
        LOG(msg, LogLevel::FATAL);
    }

#ifdef     __cplusplus
}
#endif
}
