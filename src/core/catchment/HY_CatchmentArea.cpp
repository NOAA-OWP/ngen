#include "HY_CatchmentArea.hpp"

HY_CatchmentArea::HY_CatchmentArea()
{
    //ctor
}

HY_CatchmentArea::HY_CatchmentArea(std::unique_ptr<forcing::ForcingProvider> forcing, utils::StreamHandler output_stream) : HY_CatchmentRealization(std::move(forcing)), output(output_stream) { }

HY_CatchmentArea::~HY_CatchmentArea()
{
    //dtor
}
