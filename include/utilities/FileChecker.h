#ifndef NGEN_FILECHECKER_H
#define NGEN_FILECHECKER_H

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

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

        /**
         * Find the first file path in a provided collection that points to an existing file accessible as specified,
         * proceeding through the collection in indexed order.
         *
         * @param mode The accessibility mode that the file must be accessible with, as used with `fopen`.
         * @param paths A vector of potential file paths, as strings.
         * @return The first encountered path string of an accessible file, or empty string if no such path was seen.
         */
        static std::string find_first_accessible(std::string mode, std::vector<std::string> paths)
        {
            for (unsigned int i = 0; i < paths.size(); i++) {
                // Trim whitespace
                boost::trim(paths[i]);
                // Skip empty
                if (paths[i].empty()) {
                    continue;
                }
                // Return if the file will open
                if (FILE *file = fopen(paths[i].c_str(), mode.c_str())) {
                    fclose(file);
                    return paths[i];
                }
            }
            return "";
        }

        /**
         * Find the first file path in a provided collection that points to an existing, readable file, proceeding
         * through the collection in indexed order.
         *
         * @param paths A vector of potential file paths, as strings.
         * @return The first encountered path string of a readable file, or empty string if no such path was seen.
         */
        static std::string find_first_readable(std::vector<std::string> paths)
        {
            return FileChecker::find_first_accessible("r", paths);
        }

    };

}

#endif //NGEN_FILECHECKER_H
