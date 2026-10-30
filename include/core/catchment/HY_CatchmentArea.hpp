#ifndef HY_CATCHMENTAREA_H
#define HY_CATCHMENTAREA_H

#include "HY_CatchmentRealization.hpp"

#include "GM_Object.hpp"

#include <HY_Features_Attributes.hpp>
#include <JSONGeometry.hpp>

#include <optional>


/**
 * The two-dimensional realization of a catchment: the face it covers.
 *
 * Topologically a face bounded by catchment divide and flowpath edges, so this is the
 * perspective to ask for an areal extent -- the denominator when a value is distributed over
 * the catchment.
 */
class HY_CatchmentArea : public HY_CatchmentRealization, public GM_Object
{
    public:

    HY_CatchmentArea();

    /**
     * @param catchment_id The identity of the catchment this realizes.
     * @param formulation The model producing this feature's values, or nullptr if unmodeled.
     * @param area_sqkm The extent of the face, empty where it is not known.
     * @param bounds The geometry describing the face, empty where there is none.
     */
    HY_CatchmentArea(std::string catchment_id,
                     std::shared_ptr<realization::Formulation> formulation,
                     std::optional<double> area_sqkm = std::nullopt,
                     std::shared_ptr<const geojson::geometry> bounds = nullptr);

    /**
     * Realize a catchment from the hydrofabric feature describing it.
     *
     * A convenience over the constructor above for the common case, taking the feature apart so
     * a caller does not have to: the extent comes from @ref hy_features::attributes::area_sqkm,
     * the face from the feature's own geometry. What is kept is the same either way -- an extent
     * and a geometry, never the feature.
     *
     * @param catchment_id The identity of the catchment this realizes.
     * @param formulation The model producing this feature's values, or nullptr if unmodeled.
     * @param feature The hydrofabric feature this catchment is realized from.
     * @param preferred Which of the two area sources to consult first.
     * @throws std::invalid_argument If the feature carries neither an area attribute nor a divide.
     */
    HY_CatchmentArea(std::string catchment_id,
                     std::shared_ptr<realization::Formulation> formulation,
                     const geojson::Feature& feature,
                     hy_features::attributes::AreaSource preferred =
                         hy_features::attributes::AreaSource::attribute);

    virtual ~HY_CatchmentArea();

    std::optional<double> area_sqkm() const override { return area; }

    /**
     * The geometry describing this face, empty where no hydrofabric feature backs it.
     *
     * Shared with the feature it was read from rather than copied out of it: a divide runs to
     * thousands of coordinates, and holding one per realization would duplicate what the fabric
     * already has. A handle rather than a reference, so a consumer that outlives this
     * realization -- a domain layer resampling onto the catchment -- can keep the divide after
     * both this and the feature collection are gone.
     *
     * The variant, not a polygon: a feature may describe its face as either a Polygon or a
     * MultiPolygon, and narrowing to one of them is what would force a copy of the other.
     */
    const std::shared_ptr<const geojson::geometry>& get_bounds() const { return bounds; }

    /**
     * The value representing the state of this feature after one time step.
     * E.g. depth, depth spread over the face's extent, and the rate with
     * respect to the time step.
     *
     * @throws std::runtime_error If this feature has no formulation attached, or has no extent to
     *         spread the depth over.
     */
    Response get_response(time_step_t t_index, time_step_t t_delta) override;

    private:

    //! Shared with the feature this was realized from; const, so neither can surprise the other.
    std::shared_ptr<const geojson::geometry> bounds;

    std::optional<double> area;
};

#endif // HY_CATCHMENTAREA_H
