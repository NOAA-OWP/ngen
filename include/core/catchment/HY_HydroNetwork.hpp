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

#endif // HY_HYDRONETWORK_H
