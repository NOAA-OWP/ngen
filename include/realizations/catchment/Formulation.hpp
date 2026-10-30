#ifndef FORMULATION_H
#define FORMULATION_H

#include <memory>
#include <string>
#include <map>
#include <exception>
#include <vector>

#include "JSONProperty.hpp"
#include "GenericDataProvider.hpp"
#include "utilities/output/CatchmentOutputsMgr.hpp"   // utils::OutputField

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace realization {

    /**
     * A model attached to a feature: what is run for that feature, and what it emits.
     *
     * A formulation is driven by a forcing provider, advances a time step at a time, and emits a
     * set of named output fields. It is identified both by its own `id` and by the id of the
     * feature it is attached to, which an @ref HY_CatchmentRealization uses to realize that feature.
     */
    class Formulation {
        public:
            typedef long time_step_t;

            Formulation(const std::string& id, std::shared_ptr<data_access::GenericDataProvider> forcing);
            Formulation(const std::string& id);

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

            /**
             * @name Identity
             *
             * Three roles, not three independent names. A formulation has a subject, a locator,
             * and an identity made of the two:
             *
             * - **subject** -- @ref get_feature_id, the feature whose data this model reads and
             *   produces. Not unique: every model attached to one catchment shares it. Use it to
             *   reach forcing and feature data, to name a catchment in a message, and to
             *   substitute `{{id}}` in a config path.
             * - **locator** -- @ref get_id, where this sits in that feature's tree of models. The
             *   root of that tree is the feature itself, which is why the two hold the same
             *   string for a formulation built directly against one. Unique, and what
             *   Formulation_Manager keys on.
             * - **identity** -- `Bmi_Formulation::compound_id()`, the locator together with what
             *   runs there. Unique, and what serialization records are keyed by, so a restore
             *   can tell it is loading into the same model the record came from.
             *
             * `model_type_name` is not a fourth identity; it is the component compound_id adds
             * to the locator.
             *
             * |                     | catchment formulation | domain layer        | module 0 of a multi on cat-27     |
             * | ------------------- | --------------------- | ------------------- | --------------------------------- |
             * | subject             | `cat-27`              | `layer-1`           | `cat-27`                          |
             * | locator             | `cat-27`              | `layer-1`           | `cat-27.0`                        |
             * | identity            | `cat-27:bmi_c_cfe`    | `layer-1:bmi_c_cfe` | `cat-27.0:bmi_c_cfe:bmi_multi_x`  |
             *
             * Subject and locator diverge only when one formulation composes another. The
             * composer builds each module's locator from its own locator and that module's index
             * within it, so a multi on `cat-27` gives its first module `cat-27.0`; and it hands
             * that module its own subject untouched, so the module still reads `cat-27`'s
             * forcing. Compose once more and the locator grows to `cat-27.0.1` while the subject
             * is still `cat-27`.
             *
             * A domain layer has no hydrofabric feature, but the layer is still what its model
             * belongs to, so it stands as the subject.
             */
            ///@{

            //! See @ref Identity.
            std::string get_id() const {
                return this->id;
            }

            virtual void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) = 0;
            virtual void create_formulation(geojson::PropertyMap properties) = 0;

            virtual void check_mass_balance(const int& iteration, const int& total_steps, const std::string& timestamp) const = 0;

            /**
             * @brief Invoke the state-save side of the ngen BMI Serialization Protocol.
             *
             * Called from the per-timestep driver (see `core/Layer.cpp`) alongside
             * `check_mass_balance`. Implementations build a protocol Context from
             * the arguments and the formulation's own compound identity, then
             * forward to the `NgenBmiProtocols` container. Non-BMI formulations
             * may implement as a no-op; unconfigured save protocols short-circuit
             * inside `run()` so the cost is just the virtual dispatch.
             */
            virtual void checkpoint_state(const int& iteration, const int& total_steps, const std::string& timestamp) const = 0;

            /**
             * Advance the backing model formulation through the given time step.
             *
             * @param t_index The index of the time step to run.
             * @param t_delta The duration, in seconds, of that time step.
             */
            virtual void update(time_step_t t_index, time_step_t t_delta) = 0;

            /**
             * Execute the backing model formulation for the given time step, where it is of the specified size, and
             * return the response output.
             *
             * Advances the model as @ref update does, and additionally reduces its state to one value.
             * Callers that only need the model stepped should use @ref update instead.  This function
             * should be implemented and used when a single-value representation of the model's state is required.
             *
             * Any inputs and additional parameters must be made available as instance members.
             *
             * The response is a depth, in meters. Implementations are responsible for converting
             * it: the BMI ones ask their model for "m" and convert from whatever it declares in
             * GetVarUnits. HY_CatchmentRealization takes that for granted when it records the
             * response as a depth, and HY_CatchmentArea multiplies it by an area in m^2.
             *
             * Types should clearly document anything further about their particular response.
             *
             * @param t_index The index of the time step for which to run model calculations.
             * @param t_delta The duration, in seconds, of the time step for which to run model calculations.
             * @return The response output of the model for this time step, as a depth in meters.
             */
            virtual double get_response(time_step_t t_index, time_step_t t_delta) = 0;

            /**
             * Get this formulation's output fields -- name plus metadata (units, ...) -- in the same
             * order as the values returned by @ref get_output_values_for_timestep.
             *
             * The name is the output header field (configured, or defaulting to the output variable
             * name); the units come from the same source that produces each value. So each field is
             * positionally aligned with, and describes, the corresponding value.
             *
             * @return The ordered output fields.
             */
            virtual std::vector<utils::OutputField> get_output_fields() const = 0;

            /**
             * Get the output values for the given time step, positionally aligned with @ref get_output_fields.
             *
             * Values are returned as raw doubles, with no formatting or precision applied by the formulation.
             *
             * @param timestep The time step for which values are desired.
             * @return The output values for @p timestep, one per output column.
             */
            virtual std::vector<double> get_output_values_for_timestep(int timestep) = 0;

            /**
             * Release resources of the given forcing provider
             */
            void finalize();

            //! The feature whose data this model reads and produces; see @ref Identity.
            const std::string& get_feature_id() const {
                return this->feature_id;
            }

            /**
             * Associate this formulation with a feature other than the one it is named for.
             *
             * For a formulation composing others: a nested module's id names its position within
             * the composition, so the composing formulation gives it the feature they share.
             */
            void set_feature_id(const std::string& feature_id) {
                this->feature_id = feature_id;
            }
            ///@} // Identity

        protected:

            virtual const std::vector<std::string>& get_required_parameters() const = 0;
            geojson::PropertyMap interpret_parameters(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr);
            void validate_parameters(geojson::PropertyMap options);

            //! This formulation's locator; see @ref get_id.
            std::string id;

            std::shared_ptr<data_access::GenericDataProvider> forcing;

        private:
            //! This formulation's subject; see @ref get_feature_id.
            std::string feature_id;
    };

}
#endif // FORMULATION_H
