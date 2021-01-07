#ifndef NGEN_LSTM_STATE_H
#define NGEN_LSTM_STATE_H

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE

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
        /*
        * Construct state tensors from vector
        */
        lstm_state(std::vector<double> h, std::vector<double> c)
        {
          auto options = torch::TensorOptions().dtype(torch::kFloat64);
          h_t = torch::from_blob(h.data(), {1, 1, int(h.size())}, options).clone();
          c_t = torch::from_blob(c.data(), {1, 1, int(c.size())}, options).clone();
        }
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


#endif

#endif //NGEN_LSTM_STATE_H
