#ifndef HY_POINTHYDRONEXUS_H
#define HY_POINTHYDRONEXUS_H

#include <HY_HydroNexus.hpp>

#include <unordered_map>
#include <unordered_set>

class HY_PointHydroNexus : public HY_HydroNexus
{
    public:
        HY_PointHydroNexus(int nexus_id_number, std::string nexus_id, int num_downstream);
        virtual ~HY_PointHydroNexus();

        /** get the request percentage of downstream flow through this nexus at timestep t. */
        double get_downstream_flow(long catchment_id, time_step_t t, double percent_flow);

        /** add flow to this nexus for timestep t. */
        void add_upstream_flow(double val, long catchment_id, time_step_t t);

        /** inspect a nexus to see what flows are recorded at a time step. */
        std::pair<double, int> inspect_upstream_flows(time_step_t t);

        /** inspect a nexus to see what requests are recorded at a time step. */
        virtual std::pair<double, int> inspect_downstream_requests(time_step_t t);

        /** get the units that flows are represented in. */
        std::string get_flow_units();

        void set_mintime(time_step_t);

    protected:

    private:

    typedef std::vector< std::pair<long,double> > id_flow_vector;
    typedef std::vector< std::pair<long,double> > id_request_vector;

    /** The current downstream flow through this Point Nexus.*/
    std::unordered_map<time_step_t, id_flow_vector > upstream_flows;
    std::unordered_map<time_step_t, id_request_vector > downstream_requests;
    std::unordered_map<time_step_t, double> summed_flows;
    std::unordered_map<time_step_t, double> total_requests;

    time_step_t min_timestep{0};
    std::unordered_set<time_step_t> completed;

};

#endif // HY_POINTHYDRONEXUS_H
