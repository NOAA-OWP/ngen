#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "HY_Catchment.hpp"
#include "HY_CatchmentArea.hpp"
#include "HY_CatchmentRealization.hpp"
#include "HY_FlowPath.hpp"
#include "Formulation.hpp"

/**
 * A formulation that answers a fixed depth.
 *
 * Formulation::get_response requests meters of its model, so a stub standing in for one answers in
 * meters too. Everything else here is the interface, not the subject.
 */
class Depth_Formulation : public realization::Formulation {
  public:
    explicit Depth_Formulation(double depth_m) : realization::Formulation("cat-27"), depth(depth_m) {}

    std::string get_formulation_type() const override { return "depth_stub"; }
    void create_formulation(boost::property_tree::ptree&, geojson::PropertyMap*) override {}
    void create_formulation(geojson::PropertyMap) override {}
    void check_mass_balance(const int&, const int&, const std::string&) const override {}
    void checkpoint_state(const int&, const int&, const std::string&) const override {}
    void update(time_step_t, time_step_t) override {}
    double get_response(time_step_t, time_step_t) override { return depth; }
    std::vector<utils::OutputField> get_output_fields() const override { return {}; }
    std::vector<double> get_output_values_for_timestep(int) override { return {}; }

  protected:
    const std::vector<std::string>& get_required_parameters() const override {
        static const std::vector<std::string> none;
        return none;
    }

  private:
    double depth;
};

using Quantity = HY_CatchmentRealization::Quantity;

#include <features/Features.hpp>

class HY_CatchmentRealization_Test : public ::testing::Test {
  protected:
    std::shared_ptr<HY_CatchmentArea> area;
    std::shared_ptr<HY_Catchment> catchment;

    /** A square degree of WGS84, standing in for a divide. */
    static std::shared_ptr<const geojson::geometry> square() {
        geojson::polygon_t polygon;
        for (auto c : {std::pair{0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}, {0.0, 0.0}}) {
            boost::geometry::append(polygon.outer(), geojson::coordinate_t(c.first, c.second));
        }
        return std::make_shared<const geojson::geometry>(polygon);
    }

    void SetUp() override {
        area = std::make_shared<HY_CatchmentArea>("cat-27", nullptr);
        catchment = area->realize({"nex-26"}, {"nex-27"});
    }
};

/** A realization carries the id of the catchment it realizes, from construction. */
TEST_F(HY_CatchmentRealization_Test, RealizationSharesItsCatchmentsIdentity)
{
    EXPECT_EQ(area->get_catchment_id(), "cat-27");
}

/** The realized catchment takes its identity from the realization, so they cannot disagree. */
TEST_F(HY_CatchmentRealization_Test, RealizedCatchmentTakesItsIdentityFromTheRealization)
{
    EXPECT_EQ(catchment->get_id(), area->get_catchment_id());
}

/** realize() binds both directions, so the realization reaches the catchment it realizes. */
TEST_F(HY_CatchmentRealization_Test, RealizePairIsBoundInBothDirections)
{
    EXPECT_EQ(catchment->realization, area);
    EXPECT_EQ(area->get_realized_catchment(), catchment);
}

/** The catchment's nexuses are reachable from its realization. */
TEST_F(HY_CatchmentRealization_Test, NexusRelationshipsAreReachableFromTheRealization)
{
    auto realized = area->get_realized_catchment();
    ASSERT_NE(realized, nullptr);
    EXPECT_EQ(realized->get_outflow_nexuses(), std::vector<std::string>{"nex-27"});
}

/** One perspective realizes one catchment; realizing again hands back the same one. */
TEST_F(HY_CatchmentRealization_Test, RealizingAgainReturnsTheCatchmentAlreadyRealized)
{
    auto again = area->realize({"nex-99"}, {"nex-98"});
    ASSERT_EQ(again, catchment);
    EXPECT_EQ(again->get_outflow_nexuses(), std::vector<std::string>{"nex-27"});
}

/** The catchment owns its realization, so the back reference does not keep it alive. */
TEST_F(HY_CatchmentRealization_Test, BackReferenceDoesNotOwnTheCatchment)
{
    catchment.reset();
    EXPECT_EQ(area->get_realized_catchment(), nullptr);
}

/** A realization with no model attached says so rather than dereferencing nothing. */
TEST_F(HY_CatchmentRealization_Test, UnmodeledRealizationRefusesToRespond)
{
    EXPECT_THROW(area->get_response(0, 3600), std::runtime_error);
}

/** A face with no extent cannot spread a depth over it, and says so rather than guessing one. */
TEST_F(HY_CatchmentRealization_Test, AFaceWithNoExtentRefusesToRespond)
{
    // A model is attached, so a refusal here is about the missing extent and not the missing
    // formulation -- which is the other reason this call can throw.
    auto measured = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004));
    try {
        measured->get_response(0, 3600);
        FAIL() << "expected a face with no extent to refuse";
    }
    catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("no areal extent"), std::string::npos) << e.what();
    }
}

/** The model's depth, spread over the face's extent, is the face's response. */
TEST_F(HY_CatchmentRealization_Test, AFaceSpreadsTheModelsDepthOverItsExtent)
{
    auto modeled = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004), 15.625);

    EXPECT_EQ(modeled->get_response(0, 3600).in(Quantity::volume), 0.004 * 15.625 * 1000000.0);
}

