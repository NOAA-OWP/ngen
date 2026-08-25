#ifndef NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP
#define NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP

#include <string>

#include "FeatureCollection.hpp"

namespace ngen {
namespace geopackage {

/**
 * One table to join: its rows are keyed to feature identifiers, and its columns are published onto
 * the features they match under a namespacing prefix.
 *
 * This is a pure value; it performs no file or database I/O, and it carries no knowledge of what
 * the table describes or of the schema the features came from. Building one from a realization
 * config is realization::config's job, and reading the table it names is the joiner's.
 */
struct AttributeJoinSpec
{
    //! Name of the table in the GeoPackage. Required.
    std::string table;
    //! Short stand-in for @ref table when namespacing joined columns. Empty when not declared.
    std::string alias;
    //! GeoPackage holding @ref table. Empty means the default source the caller supplies.
    std::string file;
    //! Column of @ref table whose values are matched against feature IDs. Required.
    std::string key_column;
    //! When true, a feature with no row in @ref table is an error rather than a warning.
    bool required = false;

    //! The namespace joined columns are published under: the alias when declared, else the table name.
    const std::string& prefix() const { return alias.empty() ? table : alias; }
};

/**
 * Join the columns of one GeoPackage table onto the features of a collection.
 *
 * Rows are matched to features by comparing the spec's key column against feature IDs, and each
 * matched feature gains a property `<prefix>.<column>` per non-key column holding a value. SQL NULL
 * cells yield no property. Rows keyed to a feature the collection does not hold are ignored, since
 * under partitioning most of a table's rows belong to other ranks.
 *
 * @param[in,out] collection Features to join onto, mutated in place
 * @param[in] gpkg_path Path to the GeoPackage holding the table, already resolved from the spec
 * @param[in] spec The table to join, the column keying it, and how strictly to require coverage
 * @throw std::runtime_error if the table or its key column does not exist, or if a feature has no
 *        matching row while the spec requires one
 */
void join_attributes(
    geojson::FeatureCollection& collection,
    const std::string& gpkg_path,
    const AttributeJoinSpec& spec
);

} // namespace geopackage
} // namespace ngen

#endif // NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP
