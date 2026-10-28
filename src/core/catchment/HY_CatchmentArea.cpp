#include "HY_CatchmentArea.hpp"

#include <utility>

HY_CatchmentArea::HY_CatchmentArea()
{
    //ctor
}

HY_CatchmentArea::HY_CatchmentArea(std::string catchment_id,
                                   std::shared_ptr<realization::Formulation> formulation)
    : HY_CatchmentRealization(std::move(catchment_id), std::move(formulation))
{
}


HY_CatchmentArea::~HY_CatchmentArea()
{
    //dtor
}
