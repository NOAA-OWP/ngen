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
/*  A nice example of how to disable a test
TEST_P(Network_Test1, DISABLED_TestInit1)
{
  ASSERT_TRUE( true );
}
*/

INSTANTIATE_TEST_SUITE_P(NetworkConstructionTests, Network_Test1, ::testing::Values(TestContext::CASE_1, TestContext::CASE_2)
);
