#ifndef NGEN_LSTM_PARAMS_H
#define NGEN_LSTM_PARAMS_H

namespace lstm {

    /**
     * Structure for containing and organizing the parameters of the lstm hydrological model.
     */
    struct lstm_params {
        double maxsmc;              //!< saturated soil moisture content (sometimes theta_e or smcmax)
        double wltsmc;              //!< wilting point soil moisture content
        double satdk;               //!< vertical saturated hydraulic conductivity [m s^-1] (sometimes Kperc or Ks)
        double satpsi;              //!< saturated capillary head [m]
        // TODO: explain more what this is
        double slope;               //!< SLOPE parameter
        double b;                   //!< 'b' exponent on Clapp-Hornberger soil water relations (sometime bexp)
        double multiplier;          //!< the multiplier applied to 'satdk' to route water rapidly downslope in subsurface (sometimes 'mult' or 'LKSATFAC')
        double alpha_fc;            //!< alpha constant for given soil type for relative suction head value, with respect to Hatm
        double Klf;                 //!< lateral flow independent calibration parameter
        double Kn;                  //!< Nash cascade linear reservoir coefficient lateral flow parameter
        int nash_n;                 //!< number of nash cascades
        double Cgw;                 //!< Ground water flow param
        double Cschaake;            //!< The Schaake adjusted magic constant by soil type
        double expon;               //!< Ground water flow exponent param (analogous to NWM 2.0 expon param)
        double max_soil_storage_meters;  //!< Subsurface soil water flow max storage param ("Ssmax"), calculated from maxsmc and depth
        double max_groundwater_storage_meters;    //!< Ground water flow max storage param ("Sgwmax"; analogous to NWM 2.0 zmax param)
        double max_lateral_flow;    //!< Max rate for subsurface lateral flow (i.e., max transmissivity)
        double refkdt = 3.0;        //!< Standard value taken from NOAH-MP, used for calculating Schaake magic constant
        const double depth = 2.0;         //!< Hard-coded, constant value for total soil column depth ('D') [m]

        /**
         * Constructor for instances, initializing members that correspond one-to-one with a parameter and deriving
         * remaining (non-const and hard-coded) members.
         *
         * @param maxsmc Initialization param for maxsmc member.
         * @param wltsmc Initialization param for wltsmc member.
         * @param satdk Initialization param for satdk member.
         * @param satpsi Initialization param for satpsi member.
         * @param slope Initialization param for slope member.
         * @param b Initialization param for b member.
         * @param multiplier Initialization param for multiplier member.
         * @param alpha_fc Initialization param for alpha_fc member.
         * @param Klf Initialization param for Klf member.
         * @param Kn Initialization param for Kn member.
         * @param nash_n Initialization param for nash_n member.
         * @param Cgw Initialization param for Cgw member.
         * @param expon Initialization param for expon member.
         * @param max_gw_storage Initialization param for max_groundwater_storage_meters member.
         */
        lstm_params(
            double maxsmc,
            double wltsmc,
            double satdk,
            double satpsi,
            double slope,
            double b,
            double multiplier,
            double alpha_fc,
            double Klf,
            double Kn,
            int nash_n,
            double Cgw,
            double expon,
            double max_gw_storage) :
                maxsmc(maxsmc),
                wltsmc(wltsmc),
                satdk(satdk),
                satpsi(satpsi),
                slope(slope),
                b(b),
                multiplier(multiplier),
                alpha_fc(alpha_fc),
                Klf(Klf),
                Kn(Kn),
                nash_n(nash_n),
                Cgw(Cgw),
                expon(expon),
                max_groundwater_storage_meters(max_gw_storage) {
            this->max_soil_storage_meters = this->depth * maxsmc;
            this->Cschaake = refkdt * satdk / (2.0e-6);
            this->max_lateral_flow = numeric_limits<double>::max();//satdk * multiplier * this->max_soil_storage_meters;
        }

    };
}

#endif //NGEN_LSTM_PARAMS_H
