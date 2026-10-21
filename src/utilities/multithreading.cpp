#include "utilities/multithreading.hpp"

#include <functional>
#include <map>
#include <string>
#include <system_error>

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

    thread_key::thread_key()
    {
        int err = pthread_key_create(&key, nullptr);
        if (err != 0)
            throw std::system_error(err, std::generic_category(), "pthread_key_create");
    }

    thread_key::~thread_key()
    {
        pthread_key_delete(key);
    }
}
