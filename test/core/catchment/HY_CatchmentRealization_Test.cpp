#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "HY_Catchment.hpp"
#include "HY_CatchmentArea.hpp"
#include "HY_CatchmentRealization.hpp"

class HY_CatchmentRealization_Test : public ::testing::Test {
  protected:
    std::shared_ptr<HY_CatchmentArea> area;
    std::shared_ptr<HY_Catchment> catchment;

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
