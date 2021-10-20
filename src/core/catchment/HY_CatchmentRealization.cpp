#include "HY_CatchmentRealization.hpp"

HY_CatchmentRealization::HY_CatchmentRealization()
{
    //ctor
}

HY_CatchmentRealization::HY_CatchmentRealization(std::unique_ptr<forcing::ForcingProvider> forcing) : forcing(std::move(forcing)) { }

HY_CatchmentRealization::~HY_CatchmentRealization()
{
    //dtor
}
