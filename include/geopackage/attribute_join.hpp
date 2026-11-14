#ifndef NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP
#define NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP

#include <string>
#include <vector>

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
 * matched feature gains a property `<prefix>.<column>` per non-key column holding an integer, real
 * or text value. Cells of any other type, SQL NULL among them, yield no property. Rows keyed to a
 * feature the collection does not hold are ignored, since under partitioning most of a table's rows
 * belong to other ranks.
 *
 * @param[in,out] collection Features to join onto, mutated in place
 * @param[in] gpkg_path Path to the GeoPackage holding the table, already resolved from the spec
 * @param[in] spec The table to join, the column keying it, and how strictly to require coverage
 * @throw std::runtime_error if the table or its key column does not exist, if two of its rows are
 *        keyed to the same feature, if a composed property name is already held by a feature, or if
 *        a feature has no matching row while the spec requires one
 */
void join_attributes(
    geojson::FeatureCollection& collection,
    const std::string& gpkg_path,
    const AttributeJoinSpec& spec
);

/**
 * Join every declared table onto a collection, in the order declared.
 *
 * A spec naming no file is read from @p default_source, which is why @p default_source_is_gpkg
 * exists: the caller knows whether that default is a GeoPackage, and a spec relying on a default
 * that is not one is refused here rather than surfacing as an unreadable database further down.
 * A spec that does name a file is unaffected, so attributes can be pulled from a GeoPackage
 * alongside a source in some other format.
 *
 * @param[in,out] collection Features to join onto, mutated in place
 * @param[in] specs Tables to join, in the order they were declared
 * @param[in] default_source Path a spec naming no file is read from
 * @param[in] default_source_is_gpkg Whether @p default_source is a GeoPackage
 * @param[in] context Name of the setting these specs came from; an error names the failing entry
 *            as `<context>[<position>]`, the way the parser reporting on the same list does
 * @throw std::runtime_error if a spec relies on a @p default_source that is not a GeoPackage, or
 *        for any reason join_attributes() throws, with the failing entry named
 */
void join_all(
    geojson::FeatureCollection& collection,
    const std::vector<AttributeJoinSpec>& specs,
    const std::string& default_source,
    bool default_source_is_gpkg,
    const std::string& context
);

} // namespace geopackage
} // namespace ngen

#endif // NGEN_GEOPACKAGE_ATTRIBUTE_JOIN_HPP
