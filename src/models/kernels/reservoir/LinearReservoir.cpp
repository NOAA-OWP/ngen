#include "LinearReservoir.hpp"

LinearReservoir::LinearReservoir(double init_storage, double res_max_storage, double res_constant,
                                 double internal_time_step) :
        storage(init_storage), max_storage(res_max_storage), linear_reservoir_constant(res_constant),
        internal_dt(internal_time_step) {}

void LinearReservoir::update_storage(double input_storage)
{
    this->storage += input_storage;
}

double LinearReservoir::flux(double dt_secs)
{
    return (this->storage / this->max_storage) * this->linear_reservoir_constant * (dt_secs/ internal_dt);
}

double LinearReservoir::response(double input_storage, double dt_secs)
{
    this->storage += input_storage;
    double output_flux = this->flux(dt_secs);
    this->storage -= output_flux;
    return output_flux;
}

double LinearReservoir::get_storage()
{
    return this->storage;
}

