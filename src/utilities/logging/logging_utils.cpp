#include <iostream>
#include <cstring>
#include <string>
#include "logging_utils.h"

namespace logging {

#ifdef __cplusplus
extern "C" {
#endif

    void debug(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"DEBUG: " << std::string(msg);
        #endif
    }

    void info(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"INFO: " << std::string(msg);
        #endif
    }

    void warning(const char* msg)
    {
        #ifndef NGEN_QUIET
            std::cerr<<"WARNING: " <<std::string(msg);
        #endif
    }

    void error(const char* msg)
    {
        std::cerr<<"ERROR: " <<std::string(msg);
    }

    void critical(const char* msg)
    {
        std::cerr<<"CRITICAL: " <<std::string(msg);
    }

#ifdef     __cplusplus
}
#endif
}
