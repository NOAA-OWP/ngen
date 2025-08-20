#ifndef STATESAVING_HPP
#define STATESAVING_HPP

#include <NGenConfig.hpp>

struct UnitSaver
{
    virtual save(boost::span<const char> data) = 0;
};

#endif // STATESAVING_HPP
