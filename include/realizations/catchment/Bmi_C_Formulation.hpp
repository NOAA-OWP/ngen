#ifndef NGEN_BMI_C_FORMULATION_H
#define NGEN_BMI_C_FORMULATION_H

#include "Bmi_Formulation.hpp"
#include "Bmi_C_Adapter.hpp"

namespace realization {

    class Bmi_C_Formulation : public Bmi_Formulation<models::bmi::Bmi_C_Adapter> {

    public:

        Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;

        void create_formulation(geojson::PropertyMap properties) override;

        std::string get_formulation_type() override;

        /**
         * Get a header line appropriate for a file made up of entries from this type's implementation of
         * ``get_output_line_for_timestep``.
         *
         * Note that like the output generating function, this line does not include anything for time step.
         *
         * @return An appropriate header line for this type.
         */
        std::string get_output_header_line(std::string delimiter) override;

        /**
         * Get a delimited string with all the output variable values for the given time step.
         *
         * This method is useful for preparing calculated data in a representation useful for output files, such as
         * CSV files.
         *
         * The resulting string contains only the calculated output values for the time step, and not the time step
         * index itself.
         *
         * An empty string is returned if the time step value is not in the range of valid time steps for which there
         * are calculated values for all variables.
         *
         * The default delimiter is a comma.
         *
         * @param timestep The time step for which data is desired.
         * @return A delimited string with all the output variable values for the given time step.
         */
        std::string get_output_line_for_timestep(int timestep, std::string delimiter) override;

        /**
         * Execute the backing model formulation for the given time step, where it is of the specified size, and
         * return the total discharge.
         *
         * Any inputs and additional parameters must be made available as instance members.
         *
         * Types should clearly document the details of their particular response output.
         *
         * @param t_index The index of the time step for which to run model calculations.
         * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
         * @return The total discharge of the model for this time step.
         */
        double get_response(time_step_t t_index, time_step_t t_delta) override;

        // TODO: need some way of getting the right externally sourced model accessible here

        // TODO: need way of controlling which model it is (and maybe to control what input/output variables are supported)

        // TODO: need to initialize model

        // TODO: need to pass any required subsequent variable values

        // TODO: advance model with update (account for possibility of not supporting BMI `update`, but supporting `update_until`)

        // TODO: get data out of model

        // TODO: finalize if necessary

        // TODO: consider functions for handling every call in the spec


    };

}

#endif //NGEN_BMI_C_FORMULATION_H
