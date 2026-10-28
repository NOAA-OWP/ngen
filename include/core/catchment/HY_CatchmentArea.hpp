#ifndef HY_CATCHMENTAREA_H
#define HY_CATCHMENTAREA_H

#include "HY_CatchmentRealization.hpp"

#include "GM_Object.hpp"


/**
 * The two-dimensional realization of a catchment: the face it covers.
 *
 * Topologically a face bounded by catchment divide and flowpath edges, so this is the
 * perspective to ask for an areal extent -- the denominator when a value is distributed over
 * the catchment.
 */
class HY_CatchmentArea : public HY_CatchmentRealization, public GM_Object
{
    public:

    HY_CatchmentArea();
    HY_CatchmentArea(std::string catchment_id, std::shared_ptr<realization::Formulation> formulation);
    virtual ~HY_CatchmentArea();

    protected:

    polygon_t bounds;

    private:
};

#endif // HY_CATCHMENTAREA_H
