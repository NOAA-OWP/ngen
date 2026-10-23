#include "utilities/multithreading.hpp"

#include <functional>
#include <map>
#include <string>

namespace multithreading
{
    std::mutex& details::named_library_mutex(char const* name)
    {
        // Constant initialized, by std::mutex's constexpr constructor
        static std::mutex mutexes_mutex;
        // Never destroyed, so the mutexes outlive any static object that
        // may still lock them at exit, such as a shared NetCDF provider.
        // std::map never moves its elements, so references to them stay
        // valid as names are added.
        static auto* mutexes = new std::map<std::string, std::mutex, std::less<>>();

        std::lock_guard l{mutexes_mutex};
        return (*mutexes)[name];
    }
}
