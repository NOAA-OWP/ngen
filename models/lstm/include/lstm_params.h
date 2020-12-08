#ifndef NGEN_LSTM_PARAMS_H
#define NGEN_LSTM_PARAMS_H
#include <string>


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

        lstm_params(
           std::string pytorch_model_path,
           std::string normalization_path,
           double lat, double lon, double area) :
              pytorch_model_path(pytorch_model_path),
              normalization_path(normalization_path),
              latitude(lat),
              longitude(lon),
              area(area){
        }
    };
}

#endif //NGEN_LSTM_PARAMS_H
