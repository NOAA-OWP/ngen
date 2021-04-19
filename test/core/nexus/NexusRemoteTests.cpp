#ifdef NGEN_MPI_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "HY_PointHydroNexusRemoteUpstream.hpp"
#include "HY_PointHydroNexusRemote.hpp"


#include <vector>
#include <memory>

class Nexus_Remote_Test : public ::testing::Test {

protected:

    Nexus_Remote_Test()
    {

    }

    ~Nexus_Remote_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<HY_PointHydroNexusRemoteUpstream> upstream_remote_nexus;
    std::shared_ptr<HY_PointHydroNexusRemote> downstream_remote_nexus;
};

void Nexus_Remote_Test::SetUp() {
   int nexus_id_number = 26;
   std::string nexus_id = "nex-26";
   int num_downstream = 1;

   MPI_Init(NULL, NULL);

   upstream_remote_nexus = std::make_shared<HY_PointHydroNexusRemoteUpstream>(nexus_id_number, nexus_id, num_downstream);

   std::unordered_map<long,long> loc_map;
   loc_map[27] = 0;
   downstream_remote_nexus = std::make_shared<HY_PointHydroNexusRemote>(nexus_id_number, nexus_id, num_downstream, loc_map);

}

void Nexus_Remote_Test::TearDown() {

}

//Test sending data with MPI from an upstream remote nexus
//to a downstream remote nexus.
TEST_F(Nexus_Remote_Test, TestInit0)
{
   int send_catchment_id = 27;
   double send_flow = 1.2;
   long send_time_step = 3;

   int receive_catchment_id = 0;
   double receive_flow = 0.0;
   long receive_time_step = 0;

   // Find out rank, size
   int world_rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

   // If the world_rank is 0, then call HY_PointHydroNexusRemoteUpstream to send
   // data to downstream remote nexus.
   if (world_rank == 0)
   {
      upstream_remote_nexus->add_upstream_flow(send_flow, send_catchment_id, send_time_step);
   }

   // Else if the world_rank is 1, then call HY_PointHydroNexusRemoteDownstream to receive data
   // from the upstream remote nexus.
   else if (world_rank == 1)
   {
      downstream_remote_nexus->add_upstream_flow(-9999.0, send_catchment_id, send_time_step);

      receive_catchment_id = 27;
      receive_flow = downstream_remote_nexus->inspect_upstream_flows(send_time_step).first;
      receive_time_step = downstream_remote_nexus->get_time_step();

      std::cout << "Received: \nCatchment ID " << receive_catchment_id << " \nFlow "
      << receive_flow << " \nTime Step " << receive_time_step << std::endl;

      //Assert equal for the send and receive values
      EXPECT_DOUBLE_EQ(send_flow, receive_flow);

      EXPECT_EQ(send_catchment_id, receive_catchment_id);

      EXPECT_EQ(send_time_step, receive_time_step);

   }

   MPI_Finalize();

   ASSERT_TRUE(true);

}

#endif  // NGEN_MPI_TESTS_ACTIVE

