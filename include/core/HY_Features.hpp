#ifndef HY_FEATURES_H
#define HY_FEATURES_H

#include <unordered_map>
#include <set>

#include <HY_Catchment.hpp>
#include <HY_HydroNexus.hpp>
#include <network.hpp>
#include <Formulation_Manager.hpp>

namespace hy_features {

    /**
     * @brief Collection interface to HY_Features objects and their associated model formulations.
     * 
     * A HY_Features collection provides organized access to HY_Features objects.  It is backed by a topologically
     * connected network::Network index and provides quick id based iteration of various feature types,
     * as well as helper functions for accessing pointers to the constructed features.
     * 
     * Simple usage example to execute model formulations for each catchment for a single time:
     * 
     * @code {.cpp}
     * geojson::GeoJSON catchment_collection = geojson::read("catchment_data.geojson", std::vector<std::string>());
     * std::shared_ptr<realization::Formulation_Manager> manager = std::make_shared<realization::Formulation_Manager>("realization_config.json");
     * manager->read(catchment_collection, utils::getStdOut());
     * 
     * hy_features::HY_Features features = hy_features::HY_Features(catchment_collection, manager);
     * int time_index = 0;
     * for(const auto& id : features.catchments()) {
     *     auto r = features.catchment_at(id);
     *     auto r_c = dynamic_pointer_cast<realization::Catchment_Formulation>(r);
     *     double response = r_c->get_response(time_index, 3600.0);
     *     std::string output = std::to_string(time_index)+", examlpe_time_stamp,"+
     *                          r_c->get_output_line_for_timestep(time_index)+"\n";
     *     r_c->write_output(output);
     * }
     * @endcode
     */
    class HY_Features {
      using Formulation_Manager = realization::Formulation_Manager;
      public:
        /**
         * @brief Construct a new, default HY_Features object
         * 
         */
        HY_Features() {}

        /**
         * @brief Construct a new HY_Features object from a GeoJSON feature collection and a set of formulations.
         * 
         * Constructs the network::Network index and then
         * \copydetails HY_Features::HY_Features(network::Network,std::shared_ptr<Formulation_Manager>)
         * 
         * @param fabric 
         * @param formulations 
         */
        HY_Features( geojson::GeoJSON fabric, std::shared_ptr<Formulation_Manager> formulations );

        /**
         * @brief Construct a new HY_Features object from a Network and a set of formulations.
         * 
         * Constructs the HY_Catchment objects for each catchment connecting them with the provided link_key attaches the formulation
         * associated with the catchment found in the Formulation_Manager.  Also constucts each nexus as a HY_PointHydroNexus.
         * 
         * @param catchments 
         * @param link_key
         * @param formulations 
         */

        HY_Features( geojson::GeoJSON catchments, std::string* link_key, std::shared_ptr<Formulation_Manager> formulations);

        /**
         * @brief Get the HY_CatchmentRealization pointer identified by @p id
         * 
         * If no realization exists for @p id, a nullptr is returned.
         * 
         * @param id 
         * @return std::shared_ptr<HY_CatchmentRealization> 
         */
        std::shared_ptr<HY_CatchmentRealization> catchment_at(std::string id)
        {
          if( _catchments.find(id) != _catchments.end() )
            return _catchments[id]->realization;
          return nullptr;
        }

        /**
         * @brief Get the HY_HydroNexus pointer identifed by @p id
         * 
         * If no nexus exists for @p id, a nullptr is returned.
         * 
         * @param id 
         * @return std::shared_ptr<HY_HydroNexus> 
         */
        std::shared_ptr<HY_HydroNexus> nexus_at(const std::string& id)
        {
          if( _nexuses.find(id) != _nexuses.end() )
            return _nexuses[id];
          return nullptr;
        }

        /**
         * @brief An iterator of only the catchment feature ids
         * 
         * @return auto 
         */
        inline auto catchments(){return network.filter("cat");}

        /**
         * @brief An iterator of only the catchment feature ids from only the specified layer
         * 
         * @return auto 
         */
        inline auto catchments(long lv) {
            return network.filter("cat",lv);
        }

        /**
         * @brief Return a set of levels that contain a catchment
         */

        inline const auto& levels() { return hf_levels; }

        /**
         * @brief An iterator of only the nexus feature ids
         * 
         * @return auto 
         */
        inline auto nexuses(){return network.filter("nex");}

        /**
         * @brief Get a vector of destination (downstream) nexus pointers.
         * 
         * If @p id is not a known catchment identifier, then an empty vector is returned.
         * 
         * @param id 
         * @return std::vector<std::shared_ptr<HY_HydroNexus>> 
         */
        inline std::vector<std::shared_ptr<HY_HydroNexus>> destination_nexuses(const std::string&  id)
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

        /**
         * @brief Validates that the feature topology is dendritic.
         * 
         */
        void validate_dendritic()
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
          std::cout<<"Catchment topology is dendritic."<<std::endl;
        }

        /**
         * @brief Destroy the hy features object
         * 
         */
        virtual ~HY_Features(){}

      private:

        void init();

        /**
         * @brief Internal mapping of catchment id -> HY_Catchment pointer.
         * 
         */
        std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> _catchments;

        /**
         * @brief Internal mapping of nexus id -> HY_HydroNexus pointer.
         * 
         */
        std::unordered_map<std::string, std::shared_ptr<HY_HydroNexus>> _nexuses;

        /**
         * @brief network::Network graph of identities.
         * 
         */
        network::Network network;

        /**
         * @brief Pointer to the formulation manager used to associate catchment formulations with HY_Catchment objects.
         * 
         */
        std::shared_ptr<Formulation_Manager> formulations;

        /**
         *  @brief The set of levels that contain at least one catchment
        */
        std::set<long> hf_levels;

        geojson::GeoJSON fabric;

    };
}

#endif //HY_FEATURES_H
