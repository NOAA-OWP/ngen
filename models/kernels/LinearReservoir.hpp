#ifndef LINEAR_RESERVOIR_H
#define LINEAR_RESERVOIR_H

class LinearReservoir
{
    public:
        LinearReservoir(double init_storage = 1.0,double res_max_storage = 2.0, double res_constant = 0.1) :
            storage(init_storage), max_storage(res_max_storage), linear_reservoir_constant(res_constant) {}

        void update_storage(double input_storage)
        {
            this->storage += input_storage;
        }

        double response()
        {
            return (this->storage / this->max_storage) * this->linear_reservoir_constant;
        }

        double response_and_update(double input_storage)
        {
            this->update_storage(input_storage);
            double output_flux = this->response();
            this->update_storage(-output_flux);
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
};





#endif // LINEAR_RESERVOIR_H_INCLUDED
