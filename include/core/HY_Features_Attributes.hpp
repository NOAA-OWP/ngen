#ifndef HY_FEATURES_ATTRIBUTES_H
#define HY_FEATURES_ATTRIBUTES_H

#include <JSONGeometry.hpp>
#include <features/FeatureBase.hpp>

#include <boost/geometry.hpp>

#include <optional>
#include <stdexcept>
#include <string>

namespace hy_features {
    namespace attributes {
      /**
       * Reading the hydrofabric attributes a feature is realized from.
      */

      // Hydrofabric version specific attribute names
      /** The area attribute as hydrofabric v2 spells it; tried first, being the current one. */
      static const std::string area_v2 = "area_sqkm";
      /** The area attribute as hydrofabric v1 spells it. */
      static const std::string area_v1 = "areasqkm";

      /** Where a catchment's areal extent is taken from when both are available. */
      enum class AreaSource {
        //! The area attribute the hydrofabric carries.
        attribute,
        //! The area of the divide polygon, measured on the WGS84 spheroid.
        geometry
      };

      /** The area attribute in km^2, where the feature carries it under either spelling. */
      inline std::optional<double> area_from_attribute(const geojson::Feature& feature){
        for (const std::string& key : {area_v2, area_v1}) {
          if (feature->has_property(key)) {
            return feature->get_property(key).as_real_number();
          }
        }
        return std::nullopt;
      }

      /**
       * Measures whichever geometries enclose an area; anything else has none to give.
       *
       * The coordinates have to be WGS84 longitude/latitude in degrees. geojson::coordinate_t
       * declares that coordinate system in its type, which is what makes boost::geometry measure
       * on a spheroid -- WGS84 by default -- and answer in square meters; the same ring in a
       * cartesian type would measure in square degrees and be meaningless as an extent. Both
       * input formats satisfy the requirement: GeoJSON is WGS84 by definition, and the GeoPackage
       * reader reprojects to EPSG:4326 as it builds the geometry.
      */
      struct area_from_geometry_visitor : public boost::static_visitor<std::optional<double>> {
        std::optional<double> operator()(const geojson::polygon_t& polygon) const {
          return boost::geometry::area(polygon);
        }
        std::optional<double> operator()(const geojson::multipolygon_t& multipolygon) const {
          return boost::geometry::area(multipolygon);
        }
        //! A point or a line bounds nothing.
        template <typename T>
        std::optional<double> operator()(const T&) const { return std::nullopt; }
      };

      /**
       * The area a feature's geometry encloses, in square kilometers, where it encloses one.
       *
       * Read through the feature rather than copied out of it: a divide is large, and nothing
       * measuring one needs to own it.
       *
       * A non-positive result means the polygon is degenerate or wound the wrong way, which is no
       * more an extent than no polygon at all.
      */
      inline std::optional<double> area_from_geometry(const geojson::geometry& geometry){
        const std::optional<double> sqm = boost::apply_visitor(area_from_geometry_visitor(), geometry);
        if (!sqm || !(*sqm > 0.0)) return std::nullopt;
        return *sqm / 1'000'000.0;
      }

      /**
       * The areal extent of a catchment feature, in square kilometers.
       *
       * A hydrofabric gives this twice over: as an attribute, and as the divide whose area can be
       * measured. Either answers, so @p preferred decides only which is consulted first and the
       * other still serves when it is absent. Nothing guarantees they are equal, so which one to use
       * is a choice.
       *
       * @param feature The hydrofabric feature a catchment is realized from.
       * @param preferred Which source to consult first. Defaults to attribute.
       * @throws std::invalid_argument If the feature carries neither an area attribute nor a divide.
      */
      inline double area_sqkm(const geojson::Feature& feature,
                              AreaSource preferred = AreaSource::attribute){
        // Try the preferred source
        std::optional<double> area = preferred == AreaSource::attribute
                                       ? area_from_attribute(feature)
                                       : area_from_geometry(*feature->geometry());
        // The preferred source was nullopt, try the other one, the ternary switches
        if (!area) {
          area = preferred == AreaSource::attribute ? area_from_geometry(*feature->geometry())
                                                    : area_from_attribute(feature);
        }
        if (!area) {
          throw std::invalid_argument("Catchment '" + feature->get_id() + "' has no " + area_v2 +
                                      " or " + area_v1 + " attribute and no divide to measure, so the "
                                      "area it distributes a value over is unknown.");
        }
        return *area;
      }
    }
}

#endif //HY_FEATURES_ATTRIBUTES_H
