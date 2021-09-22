#include "gtest/gtest.h"

#include <FeatureCollection.hpp>
#include <features/Features.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>

#include "network.hpp"

using namespace network;

class Network_Test {

protected:

    Network_Test()
    {

    }

    ~Network_Test() {

    }

    void add_catchment(std::string id, std::string to_id)
    {
      geojson::three_dimensional_coordinates three_dimensions {
          {
              {1.0, 2.0},
              {3.0, 4.0},
              {5.0, 6.0}
          },
          {
              {7.0, 8.0},
              {9.0, 10.0},
              {11.0, 12.0}
          }
      };
      std::vector<double> bounding_box{1.0, 2.0};
      geojson::PropertyMap properties{
          {this->link_key, geojson::JSONProperty(this->link_key, to_id)},
      };

      geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
        geojson::polygon(three_dimensions),
        id,
        properties
        //bounding_box
      ));

      catchments->add_feature(feature);
    }

    void add_nexus(std::string id, std::string to_id = {})
    {
      double x = 102.0;
      double y = 0.5;

      geojson::PropertyMap properties{};
      if (! to_id.empty())
      {
        properties.emplace(this->link_key, geojson::JSONProperty(this->link_key, to_id));
      }
      geojson::Feature feature = std::make_shared<geojson::PointFeature>(geojson::PointFeature(
        geojson::coordinate_t(x, y),
        id,
        properties
        //bounding_box
      ));

      nexuses->add_feature(feature);
    }

    std::shared_ptr<geojson::FeatureCollection> get_fabric()
    {
      std::shared_ptr<geojson::FeatureCollection> fabric = std::make_shared<geojson::FeatureCollection>();
      for(auto& feature: *this->catchments)
      {
        fabric->add_feature(feature);
      }
      for(auto& feature: *this->nexuses)
      {
        fabric->add_feature(feature);
      }
      fabric->link_features_from_property(nullptr, &this->link_key);
      return fabric;
    }

    std::shared_ptr<geojson::FeatureCollection> catchments = std::make_shared<geojson::FeatureCollection>();
    std::shared_ptr<geojson::FeatureCollection> nexuses = std::make_shared<geojson::FeatureCollection>();
    std::string link_key = "toid";
    Network n;
};

/*
* Enumerate each possible construction context
*/
enum class TestContext{
  CASE_1,
  CASE_2,
};

class  Network_Test1 : public Network_Test, public ::testing::TestWithParam<TestContext>
{
public:
  Network_Test1(){}
  void SetUp() override {
    this->add_catchment("cat-0", "nex-0");
    this->add_catchment("cat-1", "nex-0");
    this->add_nexus("nex-0");
    auto context = GetParam();
    switch( context )
    {
      case TestContext::CASE_1:
      {
        //Test construction from an already linked geojson feature collection fabric
        n = Network(this->get_fabric());
        break;
      }
      case TestContext::CASE_2:
      {
        //Test construction from just a sinle feature collection
        n = Network(catchments,  &link_key);
        break;
      }
    }

  }

  ~Network_Test1(){}
};

class Network_Test2 : public Network_Test, public ::testing::Test{
public:
  Network_Test2(){}
  void SetUp(){
    this->add_catchment("cat-0", "nex-0");
    this->add_catchment("cat-1", "nex-0");
    this->add_nexus("nex-0", "cat-2");
    this->add_catchment("cat-2", "nex-1");
    this->add_catchment("cat-3", "nex-1");
    this->add_catchment("cat-4", "nex-1");
    this->add_nexus("nex-1");
    n =  Network(this->get_fabric());
  }
};

//! Test that a network can be created.
TEST_P(Network_Test1, TestNetworkConstructionNumberOfNodes)
{
  //Test basic construction, network should have three nodes when done
  /*
  for(auto it = n.begin(); it != n.end(); it++)
  {
    std::cout<< n.get_id(*it)<<std::endl;
  }
  */
  ASSERT_TRUE( n.size() == 3 );

}

