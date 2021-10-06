#ifndef HY_HYDRONEXUS_H
#define HY_HYDRONEXUS_H

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <HY_Catchment.hpp>
#include <HY_HydroLocation.hpp>

using namespace hy_features::hydrolocation;
class HY_HydroNexus
{
    public:

    typedef long time_step_t;

    //using Catchments = std::vector<std::shared_ptr<HY_Catchment>>;
    using Catchments = std::vector<std::string>;
    using HydroLocation = std::shared_ptr<HY_HydroLocation>;

    HY_HydroNexus(std::string nexus_id, Catchments receiving_catchments);
    HY_HydroNexus(std::string nexus_id, Catchments receiving_catchments, Catchments contributing_catchments);
    HY_HydroNexus(std::string nexus_id, HydroLocation location, Catchments receiving_catchments, Catchments contributing_catchments);

    virtual ~HY_HydroNexus();

    //NJF Why protect these intrfaces??? protected:

    /** Increase the downstream flow for timestep_t by input amount*/
    virtual void add_upstream_flow(double val, std::string catchement_id, time_step_t t)=0;

    /** get a precentage of the downstream flow at requested time_step. Record the requesting percentage*/
    virtual double get_downstream_flow(std::string catchment_id, time_step_t t, double percent_flow)=0;

    virtual std::pair<double, int> inspect_upstream_flows(time_step_t t)=0;
    virtual std::pair<double, int> inspect_downstream_requests(time_step_t t)=0;

    /** get the units that the flows are described in */
    virtual std::string get_flow_units()=0;
    
    const Catchments& get_receiving_catchments() {
        return receiving_catchments;
    }

    const Catchments& get_contributing_catchments() {
    return contributing_catchments;
    }

    std::string& get_id() { return id; }

    protected:

    //Identity is a string.  If implementations NEED a numeric id for indexing,
    //they are responsible for prodcing it.
    std::string id;

    private:
    /*
    * identifies a hydrologic feature which realizes the hydro nexus.
    * A topological nexus realization is of lower dimension than the realization
    * of the corresponding catchment.
    * Example: an outflow node realizing the nexus would be of lower dimension
    * than the flowpath edge realizing the contributing catchment.
    */
    HydroLocation realization;

    /*
    * identifies the catchment that receives flow from this hydro nexus.
    * This allows connection of a catchment's inflow to an identified outflow
    * and to determine its position through referencing the outflow.
    */
    Catchments receiving_catchments;

    /*
    * identifies the catchment that contributes flow to this hydro nexus.
    * This allows connection of a catchment's outflow to an identified inflow
    * and to determine its position through referencing the inflow.
    */
    Catchments contributing_catchments;

    //TODO remove this?
    int number_of_downstream_catchments;
};

#endif // HY_HYDRONEXUS_H
