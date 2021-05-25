#ifndef GM_OBJECT_H
#define GM_OBJECT_H

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
//namespace bg = boost::geometry;
typedef boost::geometry::model::d2::point_xy<double> point_t;
typedef boost::geometry::model::polygon<point_t> polygon_t;

class GM_Object
{
    public:
        GM_Object();
        virtual ~GM_Object();

    protected:

    private:
};

#endif // GM_OBJECT_H
