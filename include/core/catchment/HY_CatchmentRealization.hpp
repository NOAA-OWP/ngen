#ifndef HY_CATCHMENTREALIZATION_H
#define HY_CATCHMENTREALIZATION_H

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//Forward Declarations
class HY_Catchment;
namespace realization {
    class Formulation;
}

typedef long time_step_t;

/**
 * A hydrology-specific perspective of a catchment.
 *
 * A catchment is realized as the face it covers (@ref HY_CatchmentArea), as the edge bounding
 * that face (@ref HY_CatchmentDivide), or as the edge running through it (@ref HY_FlowPath).
 * Each subtype carries the topology of its own perspective, and so answers what a value
 * associated with the feature is distributed over.
 *
 * A realization is constructed with the identity of the catchment it realizes. @ref realize
 * then builds that catchment and binds the two, after which the realization can reach the
 * nexuses flowing into and out of it.
 *
 * A realization may have a formulation attached, which is what produces the values associated
 * with the feature. A catchment can be realized several ways and not every realization is
 * modeled, so the attachment is optional.
 */
class HY_CatchmentRealization : public std::enable_shared_from_this<HY_CatchmentRealization>
{
    public:
    //TODO remove the default constructor? leaving temporarily to satisfy non-used realizations
    HY_CatchmentRealization();

    HY_CatchmentRealization(std::string catchment_id,
                            std::shared_ptr<realization::Formulation> formulation);

    virtual ~HY_CatchmentRealization();

    /** The model producing this feature's values, or nullptr where the feature is not modeled. */
    const std::shared_ptr<realization::Formulation>& get_formulation() const {
        return formulation;
    }

    /** The id this feature shares with the catchment it realizes. */
    const std::string& get_catchment_id() const {
        return catchment_id;
    }

    /**
     * Build the catchment this feature is a perspective of, and bind the two together.
     *
     * The catchment is given this realization's identity, so the pair cannot disagree about
     * what it names. Each then holds the other: the catchment owns its realization, and the
     * realization reaches back to the catchment for its nexuses.
     *
     * One realization is one perspective of one catchment. Calling this again returns the
     * catchment already built and does not apply the arguments; realizing a catchment from
     * another perspective means another realization, which builds its own.
     *
     * @param inflows Ids of the nexuses contributing to the catchment.
     * @param outflows Ids of the nexuses the catchment contributes to.
     * @param layer The hydrofabric layer the catchment belongs to.
     * @return The catchment, newly built or the one this realization already built.
     * @throws std::bad_weak_ptr If this realization is not already held by a shared_ptr.
     */
    std::shared_ptr<HY_Catchment> realize(std::vector<std::string> inflows,
                                          std::vector<std::string> outflows,
                                          long layer = 0);

    /**
     * The catchment this feature realizes, whose inflow and outflow nexuses it shares.
     *
     * Null until @ref realize has been called.
     */
    std::shared_ptr<HY_Catchment> get_realized_catchment() const;

    /**
     * The areal extent of this feature, in square kilometers, where the perspective has one.
     *
     * A face has an area, and so does the edge bounding that face; the edge running through it has
     * a length instead. Empty therefore says the perspective has no extent to give, never that the
     * hydrofabric withheld one: a catchment whose hydrofabric carries no extent is refused when
     * the realization is built, so it does not reach here.
     */
    virtual std::optional<double> area_sqkm() const { return std::nullopt; }

    /**
     * A quantity a perspective can express its value as, each in one canonical unit.
     *
     * Unit conversion should be handled at a higher level, outside of the perspective itself.
     */
    enum class Quantity {
        //! Meters.
        depth,
        //! Cubic meters, over the step.
        volume,
        //! Cubic meters per second.
        volumetric_flow
    };

    /** How many @ref Quantity there are */
    static constexpr std::size_t quantity_count = 3;

    /**
     * The response value distilled by this perspective, in every way the perspective can express it.
     *
     * A formulation typically answers in a depth, because that is what @ref realization::Formulation::get_response
     * asks its model for. What a depth means for the feature is the perspective's to say, so each
     * subtype expresses it in the terms its own topology implies -- a face has an extent to spread
     * it over, an edge does not.
     *
     * A perspective offers what it can express of the step
     * so a caller that is handed something it cannot read finds out by asking.
     */
    class Response {
      public:
        //! A response expressing nothing yet.
        Response() = default;

        //! A response expressing @p value as @p quantity, for a perspective that offers just one.
        Response(Quantity quantity, double value) { set(quantity, value); }

        /** The value as @p quantity, or empty where this perspective does not express it. */
        std::optional<double> in(Quantity quantity) const {
            return values[static_cast<std::size_t>(quantity)];
        }

        /** Offer @p value as @p quantity, in that quantity's canonical unit. */
        void set(Quantity quantity, double value) {
            values[static_cast<std::size_t>(quantity)] = value;
        }

      private:
        std::array<std::optional<double>, quantity_count> values{};
    };

    /**
     * Execute the attached model for the given time step, where it is of the specified size, and
     * return the perspective's response.
     *
     * A model's value is assumed to be depth by default.
     *
     * @param t_index The index of the time step for which to run model calculations.
     * @param t_delta The duration, in seconds, of the time step for which to run model calculations.
     * @return This perspective's response for the step, as each @ref Quantity it expresses.
     * @throws std::runtime_error If this feature has no formulation attached.
     */
    virtual Response get_response(time_step_t t_index, time_step_t t_delta);

    protected:

    std::string catchment_id;

    std::shared_ptr<realization::Formulation> formulation;

    unsigned long id_number;

    private:

    // Bound by realize(). Weak because the catchment owns its realizations; private with no
    // setter so the pair cannot be rewired after the fact.
    std::weak_ptr<HY_Catchment> realized_catchment;
};

#endif // HY_CATCHMENTREALIZATION_H
