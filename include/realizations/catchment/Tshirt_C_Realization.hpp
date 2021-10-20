#ifndef NGEN_TSHIRT_C_REALIZATION_HPP
#define NGEN_TSHIRT_C_REALIZATION_HPP

#include "Catchment_Formulation.hpp"
#include "core/catchment/HY_CatchmentArea.hpp"
#include "tshirt_params.h"
#include "forcing/Forcing.h"
#include "GiuhJsonReader.h"
#include "tshirt_c.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define OUT_VAR_BASE_FLOW "base_flow"
#define OUT_VAR_GIUH_RUNOFF "giuh_runoff"
#define OUT_VAR_LATERAL_FLOW "lateral_flow"
#define OUT_VAR_RAINFALL "rainfall"
#define OUT_VAR_SURFACE_RUNOFF "surface_runoff"
#define OUT_VAR_TOTAL_DISCHARGE "total_discharge"

using namespace tshirt;

namespace realization {

    class Tshirt_C_Realization : public Catchment_Formulation {

    public:

        typedef long time_step_t;

        /**
         * Constructor for initializing when a #tshirt::tshirt_params struct is passed for parameters and a
         * #giuh::GiuhJsonReader is passed for obtaining GIUH ordinates.
         *
         * @param forcing_config
         * @param output_stream
         * @param soil_storage
         * @param groundwater_storage
         * @param storage_values_are_ratios Whether the storage values are given as proportional amounts of the max (or,
         *                                  when `false`, express amounts with units of meters).
         * @param catchment_id
         * @param giuh_json_reader
         * @param params
         * @param nash_storage
         */
        Tshirt_C_Realization(forcing_params forcing_config,
                             utils::StreamHandler output_stream,
                             double soil_storage,
                             double groundwater_storage,
                             bool storage_values_are_ratios,
                             std::string catchment_id,
                             giuh::GiuhJsonReader &giuh_json_reader,
                             tshirt::tshirt_params params,
                             const std::vector<double> &nash_storage);

        /**
         * Constructor for initializing when a #tshirt::tshirt_params struct is passed for parameters and GIUH ordinates
         * are available directly and passed within a vector.
         *
         * @param forcing_config
         * @param output_stream
         * @param soil_storage
         * @param groundwater_storage
         * @param storage_values_are_ratios Whether the storage values are given as proportional amounts of the max (or,
         *                                  when `false`, express amounts with units of meters).
         * @param catchment_id
         * @param guih_ordinates
         * @param params
         * @param nash_storage
         */
        Tshirt_C_Realization(forcing_params forcing_config,
                             utils::StreamHandler output_stream,
                             double soil_storage,
                             double groundwater_storage,
                             bool storage_values_are_ratios,
                             std::string catchment_id,
                             std::vector<double> giuh_ordinates,
                             tshirt::tshirt_params params,
                             const std::vector<double> &nash_storage);

        /**
         * Constructor for when model parameters are provided individually instead of within encapsulating struct and a
         * #giuh::GiuhJsonReader is passed for obtaining GIUH ordinates.
         *
         * @param forcing_config
         * @param output_stream
         * @param soil_storage
         * @param groundwater_storage
         * @param storage_values_are_ratios Whether the storage values are given as proportional amounts of the max (or,
         *                                  when `false`, express amounts with units of meters).
         * @param catchment_id
         * @param giuh_json_reader
         * @param maxsmc
         * @param wltsmc
         * @param satdk
         * @param satpsi
         * @param slope
         * @param b
         * @param multiplier
         * @param alpha_fc
         * @param Klf
         * @param Kn
         * @param nash_n
         * @param Cgw
         * @param expon
         * @param max_gw_storage
         * @param nash_storage
         */
        Tshirt_C_Realization(
                forcing_params forcing_config,
                utils::StreamHandler output_stream,
                double soil_storage,
                double groundwater_storage,
                bool storage_values_are_ratios,
                std::string catchment_id,
                giuh::GiuhJsonReader &giuh_json_reader,
                double maxsmc,
                double wltsmc,
                double satdk,
                double satpsi,
                double slope,
                double b,
                double multiplier,
                double alpha_fc,
                double Klf,
                double Kn,
                int nash_n,
                double Cgw,
                double expon,
                double max_gw_storage,
                const std::vector<double> &nash_storage);

        Tshirt_C_Realization(std::string id, unique_ptr<forcing::ForcingProvider> forcing_provider, utils::StreamHandler output_stream);

