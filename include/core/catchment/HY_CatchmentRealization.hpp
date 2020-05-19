#ifndef HY_CATCHMENTREALIZATION_H
#define HY_CATCHMENTREALIZATION_H

#include <memory>
#include <string>
#include <Forcing.h>

using std::shared_ptr;

class HY_Catchment;

//This is the base base for CatchmentRealizations
typedef long time_step_t;
//TODO template<forcing>
//TODO template<et_datatype>
class HY_CatchmentRealization
{
    public:

    HY_CatchmentRealization();
    virtual ~HY_CatchmentRealization();

    protected:

    shared_ptr<HY_Catchment> realized_catchment;

    unsigned long id_number;
    std::string id;

  private:
    Forcing forcing;

};

#endif // HY_CATCHMENTREALIZATION_H
