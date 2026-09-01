#include "network.hpp"
#include <boost/graph/topological_sort.hpp>
#include <stdexcept>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace network;

/*
template < typename OutputIterator >
struct preorder_visitor : public boost::dfs_visitor<>
{
    template < typename Edge, typename Graph >
    void back_edge(const Edge&, Graph&)
    {
        BOOST_THROW_EXCEPTION(boost::not_a_dag());
    }

    template < typename Vertex, typename Graph >
    void discover_vertex(const Vertex& u, Graph&)
    {
        this->m_iter++ = u;
        std::cerr << "discover_vertex " << u << std::endl;
    }
};
*/

Network::Network( geojson::GeoJSON fabric ){

  std::string feature_id, downstream_id;
  Graph::vertex_descriptor v1, v2;

  for(auto& feature: *fabric)
  {
    feature_id = feature->get_id();

    if( this->descriptor_map.find( feature_id ) == this->descriptor_map.end() )
    {
      //Haven't visited this feature yet, add it to graph
      //get feature id and add vertex to graph
      v1 = add_vertex( feature_id, this->graph );
      this->descriptor_map.emplace( feature_id, v1);
    }
    else{
      v1 = this->descriptor_map[ feature_id];
    }

    if ( this->layer_map.find(feature_id) == this->layer_map.end() )
    {
      if ( feature->has_property("layer") )
      {
        const auto& prop = feature->get_property("layer");
        this->layer_map.emplace( feature_id, prop.as_natural_number() );
      }
      else
      {
        this->layer_map.emplace( feature_id, DEFAULT_LAYER_ID);
      }
    }

    //Add the downstream features/edges
    for( auto& downstream: feature->destination_features() )
    {
      downstream_id = downstream->get_id();
      if( this->descriptor_map.find(downstream_id) != this->descriptor_map.end() )
      {
        //Use existing vertex
        v2 = this->descriptor_map[ downstream_id ];
      }
      else {
        //Create new vertex
        v2 = add_vertex( downstream_id, this->graph );
        this->descriptor_map.emplace( downstream_id, v2 );
      }
      //Add the edge
      add_edge(v1, v2, this->graph);
      //std::cout<<"Added edge: "<<feature_id<<" -> "<<downstream_id<<std::endl;
    }
  }
  init_indicies();
}

void Network::init_indicies(){

  Graph::vertex_iterator begin, end;
  boost::tie(begin, end) = boost::vertices(this->graph);
  for(auto it  = begin; it !=  end;  ++it)
  {
    if( boost::in_degree(*it,  this->graph) == 0 ){
      this->headwaters_idx.push_back(*it);
      //std::cout<<"HW: "<<*it<<std::endl;
    }
    if( boost::out_degree(*it, this->graph) == 0 ){
      this->tailwaters_idx.push_back(*it);
      //std::cout<<"TW: "<<*it<<std::endl;
    }
  }

  boost::topological_sort(this->graph, std::back_inserter(this->topo_order),
                   boost::vertex_index_map(get(boost::vertex_index, this->graph)));
}

Network::Network( geojson::GeoJSON features, std::string* link_key ){

  std::string feature_id, downstream_id;
  Graph::vertex_descriptor v1, v2;

  //TODO ensure all features are the same logical HY_Features type?
  for(auto& feature: *features)
  {
    feature_id = feature->get_id();
    if( this->descriptor_map.find( feature_id ) == this->descriptor_map.end() )
    {
      //Haven't visited this feature yet, add it to graph
      //add vertex to graph
      v1 = add_vertex( feature_id, this->graph );
      this->descriptor_map.emplace( feature_id, v1 );
    }
    else{
      v1 = this->descriptor_map[ feature_id ];
    }

      if (link_key != nullptr and feature->has_property(*link_key)) {

          downstream_id = feature->get_property(*link_key).as_string();
          if( this->descriptor_map.find(downstream_id) != this->descriptor_map.end() )
          {
            v2 = this->descriptor_map[ downstream_id ];
          }
          else {
            v2 = add_vertex( downstream_id, this->graph );
            this->descriptor_map.emplace( downstream_id, v2 );
          }
            add_edge(v1, v2, this->graph);
      }
  }

  init_indicies();

}

NetworkIndexT::const_reverse_iterator Network::begin(){
  //return boost::vertices(this->graph).first;
  return this->topo_order.rbegin();
}

NetworkIndexT::const_reverse_iterator Network::end(){
  //return boost::vertices(this->graph).second;
  return this->topo_order.rend();
}

IndexPair Network::headwaters(){
  return std::make_pair(this->headwaters_idx.cbegin(),  this->headwaters_idx.cend());
}

IndexPair Network::tailwaters(){
  return std::make_pair(this->tailwaters_idx.cbegin(),  this->tailwaters_idx.cend());
}

std::string Network::get_id( Graph::vertex_descriptor idx) const{
  if( idx < 0 || idx >= num_vertices(this->graph) )
  {
    throw std::invalid_argument( std::string("Network::get_id: No vertex descriptor "+std::to_string(idx)+" in network."));
  }
  return get(boost::vertex_name, this->graph)[idx];
}

std::size_t Network::size(){
  return num_vertices(this->graph);
}

std::vector<std::string> Network::get_origination_ids(const std::string& id){
  Graph::in_edge_iterator begin, end;
  boost::tie(begin, end) = boost::in_edges (this->descriptor_map[id], this->graph);
  std::vector<std::string> ids;
  for(auto it = begin; it != end; ++it)
  {
    ids.push_back( get_id(boost::source(*it, this->graph)) );
  }
  return ids;
}

std::vector<std::string> Network::get_destination_ids(const std::string& id){
  Graph::out_edge_iterator begin, end;
  boost::tie(begin, end) = boost::out_edges (this->descriptor_map[id], this->graph);
  std::vector<std::string> ids;
  for(auto it = begin; it != end; ++it)
  {
    ids.push_back( get_id(boost::target(*it, this->graph)) );
  }

  return ids;
}

const NetworkIndexT& Network::get_sorted_index(SortOrder order, bool cache) const {
  if (order == SortOrder::TransposedDepthFirstPreorder) {
    if (!this->tdfp_order.empty()){
      return this->tdfp_order;
    }
    
    //TODO: change behavior for cache == false... don't mutate.
    auto r = make_reverse_graph(this->graph);
    df_preorder_sort(r , std::back_inserter(this->tdfp_order),
                    boost::vertex_index_map(get(boost::vertex_index, this->graph)));

    return this->tdfp_order;
  } else {
    // we know this has already been cached by the constructor
    return this->topo_order;
  }
}

void network::attach_sentinels(const geojson::GeoJSON& collection, const std::string& label,
                               const bool (*matches_type)(const std::string&))
{
    // New sentinels are added to the collection only after the scan completes, to avoid iterator
    // invalidation from inserting eagerly.
    std::vector<std::shared_ptr<geojson::FeatureBase>> sentinels;
    for (auto& feature : *collection)
    {
        if (feature->get_type() == geojson::FeatureType::Sentinel) {
            continue;
        }
        std::string id = feature->get_id();
        std::string type = id.substr(0, id.find(hy_features::identifiers::separator));
        if (matches_type(type) && feature->get_number_of_destination_features() == 0) {
            std::string sentinel_id = "wb-" + label + "-" + id;
            geojson::Feature sentinel_feature = std::make_shared<geojson::SentinelFeature>(sentinel_id);
            sentinels.push_back(sentinel_feature);
            feature->add_destination_feature(sentinel_feature.get());
        }
    }
    for (auto& sentinel : sentinels) {
        collection->add_feature(sentinel);
    }
}

