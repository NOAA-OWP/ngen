#ifndef HY_HYDRONETWORK_H
#define HY_HYDRONETWORK_H


#include "HY_CatchmentRealization.hpp"

#include "GM_Object.hpp"

#include <memory>
#include <vector>
#include <string>


class HY_FlowPath;
class HY_CatchmentArea;
class HY_CatchementDivide;
class HY_HydroNexus;

class HY_HydroNetwork : public GM_Object, public HY_CatchmentRealization
{
    public:

    HY_HydroNetwork();
    virtual ~HY_HydroNetwork();

    protected:

    private:

    std::vector< std::shared_ptr<HY_FlowPath> > flow_paths;
    std::vector< std::shared_ptr<HY_CatchmentArea> > sub_network;
    std::vector< std::shared_ptr<HY_CatchementDivide> > network_divides;
    std::vector< std::shared_ptr<HY_HydroNexus> > internal_nexuses;


};
/*
Hydrographic Nework  and Channel Network are both specializations of the generic Hydro Network.
A Hydro Network is itself a possible Catchment Realization.
In the context of moving water through the network as opposed to over land to the network,
we really want to use the Hydrographic Network (an aggregation of networked waterbodies)
and only connect the network to catchment outflows via Hydro Nexuses which are the outflow of a
Catchment AND realized by some location along a waterbody in the Hydrographic Network.
*/

#endif // HY_HYDRONETWORK_H
