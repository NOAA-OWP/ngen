#ifndef HY_CATCHMENT_H
#define HY_CATCHMENT_H

#include <memory>
#include <string>
#include <vector>

#include "HY_CatchmentRealization.hpp"
#include "HY_HydroFeature.hpp"
#include "Formulation.hpp"

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
    // HY_Catchment(std::string id, Nexuses inflows, Nexuses outflows, std::shared_ptr<realization::Formulation> realization, long lyr = 0):
    HY_Catchment(std::string id, Nexuses inflows, Nexuses outflows, std::shared_ptr<HY_CatchmentRealization> realization, long lyr = 0):
    id(std::move(id)),
    inflows(std::move(inflows)),
    outflows(std::move(outflows)),
    contained_catchments(Catchments()),
    containing_catchment({}),
    conjoint_catchment({}),
    upper_catchment({}),
    lower_catchment({}),
    realization(realization),
    layer(lyr){}
    virtual ~HY_Catchment();
    const Nexuses& get_outflow_nexuses(){ return outflows; }
    // std::shared_ptr<realization::Formulation> realization;
    std::shared_ptr<HY_CatchmentRealization> realization;

    /*! \brief get the hydrofabric layer of this catchment
     *
     * The hydrofabric is divided into multiple layers which are intended to be processed in decreasing numeric order. 
     * The current defined layers are:
     *      80 -- Atmospheric Models (high)
     *      60 -- Atmospeheric Models (low)
     *      40 -- Canopy Models
     *      20 -- Snow melt models
     *      0  -- Surface water processes
     *      -20 -- Ground Water (Shallow)
     *      -40 -- Ground Water (Deep)
     */
    long get_layer() { return layer; }

    /*! \brief set the layer of this catchment
     *
     * This function changes the layer of a catchment. It should only be used if the type of model being used in the catchment changes.
     */
     void set_layer(long lyr) { layer = lyr; }

     const long ATMOSPHERIC_LAYER_HIGH = 80;        //!< This is the catchment layer for high air atmospheric models
     const long ATMOSPHERIC_Layer_LOW = 60;         //!< This is the catchment layer for low air atmospheric models
     const long CANOPY_LAYER = 40;                  //!< This is the catchment layer for canopy layer models
     const long SNOW_LAYER = 20;                    //!< This is the catchment layer for surface snow models
     const long SURFACE_LAYER = 0;                  //!< This is the catchment layer for surface water models  
     const long GROUND_WATER_SHALLOW_LAYER = -20;   //!< This is the catchment layer for shallow groundwater models
     const long GROUND_WATER_DEEP_LAYER = -40;      //!< This is the catchment layer for deep groundwater models

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

    long layer;

};

#endif // HY_CATCHMENT_H
