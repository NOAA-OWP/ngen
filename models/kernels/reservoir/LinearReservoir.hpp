#ifndef LINEAR_RESERVOIR_H
#define LINEAR_RESERVOIR_H

class LinearReservoir
{
    public:
        LinearReservoir(double init_storage = 1.0, double res_max_storage = 2.0, double res_constant = 0.1,
                double internal_time_step = 3600.0 );

        void update_storage(double input_storage);

        double flux(double dt_secs);

        double response(double input_storage, double dt_secs);

        double get_storage();

    private:
        double linear_reservoir_constant;
        double storage;
        double max_storage;
        double internal_dt;
};





#endif // LINEAR_RESERVOIR_H_INCLUDED
