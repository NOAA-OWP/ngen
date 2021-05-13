#ifndef HY_FEATURES_H
#define HY_FEATURES_H

#include <unordered_map>

#include <HY_Catchment.hpp>
#include <network.hpp>

namespace hy_features {

    class HY_Features {
      public:
        HY_Features() {}
        HY_Features( geojson::GeoJSON fabric );
        HY_Features( network::Network network);
        HY_Features( geojson::GeoJSON catchments, geojson::GeoJSON nexuses, std::string* link_key);
        std::shared_ptr<HY_CatchmentRealization> catchment_at(std::string id)
        {
          if( _catchments.find(id) != _catchments.end() )
            return _catchments[id]->realization;
          return nullptr;
        }
        std::shared_ptr<HY_HydroNexus> nexus_at(std::string id)
        {
          if( _nexuses.find(id) != _nexuses.end() )
            return _nexuses[id];
          return nullptr;
        }
        inline auto catchments(){return network.filter("cat");}
        inline auto nexuses(){return network.filter("nex");}
        inline std::vector<std::shared_ptr<HY_HydroNexus>> destination_nexuses(std::string  id)
        {
          std::vector<std::shared_ptr<HY_HydroNexus>> downstream;
          if( _catchments.find(id) != _catchments.end())
          {
            for(const auto& nex_id : _catchments[id]->get_outflow_nexuses())
            {
              downstream.push_back(_nexuses[nex_id]);
            }
          }
          return downstream;
        }
        void validate_dendridic()
        {
          for(const auto& id : catchments())
          {
              auto downstream = network.get_destination_ids(id);
              if(downstream.size() > 1)
              {
                std::cerr << "Catchment " << id << " has more than one downstream connection." << std::endl;
                std::cerr << "Downstreams are: ";
                for(const auto& id : downstream){
                  std::cerr <<id<<" ";
                }
                std::cerr << std::endl;
                assert( false );
              }
              else if (downstream.size() == 0)
              {
                std::cerr << "Catchment " << id << " has 0 downstream connections, must have 1." << std::endl;
                assert( false );
              }
          }
          std::cout<<"Catchment topology is dendridic."<<std::endl;
        }

        virtual ~HY_Features(){}

      protected:

      private:
        std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> catchments;
        network::Network network;


    };
}

#endif //HY_GRAPH_H