TEST_P(Network_Test1, TestNetworkHeadwaterIndex)
{
  NetworkIndexT::const_iterator begin, end;
  boost::tie(begin, end) = n.headwaters();

  for(auto it = begin; it != end; ++it)
  {
    std::string id =  n.get_id(*it);
    ASSERT_TRUE( id  ==  "cat-0" | id ==  "cat-1");
  }
}

TEST_P(Network_Test1, TestNetworkTailwaterIndex)
{
  NetworkIndexT::const_iterator begin, end;
  boost::tie(begin, end) = n.tailwaters();

  for(auto it = begin; it != end; ++it)
  {
    std::string id =  n.get_id(*it);
    ASSERT_TRUE( id  ==  "nex-0");
  }
}

TEST_P(Network_Test1, TestNetworkTopologicalIndex)
{
  std::vector<std::string> expected_topo_order = {"cat-1",  "cat-0", "nex-0"};
  auto expected_it = expected_topo_order.begin();
  auto  network_it = n.begin();

  while( expected_it != expected_topo_order.end() && network_it != n.end() )
  {
    ASSERT_TRUE( *expected_it == n.get_id(*network_it) );
    ++expected_it;
    ++network_it;
  }
}

TEST_P(Network_Test1, TestNetwork_get_id)
{
  //Test valid vertex descriptors
  ASSERT_TRUE( n.get_id(0) == "cat-0" );
  ASSERT_TRUE( n.get_id(1) == "nex-0" );
  ASSERT_TRUE( n.get_id(2) == "cat-1" );
}

TEST_P(Network_Test1, TestNetwork_get_id_1)
{
  //Test an invalid vertex descriptor
  int idx = 100;
  try {
    n.get_id(idx);
    FAIL();
  } catch( std::invalid_argument const & ex ){
    EXPECT_EQ(ex.what(), std::string("Network::get_id: No vertex descriptor "+std::to_string(idx)+" in network."));
  }
  catch(...){
    FAIL() << "Expected std::invalid_argument";
  }
}

TEST_P(Network_Test1, TestNetwork_get_id_2)
{
  //Test an invalid vertex descriptor on the edge of the valid range
  int idx = 3;
  try {
    n.get_id(idx);
    FAIL();
  } catch( std::invalid_argument const & ex ){
    EXPECT_EQ(ex.what(), std::string("Network::get_id: No vertex descriptor "+std::to_string(idx)+" in network."));
  }
  catch(...){
    FAIL() << "Expected std::invalid_argument";
  }
}

/*  A nice example of how to disable a test
TEST_P(Network_Test1, DISABLED_TestInit1)
{
  ASSERT_TRUE( true );
}
*/

INSTANTIATE_TEST_SUITE_P(NetworkConstructionTests, Network_Test1, ::testing::Values(TestContext::CASE_1, TestContext::CASE_2)
);


TEST_F(Network_Test2, test_construction)
{
  //Test basic construction, network should have three nodes when done
  /*
  for(auto it = n.begin(); it != n.end(); it++)
  {
    std::cout<< n.get_id(*it)<<std::endl;
  }
  */
  ASSERT_TRUE( n.size() == 7 );

}
TEST_F(Network_Test2, test_get_origination_ids)
{
  std::vector<std::string> ids = n.get_origination_ids("nex-1");
  ASSERT_TRUE( ids.size() == 3);
  ASSERT_FALSE( std::find(ids.begin(), ids.end(), "cat-2") == ids.end() );
  ASSERT_FALSE( std::find(ids.begin(), ids.end(), "cat-3") == ids.end() );
  ASSERT_FALSE( std::find(ids.begin(), ids.end(), "cat-4") == ids.end() );
}

TEST_F(Network_Test2, test_get_destination_ids)
{
  std::vector<std::string> ids = n.get_destination_ids("nex-1");
  ASSERT_TRUE( ids.size() == 0);
}

TEST_F(Network_Test2, test_get_destination_ids1)
{
  std::vector<std::string> ids = n.get_destination_ids("nex-0");
  //n.print_network();
  ASSERT_TRUE( ids.size() == 1);
  ASSERT_FALSE( std::find(ids.begin(), ids.end(), "cat-2") == ids.end() );
}

