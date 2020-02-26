#include "HY_Catchment.h"
#include "HY_CatchmentRealization.h"
#include "HY_FlowPath.h"
#include "HY_PointHydroNexus.h"
#include <iostream>

#include "GridPolygon.h"

int main(int argc, const char** argv)
{
    HY_Catchment catchment;
    HY_CatchmentRealization catchment_realization;
    HY_FlowPath flowpath;
    HY_HydroNexus* hydro_nexus;

    hydro_nexus = new HY_PointHydroNexus(1,"test nextus",1);

    std::cout << __cplusplus << std::endl;
    std::cout.flush();

    point_t ul{0.0,4.5};
    point_t lr{4.5,0.0};

    auto grid = create_ns_cells(ul,lr,1.0,1.0);

    delete hydro_nexus;

    return 0;
}
