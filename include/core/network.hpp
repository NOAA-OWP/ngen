#ifndef NETWORK_H
#define NETWORK_H

#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <features/Features.hpp>
#include <FeatureBuilder.hpp>

#include "HY_Features_Ids.hpp"

namespace network {

  const static int DEFAULT_LAYER_ID = 0;

  /**
   * @brief Selector for using different traversal orders for linear return
   */
  enum class SortOrder {
    /**
     * @brief Topological order. Usually the default. Good for dependency ordering. This is provided directly by <a href="">boost::topological_sort</a>.
     */
    Topological,
    /**
     * @brief A depth-first, pre-order recorded traversal on a transposed graph. Good for extracting sets of highly contiguous nodes. 
     * Transposed because the hydrograph "tree" is actually most like an inverted tree structure, with headwaters as the "root" nodes
     * and coastal drainage as the "leaves". This transposed traversal lets us traverse upstream instead of downstream.
     */
    TransposedDepthFirstPreorder
  };
  /**
   * @brief The structure defining graph vertex properties.
   * 
   * @var VertexProperty::id
   * The string identifier of the vertex.
   */
  struct VertexProperty{
    std::string id;
  };

  /**
   * @brief Pre-order recording dfs_visitor. Lightly modified from <a href="https://www.boost.org/doc/libs/1_72_0/boost/graph/topological_sort.hpp">boost::topo_sort_visitor</a>.
   * @tparam OutputIterator Any std::output_iterator . Note that if for instance a std::back_insert_iterator is used, the recorded order will be reversed.
   * @see <a href="https://www.boost.org/doc/libs/1_72_0/boost/graph/topological_sort.hpp">boost::topo_sort_visitor</a>
   */
  template < typename OutputIterator >
  struct preorder_visitor : public boost::dfs_visitor<>
  {
      preorder_visitor(OutputIterator _iter) : m_iter(_iter) {}

      template < typename Edge, typename Graph >
      void back_edge(const Edge&, Graph&)
      {
          BOOST_THROW_EXCEPTION(boost::not_a_dag());
      }

      template < typename Vertex, typename Graph >
      void discover_vertex(const Vertex& u, Graph&)
      {
          *m_iter++ = u;
          //std::cerr << "vdiscover_vertex " << u << std::endl;
      }

      OutputIterator m_iter;
  };

  /**
   * @brief Pre-order depth first sort function.
   * @param g A <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/VertexListGraph.html">boost::VertexListGraph</a>
   * @param result Any std::output_iterator . Note that if for instance a std::back_insert_iterator is used, the recorded order will be reversed.
   * @param params Named params passed to <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/depth_first_search.html">boost::depth_first_search(G, params)</a>
   * @tparam VertexListGraph A <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/VertexListGraph.html">boost::VertexListGraph</a>
   * @tparam OutputIterator Any std::output_iterator . Note that if for instance a std::back_insert_iterator is used, the recorded order will be reversed.
   * @tparam P Param type, see <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/bgl_named_params.html">boost::bgl_named_params</a>
   * @tparam T Type type, see <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/bgl_named_params.html">boost::bgl_named_params</a>
   * @tparam R Rest type, see <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/bgl_named_params.html">boost::bgl_named_params</a>
   * @see <a href="https://www.boost.org/doc/libs/1_72_0/boost/graph/topological_sort.hpp">boost::topological_sort(g, result, params)</a>
   */
  template < typename VertexListGraph, typename OutputIterator, typename P,
    typename T, typename R >
  void df_preorder_sort(VertexListGraph& g, OutputIterator result,
      const boost::bgl_named_params< P, T, R >& params)
  {
      typedef preorder_visitor< OutputIterator > PreOrderVisitor;
      depth_first_search(g, params.visitor(PreOrderVisitor(result)));
  }

