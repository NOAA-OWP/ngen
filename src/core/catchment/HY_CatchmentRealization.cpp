#include "HY_CatchmentRealization.hpp"

#include <stdexcept>
#include <utility>

#include "Formulation.hpp"
#include "HY_Catchment.hpp"

HY_CatchmentRealization::HY_CatchmentRealization() = default;

HY_CatchmentRealization::HY_CatchmentRealization(std::string catchment_id,
                                                 std::shared_ptr<realization::Formulation> formulation)
    : catchment_id(std::move(catchment_id))
    , formulation(std::move(formulation))
{
}

HY_CatchmentRealization::~HY_CatchmentRealization() = default;

std::shared_ptr<HY_Catchment> HY_CatchmentRealization::realize(std::vector<std::string> inflows,
                                                               std::vector<std::string> outflows,
                                                               long layer)
{
    // One perspective realizes one catchment. Hand back the one already realized rather than
    // building a second view of it and leaving the first without a realization pointing at it.
    if (auto already_realized = realized_catchment.lock()) {
        return already_realized;
    }

    auto catchment = std::make_shared<HY_Catchment>(catchment_id, std::move(inflows),
                                                    std::move(outflows), shared_from_this(), layer);
    realized_catchment = catchment;
    return catchment;
}

std::shared_ptr<HY_Catchment> HY_CatchmentRealization::get_realized_catchment() const
{
    return realized_catchment.lock();
}

HY_CatchmentRealization::Response
HY_CatchmentRealization::get_response(time_step_t t_index, time_step_t t_delta)
{
    if (formulation == nullptr) {
        throw std::runtime_error("Catchment realization '" + get_catchment_id()
                                 + "' has no formulation attached to answer with.");
    }
    // get_response() implementers are responsible for any unit conversion; the assumption of
    // depth in meters materializes here.
    return {Quantity::depth, formulation->get_response(t_index, t_delta)};
}