/** Deriving a volume does not consume the depth it came from; both remain on offer. */
TEST_F(HY_CatchmentRealization_Test, AFaceStillOffersTheDepthItSpread)
{
    auto modeled = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004), 15.625);

    EXPECT_EQ(modeled->get_response(0, 3600).in(Quantity::depth), 0.004);
}

/**
 * The base perspective offers the model's value and nothing derived from it.
 *
 * It has no topology to interpret the depth with, so a caller wanting a volume is asking something
 * this perspective cannot answer, and finds that out rather than receiving a guess.
 */
TEST_F(HY_CatchmentRealization_Test, TheBasePerspectiveOffersOnlyTheDepth)
{
    auto base = std::make_shared<HY_CatchmentRealization>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004));

    HY_CatchmentRealization::Response response = base->get_response(0, 3600);

    EXPECT_EQ(response.in(Quantity::depth), 0.004);
    EXPECT_FALSE(response.in(Quantity::volume).has_value());
    EXPECT_FALSE(response.in(Quantity::volumetric_flow).has_value());
}

/** The volume is what the step produced, so it does not depend on how long the step was. */
TEST_F(HY_CatchmentRealization_Test, TheVolumeDoesNotDependOnTheStepLength)
{
    auto modeled = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004), 15.625);

    EXPECT_EQ(modeled->get_response(0, 3600).in(Quantity::volume),
              modeled->get_response(1, 900).in(Quantity::volume));
}

/**
 * The rate does depend on the step, and on the step the model was advanced for.
 *
 * The same volume over a quarter of the time is four times the flow. Derived where the step is
 * known rather than by a caller that may only know how often output is sampled.
 */
TEST_F(HY_CatchmentRealization_Test, TheRateIsTheVolumeOverTheStepItWasAdvancedFor)
{
    auto modeled = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004), 15.625);
    const double volume = *modeled->get_response(0, 3600).in(Quantity::volume);

    EXPECT_DOUBLE_EQ(*modeled->get_response(0, 3600).in(Quantity::volumetric_flow), volume / 3600.0);
    EXPECT_DOUBLE_EQ(*modeled->get_response(1, 900).in(Quantity::volumetric_flow), volume / 900.0);
}

/** With no interval to spread it over there is no rate, rather than a division by zero. */
TEST_F(HY_CatchmentRealization_Test, ADegenerateStepOffersNoRate)
{
    auto modeled = std::make_shared<HY_CatchmentArea>(
            "cat-27", std::make_shared<Depth_Formulation>(0.004), 15.625);

    HY_CatchmentRealization::Response response = modeled->get_response(0, 0);

    EXPECT_TRUE(response.in(Quantity::volume).has_value());
    EXPECT_FALSE(response.in(Quantity::volumetric_flow).has_value());
}

/** A face reports the extent it was realized with, which is what a value is distributed over. */
TEST_F(HY_CatchmentRealization_Test, AreaReportsTheExtentItWasRealizedWith)
{
    auto measured = std::make_shared<HY_CatchmentArea>("cat-27", nullptr, 15.625);
    EXPECT_EQ(measured->area_sqkm(), 15.625);
}

/**
 * Asking any realization for an area is answerable; having one is not.
 *
 * The question is what a perspective can give, not which type it is, so a consumer holding the
 * base asks rather than casting. An edge running through the catchment has a length instead.
 */
TEST_F(HY_CatchmentRealization_Test, APerspectiveWithNoArealExtentSaysSo)
{
    std::shared_ptr<HY_CatchmentRealization> flowpath = std::make_shared<HY_FlowPath>();
    EXPECT_FALSE(flowpath->area_sqkm().has_value());
}

/** An extent the hydrofabric did not give is absent rather than zero. */
TEST_F(HY_CatchmentRealization_Test, UnmeasuredAreaIsAbsentRatherThanZero)
{
    EXPECT_FALSE(area->area_sqkm().has_value());
}

/**
 * The feature overload reaches the same state the general constructor is given directly.
 *
 * It exists so a caller holding a hydrofabric feature need not take it apart; what it keeps is
 * an extent and a geometry either way.
 */
TEST_F(HY_CatchmentRealization_Test, TheFeatureOverloadReadsTheExtentAndTheFace)
{
    geojson::PropertyMap properties;
    properties.emplace("areasqkm", geojson::JSONProperty("areasqkm", 15.625));
    geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(
            geojson::PolygonFeature(boost::get<geojson::polygon_t>(*square()), "cat-27", properties));

    auto from_feature = std::make_shared<HY_CatchmentArea>("cat-27", nullptr, feature);

    EXPECT_EQ(from_feature->area_sqkm(), 15.625);
    EXPECT_TRUE(from_feature->get_bounds() == feature->geometry());
}

/**
 * A realized face keeps its divide reachable after the feature it came from is released.
 *
 * The geometry is shared with the feature rather than copied out of it, so it has to outlive the
 * feature: a run that frees the fabric once every catchment is realized still has a divide to
 * resample against.
 */
TEST_F(HY_CatchmentRealization_Test, TheDivideOutlivesTheFeatureItCameFrom)
{
    geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(
            geojson::PolygonFeature(boost::get<geojson::polygon_t>(*square()), "cat-27"));

    auto realized = std::make_shared<HY_CatchmentArea>("cat-27", nullptr, 15.625,
                                                       feature->geometry());
    feature.reset();

    ASSERT_TRUE(realized->get_bounds() != nullptr);
    EXPECT_GT(boost::geometry::area(boost::get<geojson::polygon_t>(*realized->get_bounds())), 0.0);
}