  /**
   * @brief Default named params wrapper (passes no named params) for df_preorder_sort(VertexListGraph &g, OutputIterator result, const boost::bgl_named_params< P, T, R > &params)
   * @tparam VertexListGraph A <a href="https://www.boost.org/doc/libs/1_72_0/libs/graph/doc/VertexListGraph.html">boost::VertexListGraph</a>
   * @tparam OutputIterator Any std::output_iterator . Note that if for instance a std::back_insert_iterator is used, the recorded order will be reversed.
   */ 
  template < typename VertexListGraph, typename OutputIterator >
  void df_preorder_sort(VertexListGraph& g, OutputIterator result)
  {
      df_preorder_sort(
          g, result, boost::bgl_named_params< int, boost::buffer_param_t >(0)); // no additional named params
  }

  /**
   * @brief Type used to index network::NodeT types
   * 
   */
  typedef boost::property<boost::vertex_index_t, std::size_t> IndexT;
  /**
   * @brief Type used to label node properties in a network::Graph
   * 
   */
  typedef boost::property<boost::vertex_name_t, std::string, IndexT> NodeT;
  /**
   * @brief Parameterized graph storing network::NodeT vertices
   * 
   * Nodes are stored as boost::setS, edges as boost::vecS, with bidirectional edges
   * 
   */
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, NodeT> Graph;
  /**
   * @brief A vector of network::Graph vertex_descriptors
   * 
   */
  typedef std::vector< Graph::vertex_descriptor > NetworkIndexT;
  /**
   * @brief A type for holding a pair of network::NetworkIndexT iterators,  pair.first=begin,  pair.second=end
   * 
   * these indicies should be const to the caller
   * 
   */
  using IndexPair = std::pair< NetworkIndexT::const_iterator, NetworkIndexT::const_iterator>;

  /**
   * @brief Graph visitor which detects and stores cycles in the network
   * 
   */
  struct detect_cycles : public boost::dfs_visitor<>
  {
    using colormap = std::map<Graph::vertex_descriptor, boost::default_color_type>;
    colormap vertex_coloring;

    using edgeColorMap = std::map<Graph::edge_descriptor, boost::default_color_type>;
    edgeColorMap  edge_coloring;
    using vertex_t = Graph::vertex_descriptor;

    //Delete the default constructor so we don't get a nullptr for cycles
    detect_cycles() = delete;
    /**
     * @brief Construct a new detect_cycle object holding a reference to a vector to store cycle pairs
     * 
     * @param out pointer to a vector to hold vertex pairs that complete cycles
     */
    detect_cycles(std::vector<std::pair<Graph::vertex_descriptor, Graph::vertex_descriptor>>* out):cycles(out){}
    
    /**
     * @brief Mark target verticies of visited edge
     * 
     * @tparam Edge 
     * @tparam Graph 
     * @param e 
     * @param g 
     */
    /*
    template <class Edge, class Graph>
    void tree_edge(Edge e, const Graph& g) {
      //Start by pushing the first vertex (source)
      if (vertexVisited.empty()) {
          vertexVisited.push(boost::source(e, g));
      }
      //Push every assoicated target vertex of the edge
      vertexVisited.push(boost::target(e, g));
    }*/
   
    /**
      * @brief When a back edge is encountered, we have a cycle
      * 
      * @tparam Edge 
      * @tparam Graph 
      * @param e 
      * @param g 
      */
    template <class Edge, class Graph>
    void back_edge(Edge e, const Graph& g) {
      // std::cout << source(e, g)
      //   << " -- "
      //   << target(e, g) << "\n";
      // std::cout<< get(boost::vertex_name, g)[ source(e, g) ] <<
      //   " -- " << get(boost::vertex_name, g)[ target(e,g)  ]<<"\n";
      vertex_t v2;
      //At this point, vertexVisited is every vertice which has been visted
      //up to the point of finding this back edge (so each is part of the total cycle)
      /*while ( vertexVisited.top() != boost::target(e, g) )
      {
          //std::cout << " Cycle middle=" << vertexVisited.top() << std::endl;
          v2 = vertexVisited.top();
          vertexVisited.pop();
      }*/
      //Reset the verticies stack to the last
      //vertexVisited.push(v2);
      if(cycles != nullptr){
        cycles->push_back(std::make_pair(source(e,g), target(e,g)));
      }
    }
    //Currently not used, but could be used to report the reset of the 
    //verticies involved in the cycle if needed
    //(would need become a pointer similar to cycles if consumed/reported externally)
    //std::stack<Graph::vertex_descriptor> vertexVisited;
    //Since visitors are passed by value, we need an external reference
    //to store the cycle in
    std::vector<std::pair<Graph::vertex_descriptor, Graph::vertex_descriptor>>* cycles;
  };

