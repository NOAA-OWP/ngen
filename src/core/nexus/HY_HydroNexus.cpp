#include "HY_HydroNexus.hpp"

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, Catchments receiving_catchments):
id(nexus_id), realization(), receiving_catchments(std::move(receiving_catchments)), contributing_catchments()
{
   this->id = nexus_id;
   this->number_of_downstream_catchments = receiving_catchments.size();
}

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, HydroLocation location, Catchments receiving_catchments, Catchments contributing_catchments):
id(nexus_id), realization(std::move(location)), receiving_catchments(std::move(contributing_catchments)), contributing_catchments(std::move(contributing_catchments))
{}

HY_HydroNexus::~HY_HydroNexus()
{
    //dtor
}
