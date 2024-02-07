#include "HY_CatchmentArea.hpp"

HY_CatchmentArea::HY_CatchmentArea()
{
    //ctor
}

HY_CatchmentArea::HY_CatchmentArea(std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream) : HY_CatchmentRealization(forcing), output(std::make_unique<utils::StreamHandler>(output_stream)) { }

HY_CatchmentArea::~HY_CatchmentArea()
{
    //dtor
}
