#ifndef HY_CATCHMENTAREA_H
#define HY_CATCHMENTAREA_H

#include "HY_CatchmentRealization.h"

#include "GM_Object.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

namespace bg = boost::geometry;

class HY_CatchmentArea : public HY_CatchmentRealization, public GM_Object
{
    public:

    typedef bg::model::d2::point_xy<double> point_t;
    typedef bg::model::polygon<point_t> polygon_t;

    HY_CatchmentArea();
    virtual ~HY_CatchmentArea();

    protected:

    polygon_t bounds;

    private:
};

#endif // HY_CATCHMENTAREA_H
