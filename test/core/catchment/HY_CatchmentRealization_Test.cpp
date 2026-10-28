#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "HY_Catchment.hpp"
#include "HY_CatchmentArea.hpp"
#include "HY_CatchmentRealization.hpp"
#include "HY_FlowPath.hpp"

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
