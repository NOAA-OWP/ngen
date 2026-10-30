#include "HY_CatchmentArea.hpp"

#include <stdexcept>
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

HY_CatchmentArea::Response HY_CatchmentArea::get_response(time_step_t t_index, time_step_t t_delta)
{
    if (!area) {
        // Early exit if we don't have area to work with
        throw std::runtime_error("Catchment '" + get_catchment_id() + "' has no areal extent, "
                                 "volumetric flow cannot be computed.");
    }
    // delegate to get the realization's formulation response for this step
    Response response = HY_CatchmentRealization::get_response(t_index, t_delta);
    // get the depth represented by the model for this step, the assumed result of a formulation's
    // main output variable.
    const double meters = *response.in(Quantity::depth);
    const double volume_m3 = meters * (*area * 1'000'000.0);
    // set the volume quantity
    response.set(Quantity::volume, volume_m3);
    // set the flow rate
    if (t_delta > 0) {
        response.set(Quantity::volumetric_flow, volume_m3 / (double)t_delta);
    }
    return response;
}
