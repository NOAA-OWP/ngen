#include "HY_HydroNexus.hpp"

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, Catchments receiving_catchments):
HY_HydroNexus(nexus_id, HydroLocation(), receiving_catchments, Catchments())
{

}

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, Catchments receiving_catchments, Catchments contributing_catchments):
HY_HydroNexus(nexus_id, HydroLocation(), receiving_catchments, contributing_catchments)
{

}

HY_HydroNexus::HY_HydroNexus(std::string nexus_id, HydroLocation location, Catchments receiving_catchments, Catchments contributing_catchments):
    id(nexus_id), 
    realization(std::move(location)), 
    receiving_catchments(std::move(contributing_catchments)), 
    contributing_catchments(std::move(contributing_catchments))
{
    this->number_of_downstream_catchments = receiving_catchments.size();
}

HY_HydroNexus::~HY_HydroNexus()
{
    //dtor
}
