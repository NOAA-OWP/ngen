#include "Tshirt_C_Realization.hpp"
#include "tshirt_c.h"

using namespace realization;

Tshirt_C_Realization::Tshirt_C_Realization(forcing_params forcing_config, double soil_storage_meters,
                                           double groundwater_storage_meters, std::string catchment_id,
                                           giuh::GiuhJsonReader &giuh_json_reader,
                                           tshirt::tshirt_params params,
                                           const vector<double> &nash_storage)
                   : HY_CatchmentArea(forcing_config), catchment_id(std::move(catchment_id)), params(params)
{

}
