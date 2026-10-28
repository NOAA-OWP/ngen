#include "HY_CatchmentArea.hpp"

#include <utility>

HY_CatchmentArea::HY_CatchmentArea()
{
    //ctor
}

HY_CatchmentArea::HY_CatchmentArea(std::string catchment_id,
                                   std::shared_ptr<realization::Formulation> formulation,
                                   std::optional<double> area_sqkm,
                                   std::shared_ptr<const geojson::geometry> bounds)
    : HY_CatchmentRealization(std::move(catchment_id), std::move(formulation))
    , bounds(std::move(bounds))
    , area(area_sqkm)
{
}


HY_CatchmentArea::HY_CatchmentArea(std::string catchment_id,
                                   std::shared_ptr<realization::Formulation> formulation,
                                   const geojson::Feature& feature,
                                   hy_features::attributes::AreaSource preferred)
    : HY_CatchmentArea(std::move(catchment_id), std::move(formulation),
                       hy_features::attributes::area_sqkm(feature, preferred),
                       feature->geometry())
{
}


HY_CatchmentArea::~HY_CatchmentArea()
{
    //dtor
}
