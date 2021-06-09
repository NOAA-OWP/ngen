#ifndef GENERIC_ADAPTER_H
#define GENERIC_ADAPTER_H

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <HY_Catchment.hpp>
#include <HY_HydroLocation.hpp>

using namespace hy_features::hydrolocation;
class GenericAdapter
{
    public:

    typedef long time_step_t;


    // create type aliases
    using Models = std::vector<std::string>;
    using long = time_point_t;
    using long = time_delta;

    GenericAdapter(std::string adapter_id, Models ls, Models rs);

    virtual ~Adapter2D();

    /** Contribute a flux with id flux_id from source contrib_id at time step t*/

    virtual void add_flux(std::string flux_id, std::string contrib_id, time_point_t t, time_delta t1, double* vals)= 0;

    /** Remove a contributed flux as aggreated between time_steps t1 and t2*/

    virtual void remove_flux(std::string flux_id, std::string requesting_id, time_point_t t1, time_delta t2, double* vals) = 0;

    virtual std::vector< std::pair<double*, std::string> > inspect_flux(std::string flux_id, time_point t)=0;

    /** get the units that the flows are described in */
    virtual std::string get_flux_units(std::string flux_id)=0;

    protected:

    //Identity is a string.  If implementations NEED a numeric id for indexing,
    //they are responsible for prodcing it.
    std::string id;

    private:

    /*
    * Identifiters for one side of this exchange
    */

    Models left_side_models;

    /*
    * Identifiters for one side of this exchange
    */

    Models right_side_models;

    /*
    * the geometric shapes of fluxes contributed on the left side of the interface
    */

};

#endif // GENERIC_ADAPTER_H
