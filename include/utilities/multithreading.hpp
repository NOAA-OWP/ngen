#ifndef NGEN_MULTITHREADING_HPP
#define NGEN_MULTITHREADING_HPP

#include <cstddef>
#include <mutex>

namespace multithreading
{
    namespace details
    {
        // A string usable as a template argument, so that
        // library_mutex() can be keyed by a library's name
        template <std::size_t N>
        struct string_literal {
            constexpr string_literal(const char (&str)[N]) {
                for (std::size_t i = 0; i < N; ++i)
                    value[i] = str[i];
            }

            char value[N];
        };

        // The one mutex in the process for the library named @arg
        // name, created on first use. Exported, so that it stays one per
        // process even if ngen is built as shared libraries with hidden
        // visibility. Call library_mutex() rather than this.
        [[gnu::visibility("default")]] std::mutex& named_library_mutex(char const* name);
    }

    // The mutex serializing calls into the library named @arg Name,
    // which can't be called from more than one thread at a time. Every
    // call into that library, anywhere in the process, must hold it:
    //
    //     std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());
    //
    // "netcdf" is the NetCDF library, and the HDF5 library beneath it
    // unless that was built thread-safe, which can't be called from more
    // than one thread at a time even for different files.
    //
    // It's a plain std::mutex, so regions holding it must not nest.
    //
    // Each executable or shared library that calls this gets its own
    // copy of the reference below, but they all refer to the one mutex
    // that details::named_library_mutex() keeps for Name. A static
    // mutex here instead would be one per shared library unless
    // exported, which GCC won't do for a template specialized on a
    // string_literal.
    template <details::string_literal Name>
    std::mutex& library_mutex()
    {
        static std::mutex& mutex = details::named_library_mutex(Name.value);
        return mutex;
    }
}

#endif // NGEN_MULTITHREADING_HPP