        //[[deprecated]]
        Tshirt_C_Realization(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        virtual ~Tshirt_C_Realization();

        /**
         * Return ``0``, as (for now) this type does not otherwise include ET within its calculations.
         *
         * @return ``0``
         */
        double calc_et() override;

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;
        void create_formulation(geojson::PropertyMap properties) override;

        /**
         * Run model formulation calculations for the next time step using the given input flux value in meters per
         * second.
         *
         * The backing model works on a collection of time steps by receiving an associated collection of input fluxes.
         * This is implemented by putting this input flux in a single-value vector and using it as the
         * arg to a nested call to ``run_formulation_for_timesteps``, returning that result code.
         *
         * @param input_flux Input flux (typically expected to be just precipitation) in meters per second.
         * @param t_delta_s The size of the time step in seconds
         * @return The result code from the execution of the model time step calculations.
         */
        int run_formulation_for_timestep(double input_flux, time_step_t t_delta_s);

        /**
         * Run model formulation calculations for a series of time steps using the given collection of input flux values
         * in meters per second.
         *
         * @param input_fluxes Ordered, per-time-step input flux (typically expected to be just precipitation) in meters
         * per second.
         * @param t_delta_s The sizes of each of the time steps in seconds
         * @return The result code from the execution of the model time step calculations.
         */
        int run_formulation_for_timesteps(std::vector<double> input_fluxes, std::vector<time_step_t> t_deltas_s);

        std::string get_formulation_type() override;

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

        // TODO: probably need to do better than this for granting and protecting access
        double get_latest_flux_base_flow();

        double get_latest_flux_giuh_runoff();

        double get_latest_flux_lateral_flow();

        double get_latest_flux_surface_runoff();

        double get_latest_flux_total_discharge();

        // TODO: look at moving these to the Formulation superclass
        /**
         * Get the number of output data variables made available from the calculations for enumerated time steps.
         *
         * @return The number of output data variables made available from the calculations for enumerated time steps.
         */
        int get_output_item_count();

        /**
         * Get the names of the output data variables that are available from calculations for enumerated time steps.
         *
         * The method must deterministically return output variable names in the same order each time.
         *
         * @return The names of the output data variables that are available from calculations for enumerated time steps.
         */
        const std::vector<std::string>& get_output_var_names();

        /**
         * Get a copy of the values for the given output variable at all available time steps.
         *
         * @param name
         * @return A vector containing copies of the output value of the variable, indexed by time step.
         */
        // TODO: note that for this type, these are all doubles, but may need template function once moving to superclass
        std::vector<double> get_value(const std::string& name);

        /**
         * Get a header line appropriate for a file made up of entries from this type's implementation of
         * ``get_output_line_for_timestep``.
         *
         * Note that like the output generating function, this line does not include anything for time step.
         *
         * @return An appropriate header line for this type.
         */
        std::string get_output_header_line(std::string delimiter=",") override;

        /**
         * Get the values making up the header line from get_output_header_line(), but organized as a vector of strings.
         *
         * @return The values making up the header line from get_output_header_line() organized as a vector.
         */
        const std::vector<std::string>& get_output_header_fields();

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
        std::string get_output_line_for_timestep(int timestep, std::string delimiter=",") override;

    private:
        /** Id of associated catchment. */
        std::string catchment_id;
        /**
         * Struct for containing model parameters for Tshirt model, using "internal" framework implementation for such a
         * struct.
         */
        std::shared_ptr<tshirt::tshirt_params> params;
        /** Struct from C-style Tshirt formulation for holding soil parameter values. */
        NWM_soil_parameters c_soil_params;

        /** Vector of GIUH CDF ordinates. */
        std::vector<double> giuh_cdf_ordinates;
        /** Vector to serve as runoff queue for GIUH convolution calculations. */
        std::vector<double> giuh_runoff_queue_per_timestep;
        std::vector<double> nash_storage;

        aorc_forcing_data c_aorc_params;

        // TODO: might want to consider having an initial time step value for reference (implied size is 1 hour)
        // TODO: this probably need to be converted to use a different fluxes type that can be dealt with externally
        std::vector<std::shared_ptr<tshirt_c_result_fluxes>> fluxes;

        conceptual_reservoir groundwater_conceptual_reservoir;

        std::vector<std::string> OUTPUT_VARIABLE_NAMES = {
                OUT_VAR_RAINFALL,
                OUT_VAR_SURFACE_RUNOFF,
                OUT_VAR_GIUH_RUNOFF,
                OUT_VAR_LATERAL_FLOW,
                OUT_VAR_BASE_FLOW,
                OUT_VAR_TOTAL_DISCHARGE
        };

        std::vector<std::string> OUTPUT_HEADER_FIELDS = {
                "Rainfall",
                "Direct Runoff",
                "GIUH Runoff",
                "Lateral Flow",
                "Base Flow",
                "Total Discharge"
        };

        conceptual_reservoir soil_conceptual_reservoir;
        std::vector<std::string> REQUIRED_PARAMETERS = {
                "maxsmc",
                "wltsmc",
                "satdk",
                "satpsi",
                "slope",
                "scaled_distribution_fn_shape_parameter",
                "multiplier",
                "alpha_fc",
                "Klf",
                "Kn",
                "nash_n",
                "Cgw",
                "expon",
                "max_groundwater_storage_meters",
                "nash_storage",
                "soil_storage_percentage",
                "groundwater_storage_percentage",
                "giuh"
        };

        function<double(tshirt_c_result_fluxes)> get_output_var_flux_extraction_func(const std::string& var_name);

        const std::vector<std::string>& get_required_parameters() override;

        void init_ground_water_reservoir(double storage, bool storage_values_are_ratios);

        void init_soil_reservoir(double storage, bool storage_values_are_ratios);

        static double init_reservoir_storage(bool is_ratio, double amount, double max_amount);

        void sync_c_storage_params();

    };
}

#endif
