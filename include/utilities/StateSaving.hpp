#ifndef STATESAVING_HPP
#define STATESAVING_HPP

#include <NGenConfig.h>

struct UnitSaver
{
    virtual void save(boost::span<const char> data) = 0;
};

#endif // STATESAVING_HPP
