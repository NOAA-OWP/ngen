#include "HY_CatchmentRealization.hpp"

HY_CatchmentRealization::HY_CatchmentRealization()
{
    //ctor
}

HY_CatchmentRealization::HY_CatchmentRealization(std::shared_ptr<data_access::GenericDataProvider> forcing) : forcing(forcing) { }

HY_CatchmentRealization::~HY_CatchmentRealization()
{
    //dtor
}
