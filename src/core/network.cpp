#include "network.hpp"
#include <boost/graph/topological_sort.hpp>

using namespace network;

Network::Network( geojson::GeoJSON fabric ){

  std::string feature_id, downstream_id;
  Graph::vertex_descriptor v1, v2;

  for(auto& feature: *fabric)
  {
    feature_id = feature->get_id();
    if( this->descritpor_map.find( feature_id ) == this->descritpor_map.end() )
    {
      //Haven't visited this feature yet, add it to graph
      //get feature id and add vertex to graph
      v1 = add_vertex( feature_id, this->graph );
      this->descritpor_map.emplace( feature_id, v1);
      //Add the downstream features/edges
      for( auto& downstream: feature->destination_features() )
      {
        downstream_id = downstream->get_id();
        if( this->descritpor_map.find(downstream_id) != this->descritpor_map.end() )
        {
          //Use existing vertex
          v2 = this->descritpor_map[ downstream_id ];
        }
        else {
          //Create new vertex
          v2 = add_vertex( downstream_id, this->graph );
          this->descritpor_map.emplace( downstream_id, v2 );
        }
        //Add the edge
        add_edge(v1, v2, this->graph);
        //std::cout<<"Added edge: "<<feature_id<<" -> "<<downstream_id<<std::endl;
      }
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

Network::Network( geojson::GeoJSON features, std::string* link_key = nullptr ){

  std::string feature_id, downstream_id;
  Graph::vertex_descriptor v1, v2;

  //TODO ensure all features are the same logical HY_Features type?
  for(auto& feature: *features)
  {
    feature_id = feature->get_id();
    if( this->descritpor_map.find( feature_id ) == this->descritpor_map.end() )
    {
      //Haven't visited this feature yet, add it to graph
      //add vertex to graph
      v1 = add_vertex( feature_id, this->graph );
      this->descritpor_map.emplace( feature_id, v1 );

      if (link_key != nullptr and feature->has_property(*link_key)) {

          downstream_id = feature->get_property(*link_key).as_string();
          if( this->descritpor_map.find(downstream_id) != this->descritpor_map.end() )
          {
            v2 = this->descritpor_map[ downstream_id ];
          }
          else {
            v2 = add_vertex( downstream_id, this->graph );
            this->descritpor_map.emplace( downstream_id, v2 );
          }
            add_edge(v1, v2, this->graph);
      }
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

std::string Network::get_id( Graph::vertex_descriptor idx){
  return get(boost::vertex_name, this->graph)[idx];
}

std::size_t Network::size(){
  return num_vertices(this->graph);
}
