#ifndef NGEN_TSHIRT_FLUXES_H
#define NGEN_TSHIRT_FLUXES_H

namespace tshirt {

     /**
      * Structure for containing and organizing the calculated flux values of the Tshirt hydrological model.
      */
    struct tshirt_fluxes {
        double surface_runoff_meters_per_second;  //!< Direct surface runoff, in meters per second
        double groundwater_flow_meters_per_second;         //!< Deep groundwater flow from groundwater reservoir to channel flow
        double soil_percolation_flow_meters_per_second;    //!< Percolation flow from subsurface to groundwater reservoir ("Qperc")
        double soil_lateral_flow_meters_per_second;        //!< Lateral subsurface flow ("Qlf")
        double et_loss_meters;         //!< Loss from ET, in meters

        tshirt_fluxes(double q_gw = 0.0, double q_perc = 0.0, double q_lf = 0.0, double runoff = 0.0,
                      double et_loss = 0.0)
                : groundwater_flow_meters_per_second(q_gw),
                  soil_percolation_flow_meters_per_second(q_perc),
                  soil_lateral_flow_meters_per_second(q_lf),
                  surface_runoff_meters_per_second(runoff),
                  et_loss_meters(et_loss) {

        }
    };
}

#endif //NGEN_TSHIRT_FLUXES_H
