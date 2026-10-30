#ifndef NGEN_FORMULATION_MANAGER_H
#define NGEN_FORMULATION_MANAGER_H

#include <NGenConfig.h>

#include <map>
#include <memory>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <FeatureBuilder.hpp>
#include <AorcForcing.hpp>
#include "features/Features.hpp"
#include "Formulation_Constructors.hpp"
#include "LayerData.hpp"
#include "realizations/config/time.hpp"
#include "realizations/config/routing.hpp"
#include "realizations/config/config.hpp"
#include "realizations/config/layer.hpp"
#include "realizations/config/output.hpp"
#include "realizations/config/global_config.hpp"
#include <AorcForcing.hpp>

namespace realization {

    class Formulation_Manager {
        public:
            Formulation_Manager(boost::property_tree::ptree loaded_tree);

            ~Formulation_Manager() = default;

            void read(simulation_time_params &simulation_time_config,
                      geojson::GeoJSON fabric);

            void add_formulation(std::shared_ptr<Formulation> formulation);

            std::shared_ptr<Formulation> get_formulation(std::string id) const;

            std::shared_ptr<Formulation> get_domain_formulation(long id) const;

            bool has_domain_formulation(int id) const;

            bool contains(std::string identifier) const;

            /**
             * @return The number of elements within the collection
             */
            int get_size();

            /**
             * @return Whether or not the collection is empty
             */
            bool is_empty();

            //! The parsed output configuration block.
            const realization::config::Output& get_output_config() const;

            std::map<std::string, std::shared_ptr<Formulation>>::const_iterator begin() const;

            std::map<std::string, std::shared_ptr<Formulation>>::const_iterator end() const;

            /**
             * @return Whether or not using routing
             */
            bool get_using_routing();

            /**
             * @return routing t_route_config_file_with_path
             */
            std::string get_t_route_config_file_with_path();

            /**
             * Release any resources that should not be held as the run is shutting down
             *
             * In particular, this should be called before MPI_Finalize()
             */
            void finalize();

            /**
             * Parse the output configuration from the realization tree (preferring
             * the "output" block, falling back to the deprecated top-level keys),
             * warn on deprecated usage, and validate build support. Called once at
             * construction so output accessors are valid before read().
             */
            void initialize_output_config();

            /**
             * @brief return the layer storage used for formulations
             * @return a reference to the LayerStorageObject
             */
            ngen::LayerDataStorage& get_layer_metadata();


        protected:
            std::shared_ptr<Formulation> construct_formulation_from_config(
                simulation_time_params &simulation_time_config,
                std::string identifier,
                realization::config::Config& catchment_formulation
            );

            std::shared_ptr<Formulation> construct_missing_formulation(geojson::Feature& feature, simulation_time_params &simulation_time_config);

            forcing_params get_forcing_params(const geojson::PropertyMap &forcing_prop_map, std::string identifier, simulation_time_params &simulation_time_config);

            /**
             * @brief Parse a `model_params` property tree and replace external parameters
             *        with values from a catchment's properties
             *
             * @param model_params Property tree with root key "model_params"
             * @param catchment_feature Associated catchment feature
             */
            void parse_external_model_params(boost::property_tree::ptree& model_params, const geojson::Feature catchment_feature);

            /**
             * @brief Parse a `model_params` property map and replace external parameters
             *        with values from a catchment's properties
             *
             * @param model_params Property map with root key "model_params"
             * @param catchment_feature Associated catchment feature
             */
            void parse_external_model_params(geojson::PropertyMap& model_params, const geojson::Feature catchment_feature);

            boost::property_tree::ptree tree;

            realization::config::Config global_config;

            /** Parsed realization-level config blocks that should be
             *  inherited into every formulation's params unless the
             *  formulation declares its own. Keyed by the JSON key
             *  string (see realization::config::GlobalConfigKey). */
            geojson::PropertyMap global_configs;

            std::map<std::string, std::shared_ptr<Formulation>> formulations;

            //Store global layer formulation pointers
            std::map<int, std::shared_ptr<Formulation> > domain_formulations;

            std::shared_ptr<routing_params> routing_config;

            bool using_routing = false;

            realization::config::Output output_config;

            ngen::LayerDataStorage layer_storage;

    };
}
#endif // NGEN_FORMULATION_MANAGER_H
