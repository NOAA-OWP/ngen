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

    static void SetUpTestSuite()
    {
        MPI_Init(NULL, NULL);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
    }

    static void TearDownTestSuite()
    {
        MPI_Finalize();
    }

    std::vector<double> stored_discharge;
    static int mpi_rank;
    static int mpi_num_procs;
};

int Nexus_Remote_Test::mpi_rank;
int Nexus_Remote_Test::mpi_num_procs;

void Nexus_Remote_Test::SetUp()
{
    //MPI_Init(NULL, NULL);
    //MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    stored_discharge.push_back(1.2);
    stored_discharge.push_back(1.8);
    stored_discharge.push_back(2.3);
}

void Nexus_Remote_Test::TearDown()
{

}



//Test sending data with MPI from an upstream remote nexus
//to a downstream remote nexus.
TEST_F(Nexus_Remote_Test, TestInit0)
{
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        loc_map[26] = 1;
        nexus = std::make_shared<HY_PointHydroNexusRemote>(27, "nexus-27", 1, loc_map);
    }
    else if ( mpi_rank == 1)
    {
        loc_map[27] == 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>(26, "nexus-26", 1, loc_map);
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

    //delete nexus;

    ASSERT_TRUE(true);

}

TEST_F(Nexus_Remote_Test, TestDeadlock1)
{
    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    std::shared_ptr<HY_PointHydroNexusRemote> nexus;

    double dummy_flow = -9999.0;
    double recieved_flow;
    long ts = 0;

    // In this test both processes send before recieving. If communciation are synchronus this will dead lock

    // create a nexus at both ranks
    if ( mpi_rank == 0)
    {
        loc_map[26] = 1;
        nexus = std::make_shared<HY_PointHydroNexusRemote>(27, "nexus-27", 1, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus->add_upstream_flow(200.0,1,ts);
        nexus->get_downstream_flow(26,ts,100);                      // sending to 26
        nexus->add_upstream_flow(dummy_flow,27,ts+1);                 // recieving from 27
        recieved_flow = nexus->get_downstream_flow(2,ts+1,100);       // get the recieved flow
    }
    else if ( mpi_rank == 1)
    {
        loc_map[27] == 0;
        nexus = std::make_shared<HY_PointHydroNexusRemote>(26, "nexus-26", 1, loc_map);

        // We use two differnt time steps becuase a nexus does not allow water to be added after a send
        nexus->add_upstream_flow(200.0,1,ts+1);
        nexus->get_downstream_flow(27,ts+1,100);                      // sending to 27
        nexus->add_upstream_flow(dummy_flow,26,ts);                 // recieving grom 26
        recieved_flow = nexus->get_downstream_flow(2,ts,100);       // get the recieved flow
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

TEST_F(Nexus_Remote_Test, TestTree1)
{
    int tree_height = 10;
    int num_nodes = std::pow(2,tree_height) - 1;  // the number of nodes in a complete binary tree of height h
    int num_non_leaf_nodes = std::pow(2,tree_height-1) - 1;

    double nodes_per_rank = num_nodes / double(mpi_num_procs);

    HY_PointHydroNexusRemote::catcment_location_map_t loc_map;

    // create the location map for use with nexi at this process
    for( int i = 0; i < num_nodes; ++i)
    {
        int target_rank = static_cast<int>(i / nodes_per_rank);
        if ( target_rank != mpi_rank )
        {
            loc_map[i] = target_rank;
        }
    }

    // now create the nexus objects that are local
    int start_id = static_cast<int>(mpi_rank * nodes_per_rank);
    int stop_id = static_cast<int>((mpi_rank + 1) * nodes_per_rank);
    std::unordered_map<int, std::shared_ptr<HY_PointHydroNexusRemote> > nexus_map;

    for( int i = start_id; i < stop_id; ++i)
    {
        nexus_map[i] = std::make_shared<HY_PointHydroNexusRemote>(i, "nexus-"+std::to_string(i), 1, loc_map);
    }

    // closure functions for determining tree in array positions

    auto left = [](int N){return 2*N+1;};           // left child postion
    auto right = [](int N){return 2*N+2;};          // right child postion
    auto parent = [](int N){return (N-1) / 2;};     // parent position
    auto valid = [num_nodes](int N)                 // is this id valid?
    {
        return N < num_nodes;
    };
    auto leaf = [num_non_leaf_nodes,num_nodes](int N)                 // is this id a leaf?
    {
        return num_non_leaf_nodes <= N && N < num_nodes;
    };

    long ts = 0;

    // for each nexus accept upstream flow
    for( int i = stop_id -1; i >= start_id; --i )
    {
        if ( leaf(i) )
        {
            // if this is a leaf we need a synthetic flow
            nexus_map[i]->add_upstream_flow(1.0,num_nodes+10,ts)
        }
        else
        {
            // if this is not a leaf try to get flow from its upstreams

            int l = left(i);
            int r = right(i);
            float dummy_value;
            float flow;

            if ( loc_map.find(l) != loc_map.end() )
            {
                // l is a remote node
                nexus_map[i]->add_upstream_flow(dummy_value,l,ts);
            }
            else
            {
                flow = nexus_map[l]->get_upstream_flow(i,ts,100.0,);
                nexus_map[i]->add_upstream_flow(flow,l,ts);
            }

            if ( loc_map.find(r) != loc_map.end() )
            {
                // l is a remote node
                nexus_map[r]->add_upstream_flow(dummy_value,l,ts);
            }
            else
            {
                flow = nexus_map[r]->get_upstream_flow(i,ts,100.0,);
                nexus_map[r]->add_upstream_flow(flow,l,ts);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

//#endif  // NGEN_MPI_TESTS_ACTIVE

