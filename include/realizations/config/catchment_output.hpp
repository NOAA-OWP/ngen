#ifndef NGEN_REALIZATION_CONFIG_CATCHMENT_OUTPUT_HPP
#define NGEN_REALIZATION_CONFIG_CATCHMENT_OUTPUT_HPP

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "realizations/config/output.hpp"
#include "utilities/output/CatchmentOutputsMgr.hpp"
#include "utilities/output/CatchmentCsvOutputMgr.hpp"

namespace realization
{
    namespace config
    {
        //! Default file name for an aggregated (per_formulation) catchment file. The rank, when
        //! applicable, is carried in the resolved root (a rank_<N>/ subdirectory), not the file name.
        static const std::string DEFAULT_AGGREGATED_FILENAME = "cat_output.csv";

        /**
         * Build the catchment output manager from the parsed output configuration. Grouping and
         * precision come from @p output_config; the resolved root comes from @c output_config.root,
         * which the caller is expected to have already adjusted for rank layout (see
         * @ref rank_output_root) so any rank_<N>/ subdirectory is baked into the root.
         *
         * The manager is constructed with the full set of @p descriptors (every catchment and its
         * columns), so the backend lays out its files up front and is ready to receive data on return.
         *
         * Side effect: constructing the manager creates the output directory tree it writes into
         * (config parsing only normalizes the root -- it does not touch the filesystem). Each output
         * manager owns creating what it writes, so no caller need pre-create the directory.
         *
         * Only the CSV format is implemented today; any other configured format is rejected here (the
         * branch on @c output_config.catchment.format is where an added format would slot in). Layout
         * follows the grouping: @c per_formulation aggregates each formulation's catchments into one
         * file (leading catchment_id column) named @p aggregated_filename; @c per_feature writes one
         * file per catchment.
         *
         * @throw std::runtime_error if a catchment output format other than CSV is configured.
         */
        inline std::shared_ptr<utils::CatchmentOutputsMgr> make_catchment_output_mgr(
                const Output& output_config,
                std::vector<utils::FeatureDescriptor> descriptors,
                const std::string& aggregated_filename = DEFAULT_AGGREGATED_FILENAME)
        {
            if (output_config.catchment.format == OutputFormat::csv) {
                const bool aggregate = output_config.catchment.grouping == OutputGrouping::per_formulation;
                return std::make_shared<utils::CatchmentCsvOutputMgr>(
                    output_config.root,
                    aggregate ? std::optional<std::string>(aggregated_filename) : std::nullopt,
                    output_config.precision, std::move(descriptors));
            } else {
                throw std::runtime_error("Catchment output: only the CSV format is currently supported.");
            }
        }
    } // namespace config
} // namespace realization

#endif // NGEN_REALIZATION_CONFIG_CATCHMENT_OUTPUT_HPP
