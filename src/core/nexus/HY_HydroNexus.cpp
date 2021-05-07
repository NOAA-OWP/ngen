#include "HY_HydroNexus.hpp"

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, Catchments contributing_catchments):
id(nexus_id), realization(), contributing_catchments(std::move(contributing_catchments)), receiving_catchments()
{
   this->id_number = std::stoi(nexus_id.substr(4)); //FIXME brittle
   this->id = nexus_id;
   this->number_of_downstream_catchments = contributing_catchments.size();
}

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, HydroLocation location, Catchments receiving_catchments, Catchments contributing_catchments):
id(nexus_id), realization(std::move(location)), contributing_catchments(std::move(contributing_catchments)), receiving_catchments(std::move(contributing_catchments))
{}

HY_HydroNexus::~HY_HydroNexus()
{
    //dtor
}
