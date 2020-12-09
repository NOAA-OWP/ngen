#ifndef NGEN_LSTM_CONFIGURATION_H
#define NGEN_LSTM_CONFIGURATION_H

#include <string>

namespace lstm {

    /**
     * Structure for containing and organizing the parameters of the lstm hydrological model.
     */
    struct lstm_config {
        std::string pytorch_model_path;
        std::string normalization_path;
        std::string initial_state_path;
        bool useGPU;
        /**
         * Constructor for instances, initializing members that correspond one-to-one with a parameter and deriving
         * @param pytorch_model_path string path to the pytorch .pt or .ptc trace file
         * @param normalization_path string path to the csv file containing input normalization parameters (mean and standard deviation)
         * @param initial_state_path string path to the csv file containing serialized state tensors h_t and c_t
         */
        lstm_config(
           std::string pytorch_model_path,
           std::string normalization_path,
           std::string initial_state_path,
           bool useGPU
          ) :
              pytorch_model_path(pytorch_model_path),
              normalization_path(normalization_path),
              initial_state_path(initial_state_path),
              useGPU(useGPU)
              {}
        lstm_config()
          : pytorch_model_path(""), normalization_path(""), initial_state_path(""), useGPU(false)
        {}
    };
}

#endif //NGEN_LSTM_CONIGURATION_H
