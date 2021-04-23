#ifndef HY_HYDROLOCATION_HPP
#define HY_HYDROLOCATION_HPP

#include <memory>

#include "GM_Object.hpp"
#include "HY_HydroLocationType.hpp"

//! HY_HydroLocation: class representing a hydrologic position
/**
    HY_HydroLocation

    This class represents a location where a hydrologic interaction of note is performed.
*/

class HY_HydroLocation : public GM_Object
{
    public:

    typedef bg::model::d2::point_xy<double> point_t;    //!< the type for a 2d point
    typedef bg::model::polygon<point_t> polygon_t       //!< the type for the polygon of the location

    //! Constructor for when the attached nexus is not known
    HY_HydroLocation(polygon_type input_shape, HY_HydroLocationType input_type, point_t input_point) :
        _shape(input_shape), _type(input_type), _point(input_point), _realized_nexus()
    {}

    //! Constructor for when attached nexus is know
    HY_HydroLocation(polygon_type input_shape, HY_HydroLocationType input_type, point_t input_point, std::shared_ptr<HY_HydroNexus> nexus) :
        _shape(input_shape), _type(input_type), _point(input_point), _realized_nexus(nexus)
    {}

    //! get the shape of the location
    std::shared_ptr<HY_HydroNexus>& geometry() { return _shape; }
    const std::shared_ptr<HY_HydroNexus>& geometry() const { return _shape; }

    //! get the type if the location
    HY_HydroLocationType& itype() { return _type; }
    const HY_HydroLocationType& itype() const { return _type; }

    //! get the central point for the location
    point_t& point() { return _point; }
    const point_t& point() const { return _point; }

    //! get the attached nexus for this location
    std::shared_ptr<HY_HydroNexus>& realized_nexus() { return _realized_nexus; }
    const std::shared_ptr<HY_HydroNexus>& realized_nexus() const { return _realized_nexus; }


    private:

    polygon_type _shape;                                //!< shape for location
    HY_HydroLocationType _type;                         //!< type for location
    point_t _referenced_position;                       //!< reference position for location
    std::shared_ptr<HY_HydroNexus> _realized_nexus;     //!< nexus connected to this location
}


#endif // HY_HYDROLOCATION_HPP
