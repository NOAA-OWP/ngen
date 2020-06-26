#ifndef NGEN_FILECHECKER_H
#define NGEN_FILECHECKER_H

#include <string>

namespace utils {

    /**
     * Simple utility class for testing things about one or more files.
     */
    class FileChecker {
    public:
        /**
         * Check whether the path provided points to an existing, readable file.
         *
         * @param path The relative or absolute filesystem path of interest, as a string.
         * @return True if a file exists at the provided path and is readable, or False otherwise.
         */
        static bool file_is_readable(std::string path)
        {
            if (FILE *file = fopen(path.c_str(), "r")) {
                fclose(file);
                return true;
            }
            else {
                return false;
            }
        }

    };

}

#endif //NGEN_FILECHECKER_H
