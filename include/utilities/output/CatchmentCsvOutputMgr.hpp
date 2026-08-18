/*
Copyright (C) 2026 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------
*/

#ifndef NGEN_CATCHMENTCSVOUTPUTMGR_HPP
#define NGEN_CATCHMENTCSVOUTPUTMGR_HPP

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include <vector>

#include "CatchmentOutputsMgr.hpp"

namespace utils
{
    /**
     * CSV backend for catchment output. A single map of open output streams, keyed by resolved file
     * path, backs both groupings:
     *   - per-feature: one file per (formulation, catchment).
     *   - aggregated: the catchments of a formulation share one file (the aggregated filename), with
     *     a leading catchment_id column; the header is written once per file.
     * In both cases the default formulation id keeps the flat "<output_root><name>" layout, and any
     * other formulation id is placed in a "<output_root><formulation_id>/" subdirectory -- so
     * independent formulation setups never collide, and each aggregated file holds a single
     * formulation's (uniform) column set.
     *
     * Rows are written immediately on receipt (@ref commit_writes only flushes). Values are rendered
     * to text at a uniform significant-digit precision fixed at construction.
     */
    class CatchmentCsvOutputMgr : public CatchmentOutputsMgr
    {

    public:

        /**
         * @param output_root The directory to write into: a resolved path with a trailing slash (as
         *        produced by realization::config normalization, e.g. "./" or "out/rank_3/"), not a
         *        filename prefix. Created here (along with any per-formulation subdirectories) if it
         *        does not exist, so callers need not pre-create it.
         * @param aggregated_filename File name used for each formulation's shared file when @p
         *        aggregate is true (e.g. "cat_output.csv", or "cat_rank_<N>.csv" for a per-rank MPI file).
         *        When present, the catchments of each formulation share one aggregated file
         *        (the catchment id becomes a leading column); nullopt is one file per catchment.
         * @param precision Significant digits applied uniformly when rendering all values to text.
         * @param descriptors The full set of catchments (and their fields) this manager will write;
         *        files are opened and headers written for all of them here, so the manager is ready to
         *        receive data immediately after construction.
         */
        CatchmentCsvOutputMgr(std::string output_root, std::optional<std::string> aggregated_filename,
                              int precision, std::vector<FeatureDescriptor> descriptors);

        ~CatchmentCsvOutputMgr() override;

        // Keep the default-formulation-id convenience overload visible alongside the override.
        using CatchmentOutputsMgr::receive_data_entry;

        void receive_data_entry(const std::string &formulation_id, const std::string &catchment_id,
                                const time_marker &data_time_marker, const std::vector<double> &values) override;

        void commit_writes() override;

        void close() override;

        bool is_closed() override;

    private:
        const std::string output_root_;
        const std::optional<std::string> aggregated_filename_;
        const int precision_;
        bool closed_ = false;

        // Open output streams keyed by resolved file path. In per-feature mode each (formulation,
        // catchment) has its own path; in aggregated mode every catchment of a formulation resolves
        // to the same path and shares one stream.
        std::map<std::filesystem::path, std::shared_ptr<std::ofstream>> streams_;

        //! Resolve the output file path for a (formulation, catchment). The file name is the
        //! catchment ("<catchment>.csv") per-feature, or the shared aggregated filename when
        //! aggregating; the default formulation id keeps the flat "<root><name>" layout, any other
        //! id nests under "<root><formulation>/".
        std::filesystem::path output_path(const std::string &formulation_id, const std::string &catchment_id);

        //! Resolve the stream a (formulation, catchment)'s rows are written to, or nullptr if not registered.
        std::ofstream* stream_for(const std::string &formulation_id, const std::string &catchment_id);

        //! Open the file(s) and write the header for one descriptor; called for each at construction.
        void add_feature(const FeatureDescriptor &descriptor);
    };
} // utils

#endif //NGEN_CATCHMENTCSVOUTPUTMGR_HPP
