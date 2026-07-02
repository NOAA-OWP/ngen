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

// Catchment output manager, modeled on NexusOutputsMgr: a concrete backend is constructed with the
// full set of catchments/fields it will handle (see FeatureDescriptor and the backend's
// constructor), then data is pushed in via receive_data_entry and the backend serializes it however
// it chooses. Constructing with the complete set means a backend has everything it needs to lay out
// its sink up front, rather than discovering features incrementally. The two hierarchies
// (nexus/catchment) are kept separate for now and can be unified behind a shared base later.

#ifndef NGEN_CATCHMENTOUTPUTSMGR_HPP
#define NGEN_CATCHMENTOUTPUTSMGR_HPP

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// For utils::time_marker, shared with the nexus managers. (A later refactor can
// move time_marker to a neutral header when the two hierarchies are unified.)
#include "NexusOutputsMgr.hpp"

namespace utils
{
    /**
     * Abstract class for managing and writing to catchment data files.
     *
     * The set of catchments a manager handles, and their output schema, is fixed at construction
     * (see the concrete backend's constructor and @ref FeatureDescriptor); this class is the
     * data sink for that fixed set.
     */
    class CatchmentOutputsMgr
    {

    public:
        /**
         * The formulation id used when a caller omits one (the common single-formulation case).
         *
         * Carrying a formulation id lets a simulation run independent formulation setups over the
         * same catchment without their output colliding; backends may organize output by formulation
         * (e.g. a per-formulation file or subdirectory).
         */
        static std::string default_formulation_id() { return "default"; }

        /**
         * Receive a data entry for a catchment at a given simulation time, specifying the formulation id.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param catchment_id The id for the catchment to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param values The catchment's output values for this time, positionally aligned with the
         *        columns the manager was constructed with. Values cross this boundary as typed
         *        doubles for the backend to serialize as it sees fit -- no string formatting or parsing.
         */
        virtual void receive_data_entry(const std::string &formulation_id,
                                        const std::string &catchment_id,
                                        const time_marker &data_time_marker,
                                        const std::vector<double> &values) = 0;

        /**
         * Receive a data entry for a catchment using the default formulation id.
         *
         * @see default_formulation_id
         */
        virtual void receive_data_entry(const std::string &catchment_id,
                                        const time_marker &data_time_marker,
                                        const std::vector<double> &values) {
            receive_data_entry(default_formulation_id(), catchment_id, data_time_marker, values);
        }

        /**
         * Flush any data buffered since the last commit out to the underlying sink.
         *
         * A backend that writes eagerly (relying on its own stream buffering) may do little here
         * beyond a flush; one that batches writes accumulated entries now. This is an optional mid-run
         * durability checkpoint -- @ref close performs a final commit regardless.
         */
        virtual void commit_writes() = 0;

        /**
         * Close down this manager: commit everything received, finalize the output, and close files.
         *
         * The expected shape is close() == @ref commit_writes (flush all received data) followed by
         * finalizing and closing the sink, so callers never need a separate commit_writes() before
         * close(). commit_writes() remains available for mid-run flushes.
         *
         * close() is the point at which a commit/finalize failure is reported, so it may throw; call
         * it explicitly if you need to observe such errors. A backend's destructor should still call
         * close() as a best-effort backstop, but must never let an exception escape the destructor.
         *
         * Once closed, a manager cannot receive new data; subsequent @ref receive_data_entry calls
         * should throw. close() is idempotent -- calling it on an already-closed instance returns.
         */
        virtual void close() = 0;

        /**
         * A test of whether this instance is closed.
         *
         * Mirrors the sibling NexusOutputsMgr interface, where backends use it as a guard against
         * writing after close; kept here for parity across the two hierarchies.
         *
         * @return Whether this instance is closed.
         * @see close
         */
        virtual bool is_closed() = 0;

        virtual ~CatchmentOutputsMgr() = default;
    };

    /**
     * One output column: the mapping from a source variable to its output, positionally aligned with
     * the values a formulation supplies to @ref CatchmentOutputsMgr::receive_data_entry.
     *
     * @c source_name is the model's output variable -- where the value comes from; @c output_name is
     * the name written to the sink (a CSV header, or a variable name in a self-describing format) and
     * may alias the source. Codifying both lets a backend describe the source->output relationship
     * (and carry richer per-column metadata in @c attributes, e.g. long_name / coordinates) rather
     * than only emitting a label. @c units is the column's units, or std::nullopt when the source
     * reports none -- distinct from a present-but-empty/dimensionless units string. A field states all
     * three explicitly (there is no defaulted or implicit construction), so units is always a
     * conscious choice at the point of construction.
     */
    struct OutputField
    {
        std::string source_name;              //!< the model's output variable -- the value's source
        std::string output_name;              //!< the name written to the sink; may alias source_name
        std::optional<std::string> units;     //!< nullopt when the source reports no units
        std::map<std::string, std::string> attributes;

        //! A column mapping a source variable to a (possibly renamed) output name.
        OutputField(std::string source_name, std::string output_name,
                    std::optional<std::string> units,
                    std::map<std::string, std::string> attributes = {})
            : source_name(std::move(source_name)), output_name(std::move(output_name)),
              units(std::move(units)), attributes(std::move(attributes)) {}
    };

    /**
     * One feature's output schema, supplied to a manager at construction.
     *
     * A backend uses the full set of these to lay out its sink before any data arrives (e.g. open
     * files and write their headers). One per (formulation, feature).
     */
    struct FeatureDescriptor
    {
        std::string formulation_id;
        std::string catchment_id;
        std::vector<OutputField> fields;   //!< output fields, in the order values are supplied

        FeatureDescriptor(std::string formulation_id,
                              std::string catchment_id,
                              std::vector<OutputField> fields)
            : formulation_id(std::move(formulation_id)),
              catchment_id(std::move(catchment_id)),
              fields(std::move(fields)) {}

        //! Register under the default formulation id (single-formulation case).
        FeatureDescriptor(std::string catchment_id, std::vector<OutputField> fields)
            : FeatureDescriptor(CatchmentOutputsMgr::default_formulation_id(),
                                    std::move(catchment_id), std::move(fields)) {}
    };
} // utils

#endif //NGEN_CATCHMENTOUTPUTSMGR_HPP
