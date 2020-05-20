#ifndef HY_HYDRONEXUS_H
#define HY_HYDRONEXUS_H

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>


namespace bg = boost::geometry;

class HY_HydroNexus
{
    public:

    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
    typedef long time_step_t;

    HY_HydroNexus(int nexus_id_num, std::string nexus_id, int num_dowstream);
    virtual ~HY_HydroNexus();

    //NJF Why protect these intrfaces??? protected:

    /** Increase the downstream flow for timestep_t by input amount*/
    virtual void add_upstream_flow(double val, long catchement_id, time_step_t t)=0;

    /** get a precentage of the downstream flow at requested time_step. Record the requesting percentage*/
    virtual double get_downstream_flow(long catchment_id, time_step_t t, double percent_flow)=0;

    /** get the units that the flows are described in */
    virtual std::string get_flow_units()=0;

    private:

    std::shared_ptr<point_t> location;

    unsigned long id_number;
    std::string id;

    int number_of_downstream_catchments;
};

#endif // HY_HYDRONEXUS_H