TEST_F(Network_Test2, test_get_destination_ids2)
{
  std::vector<std::string> ids = n.get_destination_ids("cat-2");

  ASSERT_TRUE( ids.size() == 1);
  ASSERT_FALSE( std::find(ids.begin(), ids.end(), "nex-1") == ids.end() );
}

TEST_F(Network_Test2, test_catchments_filter)
{
  //This order IS IMPORTANT, it should be the topological order of catchments.  Note that the order isn't
  //guaranteed on the leafs, the only guarantees for this test is that cat-2 comes after both cat-0 and cat-1.
  //Technically, this is a valid topological order: cat-0, cat-3, cat-1, cat-4, cat-2 ...though unlikely.
  auto catchments = n.filter("cat");
  //auto catchments_it = catchments.begin();
  for(const auto& id : n) std::cout<<"topo_id: "<<n.get_id(id)<<std::endl;
  for(const auto& id : catchments) std::cout<<"id: "<<id<<std::endl;
  auto cat0_it = std::find(catchments.begin(), catchments.end(), "cat-0");
  auto cat1_it = std::find(catchments.begin(), catchments.end(), "cat-1");
  auto cat2_it = std::find(catchments.begin(), catchments.end(), "cat-2");
  std::cout << "cat-0 to cat-2 distance: " << std::distance(cat0_it, cat2_it) << std::endl;
  std::cout << "cat-1 to cat-2 distance: " << std::distance(cat1_it, cat2_it) << std::endl;
  ASSERT_TRUE( std::distance(cat0_it, cat2_it) > 0);
  ASSERT_TRUE( std::distance(cat1_it, cat2_it) > 0);
  ASSERT_TRUE( cat2_it != catchments.end() );
}

TEST_F(Network_Test2, test_nexus_filter)
{
  //This order IS IMPORTANT, it should be the topological order of nexus.  Note that the order isn't
  //guaranteed on the leafs...
  std::vector<std::string> expected_topo_order = {"nex-0", "nex-1"};
  auto expected_it = expected_topo_order.begin();
  auto nexuses = n.filter("nex");
  auto nexus_it = nexuses.begin();
    while( expected_it != expected_topo_order.end() && nexus_it != nexuses.end() )
  {
      ASSERT_TRUE( *expected_it == *nexus_it );
    ++expected_it;
    ++nexus_it;
  }
}

TEST_F(Network_Test2, test_bad_filter)
{
  //Test a bad prefix gives no results
  auto results = n.filter("fs");
  ASSERT_TRUE( results.begin() == results.end() );
}

TEST_F(Network_Test2, test_dfr_filter)
{
  for(const auto& id : n) std::cout<<"topo_id: "<<n.get_id(id)<<std::endl;

  //This order IS IMPORTANT, it should be a pre-order depth-first traversal. So, cat-0 and cat-1
  // should appear *immediately* after cat-2, though the order of cat-0 and cat-1 is undefined.
  // Likewise the order of cat-2 vis-a-vis cat-3 and cat-4 are undefined because they are the same
  // tree level.
  auto catchments = n.filter("cat", network::SortOrder::TransposedDepthFirstPreorder);
  //for(const auto& id : catchments) std::cout<<"id: "<<id<<std::endl;

  auto cat0_it = std::find(catchments.begin(), catchments.end(), "cat-0");
  auto cat1_it = std::find(catchments.begin(), catchments.end(), "cat-1");
  auto cat2_it = std::find(catchments.begin(), catchments.end(), "cat-2");
  int c2c0dist = std::distance(cat2_it, cat0_it);
  int c2c1dist = std::distance(cat2_it, cat1_it);
  std::cout << "cat-2 to cat-0 distance: " << c2c0dist << std::endl;
  std::cout << "cat-2 to cat-1 distance: " << c2c1dist << std::endl;
  ASSERT_TRUE( c2c0dist > 0 && c2c0dist <= 2);
  ASSERT_TRUE( c2c1dist > 0 && c2c1dist <= 2);
  // Not necessary, but interesting that this somehow causes get_id to be called with an invalid index. ??!?
  //ASSERT_FALSE( std::distance(cat0_it, cat2_it) > 0 );
}

