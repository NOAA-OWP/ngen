#ifndef HY_CATCHMENTDIVIDE_H
#define HY_CATCHMENTDIVIDE_H

#include "HY_CatchmentRealization.hpp"
#include "GM_Object.hpp"

#include <memory>
#include <string>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

namespace bg = boost::geometry;

class HY_CatchmentDivide : public HY_CatchmentRealization, public GM_Object
{
    public:

    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
    typedef bg::model::linestring<point_t> linestring_t;

    HY_CatchmentDivide();
    virtual ~HY_CatchmentDivide();

    protected:

    private:

    std::shared_ptr<linestring_t> divide_path;

    unsigned long id_number;
    std::string id;
};
#endif // HY_CATCHMENTDIVIDE_H
