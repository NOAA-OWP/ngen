#ifndef NGEN_HYMOD_STATE_H
#define NGEN_HYMOD_STATE_H

//! Hymod state structure
/*!
    This structure provides storage for the state used by hymod hydrological model at a particular time step.
    Warning: to allow bindings to other languages the storage amounts for the reserviors in the nash cascade
    are stored in a pointer which is not allocated or deallocated by this code. Backing storage for these arrays
    must be provide and managed by the creator of these structures.
*/

struct hymod_state
{
    double storage_meters;             //!< the current water storage of the modeled area
    double groundwater_storage_meters; //!< the current water in the ground water nonlinear reservoir
    double* Sr;                 //!< amount of water in each nonlinear reservoir unsafe for binding suport check latter

    //! Constructuor for hymod state
    /*!
        Default constructor for hymod_state objects. This is necessary for the structure to be usable in a map
        Warning: the value of the Sr pointer must be set before this object is used by the hymod_kernel::run() or hymod()
    */
    hymod_state(double inital_storage_meters = 0.0, double gw_storage_meters = 0.0, double* storage_reservoir_ptr = 0x0) :
            storage_meters(inital_storage_meters), groundwater_storage_meters(gw_storage_meters), Sr(storage_reservoir_ptr)
    {

    }
};

#endif //NGEN_HYMOD_STATE_H
