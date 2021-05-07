#ifndef HY_HYDROLOCATION_HPP
#define HY_HYDROLOCATION_HPP

#include <memory>

#include <GM_Object.hpp>
#include <HY_HydroLocationType.hpp>
#include <HY_IndirectPosition.hpp>

//! HY_HydroLocation: class representing a hydrologic position
/**
    HY_HydroLocation

    This class represents a location where a hydrologic interaction of note is performed.
*/
namespace hy_features {namespace hydrolocation {

class HY_HydroLocation : public GM_Object
{
    //using Nexus = std::shared_ptr<HY_HydroNexus>
    using Nexus = std::string;
    public:

    //! Constructor for when the attached nexus is not known
    HY_HydroLocation(point_t shape, HY_HydroLocationType input_type, HY_IndirectPosition referenced_position) :
        _shape(shape), _type(input_type), _referenced_position(referenced_position), _realized_nexus()
    {}

    //! Constructor for when attached nexus is known
    HY_HydroLocation(point_t shape, HY_HydroLocationType input_type, HY_IndirectPosition referenced_position, Nexus nexus) :
        _shape(shape), _type(input_type), _referenced_position(referenced_position), _realized_nexus(nexus)
    {}

    //! get the shape of the location
    point_t& geometry() { return _shape; }
    const point_t& geometry() const { return _shape; }

    //! get the type if the location
    HY_HydroLocationType& itype() { return _type; }
    const HY_HydroLocationType& itype() const { return _type; }

    //! get the central point for the location
    HY_IndirectPosition& referenced_position() { return _referenced_position; }
    const HY_IndirectPosition& referenced_position() const { return _referenced_position; }

    //! get the attached nexus for this location
    Nexus& realized_nexus() { return _realized_nexus; }
    const Nexus& realized_nexus() const { return _realized_nexus; }


    private:

    point_t _shape;                                //!< shape for location
    HY_HydroLocationType _type;                         //!< type for location
    HY_IndirectPosition _referenced_position;                       //!< reference position for location
    Nexus _realized_nexus;     //!< nexus connected to this location
};
}//end hydrolocation namespace
}//end hy_features namespace

#endif // HY_HYDROLOCATION_HPP
