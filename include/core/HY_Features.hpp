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

        virtual ~HY_Features(){}

      protected:

      private:
        std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> catchments;
        network::Network network;


    };
}

#endif //HY_GRAPH_H
