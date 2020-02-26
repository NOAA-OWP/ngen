#ifndef HY_CATCHMENT_H
#define HY_CATCHMENT_H

#include <memory>
#include <string>
#include <vector>

#include "HY_CatchmentRealization.h"
#include "HY_HydroFeature.h"

class HY_HydroNexus;

class HY_Catchment : public HY_HydroFeature
{
    public:

    HY_Catchment();
    virtual ~HY_Catchment();

    protected:



    private:

    std::shared_ptr<HY_CatchmentRealization> realization;
    std::vector< std::shared_ptr<HY_HydroNexus> > inflows;
    std::vector< std::shared_ptr<HY_HydroNexus> > outflows;
    std::vector< std::shared_ptr<HY_Catchment> > recieving_catchments;
    std::vector< std::shared_ptr<HY_Catchment> > contained_catchments;
    std::shared_ptr<HY_Catchment> containing_catchment;

    unsigned long id_number;
    std::string id;

};

#endif // HY_CATCHMENT_H
