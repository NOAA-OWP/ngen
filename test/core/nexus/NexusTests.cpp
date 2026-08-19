#include "gtest/gtest.h"

#include "HY_PointHydroNexus.hpp"
#include "HY_HydroLocation.hpp"
#include "HY_IndirectPosition.hpp"

#include <vector>
#include <memory>
using namespace hy_features::hydrolocation;

class Nexus_Test : public ::testing::Test {

protected:

    Nexus_Test()
    {

    }

    ~Nexus_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    std::string abridged_json_file;
    std::string complete_json_file;
    std::string id_map_json_file;

};

void Nexus_Test::SetUp() {


}

void Nexus_Test::TearDown() {

}

//! Test that a HY_Hydrolocation can be created.
TEST_F(Nexus_Test, TestInit0)
{
    point_t point(50,30);                                                   //!< Test data location
    //HY_HydroLocationType type(HY_HydroLocationType::undefined);    //!< Test data type
    //HY_IndirectPosition pos;
    //std::shared_ptr<HY_HydroLocation>location = std::make_shared<HY_HydroLocation>(point, type, pos);
    std::vector<std::string> contrib = {"cat-1"};
    //HY_PointHydroNexus("nex-0", location, std::vector<string>(), contrib);
    HY_PointHydroNexus("nex-0", contrib);
    ASSERT_TRUE( true );
}

//! flush() releases the per-timestep flow state a nexus accumulates.
TEST_F(Nexus_Test, FlushReleasesAccumulatedFlowState)
{
    std::vector<std::string> receiving = {"cat-down"};
    std::vector<std::string> contributing = {"cat-up"};
    HY_PointHydroNexus nexus("nex-0", receiving, contributing);

    nexus.add_upstream_flow(10.0, "cat-up", 0);
    nexus.add_upstream_flow(5.0, "cat-up", 1);
    ASSERT_EQ(nexus.inspect_upstream_flows(0).second, 1);
    ASSERT_DOUBLE_EQ(nexus.inspect_upstream_flows(0).first, 10.0);
    ASSERT_EQ(nexus.inspect_upstream_flows(1).second, 1);

    nexus.flush(); // clear_completed defaults to false

    // All accumulated per-timestep flow state is released.
    EXPECT_EQ(nexus.inspect_upstream_flows(0).second, 0);
    EXPECT_DOUBLE_EQ(nexus.inspect_upstream_flows(0).first, 0.0);
    EXPECT_EQ(nexus.inspect_upstream_flows(1).second, 0);
    EXPECT_EQ(nexus.inspect_downstream_requests(0).second, 0);
}

//! clear_completed controls whether flush() also forgets which timesteps were
//! fully consumed (and are therefore locked against further operations).
TEST_F(Nexus_Test, FlushClearCompletedControlsCompletedTimestepTracking)
{
    // Build a nexus whose timestep 0 has been fully consumed. A timestep is
    // marked "completed" only when downstream requests sum to 100%, which needs
    // more than one partial request (a single 100% request does not complete it).
    auto build_completed_nexus = []() {
        std::vector<std::string> receiving = {"cat-a", "cat-b"};
        std::vector<std::string> contributing = {"cat-up"};
        auto n = std::make_unique<HY_PointHydroNexus>("nex-0", receiving, contributing);
        n->add_upstream_flow(10.0, "cat-up", 0);
        n->get_downstream_flow("cat-a", 0, 60.0);
        n->get_downstream_flow("cat-b", 0, 40.0); // total 100% -> timestep 0 completed
        return n;
    };

    // Sanity: a completed timestep rejects further upstream flow.
    {
        auto n = build_completed_nexus();
        EXPECT_ANY_THROW(n->add_upstream_flow(1.0, "cat-up", 0));
    }

    // flush(false) keeps the completed record: timestep 0 stays locked.
    {
        auto n = build_completed_nexus();
        n->flush(false);
        EXPECT_ANY_THROW(n->add_upstream_flow(1.0, "cat-up", 0));
    }

    // flush(true) clears the completed record: timestep 0 is usable again.
    {
        auto n = build_completed_nexus();
        n->flush(true);
        EXPECT_NO_THROW(n->add_upstream_flow(1.0, "cat-up", 0));
        EXPECT_EQ(n->inspect_upstream_flows(0).second, 1);
    }
}
