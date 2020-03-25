#ifndef LINEAR_RESERVOIR_H
#define LINEAR_RESERVOIR_H

class LinearReservoir
{
    public:
<<<<<<< HEAD
        LinearReservoir(double init_storage = 1.0,double res_max_storage = 2.0, double res_constant = 0.1, double internal_time_step = 3600.0 ) :
            storage(init_storage), max_storage(res_max_storage), linear_reservoir_constant(res_constant),  internal_dt(internal_time_step) {}
=======
        LinearReservoir(double init_storage = 1.0,double res_max_storage = 2.0, double res_constant = 0.1) :
            storage(init_storage), max_storage(res_max_storage), linear_reservoir_constant(res_constant) {}
>>>>>>> d11576c0aa4279d8978c5b48f83e08a20ffdb88b

        void update_storage(double input_storage)
        {
            this->storage += input_storage;
        }

        double flux(double dt_secs)
        {
<<<<<<< HEAD
            return (this->storage / this->max_storage) * this->linear_reservoir_constant * (dt_secs/ internal_dt);
=======
            return (this->storage / this->max_storage) * this->linear_reservoir_constant * (dt_secs/ 3600);
>>>>>>> d11576c0aa4279d8978c5b48f83e08a20ffdb88b
        }

        double response(double input_storage, double dt_secs)
        {
            this->storage += input_storage;
            double output_flux = this->flux(dt_secs);
            this->storage -= output_flux;
            return output_flux;
        }

        double get_storage()
        {
            return this->storage;
        }

    private:
        double linear_reservoir_constant;
        double storage;
        double max_storage;
<<<<<<< HEAD
        double internal_dt;
=======
>>>>>>> d11576c0aa4279d8978c5b48f83e08a20ffdb88b
};





#endif // LINEAR_RESERVOIR_H_INCLUDED
