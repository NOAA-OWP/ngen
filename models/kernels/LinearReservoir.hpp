#ifndef LINEAR_RESERVOIR_H
#define LINEAR_RESERVOIR_H

class LinearReservoir
{
    public:
        LinearReservoir(double k=2.0,double s=0) : K(k), S(s) {}

        double response(double input_flux)
        {
            S += input_flux;
            double Q = S / K;
            S -= Q;
            return Q;
        }

        double get_storage()
        {
            return S;
        }

    private:
        double K;
        double S;
};


#endif // LINEAR_RESERVOIR_H_INCLUDED
