#ifndef HY_CATCHMENTAREA_H
#define HY_CATCHMENTAREA_H

#include "HY_CatchmentRealization.hpp"

#include "GM_Object.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include "StreamHandler.hpp"

namespace bg = boost::geometry;

class HY_CatchmentArea : public HY_CatchmentRealization, public GM_Object
{
    public:

    typedef bg::model::d2::point_xy<double> point_t;
    typedef bg::model::polygon<point_t> polygon_t;

    HY_CatchmentArea();
    HY_CatchmentArea(forcing_params forcing_config, utils::StreamHandler output_stream); //TODO not sure I like this pattern
    virtual ~HY_CatchmentArea();

    protected:

    polygon_t bounds;

    utils::StreamHandler output;

    private:
};

#endif // HY_CATCHMENTAREA_H
