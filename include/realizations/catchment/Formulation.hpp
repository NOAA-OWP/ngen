#ifndef FORMULATION_H
#define FORMULATION_H
#include "Logger.hpp"

#include <memory>
#include <string>
#include <map>
#include <exception>
#include <vector>

#include "JSONProperty.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace realization {

    class Formulation {
        public:
            typedef long time_step_t;

            Formulation(std::string id) : id(id) {}
            
            virtual ~Formulation(){};

            virtual std::string get_formulation_type() const = 0;

            // TODO: to truly make this properly generalized (beyond catchments, and to some degree even in that
            //  context) a more complex type for the entirety of the response/output is needed, perhaps with independent
            //  access functions

            // TODO: a reference to the actual time of the initial time step is needed

            // TODO: a reference to the last calculated time step is needed

            // TODO: a mapping of previously calculated time steps to the size/delta of each is needed (unless we
            //  introduce a way to enforce immutable time step delta values for an object)

            // TODO: a convenience method for getting the actual time of calculated time steps (at least the last)

            std::string get_id() const {
                return this->id;
            }

            virtual void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) = 0;
            virtual void create_formulation(geojson::PropertyMap properties) = 0;

        protected:

            virtual const std::vector<std::string>& get_required_parameters() const = 0;
            geojson::PropertyMap interpret_parameters(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr);
            void validate_parameters(geojson::PropertyMap options);

            std::string id;
    };

}
#endif // FORMULATION_H
