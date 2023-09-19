#ifndef NGEN_LOGGING_UTILS_H
#define NGEN_LOGGING_UTILS_H

#include <iostream>
#include <cstring>
#include <string>

#define MAX_STRING_SIZE 2048

#ifdef __cplusplus
extern "C" {
#endif

namespace logging {
    /**
     * Send debug output to std::cerr
     * @param msg The variable carries the humanly readable debug text info.
     */
    inline void debug(char* msg)
    {
        #ifndef NGEN_QUIET
            std::string msg_str = std::string(msg);
            if (msg_str.size() > MAX_STRING_SIZE) {
                std::cerr << "DEBUG: the message string exceeds the maximum length limit" << std::endl;
            }

            std::cerr<<"DEBUG: " << msg_str << std::endl;
        #endif
    }

    /**
     * Send info content to std::cerr
     * @param msg The variable carries the humanly readable info text.
     */
    inline void info(char* msg)
    {
        #ifndef NGEN_QUIET
            std::string msg_str = std::string(msg);
            if (msg_str.size() > MAX_STRING_SIZE) {
                std::cerr << "INFO: the message string exceeds the maximum length limit" << std::endl;
            }

            std::cerr<<"INFO: " <<std::string(msg) << std::endl;
        #endif
    }

    /**
     * Send warning to std::cerr
     * @param msg The variable carries the humanly readable warning message.
     */
    inline void warning(char* msg)
    {
        #ifndef NGEN_QUIET
            std::string msg_str = std::string(msg);
            if (msg_str.size() > MAX_STRING_SIZE) {
                std::cerr << "WARNING: the message string exceeds the maximum length limit" << std::endl;
            }

            std::cerr<<"WARNING: " <<std::string(msg) << std::endl;
        #endif
    }

    /**
     * Send error message to std::cerr
     * @param msg The variable carries the humanly readable error message.
     */
    inline void error(char* msg)
    {
        std::string msg_str = std::string(msg);
        if (msg_str.size() > MAX_STRING_SIZE) {
            std::cerr << "ERROR: the message string exceeds the maximum length limit" << std::endl;
        }

        std::cerr<<"ERROR: " <<std::string(msg) << std::endl;
    }

    /**
     * Send critical message to std::cerr
     * @param msg The variable carries the humanly readable critical message.
     */
    inline void critical(char* msg)
    {
        std::string msg_str = std::string(msg);
        if (msg_str.size() > MAX_STRING_SIZE) {
            std::cerr << "CRITICAL: the message string exceeds the maximum length limit" << std::endl;
        }

        std::cerr<<"CRITICAL: " <<std::string(msg) << std::endl;
    }
}

#ifdef     __cplusplus
}
#endif

#endif //NGEN_LOGGING_UTILS_H
