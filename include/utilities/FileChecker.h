#ifndef NGEN_FILECHECKER_H
#define NGEN_FILECHECKER_H

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <cerrno>

namespace utils {

    /**
     * Simple utility class for testing things about one or more files.
     */
    class FileChecker {
    public:

        /**
         * Get whether a write to a file at the given path is permitted.
         *
         * Get whether the given file can be written to.  To be true, the file must either already exist and have
         * permissions that allow it to be written to, or the file must not exist and the parent directory permissions
         * must allow for a new file to be created and written to.
         *
         * @param path The relative or absolute filesystem path of interest, as a string.
         * @return True if writes to this new or existing file are allowed, or False otherwise.
         */
        static bool file_can_be_written(std::string path) {
            // TODO: This is probably easier to handle in C++ 17

            // Get whether it is just readable
            bool isJustReadable = file_is_readable(path);

            // This checks if file exists AND is readable AND is writeable (but is false with error if not existing)
            if (FILE *file_r_ex = fopen(path.c_str(), "r+")) {
                fclose(file_r_ex);
                return true;
            }
            // Clear the error, as it is really more of a side-effect of the check than a problem
            errno = 0;
            // If things get here and the file is readable, it must also exist, so it must not be writeable
            if (file_is_readable(path)) {
                return false;
            }
            // Again, clear the error
            errno = 0;
            // The file could be existing and writeable (but not readable); not existing; or existing and not writeable

            // This will check if the file is writeable, but will also create it if it didn't exist
            if (FILE *file_w_a = fopen(path.c_str(), "a")) {
                fclose(file_w_a);
                // To see if the file just got create (and needs to be removed), check if it is empty
                if (file_is_empty(path)) {
                    remove(path.c_str());
                }
                return true;
            }
            // Again, clear the error
            errno = 0;
            return false;
        }

        /**
         * Checks whether a file is empty.
         *
         * Function assumes the given file exists and will return ``false`` if it cannot open the file.
         *
         * @param path The relative or absolute filesystem path of interest, as a string.
         * @return Whether the file is determined to be empty.
         */
        static bool file_is_empty(const std::string &path) {
            bool emptyFile = false;
            char ch;
            if (FILE *f = fopen(path.c_str(), "r")) {
                if (fscanf(f,"%c",&ch) == EOF) {
                    emptyFile = true;
                }
                fclose(f);
            }
            errno = 0;
            return emptyFile;
        }

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
         * Check whether the path provided points to an existing, readable file, printing a message when it does not.
         *
         * The only difference between this and the overloaded function of the same name with a single parameter is this
         * function may also print a message. When a file is found to not be readable, a message indicating this is sent
         * to stdout, with its format being:
         *  ``<description> path <path> not readable``
         *
         * For example, if ``description`` is 'Configuration file' and ``path`` is '/home/user/file.txt', then the text
         * 'Configuration file path /home/user/file.txt not readable' is sent to stdout.
         *
         * @param path The relative or absolute filesystem path of interest, as a string.
         * @param description A name or description of file to make a printed message more informative.
         * @return True if a file exists at the provided path and is readable, or False otherwise.
         * @see file_is_readable(std::string)
         */
        static bool file_is_readable(const std::string& path, const std::string& description) {
            if (file_is_readable(path)) {
                return true;
            }
            else {
                std::cout << description << " path " << path << " not readable" << std::endl;
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
