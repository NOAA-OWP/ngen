#ifndef HY_FLOWPATH_H
#define HY_FLOWPATH_H


#include "HY_CatchmentRealization.h"

#include "GM_Object.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include <memory>
#include <string>

namespace bg = boost::geometry;

class HY_FlowPath : public GM_Object, public HY_CatchmentRealization
{
    public:

    typedef bg::model::d2::point_xy<double> point_t;
    typedef bg::model::linestring<point_t> linestring_t;

    HY_FlowPath();
    virtual ~HY_FlowPath();

    protected:

    private:

    std::shared_ptr<linestring_t> flow_path;

};

#endif // HY_FLOWPATH_H