    /**
     * @brief A lightweight, graph based index of hydrologic features.
     * 
     * This class provides various iterators and access to hydrologic feature identities based on their topological relationships.
     * It uses a boost::graph to model the relationship of features, and various iterators of the features based on traversal of this graph.
     */
    class Network {
      public:
        /**
         * @brief Construct a new empty Network object
         * 
         */
        Network() {}

        /**
         * @brief Construct a new Network object from the features in fabric, assumes features have already been linked.
         * 
         * @param fabric a geojson::GeoJSON collection of features to add as nodes to the graph
         */
        Network( geojson::GeoJSON fabric );

        /**
         * @brief Construct a new Network object from features in fabric, creating edges between feature properties defined by link_key
         * 
         * @param features a geojson::GeoJSON collection of features to add as nodes to the graph
         * @param link_key the property to read from features to determine edge linking, i.e. 'toid'
         */
        Network( geojson::GeoJSON features, std::string* link_key);

        /**
         * @brief Destroy the Network object
         * 
         */
        virtual ~Network(){}

        /**
         * @brief Iterator to the first graph vertex descriptor of the topologically ordered graph vertices
         * 
         * @return NetworkIndexT::const_reverse_iterator 
         */
        NetworkIndexT::const_reverse_iterator begin();

        /**
         * @brief Iterator to the end of the topologically ordered graph vertices
         * 
         * @return NetworkIndexT::const_reverse_iterator 
         */
        NetworkIndexT::const_reverse_iterator end();
        
        /**
         * @brief Provides a boost transform_iterator, filtered by @p type , to the topologically ordered graph vertex string id's
         * 
         * This function is useful when only interested in a single type of feature.
         * It returns the a topologically ordered set of feature ids.  For example, to print all catchments
         * in the network:
         * @code {.cpp}
         * for( auto catchment : network.filter('cat') ){
         *    std::cout << catchment;
         * }
         * @endcode
         * 
         * @param type The type of feature to filter for, i.e. 'cat', 'nex'
         * @param order What order to return results in
         * @return auto 
         */
        auto filter(std::string type, SortOrder order = SortOrder::Topological)
        {
          //todo need to worry about validating input???
          //if type isn't found as a prefix, this iterator range should be empty,
          //which is a reasonable semantic
          return get_sorted_index(order) | boost::adaptors::reversed
                        | boost::adaptors::transformed([this](int const& i) { return get_id(i); })
                        | boost::adaptors::filtered([type](std::string const& s) { 
                          //seperate the prefix from the numeric id
                          std::string id_type = s.substr(0, s.find(hy_features::identifiers::seperator) );
                          //Allow subtypes, e.g. inx, tnx, cnx, to be pass the filter for a generic nexus type
                          if(type == hy_features::identifiers::nexus){
                            return hy_features::identifiers::isNexus(id_type);
                          }
                          //Allow subtypes, e.g. wb to be pass the filter for a generic catchment type
                          if(type == hy_features::identifiers::catchment){
                            return hy_features::identifiers::isCatchment(id_type);
                          }
                          //any other subtype filter gets only exact matches
                          return id_type == type; 
                        });

        }

