#include "HY_HydroNexus.hpp

HY_HydroNexus::HY_HydroNexus(int nexus_id_num, std::string nexus_id, int num_downstream)
{
   this->id_number = nexus_id_num;
   this->id = nexus_id;
   this->number_of_downstream_catchments = num_downstream;
}

HY_HydroNexus::~HY_HydroNexus()
{
    //dtor
}
