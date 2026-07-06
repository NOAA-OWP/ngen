#ifndef NGEN_REALIZATION_CONFIG_OUTPUT_H
#define NGEN_REALIZATION_CONFIG_OUTPUT_H

#include <string>
#include <stdexcept>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

namespace realization {
  namespace config {

    //! Key for the dedicated output configuration block in a realization config.
    static const std::string OUTPUT_CONFIG_KEY = "output";

    //! Deprecated top-level keys this block replaces.
    static const std::string LEGACY_OUTPUT_ROOT_KEY        = "output_root";
    static const std::string LEGACY_DISABLE_CATCHMENT_KEY  = "disable_catchment_output";
    static const std::string LEGACY_PER_FORMULATION_KEY    = "per_formulation_nexus_files";

    //! Serialization format for an output domain.
    enum class OutputFormat { csv, netcdf };

    //! How a domain's records are distributed across files (the file-*granularity* axis):
    //!   per_feature     - one file per feature (one CSV per catchment / per nexus)
    //!   per_formulation - the features of a formulation aggregate into one file (a feature-id
    //!                     column distinguishes their rows). Fewer, larger files; columns are
    //!                     uniform within a file by construction.
    //! Orthogonal to this is @ref OutputDomain::rank_subdir, the filesystem-*layout* axis.
    enum class OutputGrouping { per_feature, per_formulation };

    inline OutputFormat parse_output_format(const std::string& s)
    {
        if (s == "csv")    return OutputFormat::csv;
        if (s == "netcdf") return OutputFormat::netcdf;
        throw std::runtime_error("Invalid output format '" + s + "'; expected 'csv' or 'netcdf'.");
    }

    inline OutputGrouping parse_output_grouping(const std::string& s)
    {
        if (s == "per_feature")     return OutputGrouping::per_feature;
        if (s == "per_formulation") return OutputGrouping::per_formulation;
        throw std::runtime_error("Invalid output grouping '" + s + "'; expected 'per_feature' or 'per_formulation'.");
    }

    //! Settings for one output domain (catchment or nexus).
    struct OutputDomain {
        bool enable = true;
        OutputFormat format = OutputFormat::csv;
        OutputGrouping grouping = OutputGrouping::per_feature;
        //! Place each MPI rank's files under a "rank_<N>/" subdirectory (the filesystem-layout axis,
        //! orthogonal to @ref grouping). Only affects distributed (multi-rank) runs; a no-op in
        //! serial. It is a free preference for per_feature; for per_formulation under MPI it is
        //! required (else ranks would collide on one aggregated file) and applied regardless.
        //! @see rank_output_root
        bool rank_subdir = false;

        OutputDomain() = default;

        explicit OutputDomain(const boost::property_tree::ptree& tree) {
            enable = tree.get<bool>("enable", true);
            rank_subdir = tree.get<bool>("rank_subdir", false);
            if (auto f = tree.get_optional<std::string>("format"))   format   = parse_output_format(*f);
            if (auto g = tree.get_optional<std::string>("grouping")) grouping = parse_output_grouping(*g);
        }
    };

    /**
     * The output root for a rank's files in this domain: appends "rank_<N>/" to @p base_root when the
     * domain requests per-rank subdirectories, or when it aggregates per formulation (which requires
     * per-rank separation so ranks don't write the same file). Only distributed (@p mpi_num_procs > 1)
     * runs are affected; in serial @p base_root is returned unchanged. The directory is created
     * lazily by the backend when it opens files.
     */
    inline std::string rank_output_root(const std::string& base_root, const OutputDomain& domain,
                                        int mpi_rank, int mpi_num_procs)
    {
        const bool needs_rank_subdir = domain.rank_subdir
                                       || domain.grouping == OutputGrouping::per_formulation;
        if (mpi_num_procs > 1 && needs_rank_subdir) {
            return base_root + "rank_" + std::to_string(mpi_rank) + "/";
        }
        return base_root;
    }

    /**
     * Parsed representation of a realization config's output settings.
     *
     * Modern form (preferred):
     * @code{.json}
     * "output": {
     *     "root": "/path/to/output/",
     *     "catchment": { "enable": true, "format": "csv", "grouping": "per_feature" },
     *     "nexus":     { "enable": true, "format": "csv", "grouping": "per_feature" }
     * }
     * @endcode
     *
     * For backward compatibility, when no "output" block is present the
     * deprecated top-level keys output_root / disable_catchment_output /
     * per_formulation_nexus_files are honored instead (see from_realization).
     */
    struct Output {
        //! Significant digits applied uniformly when a text backend (CSV) renders values.
        static constexpr int DEFAULT_PRECISION = 9;

        //! Output directory. Built through from_realization this is the normalized root: a
        //! trailing-slash path ("./" when unset). The default constructor and the public
        //! parse_block / parse_legacy leave it as the raw configured value. Creating the directory
        //! on disk is a separate step -- see create_output_directory -- performed by the caller near
        //! where output is written, so config parsing has no filesystem side effect.
        std::string root;
        OutputDomain catchment;
        OutputDomain nexus;
        //! Global value precision for text output backends.
        int precision = DEFAULT_PRECISION;
        //! True when built from the deprecated top-level keys (used to warn).
        bool from_legacy_keys = false;

        Output() = default;

        //! Parse the modern "output" block sub-tree.
        static Output parse_block(const boost::property_tree::ptree& output_tree)
        {
            Output out;
            out.root = output_tree.get<std::string>("root", "");
            out.precision = output_tree.get<int>("precision", DEFAULT_PRECISION);
            if (auto c = output_tree.get_child_optional("catchment")) out.catchment = OutputDomain(*c);
            if (auto n = output_tree.get_child_optional("nexus"))     out.nexus     = OutputDomain(*n);
            return out;
        }

        //! Build from the deprecated top-level keys.
        static Output parse_legacy(const boost::property_tree::ptree& realization_tree)
        {
            Output out;
            const auto root        = realization_tree.get_optional<std::string>(LEGACY_OUTPUT_ROOT_KEY);
            const auto disable_cat = realization_tree.get_optional<bool>(LEGACY_DISABLE_CATCHMENT_KEY);
            const auto per_form    = realization_tree.get_child_optional(LEGACY_PER_FORMULATION_KEY);

            if (root)        out.root = *root;
            if (disable_cat) out.catchment.enable = !(*disable_cat);
            if (per_form && per_form->get_value<bool>(false)) out.nexus.format = OutputFormat::netcdf;

            out.from_legacy_keys = static_cast<bool>(root) || static_cast<bool>(disable_cat) || static_cast<bool>(per_form);
            return out;
        }

        //! Prefer the modern "output" block; otherwise fall back to legacy keys, then normalize the
        //! root (see normalize_root) so an Output built through this entry point exposes a
        //! trailing-slash directory. parse_block / parse_legacy return the raw root. Creating the
        //! directory is left to the caller (see create_output_directory) -- parsing has no side effect.
        static Output from_realization(const boost::property_tree::ptree& realization_tree)
        {
            auto block = realization_tree.get_child_optional(OUTPUT_CONFIG_KEY);
            Output out = block ? parse_block(*block) : parse_legacy(realization_tree);
            out.root = normalize_root(out.root);
            return out;
        }

    private:
        //! Normalize a configured directory to a trailing-slash path, falling back to "./" when
        //! unset. Pure: creating the directory is a separate side effect (see create_output_directory).
        static std::string normalize_root(const std::string& configured_root)
        {
            if (configured_root.empty()) {
                return "./";
            }
            return configured_root.back() == '/' ? configured_root : configured_root + "/";
        }
    };

  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_OUTPUT_H