        /**
         * @brief Provides a boost transform_iterator, filtered by @p type , to the topologically ordered graph vertex string id's
         * 
         * This function is useful when only interested in a single type of feature.
         * It returns the a topologically ordered set of feature ids.  For example, to print all catchments
         * in the network:
         * @code {.cpp}
         * for( auto catchment : network.filter('cat') ){
         *    std::cout << catchment;
         * }
         * @endcode
         * 
         * @param type The type of feature to filter for, i.e. 'cat', 'nex'
         * @param target_layer The layer that filtered results should be in
         * @param order What order to return results in
         * @return auto 
         */
        auto filter(std::string type, int target_layer, SortOrder order = SortOrder::Topological)
        {
          //todo need to worry about valivdating input???
          //if type isn't found as a prefix, this iterator range should be empty,
          //which is a reasonable semantic
          return get_sorted_index(order) | boost::adaptors::reversed
                        | boost::adaptors::transformed([this](int const& i) { return get_id(i); })
                        | boost::adaptors::filtered([this,type,target_layer](std::string const& s) { 
                          //seperate the prefix from the numeric id
                          std::string id_type = s.substr(0, s.find(hy_features::identifiers::seperator) );
                          if(type == hy_features::identifiers::nexus){
                            return hy_features::identifiers::isNexus(id_type) && 
                                   (this->layer_map.find(s) != this->layer_map.end() && this->layer_map[s] == target_layer);
                          }
                          if(type == hy_features::identifiers::catchment){
                            return hy_features::identifiers::isCatchment(id_type) && 
                                   (this->layer_map.find(s) != this->layer_map.end() && this->layer_map[s] == target_layer);
                          }
                          return (id_type == type) && 
                                  (this->layer_map.find(s) != this->layer_map.end() && (this->layer_map[s] == target_layer)); 
                        });
        }
        /**
         * @brief Get the string id of a given graph vertex_descriptor @p idx
         * 
         * @param idx
         * @return std::string
         * 
         * @throw std::invalid_argument if @p idx is not in the range of valid vertex descriptors [0, num_verticies)
         */
        std::string get_id( Graph::vertex_descriptor idx);

        /**
         * @brief Get the origination (upstream) ids (immediate neighbors) of all vertices with an edge connecting to @p id
         * 
         * @param id 
         * @return std::vector<std::string> 
         */
        std::vector<std::string> get_origination_ids(const std::string& id);

        /**
         * @brief Get the destination (downstream) ids (immediate neighbors) of all vertices with an edge from @p id
         * 
         * @param id 
         * @return std::vector<std::string> 
         */
        std::vector<std::string> get_destination_ids(const std::string& id);

        /**
         * @brief The number of features in the network (number of vertices)
         * 
         * @return std::size_t 
         */
        std::size_t size();

        /**
         * @brief An iterator pair (begin, end) of the network headwater features
         * 
         * @return IndexPair 
         */
        IndexPair headwaters();

        /**
         * @brief An iterator pair (begin, end) of the network tailwater features
         * 
         * @return IndexPair 
         */
        IndexPair tailwaters();

        /**
         * @brief Print a graphviz (text) representation of the network
         * 
         */
        void print_network(){
          boost::dynamic_properties dp;
          dp.property("node_id", get(boost::vertex_name, this->graph));
          boost::write_graphviz_dp(std::cout, graph, dp);
        }

      protected:

      private:

        /**
         * @brief Initializes the head/tailwater iterators after the underlying graph is constructed.
         * 
         */
        void init_indicies();

        /**
         * @brief Vector of topologically sorted features
         * 
         */
        NetworkIndexT topo_order;

        NetworkIndexT tdfp_order;

        /**
         * @brief Vector of headwater features
         * 
         */
        NetworkIndexT headwaters_idx;
        
        /**
         * @brief Vector of tailwater features
         * 
         */
        NetworkIndexT tailwaters_idx;

        //There are "classes" of tailwater to consider, "Free" "Coastal" "Internal" etc
        //Really the same is true for head waters in the "Free" and "Internal" sense where
        //there are interactions of boundary conditions
        //TODO Distributary network??? Has topological "wildcard"
        //Diffusive routing can assume DAG, Dynamic routing cannot

        /**
         * @brief The underlying boost::Graph 
         * 
         */
        Graph graph;

        /**
         * @brief Mapping of identity to graph vertex descriptor
         * 
         */
        std::unordered_map<std::string, Graph::vertex_descriptor> descriptor_map;

        /**
         * @brief Mapping of identifier to hydrofabric layer
         * 
        */
        std::unordered_map<std::string, long> layer_map;
        
        /**
         * @brief Get an index of the graph in a particular order.
         * @param order The desired order
         * @param cache NOT YET IMPLEMENTED. Whether to cache the generated index. Default is true.
         */
        const NetworkIndexT& get_sorted_index(SortOrder order = SortOrder::Topological, bool cache = true);

    };
}

#endif //NETWORK_H
