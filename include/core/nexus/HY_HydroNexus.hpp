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

    HY_HydroNexus(std::string nexus_id, Catchments contributing_catchments);
    HY_HydroNexus(std::string nexus_id, HydroLocation location, Catchments receiving_catchments, Catchments contributing_catchments);

    virtual ~HY_HydroNexus();

    //NJF Why protect these intrfaces??? protected:

    /** Increase the downstream flow for timestep_t by input amount*/
    virtual void add_upstream_flow(double val, long catchement_id, time_step_t t)=0;

    /** get a precentage of the downstream flow at requested time_step. Record the requesting percentage*/
    virtual double get_downstream_flow(long catchment_id, time_step_t t, double percent_flow)=0;

    virtual std::pair<double, int> inspect_upstream_flows(time_step_t t)=0;
    virtual std::pair<double, int> inspect_downstream_requests(time_step_t t)=0;

    /** get the units that the flows are described in */
    virtual std::string get_flow_units()=0;

    private:
    long id_number;
    std::string id;
    HydroLocation realization;
    Catchments receiving_catchments;
    Catchments contributing_catchments;


    int number_of_downstream_catchments;
};

#endif // HY_HYDRONEXUS_H
