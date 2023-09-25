#ifndef NGEN_LOGGING_UTILS_H
#define NGEN_LOGGING_UTILS_H

#define MAX_STRING_SIZE 2048

#ifdef __cplusplus
extern "C" {
#endif

namespace logging {
    /**
     * Send debug output to std::cerr
     * @param msg The variable carries the humanly readable debug text info.
     */
    void debug(const char* msg);

    /**
     * Send info content to std::cerr
     * @param msg The variable carries the humanly readable info text.
     */
    void info(const char* msg);

    /**
     * Send warning to std::cerr
     * @param msg The variable carries the humanly readable warning message.
     */
    void warning(const char* msg);

    /**
     * Send error message to std::cerr
     * @param msg The variable carries the humanly readable error message.
     */
    void error(const char* msg);

    /**
     * Send critical message to std::cerr
     * @param msg The variable carries the humanly readable critical message.
     */
    void critical(const char* msg);
}

#ifdef     __cplusplus
}
#endif

#endif //NGEN_LOGGING_UTILS_H
