#ifndef HY_CATCHMENTAREA_H
#define HY_CATCHMENTAREA_H

#include "HY_CatchmentRealization.hpp"

#include "GM_Object.hpp"


class HY_CatchmentArea : public HY_CatchmentRealization, public GM_Object
{
    public:

    HY_CatchmentArea();
    virtual ~HY_CatchmentArea();

    protected:

    polygon_t bounds;

    private:
};

#endif // HY_CATCHMENTAREA_H
