#include "HY_PointHydroNexus.hpp"

#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_errmsg, std::string> errmsg_info;

struct invalid_downstream_request : public boost::exception, public std::exception
{
  const char *what() const noexcept { return "All downstream catchments can not request more than 100% of flux in total"; }
};

struct add_to_summed_nexus : public boost::exception, public std::exception
{
  const char *what() const noexcept { return "Can not add water to a summed point nexus"; }
};

struct request_from_empty_nexus : public boost::exception, public std::exception
{
  const char *what() const noexcept { return "Can not release water from an empty nexus"; }
};

struct completed_time_step : public boost::exception, public std::exception
{
  const char *what() const noexcept { return "Can not operate on a completed time step"; }
};

struct invalid_time_step : public boost::exception, public std::exception
{
  const char *what() const noexcept { return "Time step before minimum time step requested"; }
};

HY_PointHydroNexus::HY_PointHydroNexus(std::string nexus_id, Catchments receiving_catchments) : HY_HydroNexus( nexus_id, receiving_catchments), upstream_flows()
{

}

HY_PointHydroNexus::HY_PointHydroNexus(std::string nexus_id, Catchments receiving_catchments, Catchments contributing_catchments) : HY_HydroNexus( nexus_id, receiving_catchments, contributing_catchments), upstream_flows()
{

}

HY_PointHydroNexus::~HY_PointHydroNexus()
{
    //dtor
}

double HY_PointHydroNexus::get_downstream_flow(std::string catchment_id, time_step_t t, double percent_flow)
{

    if ( t < min_timestep ) BOOST_THROW_EXCEPTION(invalid_time_step());
    if ( completed.find(t) != completed.end() ) BOOST_THROW_EXCEPTION(completed_time_step());

    auto s1 = upstream_flows.find(t);

    if ( percent_flow > 100.0)
    {
        // no downstream may ever recieve more than 100% of flows

        BOOST_THROW_EXCEPTION(invalid_downstream_request());
    }
    else if ( s1 == upstream_flows.end() )
    {
        // there are no recorded flows for this time.
        // throw exception

        BOOST_THROW_EXCEPTION(request_from_empty_nexus() );
    }
    else
    {
        auto s2 = summed_flows.find(t);

        if ( s2 == summed_flows.end() )
        {
            // the flows have not been summed calculate the sum
            // and store it into summed_flows
            double sum {};
            for(auto& n : s1->second )
            {
                sum += n.second;
            }

            summed_flows[t] = sum;

            // mark downstream request with the amount of flow requested
            // and the catchment making the request

            flow_vector v;
            v.push_back(flows(catchment_id,percent_flow));
            downstream_requests[t] = v;

            // record the total requests for this time
            total_requests[t] = percent_flow;

            // release flux
            return sum * (percent_flow / 100);
        }
        else
        {
            // flows have been summed so some water has allready been release

            if ( total_requests[t] + percent_flow > 100.0 )
            {
                    // if the amount of flow allready released plus the amount
                    // of this release is greater than 100 throw an error
                    BOOST_THROW_EXCEPTION(invalid_downstream_request());
            }
            else
            {
                // update the total_request for this timesteo
                total_requests[t] += percent_flow;

                // add this request to recorded downstream requests
                downstream_requests[t].push_back(flows(catchment_id,percent_flow));

                double released_flux = summed_flows[t] * (percent_flow / 100.0);

                if (100.0 - total_requests[t] < 0.00005 )
                {
                    // all water has been requested remove bookeeping

                    upstream_flows.erase(upstream_flows.find(t));
                    downstream_requests.erase(downstream_requests.find(t));
                    summed_flows.erase(summed_flows.find(t));
                    total_requests.erase(total_requests.find(t));

                    completed.emplace(t);
                }

                return released_flux;
            }
        }
    }
}

void HY_PointHydroNexus::add_upstream_flow(double val, std::string catchment_id, time_step_t t)
{
     if ( t < min_timestep ) BOOST_THROW_EXCEPTION(invalid_time_step());
    if ( completed.find(t) != completed.end() ) BOOST_THROW_EXCEPTION(completed_time_step());

    auto s1 = upstream_flows.find(t);
    if (  s1 == upstream_flows.end() )
    {
        // case 1 there are no upstream flow for this time
        // create a new vector of flow id pairs and add the current flow
        // and catchment id to the vector then insert the vector

        flow_vector v;
        v.push_back(flows(catchment_id,val));
         upstream_flows[t] = v;
    }
    else
    {
        auto s2 = summed_flows.find(t);

        if ( s2 == summed_flows.end() )
        {
            // case 2 there are no summed flow for the time
            // this means there have been no downstream request and we can add water

             s1->second.push_back(flows(catchment_id,val));
        }
        else
        {
            // case 3 summed flows exist we can not add water for a time step when
            // one or more catchments have made downstream requests

             BOOST_THROW_EXCEPTION(add_to_summed_nexus());
         }
    }
}

std::pair<double, int> HY_PointHydroNexus::inspect_upstream_flows(time_step_t t)
{
    auto s1 = upstream_flows.find(t);
    if ( s1 == upstream_flows.end() )
    {
        return std::pair<double,long>(0.0, 0);
    }
    else
    {
        double total_upstream_flows = 0.0;

        auto& id_flow_vector = s1->second;
        for (auto& p : id_flow_vector )
        {
            total_upstream_flows += p.second;
        }

        return std::pair<double, long>(total_upstream_flows, id_flow_vector.size() );
    }
}

std::pair<double, int> HY_PointHydroNexus::inspect_downstream_requests(time_step_t t)
{
    auto s1 = downstream_requests.find(t);
    if ( s1 == downstream_requests.end() )
    {
        return std::pair<double,long>(0.0, 0);
    }
    else
    {
        double total_downstream_requests = 0.0;

        auto& id_request_vector = s1->second;
        for (auto& p : id_request_vector )
        {
            total_downstream_requests += p.second;
        }

        return std::pair<double, long>(total_downstream_requests, id_request_vector.size() );
    }
}

std::string HY_PointHydroNexus::get_flow_units()
{
    return std::string("m3/s");
}

void HY_PointHydroNexus::set_mintime(time_step_t t)
{
    min_timestep = t;

    // remove expired time steps from completed
    for( auto& t: completed)
    {
        if ( t < min_timestep )
        {
            completed.erase(t);
        }
    }

    // C++ 2014 would allow this do be done with a single lambda
    auto l1 = [](int min_v, std::unordered_map<long,flow_vector>& v)
    {
        for( auto& t: v)
        {
            if ( t.first < min_v )
            {
                v.erase(t.first);
            }
        }
    };

    // C++ 2014 would allow this do be done with a single lambda
    auto l2 = [](int min_v, std::unordered_map<long,double>& v)
    {
        for( auto& t: v)
        {
            if ( t.first < min_v )
            {
                v.erase(t.first);
            }
        }
    };

    // remove expired time steps from all maps
    l1(min_timestep,downstream_requests);
    l1(min_timestep,upstream_flows);
    l2(min_timestep,summed_flows);
    l2(min_timestep,total_requests);

}
