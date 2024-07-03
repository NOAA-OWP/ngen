#ifndef HY_CATCHMENTREALIZATION_H
#define HY_CATCHMENTREALIZATION_H

#include <memory>
#include <string>
#include <AorcForcing.hpp>
#include "GenericDataProvider.hpp"

//Forward Declarations
class HY_Catchment;

//This is the base base for CatchmentRealizations
typedef long time_step_t;
//TODO template<forcing>
//TODO template<et_datatype>
class HY_CatchmentRealization
{
    public:
    //TODO remove the default constructor? leaving temporarily to satisfy non-used realizations
    HY_CatchmentRealization();

    virtual ~HY_CatchmentRealization();

    /**
     * Execute the backing model formulation for the given time step, where it is of the specified size, and
     * return the response output.
     *
     * Any inputs and additional parameters must be made available as instance members.
     *
     * Types should clearly document the details of their particular response output.
     *
     * @param t_index The index of the time step for which to run model calculations.
     * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
     * @return The response output of the model for this time step.
     */
    virtual double get_response(time_step_t t_index, time_step_t t_delta) = 0;

    protected:

    std::shared_ptr<HY_Catchment> realized_catchment;

    virtual std::string get_catchment_id() const = 0;

    virtual void set_catchment_id(std::string cat_id) = 0;

    unsigned long id_number;
};

#endif // HY_CATCHMENTREALIZATION_H
