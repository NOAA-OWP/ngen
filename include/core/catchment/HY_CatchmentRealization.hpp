#ifndef HY_CATCHMENTREALIZATION_H
#define HY_CATCHMENTREALIZATION_H

#include <memory>
#include <string>
#include <Forcing.h>

using std::shared_ptr;

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
    HY_CatchmentRealization(forcing_params forcing_config);
    virtual ~HY_CatchmentRealization();
    virtual double get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params)=0;
    protected:

    shared_ptr<HY_Catchment> realized_catchment;

    unsigned long id_number;
    std::string id;

  protected:
    Forcing forcing;

  private:

};

#endif // HY_CATCHMENTREALIZATION_H
