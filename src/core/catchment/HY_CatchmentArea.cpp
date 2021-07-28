#include "HY_CatchmentArea.hpp"

HY_CatchmentArea::HY_CatchmentArea()
{
    //ctor
}

HY_CatchmentArea::HY_CatchmentArea(Forcing forcing, utils::StreamHandler output_stream) : HY_CatchmentRealization(forcing), output(output_stream) { }

HY_CatchmentArea::HY_CatchmentArea(forcing_params forcing_config, utils::StreamHandler output_stream) : HY_CatchmentRealization(forcing_config), output(output_stream)
{
    //ctor
}

HY_CatchmentArea::~HY_CatchmentArea()
{
    //dtor
}
