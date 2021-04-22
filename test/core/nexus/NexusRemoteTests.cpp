//#ifdef NGEN_MPI_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "HY_PointHydroNexusRemote.hpp"


#include <vector>
#include <memory>

class Nexus_Remote_Test : public ::testing::Test
{

protected:

    Nexus_Remote_Test()
    {

    }

    ~Nexus_Remote_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    std::vector<double> stored_discharge;
    int mpi_rank;
};

void Nexus_Remote_Test::SetUp()
{
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    stored_discharge.push_back(1.2);
    stored_discharge.push_back(1.8);
    stored_discharge.push_back(2.3);
}

void Nexus_Remote_Test::TearDown()
{
    MPI_Finalize();
}



//Test sending data with MPI from an upstream remote nexus
//to a downstream remote nexus.
TEST_F(Nexus_Remote_Test, TestInit0)
{
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    HY_PointHydroNexusRemote* nexus;

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        loc_map[26] = 1;
        nexus = new HY_PointHydroNexusRemote(27, "nexus-27", 1, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        loc_map[27] == 0;
        nexus = new HY_PointHydroNexusRemote(26, "nexus-26", 1, loc_map);
    }

    double dummy_flow = -9999.0;
    long ts = 0;

    for ( auto discharge : stored_discharge)
    {
        switch(mpi_rank)
        {
            case 0:
                std::cerr << "Rank 0: Sending flow of " << discharge << " to catchment 26\n";
                nexus->add_upstream_flow(discharge,1,ts);
                nexus->get_downstream_flow(26,ts,100);
            break;

            case 1:
                nexus->add_upstream_flow(dummy_flow,27,ts);
                double recieved_flow = nexus->get_downstream_flow(2,ts,100);
                ASSERT_EQ(discharge,recieved_flow);
                std::cerr << "Rank 1: Recieving flow of " << recieved_flow << " from catchment Nexus connected to catchment 27\n";
            break;
        }

        ++ts;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    delete nexus;

    ASSERT_TRUE(true);

}

//#endif  // NGEN_MPI_TESTS_ACTIVE

