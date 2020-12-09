#ifndef NGEN_LSTM_STATE_H
#define NGEN_LSTM_STATE_H
#include <torch/script.h>

namespace lstm {

     /**
      * Structure for containing and organizing the state values of the lstm hydrological model.
      */
    struct lstm_state {
        //TODO exapnd these names and document what "state" they represent to the lstm model
        torch::Tensor h_t;
        torch::Tensor c_t;

        lstm_state(torch::Tensor h, torch::Tensor c)
                :  h_t(h), c_t(c) {}
        lstm_state()
        {
          auto options = torch::TensorOptions().dtype(torch::kFloat64);
          h_t = torch::zeros({1, 1, 64}, options);
          c_t = torch::zeros({1, 1, 64}, options);
        }
    };

    inline void to_device( lstm_state& state, torch::Device device )
    {
      state.h_t.to(device);
      state.c_t.to(device);
    }
} //namespace lstm

#endif //NGEN_LSTM_STATE_H
