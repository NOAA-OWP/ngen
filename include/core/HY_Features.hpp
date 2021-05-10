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

        virtual ~HY_Features(){}

      protected:

      private:
        std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> catchments;
        network::Network network;


    };
}

#endif //HY_GRAPH_H
