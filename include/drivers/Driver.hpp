#pragma once

#include <string>

namespace ngen {

template<typename Tp>
struct Driver
{
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;
    Driver(Driver&&) = delete;
    Driver& operator=(Driver&&) = delete;
    virtual ~Driver() = default;

    virtual Tp read(const std::string& input) = 0;
};

} // namespace ngen
