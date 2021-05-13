#ifndef HY_CATCHMENT_H
#define HY_CATCHMENT_H

#include <memory>
#include <string>
#include <vector>

#include "HY_CatchmentRealization.hpp"
#include "HY_HydroFeature.hpp"

class HY_HydroNexus;

class HY_Catchment : public HY_HydroFeature
{
    //using Nexuses = std::vector< std::shared_ptr<HY_HydroNexus> >;
    using Nexus = std::string;
    using Nexuses = std::vector< Nexus >;
    //using Catchments = std::vector< std::shared_ptr<HY_Catchment> >;
    using Catchment  = std::string;
    using Catchments = std::vector< Catchment >;
    public:

    HY_Catchment();
    HY_Catchment(std::string id, Nexuses inflows, Nexuses outflows, std::shared_ptr<HY_CatchmentRealization> realization):
    id(std::move(id)),
    inflows(std::move(inflows)),
    outflows(std::move(outflows)),
    contained_catchments(Catchments()),
    containing_catchment({}),
    conjoint_catchment({}),
    upper_catchment({}),
    lower_catchment({}),
    realization(realization){}
    virtual ~HY_Catchment();
    const Nexuses& get_outflow_nexuses(){ return outflows; }
    std::shared_ptr<HY_CatchmentRealization> realization;
    protected:

    private:

    std::string id;
    Nexuses inflows;
    Nexuses outflows;
    Catchments contained_catchments;
    Catchment containing_catchment;
    Catchment conjoint_catchment;
    Catchment upper_catchment;
    Catchment lower_catchment;

};

#endif // HY_CATCHMENT_H
