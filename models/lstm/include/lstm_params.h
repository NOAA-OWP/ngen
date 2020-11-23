#ifndef NGEN_LSTM_PARAMS_H
#define NGEN_LSTM_PARAMS_H

namespace lstm {

    /**
     * Structure for containing and organizing the parameters of the lstm hydrological model.
     */
    struct lstm_params {
        std::string pytorch_model_path;
        std::string normalization_path;
        double latitude;
        double longitude;
        double area;



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

           std::string pytorch_model_path,
           std::string normalization_path,
           double lat, double lon, double area) :

              pytorch_model_path(pytorch_model_path),
              normalization_path(normalization_path),
              latitude(lat),
              longitude(lon),
              area(area)
              {}


    };
}

#endif //NGEN_LSTM_PARAMS_H
