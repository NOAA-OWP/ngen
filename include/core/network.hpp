#ifndef NETWORK_H
#define NETWORK_H

#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include <features/Features.hpp>
#include <FeatureBuilder.hpp>

namespace network {
  struct VertexProperty{
    std::string id;
  };

  typedef boost::property<boost::vertex_index_t, std::size_t> IndexT;
  typedef boost::property<boost::vertex_name_t, std::string, IndexT> NodeT;
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, NodeT> Graph;
  typedef std::vector< Graph::vertex_descriptor > NetworkIndexT;
  //A type for holding a pair of NetworkIndexT iterators,  pair.first=begin,  pair.second=end
  //These indicies should be const to the caller
  using IndexPair = std::pair< NetworkIndexT::const_iterator, NetworkIndexT::const_iterator>;

    class Network {
      public:
        Network() {}
        Network( geojson::GeoJSON fabric );
        Network( geojson::GeoJSON features, std::string* link_key);
        virtual ~Network(){}

        NetworkIndexT::const_reverse_iterator begin();
        NetworkIndexT::const_reverse_iterator end();
        std::string get_id( Graph::vertex_descriptor idx);
        std::size_t size();

        IndexPair headwaters();
        IndexPair tailwaters();

      protected:

      private:
        void init_indicies();
        NetworkIndexT topo_order;
        //keep an index of headwater verticies
        NetworkIndexT headwaters_idx;
        //keep  an index of tailwater verticies
        NetworkIndexT tailwaters_idx;
        //There are "classes" of tailwater to consider, "Free" "Coastal" "Internal" ect
        //Really the same is true for head waters in the "Free" and "Internal" sense where
        //there are interactions of boundary conditions
        //TODO Distributary network??? Has topological "wildcard"
        //Diffusive routing can assume DAG, Dynamic routing cannot

        //Boost Graph
        Graph graph;
        //Keep a mapping of identity to graph vertex descritpor
        std::unordered_map<std::string, Graph::vertex_descriptor> descritpor_map;

    };
}

#endif //NETWORK_H
