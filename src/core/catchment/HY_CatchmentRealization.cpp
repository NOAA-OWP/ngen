#include "HY_CatchmentRealization.hpp"

HY_CatchmentRealization::HY_CatchmentRealization():forcing(Forcing())
{
    //ctor
}

//TODO consider passing an ID to the forcing object???
HY_CatchmentRealization::HY_CatchmentRealization(forcing_params forcing_config):forcing( Forcing(forcing_config) )
{

}

HY_CatchmentRealization::~HY_CatchmentRealization()
{
    //dtor
}
