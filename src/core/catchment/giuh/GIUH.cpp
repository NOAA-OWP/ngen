#include "GIUH.hpp"

using namespace giuh;

double giuh_kernel::calc_giuh_output(double dt, double direct_runoff)
{
    // TODO: implement to actually performs some real calculations
    return direct_runoff;
}

std::string giuh_kernel::get_catchment_id()
{
    return catchment_id;
}
